#include "scroll_render.h"
#include "screen.h"

void scroll_draw_layer1(Screen *s, const WorldBuff *world, const Camera *cam,
    const Sheet *tiles, int target_layer, int sp_x, int sp_y) {
    int startTX = (int)(cam->x >> 4);
    int startTY = (int)(cam->y >> 4);
    int offX = -(int)(cam->x & 15) + sp_x;
    int offY = -(int)(cam->y & 15) + sp_y;
    for (int row = 0; row <= 13; row++) {
        for (int col = 0; col <= 20; col++) {
            int tx = startTX + col;
            int ty = startTY + row;
            if (tx < 0 || tx >= world->worldW || ty < 0 || ty >= world->worldH) continue;
            uint8_t id = world->layer1[(size_t)ty * world->worldW + tx];
            if (id == 0) continue;
            screen_blit_tile(s, target_layer, tiles, id, offX + col * 16, offY + row * 16);
        }
    }
}

void scroll_draw_layer2(Screen *s, const WorldBuff *world, const Camera *cam,
    const Sheet *tiles, int frame, int target_layer, int sp_x, int sp_y) {
    int startTX = (int)(cam->x >> 4);
    int startTY = (int)(cam->y >> 4);
    int offX = -(int)(cam->x & 15) + sp_x;
    int offY = -(int)(cam->y & 15) + sp_y;
    for (int row = 0; row <= 13; row++) {
        for (int col = 0; col <= 20; col++) {
            int tx = startTX + col;
            int ty = startTY + row;
            if (tx < 0 || tx >= world->worldW || ty < 0 || ty >= world->worldH) continue;
            size_t i = (size_t)ty * world->worldW + tx;
            int id = world->layer2[i];
            if (id == 0) continue;
            if (world->anim[i]) id += frame & 3;
            screen_blit_tile(s, target_layer, tiles, id, offX + col * 16, offY + row * 16);
        }
    }
}

void scroll_draw_sprite(Screen *s, int layer, const Sheet *sprs, int spr_id,
    int world_x, int world_y, const Camera *cam,
    const SprProp *props, int n_props, int sp_x, int sp_y) {
    int sx = world_x - (int)cam->x + sp_x;
    int sy = world_y - (int)cam->y + sp_y;
    if (sx < -24 || sx > 320 || sy < -24 || sy > 200) return;
    int ox = 0, oy = 0;
    if (props && spr_id >= 0 && spr_id < n_props) {
        ox = props[spr_id].off_x;
        oy = props[spr_id].off_y;
    }
    screen_blit_sprite(s, layer, sprs, spr_id, sx - ox, sy - oy, props, n_props);
}

void scroll_draw_hotspots(Screen *s, const Sheet *tiles, const Prefs *prefs,
    const HotSpot *hots, int n_hots, const Camera *cam, int target_layer) {
    for (int i = 0; i < n_hots; i++) {
        const HotSpot *hs = &hots[i];
        if (!hs->s || hs->t == 0) continue;
        int tile = 0;
        if (hs->t == 1) tile = (int)prefs->objectTile;
        else if (hs->t == 2) tile = (int)prefs->keyTile;
        else if (hs->t == 3) tile = (int)prefs->lifeTile;
        if (!tile) continue;
        int hx = (int)(hs->x * 16) - (int)cam->x + prefs->screenPos.x;
        int hy = (int)(hs->y * 16) - (int)cam->y + prefs->screenPos.y;
        if (hx > -16 && hx < 320 && hy > -16 && hy < 200)
            screen_blit_tile(s, target_layer, tiles, tile, hx, hy);
    }
}
