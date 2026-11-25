#ifndef BOMB_DWC2_DEVICE_H
#define BOMB_DWC2_DEVICE_H

#include <stddef.h>
#include <stdint.h>

int bomb_usb_init(void);
void bomb_usb_poll(void);
int bomb_usb_configured(void);
int bomb_usb_write(const uint8_t *data, size_t length);
int bomb_usb_read(uint8_t *data, size_t capacity);

#endif
