#ifndef SCROLL_PLAYER_H
#define SCROLL_PLAYER_H

#include "keyboard.h"
#include "player.h"
#include "types.h"
#include "world.h"

/* World-bound physics, no screen transitions (mirrors scrollMovePlayer).
 * Clamps to world bounds instead of setting attempt. */
void scroll_move_player(Keyboard *kbd, WorldBuff *world, Player *pl,
    const Prefs *prefs, uint8_t *map, PlaySfxFn play_sfx, void *sfx_ud);

#endif
