/* Phase 6 tests: title flow (pushed-key events), ending abort path,
 * new speed scancodes + poll, ending asset. Dummy video + audio.
 * Exit 0 = all pass. */
#include "config.h"
#include "assets.h"
#include "game.h"
#include "keyboard.h"
#include "pcx.h"
#include "state_ending.h"
#include "state_title.h"

#include <SDL.h>
#include <stdio.h>
#include <string.h>

static int g_fail = 0, g_pass = 0;

#define CHECK(cond, fmt, ...) do { \
    if (cond) { g_pass++; } \
    else { g_fail++; printf("FAIL %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); } \
} while (0)

static void push_key(int sdl_sc) {
    SDL_Event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = SDL_KEYDOWN;
    ev.key.keysym.scancode = (SDL_Scancode)sdl_sc;
    ev.key.state = SDL_PRESSED;
    ev.key.repeat = 0;
    SDL_PushEvent(&ev);
}

static void push_quit(void) {
    SDL_Event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = SDL_QUIT;
    SDL_PushEvent(&ev);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    /* new speed scancodes */
    CHECK(sdl_to_qb(SDL_SCANCODE_MINUS) == SC_MINUS, "map -");
    CHECK(sdl_to_qb(SDL_SCANCODE_EQUALS) == SC_EQUAL, "map =");
    CHECK(sdl_to_qb(SDL_SCANCODE_KP_MINUS) == SC_MINUS, "map kp-");
    CHECK(sdl_to_qb(SDL_SCANCODE_KP_PLUS) == SC_EQUAL, "map kp+");

    /* speed poll bounds */
    {
        Keyboard k;
        kbd_init(&k);
        double rate = 0.8;
        g_config.game_speed = 0.8;
        kbd_inject(&k, SC_MINUS, 1);
        config_poll_speed(&k, &rate);
        CHECK(rate < 0.8 && g_config.game_speed < 0.8, "speed down");
        kbd_clear(&k);
        kbd_inject(&k, SC_EQUAL, 1);
        config_poll_speed(&k, &rate);
        CHECK(g_config.game_speed == 0.8, "speed back up");
        g_config.game_speed = 0.3;
        kbd_clear(&k);
        kbd_inject(&k, SC_MINUS, 1);
        config_poll_speed(&k, &rate);
        CHECK(g_config.game_speed == 0.3 && rate == 0.3, "speed floor");
        g_config.game_speed = 2.0;
        kbd_clear(&k);
        kbd_inject(&k, SC_EQUAL, 1);
        config_poll_speed(&k, &rate);
        CHECK(g_config.game_speed == 2.0 && rate == 2.0, "speed ceiling");
        g_config.game_speed = 0.8;
    }

    /* ending asset loads */
    {
        char path[512];
        PcxImage img;
        memset(&img, 0, sizeof(img));
        CHECK(resolve_asset("GFX/ENDING.PCX", path, sizeof(path)) != NULL, "ending resolves");
        CHECK(pcx_load_file(path, &img) == 0, "ending decodes");
        CHECK(img.width == 320 && img.height == 200 && img.palette != NULL, "ending 320x200+pal");
        pcx_free(&img);
    }

    /* full game load + title flows */
    Game g;
    CHECK(game_load(&g, "states-test", 1) == 0, "game loads");

    g_config.scroll_enabled = 1;
    push_key(SDL_SCANCODE_LEFT);
    push_key(SDL_SCANCODE_RETURN);
    CHECK(show_title(&g) == 1, "title starts on enter");
    CHECK(g_config.scroll_enabled == 0, "title toggles to flip");

    push_key(SDL_SCANCODE_ESCAPE);
    CHECK(show_title(&g) == 0, "title quits on esc");

    /* ending aborts fast on window close */
    push_quit();
    CHECK(show_ending(&g) == 1, "ending aborts on quit");

    game_free(&g);
    printf("phase6: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
