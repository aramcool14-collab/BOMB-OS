#include "laptop_input.h"
#define PACKET_SIZE 6u
static uint8_t packet[PACKET_SIZE];
static unsigned used;
static int cursor_x=320, cursor_y=240, mouse_changed;
static uint8_t mouse_buttons;
static uint8_t key_code, key_pressed;
static int key_changed;

void bomb_input_feed(const uint8_t *data, unsigned length) {
    if (!data) return;
    for (unsigned i=0;i<length;i++) {
        uint8_t b=data[i];
        if (used==0 && b!='B') continue;
        packet[used++]=b;
        if (used==PACKET_SIZE) {
            used=0;
            if (packet[0]!='B' || packet[1]!='I') continue;
            if (packet[2]==1) {
                int dx=(int8_t)packet[3], dy=(int8_t)packet[4];
                cursor_x+=dx; cursor_y+=dy;
                if(cursor_x<0)cursor_x=0; if(cursor_x>639)cursor_x=639;
                if(cursor_y<0)cursor_y=0; if(cursor_y>479)cursor_y=479;
                mouse_buttons=packet[5]; mouse_changed=1;
            } else if(packet[2]==2) {
                key_code=packet[3]; key_pressed=packet[4]; key_changed=1;
            }
        }
    }
}
int bomb_input_take_mouse(int *x,int *y,uint8_t *buttons) {
    if(!mouse_changed)return 0;
    mouse_changed=0;
    if(x)*x=cursor_x; if(y)*y=cursor_y; if(buttons)*buttons=mouse_buttons;
    return 1;
}
int bomb_input_take_key(uint8_t *key,uint8_t *pressed) {
    if(!key_changed)return 0;
    key_changed=0;
    if(key)*key=key_code; if(pressed)*pressed=key_pressed;
    return 1;
}
