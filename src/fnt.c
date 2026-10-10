#include "fnt.h"
#include "assets.h"

#include <stdlib.h>
#include <string.h>

#define FNT_GLYPHS 2048
#define FNT_WIDTHS 257
#define FNT_EXPECTED (FNT_GLYPHS + FNT_WIDTHS) /* 2305 */

int fnt_load(const uint8_t *src, size_t len, Font *out) {
    if (!src || !out || len < FNT_EXPECTED) return -1;
    memset(out, 0, sizeof(*out));
    for (int i = 0; i < 256; i++) {
        memcpy(out->glyph[i].rows, src + (size_t)i * 8, 8);
        /* bitmap-derived width: rightmost set column + 1 (0 if empty) */
        int derived = 0;
        for (int col = 7; col >= 0; col--) {
            int hit = 0;
            for (int row = 0; row < 8; row++) {
                if (out->glyph[i].rows[row] & (uint8_t)(0x80u >> col)) { hit = 1; break; }
            }
            if (hit) { derived = col + 1; break; }
        }
        /* stored width wins unless 0 — mirrors JS `storedWidth || charWidth` */
        uint8_t stored = src[FNT_GLYPHS + i];
        out->widths[i] = stored ? stored : (uint8_t)(derived ? derived : 8);
        out->glyph[i].width = out->widths[i];
    }
    return 0;
}

int fnt_load_file(const char *path, Font *out) {
    uint8_t *buf = NULL;
    size_t len = 0;
    if (file_slurp(path, &buf, &len) != 0) return -1;
    int rc = fnt_load(buf, len, out);
    free(buf);
    return rc;
}
