#include "state_ending.h"
#include "assets.h"
#include "audio.h"
#include "font.h"
#include "game.h"
#include "keyboard.h"
#include "pcx.h"
#include "screen.h"

#include <SDL.h>
#include <stdio.h>
#include <string.h>

static const char *kPass[2][4] = {
    {"YOU DID IT!", "GOT ALL THE", "POTIONS...", "WELL DONE!"},
    {"BUT THERE'S", "STILL MUCH", "TO DO!", "SEE YOU SOON!"},
};

/* Pumped frame wait: presents VIDEO, drains input, honors window close. */
static int wait_frames(Game *g, int n) {
    for (int i = 0; i < n; i++) {
        kbd_poll(&g->kbd);
        if (g->kbd.quit) return 1;
        screen_present(&g->scr);
        SDL_Delay(16);
    }
    return 0;
}

int show_ending(Game *g) {
    Screen *s = &g->scr;

    char path[512];
    PcxImage img;
    memset(&img, 0, sizeof(img));
    screen_clear_layer(s, VIDEO);
    screen_pal_off(s);
    if (resolve_asset("GFX/ENDING.PCX", path, sizeof(path)) &&
        pcx_load_file(path, &img) == 0) {
        if (img.palette) screen_set_pal(s, img.palette);
        screen_load_indices(s, LAYER_3, img.data, img.width, img.height);
    }
    screen_copy_layer(s, LAYER_3, LAYER_1);

    if (g->have_audio) {
        if (audio_load_music(&g->audio, "MUSIC/MC12.ogg") == 0) {
            audio_set_music_volume(&g->audio, 0.7f);
            audio_play_music(&g->audio, -1);
        }
    }

    screen_copy_layer(s, LAYER_1, VIDEO);
    screen_present(s);

    if (wait_frames(g, 32)) goto abort;

    for (int k = 0; k < 2; k++) {
        screen_copy_layer(s, LAYER_3, LAYER_1);
        screen_filter_box(s, LAYER_1, 16, 16, 135, 95);
        screen_draw_rect(s, LAYER_1, 16, 16, 135, 95, 254);

        int y = 24;
        for (int i = 0; i < 4; i++) {
            const char *text = kPass[k][i];
            int x = 24;
            for (const char *c = text; *c; c++) {
                char ch[2] = {*c, '\0'};
                if (g->have_font) {
                    dqb_print(s, LAYER_1, &g->font, ch, x - 1, y - 1, 255);
                    dqb_print(s, LAYER_1, &g->font, ch, x, y, 254);
                }
                if (wait_frames(g, 8)) goto abort;
                screen_copy_layer(s, LAYER_1, VIDEO);
                screen_present(s);
                x += 8;
            }
            y += 16;
            if (i == 2) y += 8;
        }
        if (wait_frames(g, 120)) goto abort;
    }

    /* wait for any key */
    kbd_clear(&g->kbd);
    for (;;) {
        kbd_poll(&g->kbd);
        if (g->kbd.quit) goto abort;
        {
            int any = kbd_is_down(&g->kbd, SC_SPACE);
            for (int sc = 0; sc < 256 && !any; sc++)
                if (kbd_just_pressed(&g->kbd, sc)) any = 1;
            if (any) break;
        }
        kbd_clear_just(&g->kbd);
        SDL_Delay(16);
    }

    if (g->have_audio) audio_stop_music(&g->audio);
    screen_clear_layer(s, VIDEO);
    pcx_free(&img);
    return 0;

abort:
    if (g->have_audio) audio_stop_music(&g->audio);
    pcx_free(&img);
    return 1;
}
