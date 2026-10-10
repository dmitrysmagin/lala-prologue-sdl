#include "enemies.h"
#include "collision.h"
#include "map.h"

#include <stdlib.h>

static void sfx(PlaySfxFn fn, void *ud, int slot, int loop, int freq) {
    if (fn) fn(slot, loop, freq, ud);
}

void move_enems(Enem *enems, int n_pants, int per_pant,
    TileCell *buff, const Prefs *prefs, Player *pl, int32_t nPant,
    PlaySfxFn play_sfx, void *sfx_ud) {
    pl->gotten = 0;
    if (nPant < 0 || nPant >= n_pants) return;

    for (int i = 0; i < prefs->nEnems && i < per_pant; i++) {
        Enem *e = &enems[(size_t)nPant * per_pant + i];
        if (e->t == 0) continue;

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
                    if (buff[c_to_idx((int)xx, (int)yy)].behaviour > 7 ||
                        ((px & 15) != 0 &&
                         buff[c_to_idx((int)xx + 1, (int)yy)].behaviour > 7)) {
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
                    if (buff[c_to_idx((int)xx, (int)(yy + 1))].behaviour > 3 ||
                        ((px & 15) != 0 &&
                         buff[c_to_idx((int)xx + 1, (int)(yy + 1))].behaviour > 3)) {
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
                        if (buff[c_to_idx((int)xx, (int)yy)].behaviour > 7 ||
                            ((py & 15) != 0 &&
                             buff[c_to_idx((int)xx, (int)(yy + 1))].behaviour > 7)) {
                            pl->vx = 0;
                            pl->x = (xx + 1) << 10;
                        }
                    } else if (e->mx > 0) {
                        if (buff[c_to_idx((int)xx + 1, (int)yy)].behaviour > 7 ||
                            ((py & 15) != 0 &&
                             buff[c_to_idx((int)xx + 1, (int)(yy + 1))].behaviour > 7)) {
                            pl->vx = 0;
                            pl->x = xx << 10;
                        }
                    }
                }
            }
        } else {
            if (detect_collision(i, nPant, enems, n_pants, per_pant, pl)) {
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

        if (e->x == e->x1 || e->x == e->x2) e->mx = -e->mx;
        if (e->y == e->y1 || e->y == e->y2) e->my = -e->my;
    }
}

void calc_player_frame(Player *pl, const int *sprmap, int sprmap_len) {
    if (!sprmap || sprmap_len <= 0) return;
    int id = -1;
    if (pl->vy < 0) id = (int)pl->facing + 4;
    else if (pl->vy > 0) id = (int)pl->facing + 5;
    else if (pl->vx != 0) {
        id = (int)pl->facing + (int)pl->frame;
        pl->subFrame = (pl->subFrame + 1) & 3;
        if (pl->subFrame == 0) pl->frame = (pl->frame + 1) & 3;
    } else {
        id = (int)pl->facing;
    }
    if (id >= 0 && id < sprmap_len) pl->sprId = sprmap[id];
}

void calc_enemy_frames(Enem *enems, int n_pants, int per_pant,
    int32_t nPant, const int *sprmap, int sprmap_len, int n_enems) {
    if (nPant < 0 || nPant >= n_pants || !sprmap) return;
    for (int i = 0; i < n_enems && i < per_pant; i++) {
        Enem *e = &enems[(size_t)nPant * per_pant + i];
        if (e->t == 0) continue;
        e->subFrame = (e->subFrame + 1) & 3;
        if (e->subFrame == 0) e->frame = (e->frame + 1) & 3;
        e->facing = (e->mx + e->my > 0) ? 0 : 4;
        int sid = 12 + ((int)(e->t - 1) << 3) + (int)e->facing + (int)e->frame;
        if (sid >= 0 && sid < sprmap_len) e->sprId = sprmap[sid];
    }
}
