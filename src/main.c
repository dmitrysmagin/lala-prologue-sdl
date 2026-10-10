/* Phase 5 demo: playable game, both engines (no title/ending yet — phase 6).
 * Usage: lala.exe [flip|scroll] (default: scroll, mirrors lala-js config).
 * Controls: arrows/Ctrl jump, W+E+R cheat, D debug overlay (scroll), ESC quits. */
#include "audio.h"
#include "config.h"
#include "game.h"

#include <SDL.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    g_config.scroll_enabled = 1; /* JS default */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "flip") == 0) g_config.scroll_enabled = 0;
        else if (strcmp(argv[i], "scroll") == 0) g_config.scroll_enabled = 1;
    }

    Game g;
    if (game_load(&g, g_config.scroll_enabled ? "lala-sdl phase5 (scroll)" : "lala-sdl phase5 (flip)", 3) != 0) {
        fprintf(stderr, "phase5: game_load failed\n");
        return 1;
    }
    printf("phase5: engine=%s tiles=%d sprites=%d start pant=%d\n",
        g_config.scroll_enabled ? "scroll" : "flip",
        g.n_tileprops, g.n_sprprops, (int)g.prefs.iniPant);

    if (g.have_audio && audio_load_music(&g.audio, "MUSIC/DESORUIN.ogg") == 0) {
        audio_set_music_volume(&g.audio, 0.7f);
        audio_play_music(&g.audio, -1);
    }
    screen_fade_in(&g.scr, g.tileset_img.palette, 10);

    int res = g_config.scroll_enabled ? game_run_scroll(&g) : game_run_flip(&g, 1);
    const char *name = res == -1 ? "WIN" : res == -2 ? "LOSS" :
        res == -3 ? "ENTER" : "EXIT";
    printf("phase5: loop returned %d (%s) obj=%d keys=%d lives=%d\n",
        res, name, (int)g.player.objects, (int)g.player.keys, (int)g.player.lives);

    if (g.have_audio) {
        audio_fade_out_music(&g.audio, 250);
        audio_stop_all(&g.audio);
    }
    screen_fade_to(&g.scr, 0, 0, 0, 10);
    game_free(&g);
    printf("phase5: bye\n");
    return 0;
}
