#ifndef PREFS_H
#define PREFS_H

#include "types.h"

/* Game constants + player init (mirrors engineInitVals/engineInitPlayer/
 * createPlayer/engineInitGame from prefs.ts). */
void default_prefs(Prefs *p);
void init_player(Player *pl, const Prefs *prefs);
void init_game(const Prefs *prefs, Player *pl);

#endif
