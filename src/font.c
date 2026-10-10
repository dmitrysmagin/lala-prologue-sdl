#include "font.h"
#include "screen.h"

#include <stdio.h>
#include <string.h>

void font_draw_text(Screen *s, int layer, const Font *f,
    const char *text, int x, int y, uint8_t color) {
    if (!f || !text) return;
    int cx = x;
    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        const FntGlyph *g = &f->glyph[*p];
        int gw = g->width ? g->width : 8;
        for (int gy = 0; gy < 8; gy++) {
            int py = y + gy;
            if (py < 0 || py >= SCREEN_HEIGHT) continue;
            uint8_t row = g->rows[gy];
            for (int gx = 0; gx < gw && gx < 8; gx++) {
                if (row & (uint8_t)(0x80u >> gx))
                    screen_put_pixel(s, layer, cx + gx, py, color);
            }
        }
        cx += gw;
    }
}

void dqb_print(Screen *s, int layer, const Font *f,
    const char *text, int x, int y, uint8_t color) {
    font_draw_text(s, layer, f, text, x, y, color);
}

void engine_rprint(Screen *s, int layer, const Font *f,
    const char *text, int x, int y) {
    font_draw_text(s, layer, f, text, x + 1, y + 1, 255);
    font_draw_text(s, layer, f, text, x, y, 254);
}

char *make_number(char *buf, size_t cap, int digits, int value) {
    if (!buf || cap == 0) return buf;
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "%d", value < 0 ? 0 : value);
    size_t len = strlen(tmp);
    size_t pos = 0;
    while ((int)(len + pos) < digits && pos + 1 < cap) buf[pos++] = '0';
    for (size_t i = 0; i < len && pos + 1 < cap; i++) buf[pos++] = tmp[i];
    buf[pos] = '\0';
    return buf;
}
