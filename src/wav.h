#ifndef WAV_H
#define WAV_H

#include <stddef.h>
#include <stdint.h>

/* Minimal WAV probe. NOTE: lala-js WAVLoader.ts assumes 16-bit PCM, but the
 * real SFX are 8-bit 11025 Hz mono — this probe handles both (reports
 * actual fmt/channels/rate/bits + data length). Full decode/playback is
 * phase 3 (SDL2_mixer); here we only validate headers. */
typedef struct {
    int audio_format;   /* 1 = PCM */
    int channels;
    int sample_rate;
    int bits_per_sample;
    uint32_t data_len;  /* bytes of sample data */
} WavInfo;

int wav_probe(const uint8_t *src, size_t len, WavInfo *out);
int wav_probe_file(const char *path, WavInfo *out);

#endif
