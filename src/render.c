#include "render.h"
#include "font.h"
#include "fnt.h"
#include "screen.h"

#include <string.h>

void render_print_stats(Screen *s, const Player *pl, const Prefs *prefs,
    const Sheet *tiles, const Font *font) {
    const int layer = 1; /* original hardcodes layer 1 */
    int lives = pl->lives >= 0 ? pl->lives : 0;
    screen_blit_tile(s, layer, tiles, prefs->objectTile, 4, 4);
    screen_blit_tile(s, layer, tiles, prefs->keyTile, 4, 20);
    screen_blit_tile(s, layer, tiles, prefs->lifeTile, 4, 36);
    if (font) {
        char nb[16], xb[20];
        xb[0] = 'x'; xb[1] = '\0';
        strcat(xb, make_number(nb, sizeof(nb), 2, pl->objects));
        engine_rprint(s, layer, font, xb, 20, 4);
        xb[0] = 'x'; xb[1] = '\0';
        strcat(xb, make_number(nb, sizeof(nb), 2, pl->keys));
        engine_rprint(s, layer, font, xb, 20, 20);
        xb[0] = 'x'; xb[1] = '\0';
        strcat(xb, make_number(nb, sizeof(nb), 2, lives));
        engine_rprint(s, layer, font, xb, 20, 36);
    } else {
        /* fallback blocks (mirrors JS missing-font path) */
        screen_fill_rect(s, layer, 21, 5, 6, 8, 255);
        screen_fill_rect(s, layer, 20, 4, 6, 8, 15);
        screen_fill_rect(s, layer, 21, 21, 6, 8, 255);
        screen_fill_rect(s, layer, 20, 20, 6, 8, 15);
        screen_fill_rect(s, layer, 21, 37, 6, 8, 255);
        screen_fill_rect(s, layer, 20, 36, 6, 8, 15);
    }
}

void render_draw_hotspots(Screen *s, const Sheet *tiles, const Prefs *prefs,
    const HotSpot *hotspots, int n_pant, int target_layer) {
    if (!hotspots || n_pant < 0) return;
    const HotSpot *hs = &hotspots[n_pant];
    if (!hs->s) return;
    /* hotspot type -> tile mapping (mirrors game-loop hotSpotsTiles table:
     * [unused, objectTile, keyTile, lifeTile]) */
    int tile = 0;
    if (hs->t == 1) tile = prefs->objectTile;
    else if (hs->t == 2) tile = prefs->keyTile;
    else if (hs->t == 3) tile = prefs->lifeTile;
    if (!tile) return;
    screen_blit_tile(s, target_layer, tiles, tile,
        prefs->screenPos.x + hs->x * 16, prefs->screenPos.y + hs->y * 16);
}
