#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT)

/* Phase 0 Virtual VGA: single indexed framebuffer + 768-byte palette,
 * presented through an SDL streaming texture (nearest-neighbour upscale).
 * Later phases add layers 0..3 + blitters on top of this same present path. */
typedef struct {
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *tex;
    uint8_t fb[SCREEN_SIZE];  /* palette indices */
    uint8_t pal[768];         /* RGB triplets 0-255 */
} Screen;

int screen_init(Screen *s, const char *title, int scale);
void screen_quit(Screen *s);
void screen_set_pal(Screen *s, const uint8_t pal[768]);
void screen_blit_indexed(Screen *s, const uint8_t *indices, int w, int h);
void screen_present(Screen *s);

#endif
