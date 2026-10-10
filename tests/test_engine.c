/* Phase 4 tests: prefs, screen-prepare, player physics, enemies, frames,
 * flip prepare/draw. Uses real assets; screen under SDL_VIDEODRIVER=dummy.
 * Exit 0 = all pass. */
#include "assets.h"
#include "collision.h"
#include "config.h"
#include "enemies.h"
#include "engine_flip.h"
#include "fnt.h"
#include "map.h"
#include "map_load.h"
#include "pcx.h"
#include "player.h"
#include "prefs.h"
#include "screen.h"
#include "txt.h"

#include <SDL.h>
#include <stdio.h>
#include <string.h>

static int g_fail = 0, g_pass = 0;

#define CHECK(cond, fmt, ...) do { \
    if (cond) { g_pass++; } \
    else { g_fail++; printf("FAIL %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); } \
} while (0)

static int sfx_counts[16];
static void count_sfx(int slot, int loop, int freq, void *ud) {
    (void)loop; (void)freq; (void)ud;
    if (slot >= 0 && slot < 16) sfx_counts[slot]++;
}
static void clear_sfx(void) { memset(sfx_counts, 0, sizeof(sfx_counts)); }

static TileProp s_tileprops[TILEPROP_MAX];
static int s_ntile = 0;
static uint8_t *s_map = NULL;
static EnemRaw s_enems[ENEM_PANTS][ENEM_PER_PANT];
static HotSpotRaw s_hots[HOTSPOT_PANTS];
static int s_sprmap[SPRMAP_LEN];

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    /* ---- prefs ---- */
    Prefs prefs;
    default_prefs(&prefs);
    CHECK(prefs.mapW == 6 && prefs.screenW == 20 && prefs.screenH == 12, "dims");
    CHECK(prefs.g == 12 && prefs.gMaxVy == 192, "gravity consts");
    CHECK(prefs.walkVxMax == 128 && prefs.walkAx == 16 && prefs.walkFr == 24, "walk consts");
    CHECK(prefs.jumpVyInitial == 32 && prefs.jumpVyMax == 192 && prefs.jumpIncr == 24, "jump consts");
    CHECK(prefs.iniPant == 24 && prefs.iniTX == 2 && prefs.iniTY == 1, "start pos");
    CHECK(prefs.objectTile == 35 && prefs.keyTile == 36 && prefs.lifeTile == 34, "tiles");
    CHECK(prefs.initialLives == 15 && prefs.maxObjs == 15 && prefs.bgL1 == 9, "game consts");
    CHECK(prefs.screenPos.x == 0 && prefs.screenPos.y == 4, "screen pos");

    Player pl;
    memset(&pl, 0, sizeof(pl));
    init_player(&pl, &prefs);
    CHECK(pl.x == 2 * 16 * 64 && pl.y == 1 * 16 * 64, "player start (%d,%d)", pl.x, pl.y);
    CHECK(pl.state == STATENORMAL && pl.objects == 0, "player clean state");
    init_game(&prefs, &pl);
    CHECK(pl.lives == 15 && pl.keys == 0, "init_game");

    /* ---- c_to_idx ---- */
    CHECK(c_to_idx(0, 0) == 23, "c_to_idx origin");
    CHECK(c_to_idx(19, 11) == 284, "c_to_idx max visible");
    CHECK(c_to_idx(20, 12) == 307, "c_to_idx buffer max");

    /* ---- real assets + prepare ---- */
    char path[512];
    CHECK(resolve_asset("MAP/TILEPROP.TXT", path, sizeof(path)) != NULL, "tileprop resolves");
    s_ntile = tileprops_load(path, s_tileprops, TILEPROP_MAX);
    CHECK(s_ntile == 41, "tileprops 41");
    CHECK(map_load(&s_map, NULL) == 0, "map loads");
    CHECK(resolve_asset("MAP/ENEMS.TXT", path, sizeof(path)) != NULL, "enems resolves");
    CHECK(enems_load(path, s_enems) == 0, "enems load");
    CHECK(resolve_asset("MAP/HOTSPOTS.TXT", path, sizeof(path)) != NULL, "hots resolves");
    CHECK(hotspots_load(path, s_hots) == HOTSPOT_PANTS, "hots load");
    CHECK(resolve_asset("GFX/SPRMAP.TXT", path, sizeof(path)) != NULL, "sprmap resolves");
    CHECK(sprmap_load(path, s_sprmap) == 44, "sprmap 44");

    TileCell buff[CUR_BUFF_SIZE];
    screen_prepare(24, s_tileprops, s_ntile, s_map, buff, &prefs, s_hots, HOTSPOT_PANTS);
    CHECK(buff[c_to_idx(0, 0)].realMapIndex >= 0, "prepare fills mapping");
    {
        int expect_x = s_hots[24].s ? s_hots[24].x << 4 : 999;
        CHECK(prefs.hotSpotX == expect_x, "prepare hotspot x=%d", prefs.hotSpotX);
    }
    /* screens 30-35: no hotspot/enemy entries -> safe guards */
    screen_prepare(35, s_tileprops, s_ntile, s_map, buff, &prefs, s_hots, HOTSPOT_PANTS);
    CHECK(prefs.hotSpotX == 999, "screen 35 has no hotspot");
    screen_prepare(24, s_tileprops, s_ntile, s_map, buff, &prefs, s_hots, HOTSPOT_PANTS);

    /* ---- physics: fall on real map (smoke) ---- */
    Keyboard kbd;
    kbd_init(&kbd);
    init_player(&pl, &prefs);
    init_game(&prefs, &pl);
    clear_sfx();
    {
        int32_t y0 = pl.y;
        for (int i = 0; i < 120; i++)
            move_player(&kbd, buff, &pl, &prefs, s_map, count_sfx, NULL);
        CHECK(pl.y > y0, "gravity pulls down on real map");
        CHECK(pl.vy == 0 || pl.attempt != 0, "landed or hit edge");
        CHECK(pl.vx == 0, "no horizontal drift");
    }

    /* ---- synthetic floor room: exact landing / walk / jump ---- */
    TileCell room[CUR_BUFF_SIZE];
    uint8_t roommap[64];
    memset(room, 0, sizeof(room));
    memset(roommap, 0, sizeof(roommap));
    /* letterbox row r*22 holds tile-row r-1: floor at tile-row 8 */
    for (int x = 0; x < CUR_BUFF_W; x++)
        room[9 * CUR_BUFF_W + x].behaviour = 4; /* floor */
    init_player(&pl, &prefs);
    init_game(&prefs, &pl);
    pl.x = 5 * 16 * 64; pl.y = 2 * 16 * 64; /* tile (5,2) */
    for (int i = 0; i < 120; i++)
        move_player(&kbd, room, &pl, &prefs, roommap, count_sfx, NULL);
    CHECK(pl.vy == 0, "settled vy=0 (got %d)", pl.vy);
    CHECK(pl.y == (7 << 10), "snapped to floor top (y=%d)", pl.y);
    CHECK(pl.attempt == 0, "no edge attempt");

    /* ---- walk right: accel cap + friction stop (empty room, floor row) ---- */
    int32_t x0 = pl.x;
    kbd_inject(&kbd, SC_RIGHT, 1);
    for (int i = 0; i < 40; i++)
        move_player(&kbd, room, &pl, &prefs, roommap, count_sfx, NULL);
    CHECK(pl.x > x0, "walk moves right");
    CHECK(pl.vx == 128, "vx caps at walkVxMax (got %d)", pl.vx);
    CHECK(pl.facing == 0, "facing right");
    kbd_inject(&kbd, SC_RIGHT, 0);
    for (int i = 0; i < 20; i++)
        move_player(&kbd, room, &pl, &prefs, roommap, count_sfx, NULL);
    CHECK(pl.vx == 0, "friction stops (got %d)", pl.vx);
    /* walk left sets facing */
    kbd_inject(&kbd, SC_LEFT, 1);
    move_player(&kbd, room, &pl, &prefs, roommap, count_sfx, NULL);
    CHECK(pl.facing == 6, "facing left");
    kbd_inject(&kbd, SC_LEFT, 0);

    /* ---- jump ---- */
    for (int i = 0; i < 30; i++)
        move_player(&kbd, room, &pl, &prefs, roommap, count_sfx, NULL);
    clear_sfx();
    kbd_inject(&kbd, SC_UP, 1);
    int jumped = 0;
    for (int i = 0; i < 5; i++) {
        move_player(&kbd, room, &pl, &prefs, roommap, count_sfx, NULL);
        if (pl.vy < 0) jumped = 1;
    }
    CHECK(jumped, "jump gives negative vy");
    CHECK(sfx_counts[3] == 1, "jump sfx slot3 x1 (got %d)", sfx_counts[3]);
    kbd_inject(&kbd, SC_UP, 0);
    clear_sfx();
    for (int i = 0; i < 30; i++)
        move_player(&kbd, room, &pl, &prefs, roommap, count_sfx, NULL);

    /* ---- keyhole (synthetic) ---- */
    {
        TileCell kb[CUR_BUFF_SIZE];
        uint8_t kmap[64];
        memset(kb, 0, sizeof(kb));
        memset(kmap, 0, sizeof(kmap));
        kb[c_to_idx(5, 5)].behaviour = 10;
        kb[c_to_idx(5, 5)].realMapIndex = 7;
        kmap[7] = 35;
        Player kp;
        memset(&kp, 0, sizeof(kp));
        kp.x = 6 * 16 * 64; kp.y = 5 * 16 * 64; kp.keys = 1; kp.lives = 5;
        Keyboard kk;
        kbd_init(&kk);
        kbd_inject(&kk, SC_LEFT, 1);
        clear_sfx();
        move_player(&kk, kb, &kp, &prefs, kmap, count_sfx, NULL);
        CHECK(kp.keys == 0, "keyhole consumes key");
        CHECK(kmap[7] == 0 && kb[c_to_idx(5, 5)].behaviour == 0, "keyhole cleared");
        CHECK(sfx_counts[1] == 1, "bolt sfx slot1");
    }

    /* ---- evil tile (synthetic) ---- */
    {
        TileCell kb[CUR_BUFF_SIZE];
        uint8_t kmap[64];
        memset(kb, 0, sizeof(kb));
        memset(kmap, 0, sizeof(kmap));
        Player kp;
        memset(&kp, 0, sizeof(kp));
        kp.x = 3 * 16 * 64; kp.y = 3 * 16 * 64; kp.lives = 5;
        kb[c_to_idx(3, 3)].behaviour = 1;
        Keyboard kk;
        kbd_init(&kk);
        clear_sfx();
        move_player(&kk, kb, &kp, &prefs, kmap, count_sfx, NULL);
        CHECK(kp.lives == 4, "evil tile costs a life");
        CHECK(kp.state == STATEFLICKER && kp.ctState == 128, "flicker 128");
        CHECK(sfx_counts[7] == 1 && sfx_counts[8] == 1, "pinche+ah sfx");
        /* flicker decrements, no double damage ping... lives stay until NORMAL */
        int lives_before = kp.lives;
        kb[c_to_idx(3, 3)].behaviour = 0; /* step off */
        for (int i = 0; i < 128; i++)
            move_player(&kk, kb, &kp, &prefs, kmap, count_sfx, NULL);
        CHECK(kp.state == STATENORMAL, "flicker expires");
        CHECK(kp.lives == lives_before, "no extra damage");
    }

    /* ---- enemy patrol bounce (real ENEMS screen 0, enemy 0: mx=1) ---- */
    {
        Enem peninsula[1][ENEM_PER_PANT];
        memcpy(peninsula, s_enems, sizeof(Enem) * ENEM_PER_PANT);
        Player ep;
        memset(&ep, 0, sizeof(ep));
        ep.x = 32 << 6; ep.y = 16 << 6; ep.lives = 5;
        Keyboard kk;
        kbd_init(&kk);
        TileCell eb[CUR_BUFF_SIZE];
        memset(eb, 0, sizeof(eb));
        int start_mx = peninsula[0][0].mx;
        CHECK(start_mx == 1, "screen0 enem0 mx=1 (got %d)", start_mx);
        for (int i = 0; i < 20; i++)
            move_enems((Enem *)peninsula, 1, ENEM_PER_PANT, eb, &prefs, &ep, 0, count_sfx, NULL);
        CHECK(peninsula[0][0].mx == -1, "patrol bounce flips mx (got %d)", peninsula[0][0].mx);
    }

    /* ---- damage collision ---- */
    {
        Enem arena[1][ENEM_PER_PANT];
        memset(arena, 0, sizeof(arena));
        arena[0][0].x = 100; arena[0][0].y = 100;
        arena[0][0].x1 = 50; arena[0][0].x2 = 150;
        arena[0][0].y1 = 100; arena[0][0].y2 = 100;
        arena[0][0].mx = 1; arena[0][0].my = 0; arena[0][0].t = 2;
        Player ep;
        memset(&ep, 0, sizeof(ep));
        ep.x = 100 << 6; ep.y = 100 << 6; ep.lives = 5; ep.state = STATENORMAL;
        Keyboard kk;
        kbd_init(&kk);
        TileCell eb[CUR_BUFF_SIZE];
        memset(eb, 0, sizeof(eb));
        clear_sfx();
        move_enems((Enem *)arena, 1, ENEM_PER_PANT, eb, &prefs, &ep, 0, count_sfx, NULL);
        CHECK(ep.lives == 4, "enemy hit costs a life");
        CHECK(ep.state == STATEFLICKER, "hit flicker");
        CHECK(ep.vx == 256, "knockback away (+x, got %d)", ep.vx);
        CHECK(sfx_counts[2] == 1 && sfx_counts[8] == 1, "hit+ah sfx");
        /* flickering player is immune */
        clear_sfx();
        move_enems((Enem *)arena, 1, ENEM_PER_PANT, eb, &prefs, &ep, 0, count_sfx, NULL);
        CHECK(ep.lives == 4, "no damage while flickering");
    }

    /* ---- platform riding (synthetic horizontal platform) ---- */
    {
        Enem arena[1][ENEM_PER_PANT];
        memset(arena, 0, sizeof(arena));
        arena[0][0].x = 100; arena[0][0].y = 100;
        arena[0][0].x1 = 50; arena[0][0].x2 = 150;
        arena[0][0].y1 = 100; arena[0][0].y2 = 100;
        arena[0][0].mx = 2; arena[0][0].my = 0; arena[0][0].t = 4;
        Player ep;
        memset(&ep, 0, sizeof(ep));
        ep.x = 100 << 6; ep.y = 86 << 6; ep.vy = 0; ep.lives = 5;
        Keyboard kk;
        kbd_init(&kk);
        TileCell eb[CUR_BUFF_SIZE];
        memset(eb, 0, sizeof(eb));
        move_enems((Enem *)arena, 1, ENEM_PER_PANT, eb, &prefs, &ep, 0, count_sfx, NULL);
        CHECK(ep.gotten == 1, "platform ride engages");
        CHECK(ep.y == (100 - 16) << 6, "rider aligned to platform top");
    }

    /* ---- frame calculation ---- */
    {
        Player fp;
        memset(&fp, 0, sizeof(fp));
        calc_player_frame(&fp, s_sprmap, SPRMAP_LEN);
        CHECK(fp.sprId == s_sprmap[0], "standing frame");
        fp.vx = 10; fp.frame = 1;
        calc_player_frame(&fp, s_sprmap, SPRMAP_LEN);
        CHECK(fp.sprId == s_sprmap[1], "walking frame");
        fp.vx = 0; fp.vy = -50;
        calc_player_frame(&fp, s_sprmap, SPRMAP_LEN);
        CHECK(fp.sprId == s_sprmap[4], "jump frame");

        Enem fe[1][ENEM_PER_PANT];
        memset(fe, 0, sizeof(fe));
        fe[0][0].t = 3; fe[0][0].mx = 1; fe[0][0].my = 0;
        calc_enemy_frames((Enem *)fe, 1, ENEM_PER_PANT, 0, s_sprmap, SPRMAP_LEN, 4);
        CHECK(fe[0][0].facing == 0, "enemy facing +x");
        CHECK(fe[0][0].sprId == s_sprmap[28], "enemy sid 12+16+0+0=28");
    }

    /* ---- detect_collision edges ---- */
    {
        Enem ce[1][ENEM_PER_PANT];
        memset(ce, 0, sizeof(ce));
        ce[0][0].x = 100; ce[0][0].y = 100;
        Player cp;
        memset(&cp, 0, sizeof(cp));
        cp.state = STATENORMAL;
        cp.x = 100 << 6; cp.y = 100 << 6;
        CHECK(detect_collision(0, 0, (Enem *)ce, 1, ENEM_PER_PANT, &cp), "overlap collides");
        cp.x = (100 + 15) << 6;
        CHECK(!detect_collision(0, 0, (Enem *)ce, 1, ENEM_PER_PANT, &cp), "15px misses");
        cp.state = STATEFLICKER; cp.x = 100 << 6;
        CHECK(!detect_collision(0, 0, (Enem *)ce, 1, ENEM_PER_PANT, &cp), "flicker immune");
        CHECK(!detect_collision(0, 5, (Enem *)ce, 1, ENEM_PER_PANT, &cp), "OOB pant safe");
    }

    /* ---- flip prepare/draw smoke (dummy video) ---- */
    {
        Screen scr;
        CHECK(screen_init(&scr, "engine-test", 1) == 0, "screen init");
        PcxImage tiles_img, sprs_img;
        memset(&tiles_img, 0, sizeof(tiles_img));
        memset(&sprs_img, 0, sizeof(sprs_img));
        char tp[512], sp[512];
        resolve_asset("GFX/TILESET.PCX", tp, sizeof(tp));
        resolve_asset("GFX/SPRSET.PCX", sp, sizeof(sp));
        CHECK(pcx_load_file(tp, &tiles_img) == 0, "tileset loads");
        CHECK(pcx_load_file(sp, &sprs_img) == 0, "spriteset loads");
        Sheet tiles = {tiles_img.width, tiles_img.height, tiles_img.data};
        Sheet sprs = {sprs_img.width, sprs_img.height, sprs_img.data};

        clear_buff(buff);
        draw_layer1(&scr, &tiles, &prefs, buff, LAYER_2); /* empty: no crash */
        screen_prepare(24, s_tileprops, s_ntile, s_map, buff, &prefs, s_hots, HOTSPOT_PANTS);
        screen_clear_layer(&scr, LAYER_2);
        draw_layer1(&scr, &tiles, &prefs, buff, LAYER_2);
        {
            int any = 0;
            uint8_t *l2 = screen_layer_idx(&scr, LAYER_2);
            for (int i = 0; i < SCREEN_SIZE; i++) if (l2[i]) { any = 1; break; }
            CHECK(any, "layer1 tiles drawn");
        }
        draw_layer2(&scr, &tiles, &prefs, buff, 0, LAYER_1);

        /* engine vtable smoke */
        SprProp sprprops[SPRPROP_MAX];
        int nspr = 0;
        if (resolve_asset("GFX/SPRPROP.TXT", path, sizeof(path)))
            nspr = sprprops_load(path, sprprops, SPRPROP_MAX);
        Engine e;
        memset(&e, 0, sizeof(e));
        flip_engine_init(&e);
        CHECK(e.name && e.prepare && e.move_player && e.move_enems &&
              e.calc_frames && e.draw && e.loop, "vtable filled");
        e.scr = &scr; e.prefs = &prefs; e.player = &pl;
        e.tileprops = s_tileprops; e.n_tileprops = s_ntile;
        e.sprprops = sprprops; e.n_sprprops = nspr;
        e.sprmap = s_sprmap;
        e.tiles = &tiles; e.sprs = &sprs;
        e.map = s_map;
        e.enems = s_enems; e.hots = s_hots; e.n_hots = HOTSPOT_PANTS;
        e.buff = buff; e.font = NULL; e.audio = NULL;
        e.play_sfx = count_sfx; e.stop_sfx = NULL; e.sfx_ud = NULL;
        e.game_speed = 0.8;
        Keyboard ek;
        kbd_init(&ek);
        e.kbd = &ek;
        e.mode = 1;
        e.prepare(&e, 24);
        CHECK(e.nPant == 24, "prepare sets pant");
        e.draw(&e, 0); /* stats fallback path, no crash */
        e.mode = 0;
        e.draw(&e, 0); /* title overlay, NULL font, no crash */

        /* ---- transitions ---- */
        e.prepare(&e, 24);
        pl.attempt = DLEFT; flip_transition(&e);
        CHECK(e.nPant == 23 && pl.x == 19 << 10, "DLEFT 24->23");
        pl.attempt = DRIGHT; flip_transition(&e);
        CHECK(e.nPant == 24 && pl.x == 0, "DRIGHT 23->24");
        pl.attempt = DUP; flip_transition(&e);
        CHECK(e.nPant == 18 && pl.y == 11 << 10, "DUP 24->18");
        pl.attempt = DDOWN; flip_transition(&e);
        CHECK(e.nPant == 24 && pl.y == 0, "DDOWN 18->24? (18+6=24)");
        e.prepare(&e, 0);
        pl.attempt = DLEFT; flip_transition(&e);
        CHECK(e.nPant == 0, "DLEFT guard at 0");
        e.prepare(&e, 35);
        pl.attempt = DRIGHT; flip_transition(&e);
        CHECK(e.nPant == 35, "DRIGHT guard at 35");
        e.prepare(&e, 24);
        pl.attempt = 0;

        /* ---- hotspot pickup (screen 24 = 11,3,t1 object) ---- */
        CHECK(s_hots[24].t == 1, "screen24 hotspot is object (t=%d)", s_hots[24].t);
        pl.x = (s_hots[24].x * 16) << 6;
        pl.y = (s_hots[24].y * 16) << 6;
        pl.objects = 0;
        clear_sfx();
        flip_collect(&e);
        CHECK(s_hots[24].s == 0, "hotspot consumed");
        CHECK(pl.objects == 1, "object counted");
        CHECK(sfx_counts[6] == 1, "object sfx slot6");
        CHECK(prefs.hotSpotX == 999, "hotspot coords cleared");
        flip_collect(&e);
        CHECK(pl.objects == 1, "no double pickup");

        pcx_free(&tiles_img);
        pcx_free(&sprs_img);
        screen_quit(&scr);
    }

    free(s_map);
    printf("phase4: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
