#ifndef FONT_H
#define FONT_H

#include <stddef.h>
#include <stdint.h>

#include "fnt.h"

typedef struct Screen Screen;

/* FNT text rendering (mirrors render.ts drawFntText/dqbPrint/engineRprint).
 * Glyphs are 8 rows MSB-first; advance = widths table. Clipped to 320x200. */
void font_draw_text(Screen *s, int layer, const Font *f,
    const char *text, int x, int y, uint8_t color);

/* Single-colour print (mirrors DQBprint). */
void dqb_print(Screen *s, int layer, const Font *f,
    const char *text, int x, int y, uint8_t color);

/* Shadow (255 at +1,+1) + foreground (254) print (mirrors engineRprint). */
void engine_rprint(Screen *s, int layer, const Font *f,
    const char *text, int x, int y);

/* Zero-padded number (mirrors engineMakeNumber$). Returns buf. */
char *make_number(char *buf, size_t cap, int digits, int value);

#endif
