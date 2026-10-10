#include "scroll_enemies.h"
#include "world.h"

#include <stdlib.h>

static void sfx(PlaySfxFn fn, void *ud, int slot, int loop, int freq) {
    if (fn) fn(slot, loop, freq, ud);
}

static int iabs(int v) { return v < 0 ? -v : v; }

void scroll_move_enemies(Enem *enemies, int n_enemies, WorldBuff *world,
    const Prefs *prefs, Player *pl, PlaySfxFn play_sfx, void *sfx_ud) {
    pl->gotten = 0;

    int32_t worldPxW = world->worldW * 16;
    int32_t worldPxH = world->worldH * 16;
    int32_t plPx = pl->x >> 6;
    int32_t plPy = pl->y >> 6;

    for (int i = 0; i < n_enemies; i++) {
        Enem *e = &enemies[i];
        if (e->t == 0) continue;

        /* proximity cull */
        {
            int32_t dx = e->x - plPx;
            int32_t dy = e->y - plPy;
            if (dx > 400 || dx < -400 || dy > 400 || dy < -400) continue;
        }

        e->x += e->mx;
        e->y += e->my;

        if (e->t == prefs->enemPlat) {
            int32_t px = pl->x >> 6;
            int32_t py = pl->y >> 6;

            if (e->my < 0) {
                if (px >= e->x - 15 && px <= e->x + 15 &&
                    py >= e->y - 16 && py <= e->y - 9 &&
                    pl->vy >= -prefs->jumpIncr) {
                    pl->gotten = 1;
                    pl->y = (e->y - 16) << 6;
                    pl->vy = 0;
                    int32_t xx = px >> 4;
                    int32_t yy = py >> 4;
                    if (world_behav(world, xx, yy) > 7 ||
                        ((px & 15) != 0 && world_behav(world, xx + 1, yy) > 7)) {
                        pl->y = (yy + 1) << 10;
                    }
                }
            } else if (e->my > 0) {
                if (px >= e->x - 15 && px <= e->x + 15 &&
                    py >= e->y - 20 && py <= e->y - 14 &&
                    pl->vy >= 0) {
                    pl->gotten = 1;
                    pl->y = (e->y - 16) << 6;
                    pl->vy = 0;
                    int32_t xx = px >> 4;
                    int32_t yy = py >> 4;
                    if (world_behav(world, xx, yy + 1) > 3 ||
                        ((px & 15) != 0 && world_behav(world, xx + 1, yy + 1) > 3)) {
                        pl->y = yy << 10;
                    }
                }
            }

            if (e->mx != 0) {
                if (px >= e->x - 15 && px <= e->x + 15 &&
                    py >= e->y - 16 && py <= e->y - 11 &&
                    pl->vy >= 0) {
                    pl->gotten = 1;
                    pl->y = (e->y - 16) << 6;
                    int32_t newPx = px + e->mx;
                    pl->x = newPx << 6;
                    int32_t xx = newPx >> 4;
                    int32_t yy = py >> 4;
                    if (e->mx < 0) {
                        if (world_behav(world, xx, yy) > 7 ||
                            ((py & 15) != 0 && world_behav(world, xx, yy + 1) > 7)) {
                            pl->vx = 0;
                            pl->x = (xx + 1) << 10;
                        }
                    } else if (e->mx > 0) {
                        if (world_behav(world, xx + 1, yy) > 7 ||
                            ((py & 15) != 0 && world_behav(world, xx + 1, yy + 1) > 7)) {
                            pl->vx = 0;
                            pl->x = xx << 10;
                        }
                    }
                }
            }
        } else {
            if (pl->state == STATENORMAL) {
                int32_t px = pl->x >> 6;
                int32_t py = pl->y >> 6;
                if (e->x >= px - 14 && e->x <= px + 14 &&
                    e->y >= py - 14 && e->y <= py + 14) {
                    pl->state = STATEFLICKER;
                    pl->ctState = 128;
                    if (e->mx > 0) pl->vx = prefs->walkVxMax << 1;
                    else pl->vx = -(prefs->walkVxMax << 1);
                    if (e->my > 0) pl->vy = prefs->walkVxMax << 1;
                    else pl->vy = -(prefs->walkVxMax << 1);
                    pl->lives -= 1;
                    sfx(play_sfx, sfx_ud, 2, 0, 11025);
                    sfx(play_sfx, sfx_ud, 8, 0, 11025 + (rand() % 1024));
                }
            }
        }

        if (e->x == e->x1 || e->x == e->x2) e->mx = -e->mx;
        if (e->y == e->y1 || e->y == e->y2) e->my = -e->my;

        /* world-edge safety clamp */
        if (e->x < 0) { e->x = 0; e->mx = iabs((int)e->mx); }
        else if (e->x > worldPxW - 16) { e->x = worldPxW - 16; e->mx = -iabs((int)e->mx); }
        if (e->y < 0) { e->y = 0; e->my = iabs((int)e->my); }
        else if (e->y > worldPxH - 16) { e->y = worldPxH - 16; e->my = -iabs((int)e->my); }
    }
}
