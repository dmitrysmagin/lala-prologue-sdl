#ifndef GAME_H
#define GAME_H

#include "audio.h"
#include "engine_if.h"
#include "fnt.h"
#include "keyboard.h"
#include "pcx.h"
#include "screen.h"
#include "txt.h"
#include "types.h"
#include "world.h"

/* Game owns all loaded data + devices. game_run_flip builds an Engine view
 * over it and runs the flip-screen loop (flag 1 = play, 0 = title demo).
 * game_run_scroll runs the smooth-scrolling engine (always gameplay).
 * Flip returns 0 (exit), -1 (win), -2 (loss), -3 (Enter in title mode);
 * scroll returns 0/-1/-2. */
typedef struct Game {
    Prefs prefs;
    Player player;
    TileProp tileprops[TILEPROP_MAX];
    int n_tileprops;
    SprProp sprprops[SPRPROP_MAX];
    int n_sprprops;
    int sprmap[SPRMAP_LEN];
    PcxImage tileset_img, spriteset_img, backdrop_img;
    Sheet tiles, sprs, back;
    uint8_t *map;
    Enem enems[ENEM_PANTS][ENEM_PER_PANT];
    HotSpot hots[HOTSPOT_PANTS];
    TileCell buff[CUR_BUFF_SIZE];
    ScrollState sc;
    /* pristine snapshots (engines mutate map/enemies/hotspots per run) */
    uint8_t *orig_map;
    Enem orig_enems[ENEM_PANTS][ENEM_PER_PANT];
    HotSpot orig_hots[HOTSPOT_PANTS];
    Font font;
    int have_font;
    Audio audio;
    int have_audio;
    Screen scr;
    Keyboard kbd;
    Engine eng;
} Game;

int game_load(Game *g, const char *title, int scale);
void game_free(Game *g);
/* Restore pristine map/enemies/hotspots + fresh player (per-run reset). */
void game_reset_run(Game *g);
/* Tileset palette + forced HUD colors (call before gameplay fade-in). */
void game_apply_palette(Game *g);
int game_run_flip(Game *g, int32_t flag);
int game_run_scroll(Game *g);

#endif
