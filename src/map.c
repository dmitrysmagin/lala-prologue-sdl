#include "map.h"
#include "screen.h"

#include <string.h>

void clear_buff(TileCell *buff) {
    memset(buff, 0, sizeof(TileCell) * CUR_BUFF_SIZE);
}

void screen_prepare(int32_t nPant, const TileProp *tileprops, int n_tileprops,
    const uint8_t *map, TileCell *buff, Prefs *prefs,
    const HotSpot *hots, int n_hots) {
    clear_buff(buff);

    int32_t xPant = nPant % prefs->mapW;
    int32_t yPant = nPant / prefs->mapW;
    int32_t mapTileW = prefs->mapW * prefs->screenW;

    for (int32_t y = 0; y < prefs->screenH; y++) {
        int32_t index = (yPant * prefs->screenH + y) * mapTileW + xPant * prefs->screenW;
        for (int32_t x = 0; x < prefs->screenW; x++) {
            int iindex = c_to_idx((int)x, (int)y);
            if (iindex < 0 || iindex >= CUR_BUFF_SIZE) continue;
            uint8_t value = map[index + x];
            int loc = BACK, flags = 0;
            if (value < n_tileprops) {
                loc = tileprops[value].location;
                flags = tileprops[value].flags;
            }
            if (loc == BACK) buff[iindex].layer1 = value;
            else buff[iindex].layer2 = value;
            buff[iindex].anim = (loc == ANIMATED) ? 1 : 0;
            buff[iindex].behaviour = (uint8_t)flags;
            buff[iindex].realMapIndex = index + x;
        }
    }

    if (hots && nPant >= 0 && nPant < n_hots && hots[nPant].s) {
        prefs->hotSpotX = hots[nPant].x << 4;
        prefs->hotSpotY = hots[nPant].y << 4;
    } else {
        prefs->hotSpotX = 999;
        prefs->hotSpotY = 999;
    }
}

void draw_layer1(Screen *s, const Sheet *tiles, const Prefs *prefs,
    const TileCell *buff, int target_layer) {
    int x = 0, y = 0;
    int screenPixelW = 16 * prefs->screenW;
    int idx = 23; /* skip letterbox top row */
    for (int i = 0; i < prefs->screenH; i++) {
        for (int j = 0; j < prefs->screenW; j++) {
            uint8_t id = buff[idx].layer1;
            if (id) screen_blit_tile(s, target_layer, tiles, id,
                prefs->screenPos.x + x, prefs->screenPos.y + y);
            x += 16;
            if (x == screenPixelW) { x = 0; y += 16; }
            idx++;
        }
        idx += 2; /* border columns */
    }
}

void draw_layer2(Screen *s, const Sheet *tiles, const Prefs *prefs,
    const TileCell *buff, int frame, int target_layer) {
    int x = 0, y = 0;
    int screenPixelW = 16 * prefs->screenW;
    int idx = 23;
    for (int i = 0; i < prefs->screenH; i++) {
        for (int j = 0; j < prefs->screenW; j++) {
            int id = buff[idx].layer2;
            if (buff[idx].anim) id += frame & 3;
            if (id) screen_blit_tile(s, target_layer, tiles, id,
                prefs->screenPos.x + x, prefs->screenPos.y + y);
            x += 16;
            if (x == screenPixelW) { x = 0; y += 16; }
            idx++;
        }
        idx += 2;
    }
}
