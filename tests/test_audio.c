/* Phase 3 tests: SDL2_mixer music (OGG) + SFX slots/voices.
 * Run with SDL_AUDIODRIVER=dummy (silent but fully decoded/played).
 * Exit 0 = all pass. */
#include "assets.h"
#include "audio.h"

#include <SDL.h>
#include <SDL_mixer.h>
#include <stdio.h>

static int g_fail = 0, g_pass = 0;

#define CHECK(cond, fmt, ...) do { \
    if (cond) { g_pass++; } \
    else { g_fail++; printf("FAIL %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); } \
} while (0)

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    Audio a;
    CHECK(audio_init(&a) == 0, "audio_init (dummy driver)");

    /* ---- SFX slots ---- */
    CHECK(audio_load_sfx(&a) == SFX_NSLOTS, "all 10 slots loaded");
    for (int s = 1; s <= SFX_NSLOTS; s++)
        CHECK(audio_sfx_loaded(&a, s), "slot %d loaded", s);
    CHECK(!audio_sfx_loaded(&a, 0) && !audio_sfx_loaded(&a, 11), "OOB slots unloaded");
    /* catalog volumes took effect */
    CHECK(Mix_VolumeChunk(a.chunk[SFX_BOLT], -1) == (int)(0.75f * MIX_MAX_VOLUME),
        "BOLT vol 0.75");
    CHECK(Mix_VolumeChunk(a.chunk[SFX_AMBIENT1], -1) == (int)(0.05f * MIX_MAX_VOLUME),
        "AMBIENT1 vol 0.05");

    /* one-shots across all voices */
    for (int s = 1; s <= 8; s++)
        CHECK(audio_play_slot(&a, s, 0, 11025) >= 0, "play slot %d", s);
    /* ambient loops on voices 5/6, then voice stop (mirrors game-loop exit) */
    CHECK(audio_play_slot(&a, SFX_AMBIENT1, 1, 11025) >= 0, "ambient1 loop");
    CHECK(audio_play_slot(&a, SFX_AMBIENT2, 1, 11025) >= 0, "ambient2 loop");
    audio_stop_voice(&a, 5);
    audio_stop_voice(&a, 6);
    audio_stop_all(&a);
    /* invalid use: no crash */
    CHECK(audio_play_slot(&a, 0, 0, 11025) == -1, "slot 0 rejected");
    CHECK(audio_play_slot(&a, 11, 0, 11025) == -1, "slot 11 rejected");
    audio_stop_voice(&a, 0);
    audio_stop_voice(&a, 7);

    /* ---- music ---- */
    CHECK(audio_load_music(&a, "MUSIC/G66A.ogg") == 0, "title music loads");
    audio_set_music_volume(&a, 0.7f);
    audio_play_music(&a, -1);
    CHECK(audio_music_playing(&a), "music playing");
    audio_fade_out_music(&a, 50);
    CHECK(!audio_music_playing(&a), "fade out stops");
    CHECK(audio_load_music(&a, "MUSIC/DESORUIN.ogg") == 0, "game music loads");
    audio_free_music(&a);
    CHECK(audio_load_music(&a, "MUSIC/MC12.ogg") == 0, "ending music loads");
    audio_free_music(&a);
    /* missing file: graceful */
    CHECK(audio_load_music(&a, "MUSIC/NONEXISTENT.ogg") == -1, "missing music rejected");
    audio_play_music(&a, -1); /* no music: no-op, no crash */
    CHECK(!audio_music_playing(&a), "no music playing");

    audio_quit(&a);
    printf("phase3: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
