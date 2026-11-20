#include "../display/video.h"
#include "desktop.h"

#define C_BG       0xFF101522u
#define C_PANEL    0xFF1B2435u
#define C_CARD     0xFF222F45u
#define C_BLUE     0xFF28B8FFu
#define C_CYAN     0xFF42E8D4u
#define C_WHITE    0xFFF2F6FFu
#define C_MUTED    0xFF8FA2BCu
#define C_PURPLE   0xFF8B72FFu
#define C_ORANGE   0xFFFFB454u

static void icon(unsigned x, unsigned y, uint32_t color, unsigned kind)
{
    bomb_video_rect(x, y, 42, 42, C_CARD);
    bomb_video_rect(x + 5, y + 5, 32, 32, color);
    if (kind == 0) {
        bomb_video_rect(x + 12, y + 12, 18, 18, C_CARD);
        bomb_video_rect(x + 16, y + 16, 10, 10, color);
    } else if (kind == 1) {
        bomb_video_rect(x + 10, y + 10, 22, 4, C_CARD);
        bomb_video_rect(x + 10, y + 19, 22, 4, C_CARD);
        bomb_video_rect(x + 10, y + 28, 22, 4, C_CARD);
    } else {
        bomb_video_rect(x + 12, y + 10, 18, 22, C_CARD);
        bomb_video_rect(x + 17, y + 15, 8, 12, color);
    }
}

void bomb_desktop_draw(void)
{
    if (!bomb_video_ready()) return;

    bomb_video_clear(C_BG);

    /* Abstract blue gradient-style wallpaper. */
    bomb_video_rect(0, 0, 640, 480, C_BG);
    bomb_video_rect(390, 0, 250, 390, 0xFF131D31u);
    bomb_video_rect(430, 0, 210, 350, 0xFF17253Bu);
    bomb_video_rect(470, 0, 170, 310, 0xFF1B2D49u);
    bomb_video_rect(510, 0, 130, 270, 0xFF203653u);

    /* Header / brand area. */
    bomb_video_rect(22, 20, 596, 64, C_PANEL);
    bomb_video_rect(22, 20, 5, 64, C_BLUE);
    bomb_video_rect(42, 36, 34, 34, C_BLUE);
    bomb_video_rect(49, 43, 20, 20, C_PANEL);
    bomb_video_rect(54, 48, 10, 10, C_CYAN);
    bomb_video_rect(88, 37, 160, 7, C_WHITE);
    bomb_video_rect(88, 52, 110, 4, C_MUTED);
    bomb_video_rect(520, 42, 70, 5, C_CYAN);
    bomb_video_rect(520, 55, 48, 4, C_MUTED);

    /* Desktop app cards. */
    bomb_video_rect(22, 104, 184, 290, C_PANEL);
    bomb_video_rect(222, 104, 396, 290, C_PANEL);
    bomb_video_rect(40, 122, 148, 5, C_MUTED);
    bomb_video_rect(242, 122, 160, 5, C_MUTED);

    icon(48, 150, C_BLUE, 0);
    icon(48, 220, C_CYAN, 1);
    icon(48, 290, C_PURPLE, 2);
    bomb_video_rect(102, 166, 74, 5, C_WHITE);
    bomb_video_rect(102, 178, 48, 4, C_MUTED);
    bomb_video_rect(102, 236, 62, 5, C_WHITE);
    bomb_video_rect(102, 248, 44, 4, C_MUTED);
    bomb_video_rect(102, 306, 72, 5, C_WHITE);
    bomb_video_rect(102, 318, 46, 4, C_MUTED);

    /* Main welcome card and quick-launch tiles. */
    bomb_video_rect(244, 148, 352, 92, C_CARD);
    bomb_video_rect(262, 166, 7, 54, C_BLUE);
    bomb_video_rect(284, 168, 220, 7, C_WHITE);
    bomb_video_rect(284, 185, 170, 5, C_MUTED);
    bomb_video_rect(284, 202, 116, 5, C_CYAN);

    bomb_video_rect(244, 258, 108, 112, C_CARD);
    bomb_video_rect(366, 258, 108, 112, C_CARD);
    bomb_video_rect(488, 258, 108, 112, C_CARD);
    icon(277, 274, C_ORANGE, 1);
    icon(399, 274, C_BLUE, 0);
    icon(521, 274, C_PURPLE, 2);
    bomb_video_rect(264, 330, 68, 5, C_WHITE);
    bomb_video_rect(386, 330, 68, 5, C_WHITE);
    bomb_video_rect(508, 330, 68, 5, C_WHITE);

    /* Bottom taskbar. */
    bomb_video_rect(0, 424, 640, 56, 0xFF080D17u);
    bomb_video_rect(0, 424, 640, 2, C_BLUE);
    bomb_video_rect(18, 436, 34, 32, C_BLUE);
    bomb_video_rect(25, 443, 20, 18, 0xFF080D17u);
    bomb_video_rect(30, 448, 10, 8, C_CYAN);
    bomb_video_rect(70, 448, 70, 5, C_WHITE);
    bomb_video_rect(540, 444, 70, 4, C_MUTED);
    bomb_video_rect(558, 455, 52, 4, C_WHITE);
}
