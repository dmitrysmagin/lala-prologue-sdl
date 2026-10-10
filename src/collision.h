#ifndef COLLISION_H
#define COLLISION_H

#include <stdint.h>

#include "types.h"

/* Bounding-box enemy/player check: +-14 px (mirrors engineDetectCollision).
 * enemas is [ENEM_PANTS][ENEM_PER_PANT]; guards nPant/i. */
int detect_collision(int i, int32_t nPant, const Enem *enems, int n_pants,
    int per_pant, const Player *pl);
/* Keyhole clear at tile (xx,yy): behaviour==10 -> wipe cell + map byte.
 * Returns 1 if opened. */
int detect_keyhole(TileCell *buff, uint8_t *map, int xx, int yy);

#endif
