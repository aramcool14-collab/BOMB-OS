#include "dwc2_device.h"

/*
 * BOMB OS USB beta - Raspberry Pi Zero DWC2 device-mode bring-up.
 *
 * This first driver stage performs a bounded core reset, selects device
 * mode, configures the full-speed device controller and endpoint-zero
 * control path. CDC-ACM enumeration and frame streaming are deliberately
 * not reported as ready until their endpoint handling is implemented and
 * tested on real Pi Zero hardware.
 */

#define REG32(a) (*(volatile uint32_t *)(uintptr_t)(a))
#define DWC2_BASE       0x20980000u
#define GOTGCTL         REG32(DWC2_BASE + 0x000u)
#define GAHBCFG         REG32(DWC2_BASE + 0x008u)
#define GUSBCFG         REG32(DWC2_BASE + 0x00Cu)
#define GRSTCTL         REG32(DWC2_BASE + 0x010u)
#define GINTSTS         REG32(DWC2_BASE + 0x014u)
#define GINTMSK         REG32(DWC2_BASE + 0x018u)
#define GRXFSIZ         REG32(DWC2_BASE + 0x024u)
#define GNPTXFSIZ       REG32(DWC2_BASE + 0x028u)
#define GDFIFOCFG       REG32(DWC2_BASE + 0x05Cu)
#define DCFG            REG32(DWC2_BASE + 0x800u)
#define DCTL            REG32(DWC2_BASE + 0x804u)
#define DSTS            REG32(DWC2_BASE + 0x808u)
#define DIEPMSK         REG32(DWC2_BASE + 0x810u)
#define DOEPMSK         REG32(DWC2_BASE + 0x814u)
#define DAINTMSK        REG32(DWC2_BASE + 0x81Cu)
#define DIEPCTL(n)      REG32(DWC2_BASE + 0x900u + ((n) * 0x20u))
#define DIEPINT(n)      REG32(DWC2_BASE + 0x908u + ((n) * 0x20u))
#define DIEPTSIZ(n)     REG32(DWC2_BASE + 0x910u + ((n) * 0x20u))
#define DOEPCTL(n)      REG32(DWC2_BASE + 0xB00u + ((n) * 0x20u))
#define DOEPINT(n)      REG32(DWC2_BASE + 0xB08u + ((n) * 0x20u))
#define DOEPTSIZ(n)     REG32(DWC2_BASE + 0xB10u + ((n) * 0x20u))

#define GRSTCTL_CSFTRST (1u << 0)
#define GRSTCTL_AHBIDL  (1u << 31)
#define GUSBCFG_FDMOD   (1u << 30)
#define DCTL_SDIS       (1u << 1)
#define DCTL_CGINAK     (1u << 8)
#define DCTL_CGONAK     (1u << 10)
#define EPCTL_USBAEP    (1u << 15)
#define EPCTL_SNAK      (1u << 27)
#define EPCTL_EPDIS     (1u << 30)
#define EPCTL_EPENA     (1u << 31)

static int usb_ready;

static void wait_cycles(volatile unsigned n)
{
    while (n--) __asm__ volatile("nop");
}

static int wait_mask(volatile uint32_t *reg, uint32_t mask, uint32_t value)
{
    unsigned timeout = 2000000u;
    while (timeout--) {
        if ((*reg & mask) == value) return 1;
    }
    return 0;
}

void bomb_usb_init(void)
{
    usb_ready = 0;

    /* Wait for the AHB master, then reset the OTG core. */
    if (!wait_mask((volatile uint32_t *)(uintptr_t)(DWC2_BASE + 0x010u),
                   GRSTCTL_AHBIDL, GRSTCTL_AHBIDL))
        return;

    GRSTCTL |= GRSTCTL_CSFTRST;
    if (!wait_mask((volatile uint32_t *)(uintptr_t)(DWC2_BASE + 0x010u),
                   GRSTCTL_CSFTRST, 0))
        return;

    wait_cycles(10000u);

    /* Force peripheral mode and select the internal full-speed PHY. */
    GUSBCFG |= GUSBCFG_FDMOD;
    wait_cycles(10000u);

    /* Flush all transmit FIFOs and the receive FIFO. */
    GRSTCTL = (1u << 5);
    if (!wait_mask((volatile uint32_t *)(uintptr_t)(DWC2_BASE + 0x010u),
                   (1u << 5), 0))
        return;
    GRSTCTL = (1u << 4);
    if (!wait_mask((volatile uint32_t *)(uintptr_t)(DWC2_BASE + 0x010u),
                   (1u << 4), 0))
        return;

    /* Full-speed device, endpoint-zero control FIFO allocation. */
    DCFG = (DCFG & ~3u) | 3u;
    GRXFSIZ = 128u;
    GNPTXFSIZ = (128u << 16) | 128u;
    GDFIFOCFG = (256u << 16) | 256u;

    DIEPMSK = 0;
    DOEPMSK = 0;
    DAINTMSK = 0;
    GINTMSK = 0;
    GINTSTS = 0xFFFFFFFFu;

    /* Enable endpoint zero as a 64-byte control endpoint. */
    DIEPCTL(0) = EPCTL_USBAEP | (1u << 18) | (1u << 0);
    DOEPCTL(0) = EPCTL_USBAEP | (1u << 18);
    DOEPTSIZ(0) = (1u << 19) | ((3u << 29) | (1u << 19) | 24u);
    usb_ready = 1;

    /* Connect to the USB bus only after the core is configured. */
    DCTL &= ~DCTL_SDIS;
}

void bomb_usb_poll(void)
{
    if (!usb_ready) return;
    /* Enumeration and endpoint servicing are added in the next driver stage. */
    (void)GOTGCTL;
    (void)GINTSTS;
    (void)DSTS;
}

int bomb_usb_configured(void)
{
    return 0;
}

int bomb_usb_write(const uint8_t *data, size_t length)
{
    (void)data;
    (void)length;
    return -1;
}
