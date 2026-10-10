#ifndef RENDER_H
#define RENDER_H

#include "types.h"
#include "fnt.h"

typedef struct Screen Screen;

/* HUD + hotspot rendering (mirrors render.ts enginePrintStats /
 * engineDrawHotSpots). Stats go to hardcoded layer 1 like the original.
 * font may be NULL -> fallback blocks (mirrors JS missing-font path). */
void render_print_stats(Screen *s, const Player *pl, const Prefs *prefs,
    const Sheet *tiles, const Font *font);
void render_draw_hotspots(Screen *s, const Sheet *tiles, const Prefs *prefs,
    const HotSpot *hotspots, int n_pant, int target_layer);

#endif
