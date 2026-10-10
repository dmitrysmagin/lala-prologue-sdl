#include "screen.h"

#include <SDL.h>
#include <string.h>

static int valid_layer(int id) { return id >= 0 && id < NUM_LAYERS; }

int screen_init(Screen *s, const char *title, int scale) {
    memset(s, 0, sizeof(*s));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return -1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    s->win = SDL_CreateWindow(title ? title : "lala-sdl",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH * scale, SCREEN_HEIGHT * scale, SDL_WINDOW_SHOWN);
    if (!s->win) { SDL_Log("CreateWindow: %s", SDL_GetError()); SDL_Quit(); return -1; }

    s->ren = SDL_CreateRenderer(s->win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!s->ren) s->ren = SDL_CreateRenderer(s->win, -1, SDL_RENDERER_SOFTWARE);
    if (!s->ren) {
        /* headless fallback (e.g. dummy video driver): render to memory */
        s->fallsurf = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_WIDTH, SCREEN_HEIGHT,
            32, SDL_PIXELFORMAT_ARGB8888);
        if (s->fallsurf) s->ren = SDL_CreateSoftwareRenderer(s->fallsurf);
    }
    if (!s->ren) {
        SDL_Log("CreateRenderer: %s", SDL_GetError());
        if (s->fallsurf) SDL_FreeSurface(s->fallsurf);
        SDL_DestroyWindow(s->win);
        SDL_Quit();
        return -1;
    }
    SDL_RenderSetLogicalSize(s->ren, SCREEN_WIDTH, SCREEN_HEIGHT);

    s->tex = SDL_CreateTexture(s->ren, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!s->tex) {
        SDL_Log("CreateTexture: %s", SDL_GetError());
        SDL_DestroyRenderer(s->ren);
        if (s->fallsurf) SDL_FreeSurface(s->fallsurf);
        SDL_DestroyWindow(s->win);
        SDL_Quit();
        return -1;
    }
    return 0;
}

void screen_quit(Screen *s) {
    if (s->tex) SDL_DestroyTexture(s->tex);
    if (s->ren) SDL_DestroyRenderer(s->ren);
    if (s->fallsurf) SDL_FreeSurface(s->fallsurf);
    if (s->win) SDL_DestroyWindow(s->win);
    SDL_Quit();
    s->tex = NULL;
    s->ren = NULL;
    s->fallsurf = NULL;
    s->win = NULL;
}

void screen_clear_layer(Screen *s, int id) {
    if (!valid_layer(id)) return;
    memset(s->idx[id], 0, SCREEN_SIZE);
}

void screen_copy_layer(Screen *s, int src, int dst) {
    if (!valid_layer(src) || !valid_layer(dst) || src == dst) return;
    memcpy(s->idx[dst], s->idx[src], SCREEN_SIZE);
}

void screen_put_pixel(Screen *s, int layer, int x, int y, uint8_t ci) {
    if (!valid_layer(layer)) return;
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    s->idx[layer][(size_t)y * SCREEN_WIDTH + x] = ci;
}

void screen_fill_rect(Screen *s, int layer, int x, int y, int w, int h, uint8_t ci) {
    if (!valid_layer(layer) || w <= 0 || h <= 0) return;
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > SCREEN_WIDTH ? SCREEN_WIDTH : x + w;
    int y1 = y + h > SCREEN_HEIGHT ? SCREEN_HEIGHT : y + h;
    if (x0 >= x1 || y0 >= y1) return; /* fully off-screen */
    for (int py = y0; py < y1; py++)
        memset(s->idx[layer] + (size_t)py * SCREEN_WIDTH + x0, ci, (size_t)(x1 - x0));
}

void screen_draw_rect(Screen *s, int layer, int x1, int y1, int x2, int y2, uint8_t ci) {
    int xa = x1 < x2 ? x1 : x2, xb = x1 < x2 ? x2 : x1;
    int ya = y1 < y2 ? y1 : y2, yb = y1 < y2 ? y2 : y1;
    screen_fill_rect(s, layer, xa, ya, xb - xa + 1, 1, ci);
    screen_fill_rect(s, layer, xa, yb, xb - xa + 1, 1, ci);
    if (yb - ya >= 2) {
        screen_fill_rect(s, layer, xa, ya + 1, 1, yb - ya - 1, ci);
        screen_fill_rect(s, layer, xb, ya + 1, 1, yb - ya - 1, ci);
    }
}

/* Rebuild darken LUT: dark[i] = palette index nearest to pal[i]/2. */
static void rebuild_dark(Screen *s) {
    for (int i = 0; i < 256; i++) {
        int r = s->pal[i * 3] >> 1, g = s->pal[i * 3 + 1] >> 1, b = s->pal[i * 3 + 2] >> 1;
        int best = 0, best_dist = 0x7FFFFFFF;
        for (int j = 0; j < 256; j++) {
            int dr = s->pal[j * 3] - r, dg = s->pal[j * 3 + 1] - g, db = s->pal[j * 3 + 2] - b;
            int d = dr * dr + dg * dg + db * db;
            if (d < best_dist) { best_dist = d; best = j; }
        }
        s->dark[i] = (uint8_t)best;
    }
    s->dark_valid = 1;
}

void screen_filter_box(Screen *s, int layer, int x1, int y1, int x2, int y2) {
    if (!valid_layer(layer)) return;
    if (!s->dark_valid) rebuild_dark(s);
    int xa = x1 < x2 ? x1 : x2, xb = x1 < x2 ? x2 : x1;
    int ya = y1 < y2 ? y1 : y2, yb = y1 < y2 ? y2 : y1;
    if (xa < 0) xa = 0;
    if (ya < 0) ya = 0;
    if (xb >= SCREEN_WIDTH) xb = SCREEN_WIDTH - 1;
    if (yb >= SCREEN_HEIGHT) yb = SCREEN_HEIGHT - 1;
    for (int py = ya; py <= yb; py++) {
        uint8_t *row = s->idx[layer] + (size_t)py * SCREEN_WIDTH;
        for (int px = xa; px <= xb; px++) row[px] = s->dark[row[px]];
    }
}

void screen_load_indices(Screen *s, int layer, const uint8_t *data, int w, int h) {
    if (!valid_layer(layer) || !data) return;
    if (w > SCREEN_WIDTH) w = SCREEN_WIDTH;
    if (h > SCREEN_HEIGHT) h = SCREEN_HEIGHT;
    for (int y = 0; y < h; y++)
        memcpy(s->idx[layer] + (size_t)y * SCREEN_WIDTH, data + (size_t)y * w, (size_t)w);
}

static void blit_sheet(Screen *s, int layer, const Sheet *sh,
    int tile_w, int tile_h, int tile_idx,
    int dx, int dy, int off_x, int off_y, int transparent) {
    if (!valid_layer(layer) || !sh || !sh->data) return;
    int cols = sh->width / tile_w;
    if (cols <= 0) return;
    int sx0 = (tile_idx % cols) * tile_w;
    int sy0 = (tile_idx / cols) * tile_h;
    if (sx0 < 0 || sy0 < 0 || sx0 + tile_w > sh->width || sy0 + tile_h > sh->height) return;
    int dst_x0 = dx + off_x, dst_y0 = dy + off_y;
    for (int y = 0; y < tile_h; y++) {
        int py = dst_y0 + y;
        if (py < 0 || py >= SCREEN_HEIGHT) continue;
        uint8_t *drow = s->idx[layer] + (size_t)py * SCREEN_WIDTH;
        const uint8_t *srow = sh->data + (size_t)(sy0 + y) * sh->width + sx0;
        for (int x = 0; x < tile_w; x++) {
            int px = dst_x0 + x;
            if (px < 0 || px >= SCREEN_WIDTH) continue;
            uint8_t ci = srow[x];
            if (transparent && ci == 0) continue;
            drow[px] = ci;
        }
    }
}

void screen_blit_tile(Screen *s, int layer, const Sheet *tiles, int tile_idx, int dx, int dy) {
    blit_sheet(s, layer, tiles, 16, 16, tile_idx, dx, dy, 0, 0, 1);
}

void screen_blit_tile_solid(Screen *s, int layer, const Sheet *tiles, int tile_idx, int dx, int dy) {
    blit_sheet(s, layer, tiles, 16, 16, tile_idx, dx, dy, 0, 0, 0);
}

void screen_blit_sprite(Screen *s, int layer, const Sheet *sprs, int spr_idx,
    int dx, int dy, const SprProp *props, int n_props) {
    int ox = 0, oy = 0;
    if (props && spr_idx >= 0 && spr_idx < n_props) {
        ox = props[spr_idx].off_x;
        oy = props[spr_idx].off_y;
    }
    blit_sheet(s, layer, sprs, 24, 24, spr_idx, dx, dy, ox, oy, 1);
}

void screen_blit_from_layer(Screen *s, int src, int dst,
    int sx, int sy, int sw, int sh, int dx, int dy) {
    if (!valid_layer(src) || !valid_layer(dst) || sw <= 0 || sh <= 0) return;
    for (int y = 0; y < sh; y++) {
        int sy2 = sy + y, dy2 = dy + y;
        if (sy2 < 0 || sy2 >= SCREEN_HEIGHT || dy2 < 0 || dy2 >= SCREEN_HEIGHT) continue;
        for (int x = 0; x < sw; x++) {
            int sx2 = sx + x, dx2 = dx + x;
            if (sx2 < 0 || sx2 >= SCREEN_WIDTH || dx2 < 0 || dx2 >= SCREEN_WIDTH) continue;
            s->idx[dst][(size_t)dy2 * SCREEN_WIDTH + dx2] =
                s->idx[src][(size_t)sy2 * SCREEN_WIDTH + sx2];
        }
    }
}

void screen_set_pal(Screen *s, const uint8_t pal[768]) {
    int mx = 0;
    for (int i = 0; i < 768; i++) if (pal[i] > mx) mx = pal[i];
    if (mx <= 63) {
        /* 6-bit VGA palette — scale to 0-255 (mirrors JS setPal) */
        for (int i = 0; i < 768; i++) s->pal[i] = (uint8_t)(pal[i] * 255 / 63);
    } else {
        memcpy(s->pal, pal, 768);
    }
    s->dark_valid = 0;
}

void screen_get_pal(const Screen *s, uint8_t out[768]) {
    memcpy(out, s->pal, 768);
}

void screen_pal_off(Screen *s) {
    memset(s->pal, 0, 768);
    s->dark_valid = 0;
}

void screen_fade_to(Screen *s, uint8_t r, uint8_t g, uint8_t b, int frames) {
    if (frames < 1) frames = 1;
    uint8_t start[768];
    memcpy(start, s->pal, 768);
    for (int f = 1; f <= frames; f++) {
        for (int i = 0; i < 256; i++) {
            s->pal[i * 3] = (uint8_t)(start[i * 3] + (r - start[i * 3]) * f / frames);
            s->pal[i * 3 + 1] = (uint8_t)(start[i * 3 + 1] + (g - start[i * 3 + 1]) * f / frames);
            s->pal[i * 3 + 2] = (uint8_t)(start[i * 3 + 2] + (b - start[i * 3 + 2]) * f / frames);
        }
        s->dark_valid = 0;
        screen_present(s);
        SDL_Delay(16);
    }
}

void screen_fade_in(Screen *s, const uint8_t target[768], int frames) {
    if (frames < 1) frames = 1;
    uint8_t dst[768];
    memcpy(dst, target, 768);
    int mx = 0;
    for (int i = 0; i < 768; i++) if (dst[i] > mx) mx = dst[i];
    if (mx <= 63) {
        for (int i = 0; i < 768; i++) dst[i] = (uint8_t)(dst[i] * 255 / 63);
    }
    screen_pal_off(s);
    screen_present(s);
    for (int f = 1; f <= frames; f++) {
        for (int i = 0; i < 768; i++) s->pal[i] = (uint8_t)(dst[i] * f / frames);
        s->dark_valid = 0;
        screen_present(s);
        SDL_Delay(16);
    }
    memcpy(s->pal, dst, 768);
    s->dark_valid = 0;
    screen_present(s);
}

void screen_present(Screen *s) {
    void *pixels = NULL;
    int pitch = 0;
    if (SDL_LockTexture(s->tex, NULL, &pixels, &pitch) != 0) {
        SDL_Log("LockTexture: %s", SDL_GetError());
        return;
    }
    uint8_t *row = (uint8_t *)pixels;
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        uint32_t *dst = (uint32_t *)(row + (size_t)y * pitch);
        uint8_t *src = s->idx[VIDEO] + (size_t)y * SCREEN_WIDTH;
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            unsigned i = src[x] * 3u;
            dst[x] = 0xFF000000u
                | ((uint32_t)s->pal[i] << 16)
                | ((uint32_t)s->pal[i + 1] << 8)
                | ((uint32_t)s->pal[i + 2]);
        }
    }
    SDL_UnlockTexture(s->tex);
    SDL_RenderClear(s->ren);
    SDL_RenderCopy(s->ren, s->tex, NULL, NULL);
    SDL_RenderPresent(s->ren);
}

uint8_t *screen_layer_idx(Screen *s, int id) {
    if (!valid_layer(id)) return NULL;
    return s->idx[id];
}
