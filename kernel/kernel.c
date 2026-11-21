#include <stdint.h>
#include "../display/video.h"
#include "../desktop/desktop.h"
#include "../display/framebuffer.h"
#include "../usb/dwc2_device.h"
#include "../usb/bomb_link.h"

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

static uint16_t screen_frame[BOMB_LINK_WIDTH * BOMB_LINK_HEIGHT];

static void delay(volatile unsigned count) { while (count--) __asm__ volatile("nop"); }
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
    uart_init();
    uart_puts("\nBOMB OS booting on Raspberry Pi Zero v1.3...\n");
    if(bomb_video_init()) {
        uart_puts("VideoCore framebuffer: OK (640x480x32)\n");
        bomb_desktop_draw();
        uart_puts("BOMB OS desktop drawn.\n");
    } else {
        uart_puts("VideoCore framebuffer: FAILED; check HDMI/firmware setup.\n");
        bomb_fb_test_screen();
    }
    uart_puts("Starting USB OTG CDC device...\n");
    bomb_usb_init();
    uart_puts("USB controller initialized; waiting for host enumeration.\n");
    uart_puts("Connect the Pi Zero USB (data/OTG) port to the PC.\n");
    unsigned frame_wait=0;
    for(;;) {
        bomb_usb_poll();
        if(bomb_usb_configured()) {
            if(frame_wait==0) {
                bomb_video_copy_rgb565(screen_frame,BOMB_LINK_WIDTH,BOMB_LINK_HEIGHT);
                if(bomb_link_send_frame(usb_write,screen_frame,BOMB_LINK_WIDTH,BOMB_LINK_HEIGHT)==0)
                    uart_puts("BOMB-LINK frame sent.\n");
                frame_wait=1200000u;
            }
        }
        if(frame_wait) frame_wait--;
    }
}
