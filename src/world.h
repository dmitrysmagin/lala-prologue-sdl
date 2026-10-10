#ifndef WORLD_H
#define WORLD_H

#include <stdint.h>

#include "txt.h"
#include "types.h"

/* Full-map tile buffer (mirrors scroll-types.ts WorldBuff).
 * worldW = mapW*screenW (120), worldH = mapH*screenH (72). */
typedef struct {
    uint8_t *layer1;      /* background tiles */
    uint8_t *layer2;      /* foreground/animated tiles */
    uint8_t *behaviour;   /* tile flags 0..10 */
    uint8_t *anim;        /* animated flag */
    uint32_t *realMapIndex; /* back-reference to flat map byte */
    int32_t worldW, worldH;
} WorldBuff;

/* Scroll-run state (Engine.scroll points here; owned by Game). */
typedef struct {
    WorldBuff world;
    Camera cam;
    Enem enemies[ENEM_PANTS * ENEM_PER_PANT]; /* flat, world-absolute px */
    HotSpot hots[HOTSPOT_PANTS];              /* world-absolute tiles */
} ScrollState;

int world_alloc(WorldBuff *w, int32_t worldW, int32_t worldH);
void world_free(WorldBuff *w);
/* Build from flat map + tile properties (mirrors buildWorldBuff). */
int world_build(WorldBuff *w, const uint8_t *map,
    const TileProp *tileprops, int n_tileprops, const Prefs *prefs);
/* OOB-safe behaviour lookup (OOB -> 0). */
int world_behav(const WorldBuff *w, int32_t tx, int32_t ty);
/* Keyhole clear in world + flat map. Returns 1 if opened. */
int world_clear_keyhole(WorldBuff *w, uint8_t *map, int32_t tx, int32_t ty);

/* Per-screen [30][4] (pixel coords) -> flat world-absolute (mirrors JS). */
void convert_enemies_to_world(const Enem src[ENEM_PANTS][ENEM_PER_PANT],
    Enem dst[ENEM_PANTS * ENEM_PER_PANT], const Prefs *prefs);
/* Per-screen hotspots (tile coords) -> world-absolute tiles. */
void convert_hotspots_to_world(const HotSpot *src, HotSpot *dst, int n,
    const Prefs *prefs);

/* Follow player, clamp to world pixel bounds (mirrors updateCamera). */
void update_camera(Camera *cam, const Player *pl, int32_t worldPxW, int32_t worldPxH);

#endif
