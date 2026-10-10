#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>

#include "keyboard.h"
#include "types.h"

/* SFX callbacks (keep engine decoupled from audio; userdata = Audio*).
 * Mirrors PlaySfx/StopSfx: slot 1-10, loop flag, freq (11025 = native). */
typedef void (*PlaySfxFn)(int slot, int loop, int freq, void *ud);
typedef void (*StopSfxFn)(int voice, void *ud);

/* One physics tick (mirrors engineMovePlayer). Sets player.attempt on edge. */
void move_player(Keyboard *kbd, TileCell *buff, Player *pl,
    const Prefs *prefs, uint8_t *map, PlaySfxFn play_sfx, void *sfx_ud);

#endif
