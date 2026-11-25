#include <stdint.h>
#include "../display/video.h"
#include "../desktop/desktop.h"
#include "../display/framebuffer.h"
#include "../usb/dwc2_device.h"
#include "../usb/bomb_link.h"
#include "../input/laptop_input.h"
#include "../desktop/cursor.h"

#define MMIO_BASE       0x20000000u
#define AUX_ENABLES     (*(volatile uint32_t *)(MMIO_BASE + 0x00215004u))
#define AUX_MU_IO       (*(volatile uint32_t *)(MMIO_BASE + 0x00215040u))
#define AUX_MU_IER      (*(volatile uint32_t *)(MMIO_BASE + 0x00215044u))
#define AUX_MU_IIR      (*(volatile uint32_t *)(MMIO_BASE + 0x00215048u))
#define AUX_MU_LCR      (*(volatile uint32_t *)(MMIO_BASE + 0x0021504Cu))
#define AUX_MU_MCR      (*(volatile uint32_t *)(MMIO_BASE + 0x00215050u))
#define AUX_MU_LSR      (*(volatile uint32_t *)(MMIO_BASE + 0x00215054u))
#define AUX_MU_CNTL     (*(volatile uint32_t *)(MMIO_BASE + 0x00215060u))
#define AUX_MU_BAUD     (*(volatile uint32_t *)(MMIO_BASE + 0x00215068u))
#define GPFSEL1         (*(volatile uint32_t *)(MMIO_BASE + 0x00200004u))
#define GPPUD           (*(volatile uint32_t *)(MMIO_BASE + 0x00200094u))
#define GPPUDCLK0       (*(volatile uint32_t *)(MMIO_BASE + 0x00200098u))
#define GPFSEL4         (*(volatile uint32_t *)(MMIO_BASE + 0x00200010u))
#define GPSET1          (*(volatile uint32_t *)(MMIO_BASE + 0x00200020u))
#define GPCLR1          (*(volatile uint32_t *)(MMIO_BASE + 0x0020002Cu))
#define SYST_CLO        (*(volatile uint32_t *)(MMIO_BASE + 0x00003004u))
#define ACT_LED_BIT     (1u << 15) /* GPIO47, active-low ACT LED on Pi Zero v1.3 */

static uint16_t screen_frame[BOMB_LINK_WIDTH * BOMB_LINK_HEIGHT];
static uint8_t usb_input_buffer[64];
static uint32_t led_last_tick;
static int led_is_on;

static void delay(volatile unsigned count) { while (count--) __asm__ volatile("nop"); }

/* A heartbeat on the Pi Zero's onboard ACT LED proves that kernel_main is running. */
static void act_led_init(void) {
    GPFSEL4 = (GPFSEL4 & ~(7u << 21)) | (1u << 21); /* GPIO47 output */
    GPSET1 = ACT_LED_BIT; /* LED off (active-low) */
    led_last_tick = SYST_CLO;
    led_is_on = 0;
}
static void act_led_heartbeat(void) {
    uint32_t now = SYST_CLO;
    if ((uint32_t)(now - led_last_tick) >= 250000u) {
        led_last_tick = now;
        if (led_is_on) {
            GPSET1 = ACT_LED_BIT;
            led_is_on = 0;
        } else {
            GPCLR1 = ACT_LED_BIT;
            led_is_on = 1;
        }
    }
}

static void uart_init(void) {
    AUX_ENABLES |= 1u; AUX_MU_CNTL=0; AUX_MU_IER=0; AUX_MU_LCR=3; AUX_MU_MCR=0;
    AUX_MU_IIR=0xC6; AUX_MU_BAUD=270;
    GPFSEL1 &= ~((7u<<12)|(7u<<15)); GPFSEL1 |= ((2u<<12)|(2u<<15));
    GPPUD=0; delay(150); GPPUDCLK0=(1u<<14)|(1u<<15); delay(150);
    GPPUD=0; GPPUDCLK0=0; AUX_MU_CNTL=3;
}
static void uart_putc(char c) { while((AUX_MU_LSR&(1u<<5))==0){} AUX_MU_IO=(uint32_t)c; }
static void uart_puts(const char *s) { while(*s){ if(*s=='\n')uart_putc('\r'); uart_putc(*s++); } }
static int usb_write(const uint8_t *data,size_t length) { return bomb_usb_write(data,length); }

void kernel_main(void) {
    act_led_init();
    uart_init();
    uart_puts("\nBOMB OS booting on Raspberry Pi Zero v1.3...\n");
    if(bomb_video_init()) {
        uart_puts("VideoCore framebuffer: OK (640x480x32)\n");
    } else {
        uart_puts("HDMI framebuffer unavailable; USB software desktop remains available.\n");
    }
    bomb_desktop_draw();
    uart_puts("BOMB OS desktop rendered to HDMI and USB framebuffers.\n");
    uart_puts("Starting USB OTG CDC device...\n");
    if (bomb_usb_init()) uart_puts("USB controller ready; waiting for host enumeration.\n");
    else uart_puts("USB controller initialization FAILED; check OTG port and power.\n");
    uart_puts("Connect the Pi Zero USB (data/OTG) port to the PC.\n");
    unsigned frame_wait=0;
    for(;;) {
        act_led_heartbeat();
        bomb_usb_poll();
        int input_count=bomb_usb_read(usb_input_buffer,sizeof(usb_input_buffer));
        if(input_count>0) {
            bomb_input_feed(usb_input_buffer,(unsigned)input_count);
            int mx,my; uint8_t buttons;
            if(bomb_input_take_mouse(&mx,&my,&buttons)) { (void)buttons; bomb_desktop_draw(); bomb_desktop_draw_cursor(mx,my); }
            uint8_t key,pressed;
            while(bomb_input_take_key(&key,&pressed)) {
                if(pressed && key) { uart_puts("Laptop key event received.\\n"); }
            }
        }
        if(bomb_usb_configured()) {
            if(frame_wait==0) {
                if (bomb_video_ready()) {
                    bomb_video_copy_rgb565(screen_frame,BOMB_LINK_WIDTH,BOMB_LINK_HEIGHT);
                } else {
                    const uint16_t *fallback = bomb_fb_data();
                    for (unsigned i=0;i<BOMB_LINK_WIDTH*BOMB_LINK_HEIGHT;i++) screen_frame[i]=fallback[i];
                }
                if(bomb_link_send_frame(usb_write,screen_frame,BOMB_LINK_WIDTH,BOMB_LINK_HEIGHT)==0)
                    uart_puts("BOMB-LINK frame sent.\n");
                frame_wait=50000000u;
            }
        }
        if(frame_wait) frame_wait--;
    }
}
