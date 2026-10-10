#ifndef ENGINE_IF_H
#define ENGINE_IF_H

/* Dual-engine interface (PLAN-SDL.md §3): pure C structs + function pointers.
 * engine_flip.c implements "flip"; engine_scroll.c (phase 5) implements
 * "scroll" on the same contract. Result codes mirror engineDoGame:
 * 0 = exit (ESC), -1 = win, -2 = loss, -3 = Enter in title mode. */
#include <stdint.h>

#include "audio.h"
#include "fnt.h"
#include "keyboard.h"
#include "player.h"
#include "screen.h"
#include "txt.h"
#include "types.h"

typedef struct Engine Engine;
struct Engine {
    const char *name;
    void (*prepare)(Engine *e, int32_t nPant);
    void (*move_player)(Engine *e);
    void (*move_enems)(Engine *e);
    void (*calc_frames)(Engine *e);
    void (*draw)(Engine *e, int32_t frame);
    int32_t (*loop)(Engine *e, int32_t flag);

    /* shared context (owned by Game, never malloc'd per frame) */
    Screen *scr;
    Keyboard *kbd;
    Prefs *prefs;
    Player *player;
    TileProp *tileprops;
    int n_tileprops;
    SprProp *sprprops;
    int n_sprprops;
    int *sprmap;
    Sheet *tiles;
    Sheet *sprs;
    Sheet *back;
    uint8_t *map;
    Enem (*enems)[ENEM_PER_PANT]; /* [ENEM_PANTS][ENEM_PER_PANT] */
    HotSpot *hots;
    int n_hots;
    TileCell *buff;               /* flip: CUR_BUFF_SIZE cells */
    const Font *font;             /* may be NULL -> text fallbacks */
    Audio *audio;                 /* may be unready -> silent */
    PlaySfxFn play_sfx;
    StopSfxFn stop_sfx;
    void *sfx_ud;
    double game_speed;
    int32_t nPant;                /* loop state */
    int32_t frame;
    int32_t mode;                 /* draw mode: 1 = stats HUD, 0 = title overlay */
    void *scroll;                 /* phase 5: scroll-only state */
};

#endif
