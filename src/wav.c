#include "wav.h"
#include "assets.h"

#include <stdlib.h>
#include <string.h>

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int wav_probe(const uint8_t *src, size_t len, WavInfo *out) {
    if (!src || !out || len < 44) return -1;
    if (memcmp(src, "RIFF", 4) != 0 || memcmp(src + 8, "WAVE", 4) != 0) return -1;
    /* walk chunks starting at 12 (handles extra chunks like JS decodeAudioData) */
    size_t pos = 12;
    int have_fmt = 0;
    WavInfo wi;
    memset(&wi, 0, sizeof(wi));
    while (pos + 8 <= len) {
        const uint8_t *id = src + pos;
        uint32_t sz = rd32(src + pos + 4);
        size_t body = pos + 8;
        if (memcmp(id, "fmt ", 4) == 0) {
            if (body + 16 > len) return -1;
            wi.audio_format = rd16(src + body);
            wi.channels = rd16(src + body + 2);
            wi.sample_rate = (int)rd32(src + body + 4);
            wi.bits_per_sample = rd16(src + body + 14);
            have_fmt = 1;
        } else if (memcmp(id, "data", 4) == 0) {
            wi.data_len = sz;
            if (!have_fmt) return -1;
            *out = wi;
            return 0;
        }
        pos = body + sz + (sz & 1); /* chunks are word-aligned */
    }
    return -1;
}

int wav_probe_file(const char *path, WavInfo *out) {
    uint8_t *buf = NULL;
    size_t len = 0;
    if (file_slurp(path, &buf, &len) != 0) return -1;
    int rc = wav_probe(buf, len, out);
    free(buf);
    return rc;
}
