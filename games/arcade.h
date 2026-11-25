#ifndef BOMB_ARCADE_H
#define BOMB_ARCADE_H

#include <stdint.h>

#define BOMB_KEY_LEFT  (1u << 0)
#define BOMB_KEY_RIGHT (1u << 1)
#define BOMB_KEY_FIRE  (1u << 2)

void bomb_arcade_start(void);
void bomb_arcade_update(uint32_t keys);
void bomb_arcade_draw(void);
unsigned bomb_arcade_score(void);

#endif
