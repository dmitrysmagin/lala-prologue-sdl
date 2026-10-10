#include "scroll_player.h"
#include "world.h"

#include <stdlib.h>

static void sfx(PlaySfxFn fn, void *ud, int slot, int loop, int freq) {
    if (fn) fn(slot, loop, freq, ud);
}

void scroll_move_player(Keyboard *kbd, WorldBuff *world, Player *pl,
    const Prefs *prefs, uint8_t *map, PlaySfxFn play_sfx, void *sfx_ud) {
    pl->attempt = 0;

    if (pl->state == STATEFLICKER) {
        pl->ctState--;
        if (pl->ctState == 0) pl->state = STATENORMAL;
    }

    /* --- Vertical movement (gravity) --- */
    pl->vy += prefs->g;
    if (pl->vy > prefs->gMaxVy) pl->vy = prefs->gMaxVy;
    pl->y += pl->vy;

    if (pl->y < 0) pl->y = 0;
    {
        int32_t maxY = (world->worldH * 16 - 16) << 6;
        if (pl->y > maxY) pl->y = maxY;
    }

    if (pl->gotten) pl->vy = 0;

    int32_t y = pl->y >> 6;
    int32_t yy = y >> 4;
    int32_t x = pl->x >> 6;
    int32_t xx = x >> 4;

    /* --- Vertical tile collision --- */
    if (pl->vy < 0) {
        int b1 = world_behav(world, xx, yy);
        int b2 = (x & 15) != 0 ? world_behav(world, xx + 1, yy) : 0;
        if (b1 > 7 || b2 > 7) {
            pl->vy += prefs->jumpIncr >> 1;
            pl->y = (yy + 1) << 10;
            y = pl->y >> 6;
            yy = y >> 4;
        }
    } else if (pl->vy > 0 && (y & 15) <= (prefs->gMaxVy >> 6)) {
        int b1 = world_behav(world, xx, yy + 1);
        int b2 = (x & 15) != 0 ? world_behav(world, xx + 1, yy + 1) : 0;
        if (b1 > 3 || b2 > 3) {
            pl->vy = 0;
            pl->y = yy << 10;
            y = pl->y >> 6;
            yy = y >> 4;
        }
    }

    /* --- Jumping --- */
    if (kbd_is_down(kbd, SC_CTRL) || kbd_is_down(kbd, SC_UP)) {
        if (pl->vy == 0 && !pl->jumping) {
            int onGround = pl->gotten ||
                world_behav(world, xx, yy + 1) > 3 ||
                ((x & 15) != 0 && world_behav(world, xx + 1, yy + 1) > 3);
            if (onGround) {
                pl->jumping = -1;
                pl->ctJump = 0;
                sfx(play_sfx, sfx_ud, 3, 0, 11025 + (rand() % 1024));
                pl->x = x << 6;
            }
        }
        if (pl->jumping) {
            pl->vy -= prefs->jumpVyInitial + prefs->jumpIncr - (pl->ctJump >> 1);
            if (pl->vy < -prefs->jumpVyMax) pl->vy = -prefs->jumpVyMax;
            pl->ctJump++;
            if (pl->ctJump == 16) pl->jumping = 0;
        }
    } else {
        pl->jumping = 0;
    }

    /* --- Horizontal --- */
    if (!(kbd_is_down(kbd, SC_LEFT) || kbd_is_down(kbd, SC_RIGHT))) {
        if (pl->vx > 0) {
            pl->vx -= prefs->walkFr;
            if (pl->vx < 0) { pl->vx = 0; pl->x = x << 6; }
        } else if (pl->vx < 0) {
            pl->vx += prefs->walkFr;
            if (pl->vx > 0) { pl->vx = 0; pl->x = x << 6; }
        }
    }
    if (kbd_is_down(kbd, SC_LEFT)) {
        if (pl->vx > -prefs->walkVxMax) {
            pl->facing = 6;
            pl->vx -= prefs->walkAx;
        }
    }
    if (kbd_is_down(kbd, SC_RIGHT)) {
        if (pl->vx < prefs->walkVxMax) {
            pl->facing = 0;
            pl->vx += prefs->walkAx;
        }
    }

    pl->x += pl->vx;

    if (pl->x < 0) pl->x = 0;
    {
        int32_t maxX = (world->worldW * 16 - 16) << 6;
        if (pl->x > maxX) pl->x = maxX;
    }

    x = pl->x >> 6;
    xx = x >> 4;
    y = pl->y >> 6;
    yy = y >> 4;

    /* --- Horizontal tile collision --- */
    if (pl->vx < 0) {
        int b1 = world_behav(world, xx, yy);
        int b2 = (y & 15) != 0 ? world_behav(world, xx, yy + 1) : 0;
        if (b1 > 7 || b2 > 7) {
            pl->vx = 0;
            pl->x = (xx + 1) << 10;
        }
    } else if (pl->vx > 0) {
        int b1 = world_behav(world, xx + 1, yy);
        int b2 = (y & 15) != 0 ? world_behav(world, xx + 1, yy + 1) : 0;
        if (b1 > 7 || b2 > 7) {
            pl->vx = 0;
            pl->x = xx << 10;
        }
    } else {
        pl->x = x << 6;
    }

    x = pl->x >> 6;
    xx = x >> 4;
    y = pl->y >> 6;
    yy = y >> 4;

    /* --- Keyhole unlocking --- */
    if ((y & 15) == 0 && (x & 15) == 0 && pl->keys > 0) {
        int opened = 0;
        if (kbd_is_down(kbd, SC_LEFT))
            opened = world_clear_keyhole(world, map, xx - 1, yy);
        if (!opened && kbd_is_down(kbd, SC_RIGHT))
            opened = world_clear_keyhole(world, map, xx + 1, yy);
        if (opened) {
            pl->keys--;
            sfx(play_sfx, sfx_ud, 1, 0, 11025);
        }
    }

    /* --- Evil tiles --- */
    {
        int ev1 = world_behav(world, xx, yy) == 1;
        int ev2 = (x & 15) != 0 && world_behav(world, xx + 1, yy) == 1;
        int ev3 = (y & 15) != 0 && world_behav(world, xx, yy + 1) == 1;
        int ev4 = (x & 15) != 0 && (y & 15) != 0 &&
            world_behav(world, xx + 1, yy + 1) == 1;
        if (ev1 || ev2 || ev3 || ev4) {
            sfx(play_sfx, sfx_ud, 7, 0, 11025);
            if (pl->state == STATENORMAL) {
                sfx(play_sfx, sfx_ud, 8, 0, 11025 + (rand() % 1024));
                pl->lives--;
                pl->state = STATEFLICKER;
                pl->ctState = 128;
            }
            if (pl->vy < 0) {
                pl->vy = prefs->gMaxVy;
                pl->y = (yy + 1) << 10;
            } else {
                pl->vy = -prefs->gMaxVy;
                pl->y = yy << 10;
            }
        }
    }
}
