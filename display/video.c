#include "video.h"

#define MMIO_BASE       0x20000000u
#define MBOX_READ       (*(volatile uint32_t *)(MMIO_BASE + 0x0000B880u))
#define MBOX_STATUS     (*(volatile uint32_t *)(MMIO_BASE + 0x0000B898u))
#define MBOX_WRITE      (*(volatile uint32_t *)(MMIO_BASE + 0x0000B8A0u))
#define MBOX_FULL       0x80000000u
#define MBOX_EMPTY      0x40000000u
#define MBOX_CHANNEL    8u
#define BUS_ALIAS       0x40000000u

#define FB_W 640u
#define FB_H 480u

static volatile uint32_t property[35] __attribute__((aligned(16)));
static uint32_t *pixels;
static unsigned pitch_bytes;
static int ready;

static int mailbox_property_call(void)
{
    uint32_t address = ((uint32_t)(uintptr_t)property) | BUS_ALIAS;
    unsigned timeout = 10000000u;

    while ((MBOX_STATUS & MBOX_FULL) && --timeout) { }
    if (!timeout) return 0;

    MBOX_WRITE = (address & ~0xFu) | MBOX_CHANNEL;
    timeout = 10000000u;

    while (timeout--) {
        while (!(MBOX_STATUS & MBOX_EMPTY)) {
            uint32_t response = MBOX_READ;
            if ((response & 0xFu) == MBOX_CHANNEL &&
                (response & ~0xFu) == (address & ~0xFu))
                return property[1] == 0x80000000u;
        }
    }
    return 0;
}

int bomb_video_init(void)
{
    for (unsigned i = 0; i < 35; ++i) property[i] = 0;

    property[0] = sizeof(property);
    property[1] = 0;
    property[2] = 0x00048003u; property[3] = 8; property[4] = 8;
    property[5] = FB_W; property[6] = FB_H;
    property[7] = 0x00048004u; property[8] = 8; property[9] = 8;
    property[10] = FB_W; property[11] = FB_H;
    property[12] = 0x00048005u; property[13] = 4; property[14] = 4;
    property[15] = 32;
    property[16] = 0x00048006u; property[17] = 4; property[18] = 4;
    property[19] = 1;
    property[20] = 0x00040001u; property[21] = 8; property[22] = 8;
    property[23] = 16; property[24] = 0;
    property[25] = 0x00040008u; property[26] = 4; property[27] = 4;
    property[28] = 0;
    property[29] = 0;

    if (!mailbox_property_call()) return 0;
    if (property[23] == 0 || property[28] < FB_W * 4u) return 0;

    pixels = (uint32_t *)(uintptr_t)(property[23] & 0x3FFFFFFFu);
    pitch_bytes = property[28];
    ready = 1;
    bomb_video_clear(0xFF101522u);
    return 1;
}

void bomb_video_pixel(unsigned x, unsigned y, uint32_t color)
{
    if (!ready || x >= FB_W || y >= FB_H) return;
    pixels[(y * pitch_bytes) / 4u + x] = color;
}

void bomb_video_rect(unsigned x, unsigned y, unsigned w, unsigned h, uint32_t color)
{
    if (!ready || x >= FB_W || y >= FB_H) return;
    if (w > FB_W - x) w = FB_W - x;
    if (h > FB_H - y) h = FB_H - y;

    for (unsigned row = 0; row < h; ++row) {
        uint32_t *line = pixels + ((y + row) * pitch_bytes) / 4u + x;
        for (unsigned col = 0; col < w; ++col) line[col] = color;
    }
}

void bomb_video_clear(uint32_t color)
{
    if (!ready) return;
    for (unsigned y = 0; y < FB_H; ++y) {
        uint32_t *line = pixels + (y * pitch_bytes) / 4u;
        for (unsigned x = 0; x < FB_W; ++x) line[x] = color;
    }
}

unsigned bomb_video_width(void) { return FB_W; }
unsigned bomb_video_height(void) { return FB_H; }
int bomb_video_ready(void) { return ready; }

/* Downsample the live 640x480 ARGB framebuffer to the USB viewer's RGB565 frame. */
void bomb_video_copy_rgb565(uint16_t *out, unsigned width, unsigned height)
{
    if (!out || !ready || width == 0 || height == 0 || width > FB_W || height > FB_H) return;
    for (unsigned y = 0; y < height; ++y) {
        unsigned sy = (y * FB_H) / height;
        const uint32_t *line = pixels + (sy * pitch_bytes) / 4u;
        for (unsigned x = 0; x < width; ++x) {
            uint32_t c = line[(x * FB_W) / width];
            uint16_t r = (uint16_t)((c >> 19) & 0x1Fu);
            uint16_t g = (uint16_t)((c >> 10) & 0x3Fu);
            uint16_t b = (uint16_t)((c >> 3) & 0x1Fu);
            out[y * width + x] = (uint16_t)((r << 11) | (g << 5) | b);
        }
    }
}
