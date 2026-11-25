#include "arcade.h"
#include "../display/video.h"

#define SCREEN_W 640u
#define SCREEN_H 480u

typedef struct {
    int x;
    int y;
    int active;
} Shot;

typedef struct {
    int x;
    int y;
    int active;
} Target;

static int player_x;
static Shot shot;
static Target targets[5];
static unsigned score;
static unsigned tick;

void bomb_arcade_start(void)
{
    player_x = 304;
    shot.active = 0;
    score = 0;
    tick = 0;
    for (unsigned i = 0; i < 5; ++i) {
        targets[i].x = 70 + (int)i * 112;
        targets[i].y = 90 + (int)(i % 2u) * 45;
        targets[i].active = 1;
    }
}

void bomb_arcade_update(uint32_t keys)
{
    if (keys & BOMB_KEY_LEFT) player_x -= 5;
    if (keys & BOMB_KEY_RIGHT) player_x += 5;
    if (player_x < 12) player_x = 12;
    if (player_x > 604) player_x = 604;

    if ((keys & BOMB_KEY_FIRE) && !shot.active) {
        shot.x = player_x + 12;
        shot.y = 400;
        shot.active = 1;
    }

    if (shot.active) {
        shot.y -= 8;
        if (shot.y < 28) shot.active = 0;
        for (unsigned i = 0; i < 5; ++i) {
            if (targets[i].active &&
                shot.x >= targets[i].x - 4 && shot.x <= targets[i].x + 28 &&
                shot.y >= targets[i].y - 4 && shot.y <= targets[i].y + 28) {
                targets[i].active = 0;
                shot.active = 0;
                ++score;
                break;
            }
        }
    }

    ++tick;
    if ((tick % 90u) == 0u) {
        for (unsigned i = 0; i < 5; ++i) {
            if (!targets[i].active) {
                targets[i].active = 1;
                targets[i].y = 55;
                targets[i].x = 35 + (int)((tick / 90u + i * 97u) % 560u);
                break;
            }
        }
    }
}

void bomb_arcade_draw(void)
{
    if (!bomb_video_ready()) return;
    bomb_video_clear(0xFF0B1220u);

    /* Star-like background dots. */
    for (unsigned i = 0; i < 36; ++i) {
        unsigned x = (i * 173u + 29u) % SCREEN_W;
        unsigned y = (i * 97u + 17u) % SCREEN_H;
        bomb_video_rect(x, y, 2, 2, 0xFF526783u);
    }

    /* Player craft. */
    bomb_video_rect((unsigned)player_x, 418, 32, 10, 0xFF40D8E8u);
    bomb_video_rect((unsigned)player_x + 10u, 408, 12, 12, 0xFFB9F6FFu);

    if (shot.active)
        bomb_video_rect((unsigned)shot.x + 13u, (unsigned)shot.y, 5, 15, 0xFFFFD166u);

    for (unsigned i = 0; i < 5; ++i) {
        if (!targets[i].active) continue;
        bomb_video_rect((unsigned)targets[i].x, (unsigned)targets[i].y, 28, 20, 0xFF9B8CFFu);
        bomb_video_rect((unsigned)targets[i].x + 5u, (unsigned)targets[i].y + 20u, 18, 8, 0xFF6657C8u);
    }

    /* Score indicator blocks; text rendering is a later UI layer. */
    bomb_video_rect(18, 18, 8u + score * 8u, 5, 0xFF40D8E8u);
}

unsigned bomb_arcade_score(void)
{
    return score;
}
