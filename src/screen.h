#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

#include "types.h"

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Surface SDL_Surface;

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT)

/* Virtual VGA (mirrors lala-js screen.ts):
 * 4 indexed layers (VIDEO + LAYER_1..3), 768-byte palette, presented through
 * an SDL streaming texture (nearest-neighbour upscale). present() always
 * expands VIDEO indices via the *current* palette, so fades animate free. */
typedef struct Screen {
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *tex;
    SDL_Surface *fallsurf;   /* headless fallback (dummy video driver) */
    uint8_t idx[NUM_LAYERS][SCREEN_SIZE];
    uint8_t pal[768];        /* RGB triplets 0-255 */
    uint8_t dark[256];       /* cached darken LUT for filter_box */
    int dark_valid;
} Screen;

int screen_init(Screen *s, const char *title, int scale);
void screen_quit(Screen *s);

/* Layer ops */
void screen_clear_layer(Screen *s, int id);
void screen_copy_layer(Screen *s, int src, int dst);
void screen_put_pixel(Screen *s, int layer, int x, int y, uint8_t ci);
void screen_fill_rect(Screen *s, int layer, int x, int y, int w, int h, uint8_t ci);
void screen_draw_rect(Screen *s, int layer, int x1, int y1, int x2, int y2, uint8_t ci);
/* Darken rect (mirrors DQB filterBox; blend args ignored like JS). */
void screen_filter_box(Screen *s, int layer, int x1, int y1, int x2, int y2);
/* Copy raw indices into a layer with clipping. */
void screen_load_indices(Screen *s, int layer, const uint8_t *data, int w, int h);

/* Blitters (index 0 = transparent, mirrors DQBsetTransPut) */
void screen_blit_tile(Screen *s, int layer, const Sheet *tiles, int tile_idx, int dx, int dy);
void screen_blit_tile_solid(Screen *s, int layer, const Sheet *tiles, int tile_idx, int dx, int dy);
/* 24x24 sprite + per-sprite offset table (SPRPROP.TXT, may be NULL). */
void screen_blit_sprite(Screen *s, int layer, const Sheet *sprs, int spr_idx,
    int dx, int dy, const SprProp *props, int n_props);
/* Rect copy between layers (mirrors DQBput/DQBget). */
void screen_blit_from_layer(Screen *s, int src, int dst,
    int sx, int sy, int sw, int sh, int dx, int dy);

/* Palette (mirrors setPal/getPal/palOff; set_pal scales 6-bit 0-63 like JS) */
void screen_set_pal(Screen *s, const uint8_t pal[768]);
void screen_get_pal(const Screen *s, uint8_t out[768]);
void screen_pal_off(Screen *s);
/* Blocking fades (N frames, ~60fps present each step). */
void screen_fade_to(Screen *s, uint8_t r, uint8_t g, uint8_t b, int frames);
void screen_fade_in(Screen *s, const uint8_t target[768], int frames);

void screen_present(Screen *s);

/* Test accessors */
uint8_t *screen_layer_idx(Screen *s, int id);

#endif
