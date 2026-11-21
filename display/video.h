#ifndef BOMB_VIDEO_H
#define BOMB_VIDEO_H

#include <stdint.h>

int bomb_video_init(void);
void bomb_video_clear(uint32_t color);
void bomb_video_rect(unsigned x, unsigned y, unsigned w, unsigned h, uint32_t color);
void bomb_video_pixel(unsigned x, unsigned y, uint32_t color);
unsigned bomb_video_width(void);
unsigned bomb_video_height(void);
int bomb_video_ready(void);
void bomb_video_copy_rgb565(uint16_t *out, unsigned width, unsigned height);

#endif
