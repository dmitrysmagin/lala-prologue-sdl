#include "engine_flip.h"
#include "config.h"
#include "enemies.h"
#include "font.h"
#include "map.h"
#include "render.h"

#include <SDL.h>
#include <string.h>

static void flip_prepare(Engine *e, int32_t nPant) {
    e->nPant = nPant;
    screen_prepare(nPant, e->tileprops, e->n_tileprops, e->map,
        e->buff, e->prefs, e->hots, e->n_hots);
    screen_copy_layer(e->scr, LAYER_3, LAYER_2);
    draw_layer1(e->scr, e->tiles, e->prefs, e->buff, LAYER_2);
    e->player->attempt = 0;
}

static void flip_move_player(Engine *e) {
    move_player(e->kbd, e->buff, e->player, e->prefs, e->map,
        e->play_sfx, e->sfx_ud);
}

static void flip_move_enems(Engine *e) {
    move_enems((Enem *)e->enems, ENEM_PANTS, ENEM_PER_PANT,
        e->buff, e->prefs, e->player, e->nPant, e->play_sfx, e->sfx_ud);
}

static void flip_calc_frames(Engine *e) {
    calc_player_frame(e->player, e->sprmap, SPRMAP_LEN);
    calc_enemy_frames((Enem *)e->enems, ENEM_PANTS, ENEM_PER_PANT,
        e->nPant, e->sprmap, SPRMAP_LEN, e->prefs->nEnems);
}

/* Sprite blit mirroring game-loop.ts: caller subtracts the SPRPROP offset
 * and does NOT pass props (blitter adds nothing) -> net = screenPos + pos - off. */
static void draw_sprite_at(Engine *e, int layer, const Sheet *sheet, int sid,
    int px, int py) {
    int ox = 0, oy = 0;
    if (sid >= 0 && sid < e->n_sprprops) {
        ox = e->sprprops[sid].off_x;
        oy = e->sprprops[sid].off_y;
    }
    screen_blit_sprite(e->scr, layer, sheet, sid,
        e->prefs->screenPos.x + px - ox, e->prefs->screenPos.y + py - oy,
        NULL, 0);
}

static void flip_draw_title_overlay(Engine *e); /* fwd */

void flip_transition(Engine *e) {
    Prefs *prefs = e->prefs;
    Player *pl = e->player;
    int32_t lastRow = (prefs->mapH - 1) * prefs->mapW;
    int32_t lastPant = prefs->mapH * prefs->mapW - 1;
    switch (pl->attempt) {
    case DLEFT:
        if (e->nPant > 0) { e->nPant--; pl->x = (prefs->screenW - 1) << 10; }
        break;
    case DRIGHT:
        if (e->nPant < lastPant) { e->nPant++; pl->x = 0; }
        break;
    case DUP:
        if (e->nPant >= prefs->mapW) { e->nPant -= prefs->mapW; pl->y = (prefs->screenH - 1) << 10; }
        break;
    case DDOWN:
        if (e->nPant < lastRow) { e->nPant += prefs->mapW; pl->y = 0; }
        break;
    default: break;
    }
    e->prepare(e, e->nPant);
}

void flip_collect(Engine *e) {
    Prefs *prefs = e->prefs;
    Player *pl = e->player;
    int32_t x = pl->x >> 6;
    int32_t y = pl->y >> 6;
    if (x >= prefs->hotSpotX - 15 && x <= prefs->hotSpotX + 15 &&
        y >= prefs->hotSpotY - 15 && y <= prefs->hotSpotY + 15) {
        if (e->nPant >= 0 && e->nPant < e->n_hots) {
            HotSpot *hs = &e->hots[e->nPant];
            if (hs->s) {
                hs->s = 0;
                if (hs->t == 1) {
                    pl->objects++;
                    if (e->play_sfx) e->play_sfx(6, 0, 11025, e->sfx_ud);
                } else if (hs->t == 2) {
                    pl->keys++;
                    if (e->play_sfx) e->play_sfx(4, 0, 11025, e->sfx_ud);
                } else if (hs->t == 3) {
                    pl->lives += prefs->refill;
                    if (e->play_sfx) e->play_sfx(5, 0, 11025, e->sfx_ud);
                }
                prefs->hotSpotX = 999;
                prefs->hotSpotY = 999;
            }
        }
    }
}

static void flip_draw(Engine *e, int32_t frame) {
    Screen *s = e->scr;
    Player *pl = e->player;
    Prefs *prefs = e->prefs;

    screen_copy_layer(s, LAYER_2, LAYER_1);
    if (!pl->gameOver) {
        int show = (pl->state == STATENORMAL) || ((SDL_GetTicks() / 66) & 1) == 0;
        if (show) {
            int32_t px = pl->x >> 6;
            int32_t py = pl->y >> 6;
            draw_sprite_at(e, LAYER_1, e->sprs, (int)pl->sprId, (int)px, (int)py);
        }
    }
    if (e->nPant >= 0 && e->nPant < ENEM_PANTS) {
        for (int i = 0; i < prefs->nEnems && i < ENEM_PER_PANT; i++) {
            Enem *en = &e->enems[e->nPant][i];
            if (en->t == 0) continue;
            draw_sprite_at(e, LAYER_1, e->sprs, (int)en->sprId, (int)en->x, (int)en->y);
        }
    }
    draw_layer2(s, e->tiles, prefs, e->buff, (int)frame, LAYER_1);
    render_draw_hotspots(s, e->tiles, prefs, e->hots, (int)e->nPant, LAYER_1);

    if (e->mode) render_print_stats(s, pl, prefs, e->tiles, e->font);
    else flip_draw_title_overlay(e);

    screen_copy_layer(s, LAYER_1, VIDEO);
    screen_present(s);
}

/* Title-mode overlay (flag==0): prompt or game-over box. */
static void flip_draw_title_overlay(Engine *e) {
    Screen *s = e->scr;
    Player *pl = e->player;
    if (pl->gameOver) {
        screen_filter_box(s, LAYER_1, 116, 92, 203, 107);
        if (e->font) engine_rprint(s, LAYER_1, e->font, "GAME OVER", 123, 95);
    } else {
        /* JS `frame & 16` is always 0 (frame cycles 0-3): always shown */
        screen_filter_box(s, LAYER_1, 80, 136, 239, 151);
        if (e->font)
            engine_rprint(s, LAYER_1, e->font, "PRESS ENTER TO PLAY", 89, 141);
    }
}

static int32_t flip_loop(Engine *e, int32_t flag) {
    Keyboard *kbd = e->kbd;
    Prefs *prefs = e->prefs;
    Player *pl = e->player;

    int32_t nPant = flag ? prefs->iniPant : prefs->mapW * (prefs->mapH - 1);
    e->prepare(e, nPant);

    int frame = 0, subFrame = 0;
    double accum = 0.0;
    int32_t res = 0;
    int running = 1;

    if (flag) {
        if (prefs->bgL1 && e->play_sfx) e->play_sfx((int)prefs->bgL1, 1, 11025, e->sfx_ud);
        if (prefs->bgL2 && e->play_sfx) e->play_sfx((int)prefs->bgL2, 1, 11025, e->sfx_ud);
    }

    uint64_t last = SDL_GetTicks64();
    while (running) {
        uint64_t now = SDL_GetTicks64();
        double dt = (now - last) / 1000.0;
        last = now;
        if (dt < 0) dt = 0;
        if (dt > 0.1) dt = 0.1;
        accum += dt * 60.0 * e->game_speed;

        /* input polling lives in the render-rate domain (like DOM events) */
        kbd_poll(kbd);
        config_poll_speed(kbd, &e->game_speed);

        int ticks = 0;
        while (accum >= 1.0 && ticks < 5) {
            accum -= 1.0;
            ticks++;

            subFrame = (subFrame + 1) & 3;
            if (subFrame == 0) frame = (frame + 1) & 3;

            if (flag && !pl->gameOver) e->move_player(e);
            if (flag) e->move_enems(e);
            if (flag) e->calc_frames(e);
            else {
                /* title mode still animates enemies */
                calc_enemy_frames((Enem *)e->enems, ENEM_PANTS, ENEM_PER_PANT,
                    e->nPant, e->sprmap, SPRMAP_LEN, prefs->nEnems);
            }
        }

        /* --- screen transition + hotspot collection (testable units) --- */
        if (pl->attempt && flag) flip_transition(e);
        if (flag) flip_collect(e);

        /* --- render (via vtable) --- */
        e->mode = flag ? 1 : 0;
        e->draw(e, frame);
        e->frame = frame;

        /* --- cheat W+E+R --- */
        if (kbd_is_down(kbd, SC_W) && kbd_is_down(kbd, SC_E) && kbd_is_down(kbd, SC_R)) {
            pl->objects++;
            kbd_clear(kbd);
            SDL_Delay(200);
        }

        /* --- win/loss --- */
        if (flag) {
            if (pl->objects >= prefs->maxObjs) { res = -1; running = 0; }
            else if (pl->lives < 0) { res = -2; pl->gameOver = -1; }
        }

        /* --- exit --- */
        if (kbd->quit) { res = 0; running = 0; }
        else if (!flag && kbd_is_down(kbd, SC_ENTER)) { res = -3; running = 0; }
        else if (kbd_is_down(kbd, SC_ESC)) { res = 0; running = 0; }

        kbd_clear_just(kbd);
        SDL_Delay(1); /* yield; pacing is wall-clock driven */
    }

    if (flag) {
        if (prefs->bgL1 && e->stop_sfx) e->stop_sfx(5, e->sfx_ud);
        if (prefs->bgL2 && e->stop_sfx) e->stop_sfx(6, e->sfx_ud);
    }
    return res;
}

void flip_engine_init(Engine *e) {
    e->name = "flip";
    e->prepare = flip_prepare;
    e->move_player = flip_move_player;
    e->move_enems = flip_move_enems;
    e->calc_frames = flip_calc_frames;
    e->draw = flip_draw;
    e->loop = flip_loop;
}
