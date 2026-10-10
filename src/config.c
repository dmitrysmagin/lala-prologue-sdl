#include "config.h"

#include <stdio.h>

Config g_config = {0.8, 1};

void config_poll_speed(Keyboard *kbd, double *rate) {
    if (kbd_just_pressed(kbd, SC_MINUS)) {
        g_config.game_speed -= 0.1;
        if (g_config.game_speed < 0.3) g_config.game_speed = 0.3;
        *rate = g_config.game_speed;
        printf("[speed] %.1f (%d tps)\n",
            g_config.game_speed, (int)(60 * g_config.game_speed));
    }
    if (kbd_just_pressed(kbd, SC_EQUAL)) {
        g_config.game_speed += 0.1;
        if (g_config.game_speed > 2.0) g_config.game_speed = 2.0;
        *rate = g_config.game_speed;
        printf("[speed] %.1f (%d tps)\n",
            g_config.game_speed, (int)(60 * g_config.game_speed));
    }
}
