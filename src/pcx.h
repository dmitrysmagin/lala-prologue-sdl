#ifndef PCX_H
#define PCX_H

#include <stddef.h>
#include <stdint.h>

/* 8-bit indexed image decoded from a 256-color PCX file.
 * Mirrors lala-js src/assets/PCXLoader.ts. Caller owns data/palette. */
typedef struct {
    int width;
    int height;
    uint8_t *data;      /* width*height palette indices */
    uint8_t *palette;   /* 768 bytes RGB 0-255, or NULL if absent */
} PcxImage;

/* Decode PCX from memory. Returns 0 on success, nonzero on error. */
int pcx_load(const uint8_t *src, size_t len, PcxImage *out);

/* Load PCX from file. Returns 0 on success, nonzero on error. */
int pcx_load_file(const char *path, PcxImage *out);

void pcx_free(PcxImage *img);

#endif
