#include "assets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *kPrefixes[] = {
    "dist/assets/",
    "assets/",
    "../lala-js/public/",
    "",
};

const char *resolve_asset(const char *rel, char *buf, size_t cap) {
    if (!rel || !buf || cap == 0) return NULL;
    for (unsigned i = 0; i < sizeof(kPrefixes) / sizeof(kPrefixes[0]); i++) {
        size_t n = strlen(kPrefixes[i]) + strlen(rel) + 1;
        if (n > cap) continue;
        strcpy(buf, kPrefixes[i]);
        strcat(buf, rel);
        FILE *f = fopen(buf, "rb");
        if (f) { fclose(f); return buf; }
    }
    return NULL;
}

int file_slurp(const char *path, uint8_t **out, size_t *len_out) {
    if (!path || !out) return -1;
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return -1; }
    uint8_t *buf = (uint8_t *)malloc(n > 0 ? (size_t)n : 1);
    if (!buf) { fclose(f); return -1; }
    size_t got = n > 0 ? fread(buf, 1, (size_t)n, f) : 0;
    fclose(f);
    if (got != (size_t)n) { free(buf); return -1; }
    *out = buf;
    if (len_out) *len_out = got;
    return 0;
}
