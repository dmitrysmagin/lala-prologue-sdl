#ifndef SCROLL_ENEMIES_H
#define SCROLL_ENEMIES_H

#include "player.h"
#include "types.h"
#include "world.h"

/* World-absolute enemies, flat array (mirrors scrollMoveEnemies).
 * Proximity cull (400px), patrol bounce, world-edge clamp. */
void scroll_move_enemies(Enem *enemies, int n_enemies, WorldBuff *world,
    const Prefs *prefs, Player *pl, PlaySfxFn play_sfx, void *sfx_ud);

#endif
