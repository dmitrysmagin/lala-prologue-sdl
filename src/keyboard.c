#include "keyboard.h"

#include <SDL.h>
#include <string.h>

void kbd_init(Keyboard *k) {
    memset(k, 0, sizeof(*k));
}

int sdl_to_qb(int sdl_scancode) {
    switch (sdl_scancode) {
    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL: return SC_CTRL;
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_KP_ENTER: return SC_ENTER;
    case SDL_SCANCODE_LEFT: return SC_LEFT;
    case SDL_SCANCODE_UP: return SC_UP;
    case SDL_SCANCODE_RIGHT: return SC_RIGHT;
    case SDL_SCANCODE_DOWN: return SC_DOWN;
    case SDL_SCANCODE_W: return SC_W;
    case SDL_SCANCODE_E: return SC_E;
    case SDL_SCANCODE_R: return SC_R;
    case SDL_SCANCODE_A: return SC_A;
    case SDL_SCANCODE_S: return SC_S;
    case SDL_SCANCODE_D: return SC_D;
    case SDL_SCANCODE_SPACE: return SC_SPACE;
    case SDL_SCANCODE_ESCAPE: return SC_ESC;
    default: return -1;
    }
}

void kbd_inject(Keyboard *k, int sc, int is_down) {
    if (sc < 0 || sc > 255) return;
    if (is_down) {
        if (!k->down[sc]) k->just[sc] = 1;
        k->down[sc] = 1;
    } else {
        k->down[sc] = 0;
    }
}

void kbd_poll(Keyboard *k) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) { k->quit = 1; continue; }
        if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
            int sc = sdl_to_qb((int)ev.key.keysym.scancode);
            if (sc < 0) continue;
            if (ev.type == SDL_KEYDOWN) {
                /* ignore auto-repeat for edge state (mirrors JS e.repeat guard) */
                if (ev.key.repeat) { k->down[sc] = 1; continue; }
                kbd_inject(k, sc, 1);
            } else {
                kbd_inject(k, sc, 0);
            }
        }
    }
}

int kbd_is_down(const Keyboard *k, int sc) {
    if (sc < 0 || sc > 255) return 0;
    return k->down[sc];
}

int kbd_just_pressed(const Keyboard *k, int sc) {
    if (sc < 0 || sc > 255) return 0;
    return k->just[sc];
}

void kbd_clear_just(Keyboard *k) {
    memset(k->just, 0, sizeof(k->just));
}

void kbd_clear(Keyboard *k) {
    memset(k->down, 0, sizeof(k->down));
    memset(k->just, 0, sizeof(k->just));
}
