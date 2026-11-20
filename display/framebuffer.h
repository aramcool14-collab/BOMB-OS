#ifndef BOMB_FRAMEBUFFER_H
#define BOMB_FRAMEBUFFER_H

#include <stdint.h>

#define BOMB_FB_WIDTH  320u
#define BOMB_FB_HEIGHT 240u

void bomb_fb_clear(uint16_t color);
void bomb_fb_pixel(unsigned x, unsigned y, uint16_t color);
void bomb_fb_rect(unsigned x, unsigned y, unsigned w, unsigned h, uint16_t color);
void bomb_fb_test_screen(void);
const uint16_t *bomb_fb_data(void);

#endif
