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

/* Game owns all loaded data + devices. game_run_flip builds an Engine view
 * over it and runs the flip-screen loop (flag 1 = play, 0 = title demo).
 * Returns 0 (exit), -1 (win), -2 (loss), -3 (Enter in title mode). */
typedef struct {
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
int game_run_flip(Game *g, int32_t flag);

#endif
