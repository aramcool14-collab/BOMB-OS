#include "bomb_link.h"

static void put_u16_le(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put_u32_le(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

int bomb_link_send_frame(bomb_link_write_fn write_bytes,
                         const uint16_t *pixels,
                         uint16_t width,
                         uint16_t height)
{
    if (!write_bytes || !pixels || width == 0 || height == 0 ||
        width > BOMB_LINK_WIDTH || height > BOMB_LINK_HEIGHT)
        return -1;

    uint8_t header[12] = {
        'B', 'O', 'M', 'B', 0, 0, 0, 0, 0, 0, 0, 0
    };

    uint32_t payload_bytes = (uint32_t)width * height * 2u;
    put_u16_le(&header[4], width);
    put_u16_le(&header[6], height);
    put_u32_le(&header[8], payload_bytes);

    if (write_bytes(header, sizeof(header)) != 0)
        return -2;

    const uint8_t *bytes = (const uint8_t *)pixels;
    size_t offset = 0;
    while (offset < payload_bytes) {
        size_t chunk = payload_bytes - offset;
        if (chunk > 512u) chunk = 512u;
        if (write_bytes(bytes + offset, chunk) != 0)
            return -3;
        offset += chunk;
    }
    return 0;
}
