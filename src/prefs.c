#include "prefs.h"

#include <string.h>

void default_prefs(Prefs *p) {
    memset(p, 0, sizeof(*p));
    p->mapW = 6;
    p->mapH = 6;
    p->screenW = 20;
    p->screenH = 12;
    p->screenPos.x = 0;
    p->screenPos.y = 4;
    p->gMaxVy = 192;
    p->g = 12;
    p->jumpVyInitial = 32;
    p->jumpVyMax = 192;
    p->jumpIncr = 24;
    p->walkVxMax = 128;
    p->walkAx = 16;
    p->walkFr = 24;
    p->iniPant = 24;
    p->iniTX = 2;
    p->iniTY = 1;
    p->nEnems = 4;
    p->enemPlat = 4;
    p->boltTile = 22;
    p->lifeTile = 34;
    p->objectTile = 35;
    p->keyTile = 36;
    p->initialLives = 15;
    p->maxObjs = 15;
    p->refill = 1;
    p->hotSpotX = 999;
    p->hotSpotY = 999;
    p->bgL1 = 9;
    p->bgL2 = 10;
}

void init_player(Player *pl, const Prefs *prefs) {
    int32_t lives = pl->lives, keys = pl->keys;
    memset(pl, 0, sizeof(*pl));
    pl->x = prefs->iniTX * 16 * 64;
    pl->y = prefs->iniTY * 16 * 64;
    pl->state = STATENORMAL;
    /* lives/keys kept (caller resets via init_game); objects zeroed (mirrors QB) */
    pl->lives = lives;
    pl->keys = keys;
    pl->objects = 0;
}

void init_game(const Prefs *prefs, Player *pl) {
    pl->keys = 0;
    pl->lives = prefs->initialLives;
    pl->objects = 0;
    pl->gameOver = 0;
}
