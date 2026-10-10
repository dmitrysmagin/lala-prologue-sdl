#ifndef ENEMIES_H
#define ENEMIES_H

#include <stdint.h>

#include "player.h"
#include "types.h"

/* Move all enemies on screen nPant + platform riding + damage (mirrors
 * engineMoveEnems). enems is [n_pants][per_pant]; guards nPant. */
void move_enems(Enem *enems, int n_pants, int per_pant,
    TileCell *buff, const Prefs *prefs, Player *pl, int32_t nPant,
    PlaySfxFn play_sfx, void *sfx_ud);

/* Animation frames for player + enemies (mirrors ENGINE.BAS:20-53 block in
 * game-loop.ts). sprmap is SPRMAP_LEN ints. */
void calc_player_frame(Player *pl, const int *sprmap, int sprmap_len);
void calc_enemy_frames(Enem *enems, int n_pants, int per_pant,
    int32_t nPant, const int *sprmap, int sprmap_len, int n_enems);

#endif
