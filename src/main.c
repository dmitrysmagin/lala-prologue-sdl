/* Phase 4 demo: playable flip-screen game (no title/ending yet — phase 6).
 * Controls: arrows/Ctrl jump, W+E+R cheat, ESC quits. */
#include "audio.h"
#include "game.h"

#include <SDL.h>
#include <stdio.h>

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    Game g;
    if (game_load(&g, "lala-sdl phase4 (flip)", 3) != 0) {
        fprintf(stderr, "phase4: game_load failed\n");
        return 1;
    }
    printf("phase4: tiles=%d sprites=%d map=8640 start pant=%d\n",
        g.n_tileprops, g.n_sprprops, (int)g.prefs.iniPant);

    if (g.have_audio && audio_load_music(&g.audio, "MUSIC/DESORUIN.ogg") == 0) {
        audio_set_music_volume(&g.audio, 0.7f);
        audio_play_music(&g.audio, -1);
    }
    screen_fade_in(&g.scr, g.tileset_img.palette, 10);

    int res = game_run_flip(&g, 1);
    const char *name = res == -1 ? "WIN" : res == -2 ? "LOSS" :
        res == -3 ? "ENTER" : "EXIT";
    printf("phase4: loop returned %d (%s) obj=%d keys=%d lives=%d\n",
        res, name, (int)g.player.objects, (int)g.player.keys, (int)g.player.lives);

    if (g.have_audio) {
        audio_fade_out_music(&g.audio, 250);
        audio_stop_all(&g.audio);
    }
    screen_fade_to(&g.scr, 0, 0, 0, 10);
    game_free(&g);
    printf("phase4: bye\n");
    return 0;
}
