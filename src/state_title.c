#include "state_title.h"
#include "assets.h"
#include "audio.h"
#include "config.h"
#include "font.h"
#include "game.h"
#include "keyboard.h"
#include "pcx.h"
#include "screen.h"

#include <SDL.h>
#include <stdio.h>
#include <string.h>

int show_title(Game *g) {
    Screen *s = &g->scr;
    Keyboard *kbd = &g->kbd;

    /* ---- load TITLE.PCX (own palette, like JS) ---- */
    char path[512];
    PcxImage img;
    memset(&img, 0, sizeof(img));
    if (resolve_asset("GFX/TITLE.PCX", path, sizeof(path)) &&
        pcx_load_file(path, &img) == 0) {
        if (img.palette) screen_set_pal(s, img.palette);
        screen_clear_layer(s, LAYER_3);
        screen_clear_layer(s, LAYER_1);
        screen_clear_layer(s, VIDEO);
        screen_load_indices(s, LAYER_3, img.data, img.width, img.height);
    } else {
        fprintf(stderr, "show_title: TITLE.PCX missing, fallback box\n");
        screen_fill_rect(s, LAYER_3, 0, 0, 320, 200, 1);
        screen_draw_rect(s, LAYER_3, 0, 0, 319, 199, 15);
    }
    screen_copy_layer(s, LAYER_3, LAYER_1);
    screen_copy_layer(s, LAYER_1, VIDEO);

    uint8_t pal[768];
    screen_get_pal(s, pal);
    screen_fade_in(s, pal, 12);

    /* ---- music (fire-and-forget like JS) ---- */
    if (g->have_audio) {
        if (audio_load_music(&g->audio, "MUSIC/G66A.ogg") == 0) {
            audio_set_music_volume(&g->audio, 0.7f);
            audio_play_music(&g->audio, -1);
        }
    }

    kbd_clear(kbd);
    int title_frame = 0;
    int res = -1;
    while (res < 0) {
        kbd_poll(kbd);
        title_frame++;

        if (kbd_just_pressed(kbd, SC_LEFT) || kbd_just_pressed(kbd, SC_RIGHT)) {
            g_config.scroll_enabled = !g_config.scroll_enabled;
            printf("[title] game mode: %s\n",
                g_config.scroll_enabled ? "scrolling" : "flip-screen");
        }
        if (kbd_just_pressed(kbd, SC_ENTER) || kbd_is_down(kbd, SC_SPACE)) res = 1;
        if (kbd->quit || kbd_is_down(kbd, SC_ESC)) res = 0;
        config_poll_speed(kbd, &g_config.game_speed);
        kbd_clear_just(kbd);
        if (res >= 0) break;

        /* redraw over clean background (filterBox mutates) */
        screen_copy_layer(s, LAYER_3, LAYER_1);
        if (g->have_font) {
            const char *mode = g_config.scroll_enabled ?
                "GAME MODE: SCROLLING" : "GAME MODE: FLIP-SCREEN";
            dqb_print(s, LAYER_1, &g->font, mode, 88, 120, 0);
            dqb_print(s, LAYER_1, &g->font, mode, 87, 119, 1);
            if ((title_frame % 96) < 48) {
                screen_filter_box(s, LAYER_1, 80, 136, 239, 151);
                dqb_print(s, LAYER_1, &g->font, "PRESS ENTER TO PLAY", 89, 141, 0);
                dqb_print(s, LAYER_1, &g->font, "PRESS ENTER TO PLAY", 88, 140, 1);
            }
        }
        screen_copy_layer(s, LAYER_1, VIDEO);
        screen_present(s);
        SDL_Delay(16);
    }

    pcx_free(&img);
    return res;
}
