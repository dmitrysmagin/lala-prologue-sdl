/* Phase 2 tests: layers/blitters/palette, font text, render HUD, keyboard.
 * Needs SDL (screen/keyboard) — run with SDL_VIDEODRIVER=dummy.
 * Exit 0 = all pass. */
#include "assets.h"
#include "fnt.h"
#include "font.h"
#include "keyboard.h"
#include "pcx.h"
#include "render.h"
#include "screen.h"
#include "txt.h"

#include <SDL.h>
#include <stdio.h>
#include <string.h>

static int g_fail = 0, g_pass = 0;

#define CHECK(cond, fmt, ...) do { \
    if (cond) { g_pass++; } \
    else { g_fail++; printf("FAIL %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); } \
} while (0)

static uint8_t at(Screen *s, int layer, int x, int y) {
    return screen_layer_idx(s, layer)[(size_t)y * SCREEN_WIDTH + x];
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    Screen s;
    CHECK(screen_init(&s, "phase2-test", 1) == 0, "screen_init (dummy video)");

    /* ---- layer ops ---- */
    screen_clear_layer(&s, VIDEO);
    CHECK(at(&s, VIDEO, 0, 0) == 0, "clear zeroes");
    screen_put_pixel(&s, VIDEO, 10, 20, 42);
    CHECK(at(&s, VIDEO, 10, 20) == 42, "put_pixel");
    screen_put_pixel(&s, VIDEO, -1, 0, 7);
    screen_put_pixel(&s, VIDEO, 320, 200, 7);
    CHECK(at(&s, VIDEO, 0, 0) == 0, "put_pixel clips OOB");
    screen_fill_rect(&s, VIDEO, 0, 0, 4, 2, 9);
    CHECK(at(&s, VIDEO, 3, 1) == 9 && at(&s, VIDEO, 4, 1) == 0, "fill_rect");
    screen_fill_rect(&s, VIDEO, -2, -2, 4, 4, 11); /* clipped */
    CHECK(at(&s, VIDEO, 0, 0) == 11 && at(&s, VIDEO, 2, 1) == 9, "fill_rect clips");
    screen_draw_rect(&s, LAYER_1, 0, 0, 2, 2, 5);
    CHECK(at(&s, LAYER_1, 0, 0) == 5 && at(&s, LAYER_1, 1, 1) == 0, "draw_rect");
    screen_copy_layer(&s, VIDEO, LAYER_2);
    CHECK(at(&s, LAYER_2, 10, 20) == 42, "copy_layer");

    /* ---- blitters: 2-tile sheet side by side (tile0: transparent row0 + idx7,
       tile1: solid idx9) ---- */
    static uint8_t tiledata[32 * 16];
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 32; x++)
            tiledata[y * 32 + x] = (uint8_t)(x < 16 ? (y == 0 ? 0 : 7) : 9);
    Sheet tiles = {32, 16, tiledata};
    screen_clear_layer(&s, LAYER_1);
    screen_blit_tile(&s, LAYER_1, &tiles, 0, 0, 0);
    CHECK(at(&s, LAYER_1, 0, 0) == 0, "tile transparent idx0 skipped");
    CHECK(at(&s, LAYER_1, 5, 5) == 7, "tile body blitted");
    screen_blit_tile_solid(&s, LAYER_1, &tiles, 0, 16, 0);
    CHECK(at(&s, LAYER_1, 16, 0) == 0, "solid blit writes idx0");
    screen_blit_tile(&s, LAYER_1, &tiles, 99, 0, 0);          /* OOB tile: no-op */
    CHECK(at(&s, LAYER_1, 0, 1) == 7, "OOB tile no-op");
    screen_blit_tile(&s, LAYER_1, &tiles, 1, 312, 192);       /* clipped dst */
    CHECK(at(&s, LAYER_1, 319, 199) == 9, "tile clips at edge");

    /* ---- sprites: 24x24 + SPRPROP offsets ---- */
    static uint8_t sprdata[48 * 24];
    memset(sprdata, 3, sizeof(sprdata));
    sprdata[0] = 0; /* transparent corner */
    Sheet sprs = {48, 24, sprdata};
    SprProp props[2] = {{4, 8}, {0, 0}};
    screen_clear_layer(&s, LAYER_1);
    screen_blit_sprite(&s, LAYER_1, &sprs, 0, 10, 10, props, 2);
    CHECK(at(&s, LAYER_1, 15, 18) == 3, "sprite offset applied");
    CHECK(at(&s, LAYER_1, 14, 18) == 0, "sprite transparent corner kept");
    screen_blit_sprite(&s, LAYER_1, &sprs, 1, 0, 0, NULL, 0);
    CHECK(at(&s, LAYER_1, 0, 0) == 3, "sprite NULL props, no offset");

    /* ---- blit_from_layer ---- */
    screen_clear_layer(&s, LAYER_2);
    screen_blit_from_layer(&s, LAYER_1, LAYER_2, 14, 18, 4, 4, 100, 50);
    CHECK(at(&s, LAYER_2, 100, 50) == 3, "blit_from_layer copies");

    /* ---- palette + filter_box + fades ---- */
    uint8_t pal[768];
    memset(pal, 0, sizeof(pal));
    pal[5 * 3] = 200; /* bright red at 5, black at 0 */
    screen_set_pal(&s, pal);
    uint8_t got[768];
    screen_get_pal(&s, got);
    CHECK(got[5 * 3] == 200 && got[0] == 0, "set/get pal roundtrip");
    screen_clear_layer(&s, VIDEO);
    screen_put_pixel(&s, VIDEO, 7, 7, 5);
    screen_filter_box(&s, VIDEO, 0, 0, 10, 10);
    CHECK(at(&s, VIDEO, 7, 7) == 0, "filter_box darkens red->black (tie->first)");
    CHECK(at(&s, VIDEO, 20, 20) == 0, "filter_box outside rect untouched");
    /* 6-bit palette scaling */
    uint8_t pal6[768];
    memset(pal6, 0, sizeof(pal6));
    pal6[1 * 3] = 63; pal6[1 * 3 + 1] = 31;
    screen_set_pal(&s, pal6);
    screen_get_pal(&s, got);
    CHECK(got[3] == 255 && got[4] == 125, "6-bit pal scaled (63->255,31->125)");
    screen_pal_off(&s);
    screen_get_pal(&s, got);
    CHECK(got[100] == 0, "pal_off blacks out");
    screen_set_pal(&s, pal);
    screen_fade_to(&s, 0, 0, 0, 2);
    screen_get_pal(&s, got);
    CHECK(got[5 * 3] == 0, "fade_to black");
    screen_fade_in(&s, pal, 2);
    screen_get_pal(&s, got);
    CHECK(got[5 * 3] == 200, "fade_in restores");

    /* ---- font ---- */
    char fpath[512];
    CHECK(resolve_asset("GFX/lala.fnt", fpath, sizeof(fpath)) != NULL, "font resolves");
    Font f;
    CHECK(fnt_load_file(fpath, &f) == 0, "font loads");
    screen_clear_layer(&s, VIDEO);
    font_draw_text(&s, VIDEO, &f, "A", 10, 10, 44);
    /* glyph 'A' row1 = 0x3C -> pixels gx 2..5 set */
    CHECK(at(&s, VIDEO, 12, 11) == 44, "font draws glyph bits");
    CHECK(at(&s, VIDEO, 10, 11) == 0, "font respects zero bits");
    dqb_print(&s, VIDEO, &f, "A", 30, 10, 45);
    CHECK(at(&s, VIDEO, 32, 11) == 45, "dqb_print colour");
    screen_clear_layer(&s, VIDEO);
    engine_rprint(&s, VIDEO, &f, "A", 10, 10);
    CHECK(at(&s, VIDEO, 12, 11) == 254, "rprint fg");
    CHECK(at(&s, VIDEO, 14, 12) == 255, "rprint shadow offset");
    char nb[16];
    CHECK(strcmp(make_number(nb, sizeof(nb), 2, 5), "05") == 0, "make_number pads");
    CHECK(strcmp(make_number(nb, sizeof(nb), 2, 15), "15") == 0, "make_number exact");
    CHECK(strcmp(make_number(nb, sizeof(nb), 2, 123), "123") == 0, "make_number overflow");

    /* ---- render: stats + hotspots ---- */
    Prefs prefs;
    memset(&prefs, 0, sizeof(prefs));
    prefs.screenPos.x = 0; prefs.screenPos.y = 4;
    prefs.objectTile = 1; prefs.keyTile = 1; prefs.lifeTile = 1;
    Player pl;
    memset(&pl, 0, sizeof(pl));
    pl.objects = 5; pl.keys = 2; pl.lives = 7;
    screen_clear_layer(&s, LAYER_1);
    render_print_stats(&s, &pl, &prefs, &tiles, &f);
    CHECK(at(&s, LAYER_1, 4, 4) == 9, "stats object icon");
    CHECK(at(&s, LAYER_1, 4, 20) == 9, "stats key icon");
    CHECK(at(&s, LAYER_1, 4, 36) == 9, "stats life icon");
    screen_clear_layer(&s, LAYER_1);
    render_print_stats(&s, &pl, &prefs, &tiles, NULL);
    CHECK(at(&s, LAYER_1, 20, 4) == 15, "stats fallback text block");
    HotSpot hs[2] = {{1, 2, 1, 1}, {0, 0, 0, 0}};
    screen_clear_layer(&s, LAYER_1);
    render_draw_hotspots(&s, &tiles, &prefs, hs, 0, LAYER_1);
    CHECK(at(&s, LAYER_1, 16, 36) == 9, "hotspot tile at (16,36)");
    screen_clear_layer(&s, LAYER_1);
    render_draw_hotspots(&s, &tiles, &prefs, hs, 1, LAYER_1);
    CHECK(at(&s, LAYER_1, 16, 36) == 0, "inactive hotspot skipped");

    /* ---- keyboard ---- */
    Keyboard k;
    kbd_init(&k);
    CHECK(!kbd_is_down(&k, SC_LEFT), "kbd initially up");
    kbd_inject(&k, SC_LEFT, 1);
    CHECK(kbd_is_down(&k, SC_LEFT) && kbd_just_pressed(&k, SC_LEFT), "kbd inject down+just");
    kbd_clear_just(&k);
    CHECK(kbd_is_down(&k, SC_LEFT) && !kbd_just_pressed(&k, SC_LEFT), "kbd clear_just");
    kbd_inject(&k, SC_LEFT, 0);
    CHECK(!kbd_is_down(&k, SC_LEFT), "kbd release");
    kbd_inject(&k, SC_ENTER, 1);
    kbd_clear(&k);
    CHECK(!kbd_is_down(&k, SC_ENTER), "kbd clear");
    CHECK(sdl_to_qb(SDL_SCANCODE_LEFT) == SC_LEFT, "map Left");
    CHECK(sdl_to_qb(SDL_SCANCODE_RIGHT) == SC_RIGHT, "map Right");
    CHECK(sdl_to_qb(SDL_SCANCODE_UP) == SC_UP, "map Up");
    CHECK(sdl_to_qb(SDL_SCANCODE_DOWN) == SC_DOWN, "map Down");
    CHECK(sdl_to_qb(SDL_SCANCODE_LCTRL) == SC_CTRL, "map LCtrl");
    CHECK(sdl_to_qb(SDL_SCANCODE_RCTRL) == SC_CTRL, "map RCtrl");
    CHECK(sdl_to_qb(SDL_SCANCODE_RETURN) == SC_ENTER, "map Enter");
    CHECK(sdl_to_qb(SDL_SCANCODE_W) == SC_W, "map W");
    CHECK(sdl_to_qb(SDL_SCANCODE_E) == SC_E, "map E");
    CHECK(sdl_to_qb(SDL_SCANCODE_R) == SC_R, "map R");
    CHECK(sdl_to_qb(SDL_SCANCODE_ESCAPE) == SC_ESC, "map Esc");
    CHECK(sdl_to_qb(SDL_SCANCODE_SPACE) == SC_SPACE, "map Space");
    CHECK(sdl_to_qb(SDL_SCANCODE_F1) == -1, "unmapped -> -1");
    kbd_poll(&k); /* no events: must not crash, quit stays clear */
    CHECK(!k.quit, "no quit without events");

    screen_quit(&s);
    printf("phase2: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
