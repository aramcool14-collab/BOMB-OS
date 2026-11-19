#include <stdint.h>

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

static void delay(volatile unsigned count)
{
    while (count--) {
        __asm__ volatile("nop");
    }
}

static void uart_init(void)
{
    AUX_ENABLES |= 1u;
    AUX_MU_CNTL = 0;
    AUX_MU_IER = 0;
    AUX_MU_LCR = 3;
    AUX_MU_MCR = 0;
    AUX_MU_IIR = 0xC6;
    AUX_MU_BAUD = 270;

    GPFSEL1 &= ~((7u << 12) | (7u << 15));
    GPFSEL1 |=  ((2u << 12) | (2u << 15));

    GPPUD = 0;
    delay(150);
    GPPUDCLK0 = (1u << 14) | (1u << 15);
    delay(150);
    GPPUD = 0;
    GPPUDCLK0 = 0;

    AUX_MU_CNTL = 3;
}

static void uart_putc(char c)
{
    while ((AUX_MU_LSR & (1u << 5)) == 0) { }
    AUX_MU_IO = (uint32_t)c;
}

static void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

void kernel_main(void)
{
    uart_init();
    uart_puts("\n");
    uart_puts("================================\n");
    uart_puts("          BOMB OS 0.01          \n");
    uart_puts("================================\n");
    uart_puts("Hello! This is your own C kernel.\n");
    uart_puts("Target: Raspberry Pi Zero v1.3 / ARMv6\n");
    uart_puts("Next milestone: USB display + graphics.\n");

    for (;;) {
        __asm__ volatile("wfi");
    }
}
