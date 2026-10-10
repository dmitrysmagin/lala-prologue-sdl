/* LaLa SDL — Title -> Play -> Ending outer loop (mirrors main.ts).
 * Usage: lala.exe [flip|scroll] (seed engine; LEFT/RIGHT on title toggles).
 * Title: arrows toggle engine, ENTER/SPACE starts, ESC quits.
 * Game: arrows/Ctrl move+jump, W+E+R cheat, -/= speed, D debug (scroll), ESC quits. */
#include "audio.h"
#include "config.h"
#include "game.h"
#include "state_ending.h"
#include "state_title.h"

#include <SDL.h>
#include <stdio.h>
#include <string.h>

static void clear_all_layers(Game *g) {
    screen_clear_layer(&g->scr, 0);
    screen_clear_layer(&g->scr, 1);
    screen_clear_layer(&g->scr, 2);
    screen_clear_layer(&g->scr, 3);
}

int main(int argc, char **argv) {
    g_config.scroll_enabled = 1; /* JS default */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "flip") == 0) g_config.scroll_enabled = 0;
        else if (strcmp(argv[i], "scroll") == 0) g_config.scroll_enabled = 1;
    }

    Game g;
    if (game_load(&g, "LaLa SDL", 3) != 0) {
        fprintf(stderr, "main: game_load failed (hint: run `make assets`)\n");
        return 1;
    }
    printf("main: engine=%s tiles=%d sprites=%d\n",
        g_config.scroll_enabled ? "scroll" : "flip",
        g.n_tileprops, g.n_sprprops);

    for (;;) {
        /* ---- title ---- */
        if (!show_title(&g)) break;
        printf("main: start (%s engine)\n",
            g_config.scroll_enabled ? "scroll" : "flip");

        if (g.have_audio) audio_fade_out_music(&g.audio, 250);
        screen_fade_to(&g.scr, 0, 0, 0, 8);
        kbd_clear(&g.kbd);
        clear_all_layers(&g);

        /* ---- game ---- */
        game_apply_palette(&g);
        screen_fade_in(&g.scr, g.tileset_img.palette, 10);
        if (g.have_audio && audio_load_music(&g.audio, "MUSIC/DESORUIN.ogg") == 0) {
            audio_set_music_volume(&g.audio, 0.7f);
            audio_play_music(&g.audio, -1);
        }
        int res = g_config.scroll_enabled ? game_run_scroll(&g) : game_run_flip(&g, 1);
        printf("main: run returned %d\n", res);

        if (g.have_audio) audio_fade_out_music(&g.audio, 300);
        screen_fade_to(&g.scr, 0, 0, 0, 10);
        if (g.have_audio) audio_stop_music(&g.audio);
        kbd_clear(&g.kbd);

        /* ---- ending on win ---- */
        if (res == -1) {
            if (show_ending(&g)) break; /* quit requested */
            screen_fade_to(&g.scr, 0, 0, 0, 10);
        }
    }

    game_free(&g);
    printf("main: bye\n");
    return 0;
}
