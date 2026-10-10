#ifndef CONFIG_H
#define CONFIG_H

#include "keyboard.h"

/* Global runtime config (mirrors engine/config.ts).
 * game_speed: physics ticks/sec = 60 * game_speed (0.8 -> 48 tps),
 *   wall-clock anchored, identical on any display refresh rate.
 *   -/= keys adjust live (0.3..2.0) via config_poll_speed().
 * scroll_enabled: title-screen engine toggle (LEFT/RIGHT on title). */
typedef struct {
    double game_speed;
    int scroll_enabled;
} Config;

extern Config g_config;

/* Edge-triggered -/= handling; writes back the active rate. Call after
 * kbd_poll, before kbd_clear_just. */
void config_poll_speed(Keyboard *kbd, double *rate);

#endif
