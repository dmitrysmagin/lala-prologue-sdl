#ifndef CONFIG_H
#define CONFIG_H

/* Global runtime config (mirrors engine/config.ts).
 * game_speed: physics ticks/sec = 60 * game_speed (0.8 -> 48 tps),
 *   wall-clock anchored, identical on any display refresh rate.
 * scroll_enabled: title-screen engine toggle (phase 5 implements scroll). */
typedef struct {
    double game_speed;
    int scroll_enabled;
} Config;

extern Config g_config;

#endif
