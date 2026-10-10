#include "game.h"
#include "assets.h"
#include "config.h"
#include "engine_flip.h"
#include "map.h"
#include "map_load.h"
#include "prefs.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void game_play_sfx(int slot, int loop, int freq, void *ud) {
    audio_play_slot((Audio *)ud, slot, loop, freq);
}

static void game_stop_sfx(int voice, void *ud) {
    audio_stop_voice((Audio *)ud, voice);
}

/* Blit 320-wide backdrop PCX at y-offset (mirrors blitBackdropToLayer). */
static void blit_backdrop(Screen *s, int layer, const Sheet *back, int at_y) {
    if (!back->data) return;
    uint8_t *dst_base = screen_layer_idx(s, layer);
    for (int y = 0; y < back->height; y++) {
        int py = at_y + y;
        if (py < 0 || py >= SCREEN_HEIGHT) continue;
        int w = back->width < SCREEN_WIDTH ? back->width : SCREEN_WIDTH;
        memcpy(dst_base + (size_t)py * SCREEN_WIDTH,
            back->data + (size_t)y * back->width, (size_t)w);
    }
}

int game_load(Game *g, const char *title, int scale) {
    memset(g, 0, sizeof(*g));
    default_prefs(&g->prefs);
    init_player(&g->player, &g->prefs);
    init_game(&g->prefs, &g->player);

    if (screen_init(&g->scr, title ? title : "lala-sdl", scale) != 0) return -1;

    char path[512];
    /* tile + sprite properties */
    if (!resolve_asset("MAP/TILEPROP.TXT", path, sizeof(path))) goto fail;
    g->n_tileprops = tileprops_load(path, g->tileprops, TILEPROP_MAX);
    if (g->n_tileprops <= 0) goto fail;
    g->prefs.numTiles = g->n_tileprops;
    if (!resolve_asset("GFX/SPRPROP.TXT", path, sizeof(path))) goto fail;
    g->n_sprprops = sprprops_load(path, g->sprprops, SPRPROP_MAX);
    if (g->n_sprprops <= 0) goto fail;
    g->prefs.numSprites = g->n_sprprops;
    if (!resolve_asset("GFX/SPRMAP.TXT", path, sizeof(path))) goto fail;
    if (sprmap_load(path, g->sprmap) < 0) goto fail;
    /* map + enemies + hotspots */
    if (map_load(&g->map, NULL) != 0) goto fail;
    if (!resolve_asset("MAP/ENEMS.TXT", path, sizeof(path))) goto fail;
    if (enems_load(path, g->enems) != 0) goto fail;
    if (!resolve_asset("MAP/HOTSPOTS.TXT", path, sizeof(path))) goto fail;
    if (hotspots_load(path, g->hots) != HOTSPOT_PANTS) goto fail;
    /* sheets */
    if (!resolve_asset("GFX/TILESET.PCX", path, sizeof(path))) goto fail;
    if (pcx_load_file(path, &g->tileset_img) != 0) goto fail;
    if (!resolve_asset("GFX/SPRSET.PCX", path, sizeof(path))) goto fail;
    if (pcx_load_file(path, &g->spriteset_img) != 0) goto fail;
    if (!resolve_asset("GFX/BACKDROP.PCX", path, sizeof(path))) goto fail;
    if (pcx_load_file(path, &g->backdrop_img) != 0) goto fail;
    g->tiles.width = g->tileset_img.width;
    g->tiles.height = g->tileset_img.height;
    g->tiles.data = g->tileset_img.data;
    g->sprs.width = g->spriteset_img.width;
    g->sprs.height = g->spriteset_img.height;
    g->sprs.data = g->spriteset_img.data;
    g->back.width = g->backdrop_img.width;
    g->back.height = g->backdrop_img.height;
    g->back.data = g->backdrop_img.data;

    /* palette (force HUD colours like main.ts) */
    if (g->tileset_img.palette) screen_set_pal(&g->scr, g->tileset_img.palette);
    {
        uint8_t pal[768];
        screen_get_pal(&g->scr, pal);
        pal[254 * 3] = 255; pal[254 * 3 + 1] = 255; pal[254 * 3 + 2] = 255;
        pal[255 * 3] = 0; pal[255 * 3 + 1] = 0; pal[255 * 3 + 2] = 0;
        screen_set_pal(&g->scr, pal);
    }

    /* font (non-fatal) */
    if (resolve_asset("GFX/lala.fnt", path, sizeof(path)) &&
        fnt_load_file(path, &g->font) == 0)
        g->have_font = 1;

    /* audio (non-fatal, silent fallback) */
    g->have_audio = (audio_init(&g->audio) == 0);
    if (g->have_audio) audio_load_sfx(&g->audio);

    kbd_init(&g->kbd);
    srand((unsigned)SDL_GetTicks());
    return 0;

fail:
    fprintf(stderr, "game_load: failed (missing/undecodable asset)\n");
    game_free(g);
    return -1;
}

void game_free(Game *g) {
    if (g->have_audio) {
        audio_quit(&g->audio);
        g->have_audio = 0;
    }
    pcx_free(&g->tileset_img);
    pcx_free(&g->spriteset_img);
    pcx_free(&g->backdrop_img);
    free(g->map);
    g->map = NULL;
    screen_quit(&g->scr);
}

int game_run_flip(Game *g, int32_t flag) {
    Engine *e = &g->eng;
    memset(e, 0, sizeof(*e));
    flip_engine_init(e);
    e->scr = &g->scr;
    e->kbd = &g->kbd;
    e->prefs = &g->prefs;
    e->player = &g->player;
    e->tileprops = g->tileprops;
    e->n_tileprops = g->n_tileprops;
    e->sprprops = g->sprprops;
    e->n_sprprops = g->n_sprprops;
    e->sprmap = g->sprmap;
    e->tiles = &g->tiles;
    e->sprs = &g->sprs;
    e->back = &g->back;
    e->map = g->map;
    e->enems = g->enems;
    e->hots = g->hots;
    e->n_hots = HOTSPOT_PANTS;
    e->buff = g->buff;
    e->font = g->have_font ? &g->font : NULL;
    e->audio = &g->audio;
    e->play_sfx = game_play_sfx;
    e->stop_sfx = game_stop_sfx;
    e->sfx_ud = &g->audio;
    e->game_speed = g_config.game_speed;

    /* backdrop -> LAYER_3 (QB: clear + loadImage + copy + drawLayer1 in prepare) */
    screen_clear_layer(&g->scr, LAYER_3);
    blit_backdrop(&g->scr, LAYER_3, &g->back, (int)g->prefs.screenPos.y);

    return (int)e->loop(e, flag);
}
