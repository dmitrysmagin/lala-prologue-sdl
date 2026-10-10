#ifndef SCROLL_RENDER_H
#define SCROLL_RENDER_H

#include "types.h"
#include "world.h"

typedef struct Screen Screen;

/* Tile layers from world buffer with camera offset (mirrors
 * scrollDrawLayer1/2: 21x14 tiles for sub-tile coverage). */
void scroll_draw_layer1(Screen *s, const WorldBuff *world, const Camera *cam,
    const Sheet *tiles, int target_layer, int sp_x, int sp_y);
void scroll_draw_layer2(Screen *s, const WorldBuff *world, const Camera *cam,
    const Sheet *tiles, int frame, int target_layer, int sp_x, int sp_y);
/* World-absolute sprite -> screen coords (mirrors scrollDrawSprite). */
void scroll_draw_sprite(Screen *s, int layer, const Sheet *sprs, int spr_id,
    int world_x, int world_y, const Camera *cam,
    const SprProp *props, int n_props, int sp_x, int sp_y);
/* World hotspots (tile coords) with culling (mirrors scroll loop block). */
void scroll_draw_hotspots(Screen *s, const Sheet *tiles, const Prefs *prefs,
    const HotSpot *hots, int n_hots, const Camera *cam, int target_layer);

#endif
