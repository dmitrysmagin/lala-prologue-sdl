/* Phase 2 demo: TITLE.PCX via layers, FNT text, keyboard input, palette fade. */
#include "assets.h"
#include "fnt.h"
#include "font.h"
#include "keyboard.h"
#include "pcx.h"
#include "screen.h"

#include <SDL.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    Screen scr;
    if (screen_init(&scr, "lala-sdl phase2", 3) != 0) return 1;

    char path[512];
    if (!resolve_asset("GFX/TITLE.PCX", path, sizeof(path))) {
        fprintf(stderr, "phase2: TITLE.PCX not found (hint: run `make assets`)\n");
        screen_quit(&scr);
        return 1;
    }
    PcxImage title;
    memset(&title, 0, sizeof(title));
    if (pcx_load_file(path, &title) != 0) {
        fprintf(stderr, "phase2: could not decode %s\n", path);
        screen_quit(&scr);
        return 1;
    }
    printf("phase2: loaded %s (%dx%d, pal=%s)\n", path, title.width, title.height,
        title.palette ? "yes" : "no");

    uint8_t pal[768];
    memset(pal, 0, sizeof(pal));
    if (title.palette) memcpy(pal, title.palette, 768);
    screen_load_indices(&scr, LAYER_3, title.data, title.width, title.height);
    screen_copy_layer(&scr, LAYER_3, VIDEO);

    Font font;
    int have_font = 0;
    if (resolve_asset("GFX/lala.fnt", path, sizeof(path)) &&
        fnt_load_file(path, &font) == 0) {
        have_font = 1;
    }

    screen_fade_in(&scr, pal, 10);
    if (have_font) engine_rprint(&scr, VIDEO, &font, "PRESS ENTER", 120, 180);

    Keyboard kbd;
    kbd_init(&kbd);
    int running = 1;
    while (running) {
        kbd_poll(&kbd);
        if (kbd.quit || kbd_just_pressed(&kbd, SC_ENTER) || kbd_is_down(&kbd, SC_ESC))
            running = 0;
        kbd_clear_just(&kbd);
        screen_present(&scr);
        SDL_Delay(16);
    }

    pcx_free(&title);
    screen_quit(&scr);
    printf("phase2: bye\n");
    return 0;
}
