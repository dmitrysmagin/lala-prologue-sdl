#include "pcx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int pcx_load(const uint8_t *src, size_t len, PcxImage *out) {
    if (!src || !out) return -1;
    memset(out, 0, sizeof(*out));
    if (len < 128) return -1;
    if (src[0] != 0x0A) return -1;

    unsigned encoding = src[2];
    /* unsigned bits_per_pixel = src[3]; (always 8 for our assets) */
    unsigned x_min = (unsigned)src[4] | ((unsigned)src[5] << 8);
    unsigned y_min = (unsigned)src[6] | ((unsigned)src[7] << 8);
    unsigned x_max = (unsigned)src[8] | ((unsigned)src[9] << 8);
    unsigned y_max = (unsigned)src[10] | ((unsigned)src[11] << 8);
    unsigned bytes_per_line = (unsigned)src[66] | ((unsigned)src[67] << 8);

    if (x_max < x_min || y_max < y_min) return -1;
    int width = (int)(x_max - x_min + 1);
    int height = (int)(y_max - y_min + 1);
    if (width <= 0 || height <= 0) return -1;
    if (bytes_per_line < (unsigned)width) bytes_per_line = (unsigned)width;

    int has_pal = (len >= 769 && src[len - 769] == 0x0C);
    size_t image_end = has_pal ? len - 769 : len;

    uint8_t *data = (uint8_t *)malloc((size_t)width * (size_t)height);
    if (!data) return -1;
    uint8_t *line = (uint8_t *)malloc(bytes_per_line ? bytes_per_line : (unsigned)width);
    if (!line) { free(data); return -1; }

    size_t pos = 128;
    size_t dst = 0;
    for (int y = 0; y < height; y++) {
        size_t line_pos = 0;
        while (line_pos < bytes_per_line && pos < image_end) {
            uint8_t b = src[pos++];
            size_t count = 1;
            uint8_t value = b;
            if (encoding == 1 && (b & 0xC0) == 0xC0) {
                count = b & 0x3F;
                if (pos >= image_end) break;
                value = src[pos++];
            }
            for (size_t i = 0; i < count && line_pos < bytes_per_line; i++)
                line[line_pos++] = value;
        }
        size_t copy = line_pos < (size_t)width ? line_pos : (size_t)width;
        memcpy(data + dst, line, copy);
        if (copy < (size_t)width)
            memset(data + dst + copy, 0, (size_t)width - copy);
        dst += (size_t)width;
    }
    free(line);

    uint8_t *pal = NULL;
    if (has_pal) {
        pal = (uint8_t *)malloc(768);
        if (!pal) { free(data); return -1; }
        memcpy(pal, src + len - 768, 768);
    }

    out->width = width;
    out->height = height;
    out->data = data;
    out->palette = pal;
    return 0;
}

int pcx_load_file(const char *path, PcxImage *out) {
    if (!path || !out) return -1;
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); return -1; }
    uint8_t *buf = (uint8_t *)malloc((size_t)n);
    if (!buf) { fclose(f); return -1; }
    size_t got = fread(buf, 1, (size_t)n, f);
    fclose(f);
    if (got != (size_t)n) { free(buf); return -1; }
    int rc = pcx_load(buf, got, out);
    free(buf);
    return rc;
}

void pcx_free(PcxImage *img) {
    if (!img) return;
    free(img->data);
    free(img->palette);
    img->data = NULL;
    img->palette = NULL;
}
