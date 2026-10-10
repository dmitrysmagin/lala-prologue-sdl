#include "screen.h"

#include <SDL.h>
#include <string.h>

int screen_init(Screen *s, const char *title, int scale) {
    memset(s, 0, sizeof(*s));
    memset(s->pal, 0, sizeof(s->pal));

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
        SDL_Log("CreateRenderer: %s", SDL_GetError());
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
        SDL_DestroyWindow(s->win);
        SDL_Quit();
        return -1;
    }
    return 0;
}

void screen_quit(Screen *s) {
    if (s->tex) SDL_DestroyTexture(s->tex);
    if (s->ren) SDL_DestroyRenderer(s->ren);
    if (s->win) SDL_DestroyWindow(s->win);
    SDL_Quit();
    s->tex = NULL;
    s->ren = NULL;
    s->win = NULL;
}

void screen_set_pal(Screen *s, const uint8_t pal[768]) {
    memcpy(s->pal, pal, 768);
}

void screen_blit_indexed(Screen *s, const uint8_t *indices, int w, int h) {
    if (w > SCREEN_WIDTH) w = SCREEN_WIDTH;
    if (h > SCREEN_HEIGHT) h = SCREEN_HEIGHT;
    for (int y = 0; y < h; y++)
        memcpy(s->fb + (size_t)y * SCREEN_WIDTH, indices + (size_t)y * w, (size_t)w);
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
        uint8_t *src = s->fb + (size_t)y * SCREEN_WIDTH;
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
