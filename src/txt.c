#include "txt.h"
#include "assets.h"

#include <stdlib.h>

/* Mirrors JS: text.split(/[\s,]+/); parseInt(tok,10); skip NaN. */
int parse_ints(const char *text, int *out, int cap) {
    int n = 0;
    const char *p = text;
    while (*p && n < cap) {
        /* skip separators: whitespace, comma, semicolon */
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ||
               *p == ',' || *p == ';')
            p++;
        if (!*p) break;
        /* optional sign + digits only (else skip token like NaN) */
        const char *t = p;
        if (*t == '+' || *t == '-') t++;
        if (*t < '0' || *t > '9') {
            while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' &&
                   *p != ',' && *p != ';')
                p++;
            continue;
        }
        int neg = 0;
        if (*p == '+' || *p == '-') { neg = (*p == '-'); p++; }
        long v = 0;
        while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); p++; }
        out[n++] = neg ? (int)-v : (int)v;
        /* skip rest of token (e.g. trailing garbage) */
        while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' &&
               *p != ',' && *p != ';')
            p++;
    }
    return n;
}

int txt_load_ints(const char *path, int *out, int cap) {
    uint8_t *buf = NULL;
    size_t len = 0;
    if (file_slurp(path, &buf, &len) != 0) return -1;
    char *text = (char *)malloc(len + 1);
    if (!text) { free(buf); return -1; }
    for (size_t i = 0; i < len; i++) text[i] = (char)buf[i];
    text[len] = '\0';
    free(buf);
    int n = parse_ints(text, out, cap);
    free(text);
    return n;
}

int tileprops_load(const char *path, TileProp *out, int cap) {
    int *tmp = (int *)malloc(sizeof(int) * (size_t)(cap * 2 > 16 ? cap * 2 : 16));
    if (!tmp) return -1;
    int n = txt_load_ints(path, tmp, cap * 2);
    if (n < 0) { free(tmp); return -1; }
    int pairs = n / 2;
    if (pairs > cap) pairs = cap;
    for (int i = 0; i < pairs; i++) {
        out[i].location = tmp[i * 2];
        out[i].flags = tmp[i * 2 + 1];
    }
    free(tmp);
    return pairs;
}

int sprprops_load(const char *path, SprProp *out, int cap) {
    int *tmp = (int *)malloc(sizeof(int) * (size_t)(cap * 2 > 16 ? cap * 2 : 16));
    if (!tmp) return -1;
    int n = txt_load_ints(path, tmp, cap * 2);
    if (n < 0) { free(tmp); return -1; }
    int pairs = n / 2;
    if (pairs > cap) pairs = cap;
    for (int i = 0; i < pairs; i++) {
        out[i].off_x = tmp[i * 2];
        out[i].off_y = tmp[i * 2 + 1];
    }
    free(tmp);
    return pairs;
}

int sprmap_load(const char *path, int out[SPRMAP_LEN]) {
    for (int i = 0; i < SPRMAP_LEN; i++) out[i] = 0;
    int tmp[SPRMAP_LEN];
    int n = txt_load_ints(path, tmp, SPRMAP_LEN);
    if (n < 0) return -1;
    for (int i = 0; i < n; i++) out[i] = tmp[i];
    return n;
}

int enems_load(const char *path, EnemRaw out[ENEM_PANTS][ENEM_PER_PANT]) {
    static int tmp[ENEM_PANTS * ENEM_PER_PANT * 9];
    int n = txt_load_ints(path, tmp, (int)(sizeof(tmp) / sizeof(tmp[0])));
    if (n < 0) return -1;
    int need = ENEM_PANTS * ENEM_PER_PANT * 9; /* 1080 */
    int p = 0;
    for (int i = 0; i < ENEM_PANTS; i++) {
        for (int j = 0; j < ENEM_PER_PANT; j++) {
            int v[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
            for (int k = 0; k < 9 && p < n; k++) v[k] = tmp[p++];
            out[i][j].x = v[0]; out[i][j].y = v[1];
            out[i][j].x1 = v[2]; out[i][j].y1 = v[3];
            out[i][j].x2 = v[4]; out[i][j].y2 = v[5];
            out[i][j].mx = v[6]; out[i][j].my = v[7];
            out[i][j].t = v[8];
            out[i][j].facing = 0; out[i][j].frame = 0;
            out[i][j].subFrame = 0; out[i][j].sprId = 0;
        }
    }
    return (n >= need) ? 0 : -1;
}

int hotspots_load(const char *path, HotSpotRaw out[HOTSPOT_PANTS]) {
    int tmp[HOTSPOT_PANTS * 4];
    int n = txt_load_ints(path, tmp, (int)(sizeof(tmp) / sizeof(tmp[0])));
    if (n < 0) return -1;
    int p = 0;
    for (int i = 0; i < HOTSPOT_PANTS; i++) {
        int x = p < n ? tmp[p++] : 0;
        int y = p < n ? tmp[p++] : 0;
        int t = p < n ? tmp[p++] : 0;
        if (p < n) p++; /* dummy (chumi) */
        out[i].x = x; out[i].y = y; out[i].t = t; out[i].s = 1;
    }
    return (n >= HOTSPOT_PANTS * 4) ? HOTSPOT_PANTS : -1;
}
