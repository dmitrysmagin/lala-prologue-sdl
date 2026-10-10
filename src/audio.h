#ifndef AUDIO_H
#define AUDIO_H

/* Audio via SDL2_mixer (mirrors music-player.ts + sound-effects.ts).
 * Music: pre-converted OGGs (DESORUIN/G66A/MC12), looped, volume + fade.
 * SFX: 10 original WAVs in DQB slots 1-10, 6 voices, per-slot volume.
 * All functions are safe to call when audio is muted/failed (no-ops).
 * SDL audio lifecycle: audio_init inits SDL audio; SDL_Quit stays with
 * screen/main (audio_quit only closes mixer). */

#include <SDL_mixer.h>

/* Semantic SFX slots — mirrors LALA.BAS:47-56 / SFX_SLOT */
#define SFX_BOLT 1
#define SFX_HIT 2
#define SFX_JUMP 3
#define SFX_KEY 4
#define SFX_LIFE 5
#define SFX_OBJECT 6
#define SFX_PINCHE 7
#define SFX_AH 8
#define SFX_AMBIENT1 9
#define SFX_AMBIENT2 10
#define SFX_NSLOTS 10
#define SFX_NVOICES 6

typedef struct {
    int ready;                          /* 0 = muted fallback */
    Mix_Music *music;
    Mix_Chunk *chunk[SFX_NSLOTS + 1];   /* 1-based slots */
    int music_vol;                      /* 0-128 */
} Audio;

int audio_init(Audio *a);               /* 0 ok (ready), -1 muted fallback */
int audio_load_sfx(Audio *a);           /* loads all 10, returns # loaded */
int audio_load_music(Audio *a, const char *rel); /* e.g. "MUSIC/G66A.ogg" */
void audio_free_music(Audio *a);
void audio_play_music(Audio *a, int loops);      /* loops: -1 = infinite */
void audio_stop_music(Audio *a);
void audio_set_music_volume(Audio *a, float v);  /* 0.0-1.0 */
int audio_music_playing(Audio *a);
/* Blocking fade (like screen fades): ramps out over ms, then stops and
 * restores the previous volume (mirrors MusicPlayer.fadeOut). */
void audio_fade_out_music(Audio *a, int ms);
/* Play slot (1-10). loop: 0/1. freq: kept for engine API compat (11025 =
 * native); SDL_mixer has no per-voice resample so pitch shift is ignored.
 * Returns channel or -1. */
int audio_play_slot(Audio *a, int slot, int loop, int freq);
void audio_stop_voice(Audio *a, int voice);      /* 1-6 (QB DQBstopVoice) */
void audio_stop_all(Audio *a);
void audio_quit(Audio *a);
int audio_sfx_loaded(const Audio *a, int slot);

#endif
