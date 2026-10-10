#ifndef FNT_H
#define FNT_H

#include <stddef.h>
#include <stdint.h>

/* DirectQB .FNT loader. Mirrors lala-js FNTLoader.ts (from FONT.ASM):
 *   offset 0:    2048 bytes glyph bitmap (256 chars x 8 rows, MSB-first)
 *   offset 2048: 257 bytes CharLen table (width per char + 1 extra)
 *   total: 2305 bytes, no header. */
typedef struct {
    uint8_t rows[8];
    uint8_t width;
} FntGlyph;

typedef struct {
    FntGlyph glyph[256];
    uint8_t widths[256];
} Font;

int fnt_load(const uint8_t *src, size_t len, Font *out);
int fnt_load_file(const char *path, Font *out);

#endif
