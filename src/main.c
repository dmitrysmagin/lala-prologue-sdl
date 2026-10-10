/* Phase 0: open SDL window, decode TITLE.PCX, show it until quit.
 * Forward-compatible skeleton: indexed framebuffer + palette present path
 * that later phases reuse for layers/blitters/engines. */
#include "pcx.h"
#include "screen.h"

#include <SDL.h>
#include <stdio.h>

static const char *kCandidates[] = {
    "dist/assets/GFX/TITLE.PCX",      /* cwd = lala-sdl (make run) */
    "assets/GFX/TITLE.PCX",           /* cwd = dist */
    "../lala-js/public/GFX/TITLE.PCX" /* fallback to source checkout */
};

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    Screen scr;
    if (screen_init(&scr, "lala-sdl phase0", 3) != 0) return 1;

    PcxImage img;
    memset(&img, 0, sizeof(img));
    const char *used = NULL;
    for (unsigned i = 0; i < sizeof(kCandidates) / sizeof(kCandidates[0]); i++) {
        if (pcx_load_file(kCandidates[i], &img) == 0) { used = kCandidates[i]; break; }
    }
    if (!used) {
        fprintf(stderr, "phase0: could not load TITLE.PCX (tried dist/assets, assets, lala-js/public)\n");
        fprintf(stderr, "hint: run `make assets` first\n");
        screen_quit(&scr);
        return 1;
    }
    printf("phase0: loaded %s (%dx%d, pal=%s)\n", used, img.width, img.height,
        img.palette ? "yes" : "no");
    if (img.palette) screen_set_pal(&scr, img.palette);
    screen_blit_indexed(&scr, img.data, img.width, img.height);

    int running = 1;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = 0;
            else if (ev.type == SDL_KEYDOWN &&
                (ev.key.keysym.sym == SDLK_ESCAPE || ev.key.keysym.sym == SDLK_RETURN))
                running = 0;
        }
        screen_present(&scr);
        SDL_Delay(16); /* ~60fps present; timing engine comes in later phases */
    }

    pcx_free(&img);
    screen_quit(&scr);
    return 0;
}
