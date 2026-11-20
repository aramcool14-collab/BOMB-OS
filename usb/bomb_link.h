#ifndef BOMB_LINK_H
#define BOMB_LINK_H

#include <stdint.h>
#include <stddef.h>

#define BOMB_LINK_WIDTH  320u
#define BOMB_LINK_HEIGHT 240u

typedef int (*bomb_link_write_fn)(const uint8_t *data, size_t length);

int bomb_link_send_frame(bomb_link_write_fn write_bytes,
                         const uint16_t *pixels,
                         uint16_t width,
                         uint16_t height);

#endif
