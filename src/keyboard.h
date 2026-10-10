#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/* Scancode-based keyboard (mirrors keyboard.ts QB scancodes from INT 9h).
 * kbd_poll() drains SDL events; per-frame the caller runs:
 *   kbd_poll(&kbd); ...use is_down/just_pressed...; kbd_clear_just(&kbd); */
#define SC_ESC 0x01
#define SC_W 0x11
#define SC_E 0x12
#define SC_R 0x13
#define SC_A 0x1E
#define SC_S 0x1F
#define SC_D 0x20
#define SC_CTRL 0x1D
#define SC_ENTER 0x1C
#define SC_SPACE 0x39
#define SC_LEFT 0x4B
#define SC_UP 0x48
#define SC_RIGHT 0x4D
#define SC_DOWN 0x50

typedef struct {
    uint8_t down[256];   /* held state */
    uint8_t just[256];   /* pressed since last kbd_clear_just */
    int quit;            /* SDL_QUIT received */
} Keyboard;

void kbd_init(Keyboard *k);
void kbd_poll(Keyboard *k);
int kbd_is_down(const Keyboard *k, int sc);
int kbd_just_pressed(const Keyboard *k, int sc);
void kbd_clear_just(Keyboard *k);
void kbd_clear(Keyboard *k);   /* clears keys, keeps quit flag */
/* Test/demo helper: inject a QB-scancode press/release. */
void kbd_inject(Keyboard *k, int sc, int is_down);
/* Map SDL scancode enum value to QB scancode, or -1 if unmapped. */
int sdl_to_qb(int sdl_scancode);

#endif
