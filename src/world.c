#include "world.h"

#include <stdlib.h>
#include <string.h>

int world_alloc(WorldBuff *w, int32_t worldW, int32_t worldH) {
    memset(w, 0, sizeof(*w));
    size_t n = (size_t)worldW * (size_t)worldH;
    w->layer1 = (uint8_t *)calloc(n, 1);
    w->layer2 = (uint8_t *)calloc(n, 1);
    w->behaviour = (uint8_t *)calloc(n, 1);
    w->anim = (uint8_t *)calloc(n, 1);
    w->realMapIndex = (uint32_t *)calloc(n, sizeof(uint32_t));
    if (!w->layer1 || !w->layer2 || !w->behaviour || !w->anim || !w->realMapIndex) {
        world_free(w);
        return -1;
    }
    w->worldW = worldW;
    w->worldH = worldH;
    return 0;
}

void world_free(WorldBuff *w) {
    free(w->layer1);
    free(w->layer2);
    free(w->behaviour);
    free(w->anim);
    free(w->realMapIndex);
    memset(w, 0, sizeof(*w));
}

int world_build(WorldBuff *w, const uint8_t *map,
    const TileProp *tileprops, int n_tileprops, const Prefs *prefs) {
    int32_t worldW = prefs->mapW * prefs->screenW;
    int32_t worldH = prefs->mapH * prefs->screenH;
    world_free(w);
    if (world_alloc(w, worldW, worldH) != 0) return -1;

    for (int32_t sy = 0; sy < prefs->mapH; sy++) {
        for (int32_t sx = 0; sx < prefs->mapW; sx++) {
            int32_t baseX = sx * prefs->screenW;
            int32_t baseY = sy * prefs->screenH;
            for (int32_t ty = 0; ty < prefs->screenH; ty++) {
                int32_t mapRow = (sy * prefs->screenH + ty) * worldW + sx * prefs->screenW;
                int32_t wRow = (baseY + ty) * worldW + baseX;
                for (int32_t tx = 0; tx < prefs->screenW; tx++) {
                    int32_t mapIdx = mapRow + tx;
                    int32_t wIdx = wRow + tx;
                    uint8_t value = map[mapIdx];
                    int loc = BACK, flags = 0;
                    if (value < n_tileprops) {
                        loc = tileprops[value].location;
                        flags = tileprops[value].flags;
                    }
                    if (loc == BACK) w->layer1[wIdx] = value;
                    else w->layer2[wIdx] = value;
                    w->anim[wIdx] = (loc == ANIMATED) ? 1 : 0;
                    w->behaviour[wIdx] = (uint8_t)flags;
                    w->realMapIndex[wIdx] = (uint32_t)mapIdx;
                }
            }
        }
    }
    return 0;
}

int world_behav(const WorldBuff *w, int32_t tx, int32_t ty) {
    if (tx < 0 || tx >= w->worldW || ty < 0 || ty >= w->worldH) return 0;
    if (!w->behaviour) return 0;
    return w->behaviour[(size_t)ty * w->worldW + tx];
}

int world_clear_keyhole(WorldBuff *w, uint8_t *map, int32_t tx, int32_t ty) {
    if (tx < 0 || tx >= w->worldW || ty < 0 || ty >= w->worldH) return 0;
    size_t i = (size_t)ty * w->worldW + tx;
    if (w->behaviour[i] != 10) return 0;
    w->layer1[i] = 0;
    w->layer2[i] = 0;
    w->behaviour[i] = 0;
    map[w->realMapIndex[i]] = 0;
    return 1;
}

void convert_enemies_to_world(const Enem src[ENEM_PANTS][ENEM_PER_PANT],
    Enem dst[ENEM_PANTS * ENEM_PER_PANT], const Prefs *prefs) {
    for (int32_t sy = 0; sy < prefs->mapH; sy++) {
        for (int32_t sx = 0; sx < prefs->mapW; sx++) {
            int32_t nPant = sy * prefs->mapW + sx;
            if (nPant < 0 || nPant >= ENEM_PANTS) continue;
            int32_t offX = sx * prefs->screenW * 16;
            int32_t offY = sy * prefs->screenH * 16;
            for (int i = 0; i < ENEM_PER_PANT; i++) {
                Enem e = src[nPant][i];
                e.x += offX; e.y += offY;
                e.x1 += offX; e.y1 += offY;
                e.x2 += offX; e.y2 += offY;
                dst[nPant * ENEM_PER_PANT + i] = e;
            }
        }
    }
}

void convert_hotspots_to_world(const HotSpot *src, HotSpot *dst, int n,
    const Prefs *prefs) {
    for (int32_t sy = 0; sy < prefs->mapH; sy++) {
        for (int32_t sx = 0; sx < prefs->mapW; sx++) {
            int32_t nPant = sy * prefs->mapW + sx;
            if (nPant < 0 || nPant >= n) continue;
            dst[nPant].x = src[nPant].x + sx * prefs->screenW;
            dst[nPant].y = src[nPant].y + sy * prefs->screenH;
            dst[nPant].t = src[nPant].t;
            dst[nPant].s = src[nPant].s;
        }
    }
}

void update_camera(Camera *cam, const Player *pl, int32_t worldPxW, int32_t worldPxH) {
    int32_t px = pl->x >> 6;
    int32_t py = pl->y >> 6;
    cam->x = px + 12 - 160;
    cam->y = py + 12 - 100;
    if (cam->x < 0) cam->x = 0;
    if (cam->y < 0) cam->y = 0;
    if (cam->x > worldPxW - 320) cam->x = worldPxW - 320;
    if (cam->y > worldPxH - 200) cam->y = worldPxH - 200;
}
