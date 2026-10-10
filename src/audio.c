#include "audio.h"
#include "assets.h"

#include <SDL.h>
#include <stdio.h>
#include <string.h>

/* Mirrors SFX_CATALOG: file, QB voice, default volume. */
static const struct {
    const char *file;
    int voice;
    float vol;
} kCat[SFX_NSLOTS + 1] = {
    {NULL, 0, 0.0f},
    {"BOLT.WAV", 2, 0.75f},
    {"HIT.WAV", 4, 0.75f},
    {"JUMP.WAV", 1, 0.75f},
    {"KEY.WAV", 4, 0.75f},
    {"LIFE.WAV", 4, 0.75f},
    {"OBJECT.WAV", 4, 0.75f},
    {"PINCHE.WAV", 4, 0.75f},
    {"AH.WAV", 3, 0.75f},
    {"AMBIENT1.WAV", 5, 0.05f},
    {"AMBIENT2.WAV", 6, 0.05f},
};

static int slot_volume(int slot) {
    return (int)(kCat[slot].vol * MIX_MAX_VOLUME);
}

int audio_init(Audio *a) {
    memset(a, 0, sizeof(*a));
    a->music_vol = (int)(0.7f * MIX_MAX_VOLUME); /* JS default 0.7 */
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        SDL_Log("audio: SDL_Init audio failed: %s (muted)", SDL_GetError());
        return -1;
    }
    Mix_Init(MIX_INIT_OGG);
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        SDL_Log("audio: Mix_OpenAudio failed: %s (muted)", Mix_GetError());
        return -1;
    }
    Mix_AllocateChannels(8); /* voices 1-6 -> channels 0-5, plus spare */
    a->ready = 1;
    return 0;
}

int audio_load_sfx(Audio *a) {
    int n = 0;
    for (int slot = 1; slot <= SFX_NSLOTS; slot++) {
        char rel[64], path[512];
        snprintf(rel, sizeof(rel), "SFX/%s", kCat[slot].file);
        if (!a->ready || !resolve_asset(rel, path, sizeof(path))) {
            SDL_Log("audio: missing %s", rel);
            continue;
        }
        Mix_Chunk *c = Mix_LoadWAV(path);
        if (!c) {
            SDL_Log("audio: Mix_LoadWAV %s: %s", path, Mix_GetError());
            continue;
        }
        Mix_VolumeChunk(c, slot_volume(slot));
        a->chunk[slot] = c;
        n++;
    }
    return n;
}

int audio_load_music(Audio *a, const char *rel) {
    audio_free_music(a);
    if (!a->ready || !rel) return -1;
    char path[512];
    if (!resolve_asset(rel, path, sizeof(path))) {
        SDL_Log("audio: missing %s", rel);
        return -1;
    }
    a->music = Mix_LoadMUS(path);
    if (!a->music) {
        SDL_Log("audio: Mix_LoadMUS %s: %s", path, Mix_GetError());
        return -1;
    }
    return 0;
}

void audio_free_music(Audio *a) {
    if (a->music) {
        Mix_HaltMusic();
        Mix_FreeMusic(a->music);
        a->music = NULL;
    }
}

void audio_play_music(Audio *a, int loops) {
    if (!a->ready || !a->music) return;
    Mix_VolumeMusic(a->music_vol);
    if (Mix_PlayMusic(a->music, loops) != 0)
        SDL_Log("audio: Mix_PlayMusic: %s", Mix_GetError());
}

void audio_stop_music(Audio *a) {
    if (!a->ready) return;
    Mix_HaltMusic();
}

void audio_set_music_volume(Audio *a, float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    a->music_vol = (int)(v * MIX_MAX_VOLUME);
    if (a->ready) Mix_VolumeMusic(a->music_vol);
}

int audio_music_playing(Audio *a) {
    if (!a->ready) return 0;
    return Mix_PlayingMusic();
}

void audio_fade_out_music(Audio *a, int ms) {
    if (!a->ready || !Mix_PlayingMusic()) return;
    int saved = a->music_vol;
    if (ms < 0) ms = 0;
    Mix_FadeOutMusic(ms);
    if (ms > 0) SDL_Delay((Uint32)ms);
    Mix_HaltMusic(); /* guaranteed stop (mirrors JS fadeOut -> stop()) */
    audio_set_music_volume(a, saved / (float)MIX_MAX_VOLUME);
}

int audio_play_slot(Audio *a, int slot, int loop, int freq) {
    (void)freq; /* documented: pitch shift ignored (no mixer resample) */
    if (!a->ready || slot < 1 || slot > SFX_NSLOTS || !a->chunk[slot]) return -1;
    int chan = kCat[slot].voice - 1; /* voice 1-6 -> channel 0-5 */
    Mix_HaltChannel(chan);           /* steal voice (mirrors stopVoice) */
    Mix_Volume(chan, slot_volume(slot));
    return Mix_PlayChannel(chan, a->chunk[slot], loop ? -1 : 0);
}

void audio_stop_voice(Audio *a, int voice) {
    if (!a->ready || voice < 1 || voice > SFX_NVOICES) return;
    Mix_HaltChannel(voice - 1);
}

void audio_stop_all(Audio *a) {
    if (!a->ready) return;
    Mix_HaltChannel(-1);
    Mix_HaltMusic();
}

void audio_quit(Audio *a) {
    audio_stop_all(a);
    for (int i = 1; i <= SFX_NSLOTS; i++) {
        if (a->chunk[i]) {
            Mix_FreeChunk(a->chunk[i]);
            a->chunk[i] = NULL;
        }
    }
    audio_free_music(a);
    if (a->ready) {
        Mix_CloseAudio();
        Mix_Quit();
        a->ready = 0;
    }
}

int audio_sfx_loaded(const Audio *a, int slot) {
    if (slot < 1 || slot > SFX_NSLOTS) return 0;
    return a->chunk[slot] != NULL;
}
