#include "framebuffer.h"

static uint16_t framebuffer[BOMB_FB_WIDTH * BOMB_FB_HEIGHT];

void bomb_fb_clear(uint16_t color)
{
    for (unsigned i = 0; i < BOMB_FB_WIDTH * BOMB_FB_HEIGHT; ++i)
        framebuffer[i] = color;
}

void bomb_fb_pixel(unsigned x, unsigned y, uint16_t color)
{
    if (x >= BOMB_FB_WIDTH || y >= BOMB_FB_HEIGHT)
        return;
    framebuffer[y * BOMB_FB_WIDTH + x] = color;
}

void bomb_fb_rect(unsigned x, unsigned y, unsigned w, unsigned h, uint16_t color)
{
    if (x >= BOMB_FB_WIDTH || y >= BOMB_FB_HEIGHT)
        return;

    if (w > BOMB_FB_WIDTH - x) w = BOMB_FB_WIDTH - x;
    if (h > BOMB_FB_HEIGHT - y) h = BOMB_FB_HEIGHT - y;

    for (unsigned row = 0; row < h; ++row)
        for (unsigned col = 0; col < w; ++col)
            framebuffer[(y + row) * BOMB_FB_WIDTH + (x + col)] = color;
}

void bomb_fb_test_screen(void)
{
    bomb_fb_clear(0x0841);
    bomb_fb_rect(12, 12, 296, 34, 0xF800);
    bomb_fb_rect(12, 56, 90, 170, 0x2104);
    bomb_fb_rect(110, 56, 198, 170, 0x4208);
    bomb_fb_rect(20, 64, 74, 8, 0xFFFF);
    bomb_fb_rect(20, 82, 58, 5, 0x7BEF);
    bomb_fb_rect(120, 66, 178, 8, 0x07E0);
    bomb_fb_rect(120, 84, 130, 5, 0xFFFF);
    bomb_fb_rect(120, 105, 150, 90, 0x001F);
    bomb_fb_rect(20, 210, 280, 8, 0xFFE0);
}

const uint16_t *bomb_fb_data(void)
{
    return framebuffer;
}
