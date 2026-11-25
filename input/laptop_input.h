#ifndef BOMB_LAPTOP_INPUT_H
#define BOMB_LAPTOP_INPUT_H
#include <stdint.h>
void bomb_input_feed(const uint8_t *data, unsigned length);
int bomb_input_take_mouse(int *x, int *y, uint8_t *buttons);
int bomb_input_take_key(uint8_t *key, uint8_t *pressed);
#endif
