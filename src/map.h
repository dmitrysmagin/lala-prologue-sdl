#ifndef MAP_H
#define MAP_H

#include "types.h"

typedef struct Screen Screen;

/* Screen preparation + layer draws (mirrors map.ts engineScreenPrepare /
 * engineScreenDrawLayer1/2). Guards nPant against the 30-entry
 * enem/hotspot tables (screens 30-35 have none, like JS `?.` chains). */
static inline int c_to_idx(int x, int y) {
    int yy = 1 + y;
    return 1 + x + (yy << 4) + (yy << 2) + (yy << 1); /* 1+x+(y+1)*22 */
}

void clear_buff(TileCell *buff);
void screen_prepare(int32_t nPant, const TileProp *tileprops, int n_tileprops,
    const uint8_t *map, TileCell *buff, Prefs *prefs,
    const HotSpot *hots, int n_hots);
void draw_layer1(Screen *s, const Sheet *tiles, const Prefs *prefs,
    const TileCell *buff, int target_layer);
void draw_layer2(Screen *s, const Sheet *tiles, const Prefs *prefs,
    const TileCell *buff, int frame, int target_layer);

#endif
