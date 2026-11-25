#include "cursor.h"
#include "../display/video.h"
void bomb_desktop_draw_cursor(int x,int y) {
    if(!bomb_video_ready())return;
    /* Small high-contrast pointer; drawn over the desktop after each update. */
    bomb_video_rect((unsigned)x,(unsigned)y,3,18,0xFFFFFFFFu);
    bomb_video_rect((unsigned)x,(unsigned)y,12,3,0xFFFFFFFFu);
    bomb_video_rect((unsigned)x+3u,(unsigned)y+12u,7,3,0xFF28B8FFu);
    bomb_video_rect((unsigned)x+7u,(unsigned)y+15u,4,3,0xFF28B8FFu);
}
