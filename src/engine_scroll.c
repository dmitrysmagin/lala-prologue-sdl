#include "engine_scroll.h"
#include "enemies.h"
#include "font.h"
#include "render.h"
#include "scroll_enemies.h"
#include "scroll_player.h"
#include "scroll_render.h"
#include "world.h"

#include <SDL.h>

static ScrollState *st(Engine *e) { return (ScrollState *)e->scroll; }

static void eng_scroll_move_player(Engine *e) {
    ScrollState *s = st(e);
    scroll_move_player(e->kbd, &s->world, e->player, e->prefs, e->map,
        e->play_sfx, e->sfx_ud);
}

static void eng_scroll_move_enems(Engine *e) {
    ScrollState *s = st(e);
    scroll_move_enemies(s->enemies, ENEM_PANTS * ENEM_PER_PANT, &s->world,
        e->prefs, e->player, e->play_sfx, e->sfx_ud);
}

static void eng_scroll_calc_frames(Engine *e) {
    ScrollState *s = st(e);
    calc_player_frame(e->player, e->sprmap, SPRMAP_LEN);
    for (int i = 0; i < ENEM_PANTS * ENEM_PER_PANT; i++) {
        Enem *en = &s->enemies[i];
        if (en->t == 0) continue;
        en->subFrame = (en->subFrame + 1) & 3;
        if (en->subFrame == 0) en->frame = (en->frame + 1) & 3;
        en->facing = (en->mx + en->my > 0) ? 0 : 4;
        int sid = 12 + ((int)(en->t - 1) << 3) + (int)en->facing + (int)en->frame;
        if (sid >= 0 && sid < SPRMAP_LEN) en->sprId = e->sprmap[sid];
    }
}

static void scroll_prepare(Engine *e, int32_t nPant) {
    /* Park camera at the origin of screen nPant (mirrors main.ts start). */
    ScrollState *s = st(e);
    int32_t sx = nPant % e->prefs->mapW;
    int32_t sy = nPant / e->prefs->mapW;
    s->cam.x = sx * e->prefs->screenW * 16;
    s->cam.y = sy * e->prefs->screenH * 16;
    e->nPant = nPant;
    e->player->attempt = 0;
}

void scroll_collect(Engine *e) {
    ScrollState *s = st(e);
    Prefs *prefs = e->prefs;
    Player *pl = e->player;
    int32_t px = pl->x >> 6;
    int32_t py = pl->y >> 6;
    for (int i = 0; i < HOTSPOT_PANTS; i++) {
        HotSpot *hs = &s->hots[i];
        if (!hs->s || hs->t == 0) continue;
        int32_t hx = hs->x * 16;
        int32_t hy = hs->y * 16;
        if (px >= hx - 15 && px <= hx + 15 && py >= hy - 15 && py <= hy + 15) {
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
        }
    }
}

/* Sprite helper: same subtract-then-add pattern as flip (net = screen pos). */
static void scroll_sprite_at(Engine *e, const Camera *cam, int world_x, int world_y,
    const Sheet *sheet, int sid) {
    int ox = 0, oy = 0;
    if (sid >= 0 && sid < e->n_sprprops) {
        ox = e->sprprops[sid].off_x;
        oy = e->sprprops[sid].off_y;
    }
    int sx = world_x - (int)cam->x + e->prefs->screenPos.x;
    int sy = world_y - (int)cam->y + e->prefs->screenPos.y;
    if (sx < -24 || sx > 320 || sy < -24 || sy > 200) return;
    screen_blit_sprite(e->scr, LAYER_1, sheet, sid,
        sx - ox, sy - oy, e->sprprops, e->n_sprprops);
}

static void scroll_draw(Engine *e, int32_t frame) {
    ScrollState *s = st(e);
    Screen *scr = e->scr;
    Player *pl = e->player;
    Prefs *prefs = e->prefs;
    int spx = (int)prefs->screenPos.x;
    int spy = (int)prefs->screenPos.y;

    /* restore backdrop, draw world tiles over it */
    screen_copy_layer(scr, LAYER_3, LAYER_2);
    scroll_draw_layer1(scr, &s->world, &s->cam, e->tiles, LAYER_2, spx, spy);
    scroll_draw_layer2(scr, &s->world, &s->cam, e->tiles, (int)frame, LAYER_2, spx, spy);
    screen_copy_layer(scr, LAYER_2, LAYER_1);

    scroll_draw_hotspots(scr, e->tiles, prefs, s->hots, HOTSPOT_PANTS, &s->cam, LAYER_1);

    if (!pl->gameOver) {
        int show = (pl->state == STATENORMAL) || ((SDL_GetTicks() / 66) & 1) == 0;
        if (show)
            scroll_sprite_at(e, &s->cam, (int)(pl->x >> 6), (int)(pl->y >> 6),
                e->sprs, (int)pl->sprId);
    }
    for (int i = 0; i < ENEM_PANTS * ENEM_PER_PANT; i++) {
        Enem *en = &s->enemies[i];
        if (en->t == 0) continue;
        scroll_sprite_at(e, &s->cam, (int)en->x, (int)en->y, e->sprs, (int)en->sprId);
    }

    render_print_stats(scr, pl, prefs, e->tiles, e->font);

    screen_copy_layer(scr, LAYER_1, VIDEO);
    screen_present(scr);
}

static int32_t scroll_loop(Engine *e, int32_t flag) {
    (void)flag; /* scroll is always full gameplay (no title mode) */
    ScrollState *s = st(e);
    Keyboard *kbd = e->kbd;
    Prefs *prefs = e->prefs;
    Player *pl = e->player;
    int32_t worldPxW = s->world.worldW * 16;
    int32_t worldPxH = s->world.worldH * 16;

    e->prepare(e, prefs->iniPant);

    int frame = 0, subFrame = 0;
    double accum = 0.0;
    int32_t res = 0;
    int running = 1;
    int debug_mode = 0;
    int debug_was_down = 0;

    if (prefs->bgL1 && e->play_sfx) e->play_sfx((int)prefs->bgL1, 1, 11025, e->sfx_ud);
    if (prefs->bgL2 && e->play_sfx) e->play_sfx((int)prefs->bgL2, 1, 11025, e->sfx_ud);

    update_camera(&s->cam, pl, worldPxW, worldPxH);

    uint64_t last = SDL_GetTicks64();
    while (running) {
        uint64_t now = SDL_GetTicks64();
        double dt = (now - last) / 1000.0;
        last = now;
        if (dt < 0) dt = 0;
        if (dt > 0.1) dt = 0.1;
        accum += dt * 60.0 * e->game_speed;

        kbd_poll(kbd);

        /* debug overlay toggle (D key, edge-triggered) */
        {
            int d = kbd_is_down(kbd, SC_D);
            if (d && !debug_was_down) debug_mode = !debug_mode;
            debug_was_down = d;
        }

        int ticks = 0;
        while (accum >= 1.0 && ticks < 5) {
            accum -= 1.0;
            ticks++;

            e->move_player(e);
            e->move_enems(e);

            subFrame = (subFrame + 1) & 3;
            if (subFrame == 0) frame = (frame + 1) & 3;

            e->calc_frames(e);
        }

        update_camera(&s->cam, pl, worldPxW, worldPxH);
        scroll_collect(e);

        e->mode = 1;
        e->draw(e, frame);
        e->frame = frame;

        /* debug overlay: white enemy rects, pink patrol rects */
        if (debug_mode) {
            for (int i = 0; i < ENEM_PANTS * ENEM_PER_PANT; i++) {
                Enem *en = &s->enemies[i];
                if (en->t == 0) continue;
                int sx = (int)en->x - (int)s->cam.x + (int)prefs->screenPos.x;
                int sy = (int)en->y - (int)s->cam.y + (int)prefs->screenPos.y;
                screen_draw_rect(e->scr, LAYER_1, sx, sy, sx + 15, sy + 15, 15);
                int x1 = (int)en->x1 - (int)s->cam.x + (int)prefs->screenPos.x;
                int y1 = (int)en->y1 - (int)s->cam.y + (int)prefs->screenPos.y;
                int x2 = (int)en->x2 - (int)s->cam.x + (int)prefs->screenPos.x + 15;
                int y2 = (int)en->y2 - (int)s->cam.y + (int)prefs->screenPos.y + 15;
                screen_draw_rect(e->scr, LAYER_1, x1, y1, x2, y2, 6);
            }
            screen_copy_layer(e->scr, LAYER_1, VIDEO);
            screen_present(e->scr);
        }

        /* cheat W+E+R */
        if (kbd_is_down(kbd, SC_W) && kbd_is_down(kbd, SC_E) && kbd_is_down(kbd, SC_R)) {
            pl->objects++;
            kbd_clear(kbd);
            SDL_Delay(200);
        }

        /* win/loss */
        if (pl->objects >= prefs->maxObjs) { res = -1; running = 0; }
        else if (pl->lives < 0) { res = -2; pl->gameOver = -1; }

        /* game-over display (scroll shows GAME OVER 2s, then exits) */
        if (pl->gameOver == -1) {
            screen_filter_box(e->scr, LAYER_1, 116, 92, 203, 107);
            if (e->font) engine_rprint(e->scr, LAYER_1, e->font, "GAME OVER", 123, 95);
            screen_copy_layer(e->scr, LAYER_1, VIDEO);
            screen_present(e->scr);
            SDL_Delay(2000);
            running = 0;
        }

        if (kbd->quit) { res = 0; running = 0; }
        else if (kbd_is_down(kbd, SC_ESC)) { res = 0; running = 0; }

        kbd_clear_just(kbd);
        SDL_Delay(1);
    }

    if (prefs->bgL1 && e->stop_sfx) e->stop_sfx(5, e->sfx_ud);
    if (prefs->bgL2 && e->stop_sfx) e->stop_sfx(6, e->sfx_ud);
    return res;
}

void scroll_engine_init(Engine *e) {
    e->name = "scroll";
    e->prepare = scroll_prepare;
    e->move_player = eng_scroll_move_player;
    e->move_enems = eng_scroll_move_enems;
    e->calc_frames = eng_scroll_calc_frames;
    e->draw = scroll_draw;
    e->loop = scroll_loop;
}
