/* Phase 5 tests: world buffer, converts, camera, scroll physics/enemies,
 * scroll render + vtable. Real assets; screen under SDL_VIDEODRIVER=dummy.
 * Exit 0 = all pass. */
#include "assets.h"
#include "collision.h"
#include "config.h"
#include "enemies.h"
#include "engine_scroll.h"
#include "fnt.h"
#include "map.h"
#include "map_load.h"
#include "pcx.h"
#include "prefs.h"
#include "screen.h"
#include "scroll_enemies.h"
#include "scroll_player.h"
#include "scroll_render.h"
#include "txt.h"
#include "world.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
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

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    Prefs prefs;
    default_prefs(&prefs);

    char path[512];
    TileProp tileprops[TILEPROP_MAX];
    resolve_asset("MAP/TILEPROP.TXT", path, sizeof(path));
    int ntp = tileprops_load(path, tileprops, TILEPROP_MAX);
    uint8_t *map = NULL;
    map_load(&map, NULL);
    EnemRaw enems[ENEM_PANTS][ENEM_PER_PANT];
    resolve_asset("MAP/ENEMS.TXT", path, sizeof(path));
    enems_load(path, enems);
    HotSpotRaw hots[HOTSPOT_PANTS];
    resolve_asset("MAP/HOTSPOTS.TXT", path, sizeof(path));
    hotspots_load(path, hots);
    int sprmap[SPRMAP_LEN];
    resolve_asset("GFX/SPRMAP.TXT", path, sizeof(path));
    sprmap_load(path, sprmap);

    /* ---- world build ---- */
    WorldBuff world;
    memset(&world, 0, sizeof(world));
    CHECK(world_build(&world, map, tileprops, ntp, &prefs) == 0, "world builds");
    CHECK(world.worldW == 120 && world.worldH == 72, "world 120x72");
    /* spot-check vs flip prepare: screen 24 tile (0,0) */
    {
        TileCell buff[CUR_BUFF_SIZE];
        screen_prepare(24, tileprops, ntp, map, buff, &prefs, hots, HOTSPOT_PANTS);
        TileCell *c = &buff[c_to_idx(0, 0)];
        size_t wi = (size_t)48 * 120 + 0; /* screen24 base (0,48) */
        uint8_t value = map[(4 * 12 + 0) * 120 + 0];
        int loc = tileprops[value].location;
        uint8_t wl = loc == BACK ? world.layer1[wi] : world.layer2[wi];
        uint8_t fl = loc == BACK ? c->layer1 : c->layer2;
        CHECK(wl == fl && wl == value, "world/flip tile agree (%d)", wl);
        CHECK(world.behaviour[wi] == c->behaviour, "world/flip behav agree");
        CHECK(map[world.realMapIndex[wi]] == value, "realMapIndex back-ref");
    }
    CHECK(world_behav(&world, -1, 0) == 0, "behav OOB neg");
    CHECK(world_behav(&world, 120, 0) == 0, "behav OOB w");
    CHECK(world_behav(&world, 0, 72) == 0, "behav OOB h");

    /* ---- converts ---- */
    {
        Enem flat[ENEM_PANTS * ENEM_PER_PANT];
        memset(flat, 0, sizeof(flat));
        convert_enemies_to_world(enems, flat, &prefs);
        CHECK(flat[0].x == enems[0][0].x, "screen0 enemy unshifted");
        CHECK(flat[1 * ENEM_PER_PANT].x == enems[1][0].x + 320, "screen1 +320px");
        CHECK(flat[1 * ENEM_PER_PANT].y == enems[1][0].y, "screen1 y same row");
        CHECK(flat[6 * ENEM_PER_PANT].y == enems[6][0].y + 192, "screen6 +192px");
        CHECK(flat[6 * ENEM_PER_PANT].x1 == enems[6][0].x1, "screen6 x1 same col");
        HotSpot wh[HOTSPOT_PANTS];
        memset(wh, 0, sizeof(wh));
        convert_hotspots_to_world(hots, wh, HOTSPOT_PANTS, &prefs);
        CHECK(wh[0].x == hots[0].x && wh[0].y == hots[0].y, "screen0 hotspot same");
        CHECK(wh[1].x == hots[1].x + 20 && wh[1].y == hots[1].y, "screen1 +20 tiles");
        CHECK(wh[24].x == hots[24].x && wh[24].y == hots[24].y + 48, "screen24 +48 rows");
    }

    /* ---- camera ---- */
    {
        Camera cam;
        Player pl;
        memset(&pl, 0, sizeof(pl));
        /* scroll start pos: screen 24 -> world px (32, 784) */
        pl.x = (0 * 20 + 2) * 16 * 64;
        pl.y = (4 * 12 + 1) * 16 * 64;
        update_camera(&cam, &pl, 1920, 1152);
        CHECK(cam.x == 0 && cam.y == 696, "start cam (0,696) got (%d,%d)", cam.x, cam.y);
        pl.x = 960 << 6; pl.y = 576 << 6;
        update_camera(&cam, &pl, 1920, 1152);
        CHECK(cam.x == 812 && cam.y == 488, "center cam got (%d,%d)", cam.x, cam.y);
        pl.x = 5000 << 6; pl.y = 5000 << 6;
        update_camera(&cam, &pl, 1920, 1152);
        CHECK(cam.x == 1600 && cam.y == 952, "clamp cam got (%d,%d)", cam.x, cam.y);
    }

    /* ---- scroll physics: synthetic floor world ---- */
    Keyboard kbd;
    kbd_init(&kbd);
    {
        WorldBuff w;
        world_alloc(&w, 120, 72);
        for (int x = 0; x < 120; x++) w.behaviour[(size_t)50 * 120 + x] = 4;
        uint8_t zmap[64];
        memset(zmap, 0, sizeof(zmap));
        Player pl;
        memset(&pl, 0, sizeof(pl));
        init_player(&pl, &prefs);
        init_game(&prefs, &pl);
        pl.x = 10 * 16 * 64; pl.y = 40 * 16 * 64;
        clear_sfx();
        for (int i = 0; i < 120; i++)
            scroll_move_player(&kbd, &w, &pl, &prefs, zmap, count_sfx, NULL);
        CHECK(pl.vy == 0, "scroll settle vy=0");
        CHECK(pl.y == (49 << 10), "scroll lands on floor (y=%d)", pl.y);
        CHECK(pl.attempt == 0, "scroll never sets attempt");
        /* walk cap */
        kbd_inject(&kbd, SC_RIGHT, 1);
        for (int i = 0; i < 40; i++)
            scroll_move_player(&kbd, &w, &pl, &prefs, zmap, count_sfx, NULL);
        CHECK(pl.vx == 128, "scroll vx cap");
        kbd_inject(&kbd, SC_RIGHT, 0);
        /* jump */
        for (int i = 0; i < 30; i++)
            scroll_move_player(&kbd, &w, &pl, &prefs, zmap, count_sfx, NULL);
        clear_sfx();
        kbd_inject(&kbd, SC_UP, 1);
        int jumped = 0;
        for (int i = 0; i < 5; i++) {
            scroll_move_player(&kbd, &w, &pl, &prefs, zmap, count_sfx, NULL);
            if (pl.vy < 0) jumped = 1;
        }
        CHECK(jumped && sfx_counts[3] == 1, "scroll jump + sfx");
        kbd_inject(&kbd, SC_UP, 0);
        /* world clamp, no attempt */
        pl.x = ((120 * 16 - 16) << 6) + 5000;
        scroll_move_player(&kbd, &w, &pl, &prefs, zmap, count_sfx, NULL);
        CHECK(pl.x == (120 * 16 - 16) << 6 && pl.attempt == 0, "scroll clamps right edge");
        world_free(&w);
    }

    /* ---- scroll keyhole + evil (small synthetic world) ---- */
    {
        WorldBuff w;
        world_alloc(&w, 40, 30);
        uint8_t zmap[64];
        memset(zmap, 0, sizeof(zmap));
        w.behaviour[(size_t)5 * 40 + 5] = 10;
        w.realMapIndex[(size_t)5 * 40 + 5] = 7;
        zmap[7] = 35;
        Player pl;
        memset(&pl, 0, sizeof(pl));
        pl.x = 6 * 16 * 64; pl.y = 5 * 16 * 64; pl.keys = 1; pl.lives = 5;
        Keyboard kk;
        kbd_init(&kk);
        kbd_inject(&kk, SC_LEFT, 1);
        clear_sfx();
        scroll_move_player(&kk, &w, &pl, &prefs, zmap, count_sfx, NULL);
        CHECK(pl.keys == 0 && zmap[7] == 0 && sfx_counts[1] == 1, "scroll keyhole opens");
        /* evil */
        memset(w.behaviour, 0, (size_t)40 * 30);
        w.behaviour[(size_t)5 * 40 + 5] = 1;
        memset(&pl, 0, sizeof(pl));
        pl.x = 5 * 16 * 64; pl.y = 5 * 16 * 64; pl.lives = 5;
        kbd_init(&kk);
        clear_sfx();
        scroll_move_player(&kk, &w, &pl, &prefs, zmap, count_sfx, NULL);
        CHECK(pl.lives == 4 && pl.state == STATEFLICKER, "scroll evil damage");
        CHECK(sfx_counts[7] == 1, "scroll pinche sfx");
        world_free(&w);
    }

    /* ---- scroll enemies: bounce / clamp / cull / damage ---- */
    {
        WorldBuff w;
        world_alloc(&w, 120, 72);
        Keyboard kk;
        kbd_init(&kk);
        /* bounce */
        Enem eb[2];
        memset(eb, 0, sizeof(eb));
        eb[0].x = 96; eb[0].y = 32; eb[0].x1 = 96; eb[0].y1 = 32;
        eb[0].x2 = 112; eb[0].y2 = 160; eb[0].mx = 1; eb[0].my = 1; eb[0].t = 3;
        Player pl;
        memset(&pl, 0, sizeof(pl));
        pl.x = 32 << 6; pl.y = 16 << 6; pl.lives = 5;
        for (int i = 0; i < 20; i++)
            scroll_move_enemies(eb, 2, &w, &prefs, &pl, count_sfx, NULL);
        CHECK(eb[0].mx == -1, "scroll patrol bounce");
        /* world-edge clamp */
        memset(eb, 0, sizeof(eb));
        eb[1].x = -5; eb[1].y = 100; eb[1].x1 = -100; eb[1].x2 = -50;
        eb[1].y1 = 100; eb[1].y2 = 100; eb[1].mx = -2; eb[1].t = 2;
        pl.x = 200 << 6; pl.y = 100 << 6;
        scroll_move_enemies(eb, 2, &w, &prefs, &pl, count_sfx, NULL);
        CHECK(eb[1].x == 0 && eb[1].mx == 2, "scroll edge clamp +mx");
        /* proximity cull */
        memset(eb, 0, sizeof(eb));
        eb[0].x = 900; eb[0].y = 900; eb[0].x1 = 800; eb[0].x2 = 1000;
        eb[0].y1 = 900; eb[0].y2 = 900; eb[0].mx = 1; eb[0].t = 2;
        pl.x = 100 << 6; pl.y = 100 << 6;
        scroll_move_enemies(eb, 2, &w, &prefs, &pl, count_sfx, NULL);
        CHECK(eb[0].x == 900, "far enemy culled");
        /* damage */
        memset(eb, 0, sizeof(eb));
        eb[0].x = 100; eb[0].y = 100; eb[0].x1 = 50; eb[0].x2 = 150;
        eb[0].y1 = 100; eb[0].y2 = 100; eb[0].mx = 1; eb[0].t = 2;
        memset(&pl, 0, sizeof(pl));
        pl.x = 100 << 6; pl.y = 100 << 6; pl.lives = 5;
        clear_sfx();
        scroll_move_enemies(eb, 2, &w, &prefs, &pl, count_sfx, NULL);
        CHECK(pl.lives == 4 && pl.state == STATEFLICKER && sfx_counts[2] == 1,
            "scroll enemy damage");
        world_free(&w);
    }

    /* ---- scroll render + vtable smoke (dummy video) ---- */
    {
        Screen scr;
        CHECK(screen_init(&scr, "scroll-test", 1) == 0, "screen init");
        PcxImage tiles_img, sprs_img;
        memset(&tiles_img, 0, sizeof(tiles_img));
        memset(&sprs_img, 0, sizeof(sprs_img));
        char tp[512], sp[512];
        resolve_asset("GFX/TILESET.PCX", tp, sizeof(tp));
        resolve_asset("GFX/SPRSET.PCX", sp, sizeof(sp));
        pcx_load_file(tp, &tiles_img);
        pcx_load_file(sp, &sprs_img);
        Sheet tiles = {tiles_img.width, tiles_img.height, tiles_img.data};
        Sheet sprs = {sprs_img.width, sprs_img.height, sprs_img.data};
        SprProp sprprops[SPRPROP_MAX];
        int nspr = 0;
        if (resolve_asset("GFX/SPRPROP.TXT", path, sizeof(path)))
            nspr = sprprops_load(path, sprprops, SPRPROP_MAX);

        ScrollState sc;
        memset(&sc, 0, sizeof(sc));
        CHECK(world_build(&sc.world, map, tileprops, ntp, &prefs) == 0, "rebuild world");
        convert_enemies_to_world(enems, sc.enemies, &prefs);
        convert_hotspots_to_world(hots, sc.hots, HOTSPOT_PANTS, &prefs);

        /* camera at scroll start (screen 24 -> (0,768)) */
        Player pl;
        memset(&pl, 0, sizeof(pl));
        init_player(&pl, &prefs);
        init_game(&prefs, &pl);
        pl.x = (0 * 20 + 2) * 16 * 64;
        pl.y = (4 * 12 + 1) * 16 * 64;
        update_camera(&sc.cam, &pl, 1920, 1152);
        screen_clear_layer(&scr, LAYER_2);
        scroll_draw_layer1(&scr, &sc.world, &sc.cam, &tiles, LAYER_2, 0, 4);
        {
            int any = 0;
            uint8_t *l2 = screen_layer_idx(&scr, LAYER_2);
            for (int i = 0; i < SCREEN_SIZE; i++) if (l2[i]) { any = 1; break; }
            CHECK(any, "scroll layer1 draws");
        }
        scroll_draw_layer2(&scr, &sc.world, &sc.cam, &tiles, 0, LAYER_2, 0, 4);
        scroll_draw_sprite(&scr, LAYER_1, &sprs, 0, 32, 784, &sc.cam,
            sprprops, nspr, 0, 4);
        /* regression: SPRPROP offset applied exactly once (net = pos - off).
         * 24x24 solid sprite, off (4,8), world (100,100), cam (0,0), sp (0,4)
         * -> top-left lands at (96,96); re-adding the offset (bug) would
         * cover (120,100) instead. */
        {
            static uint8_t sol24[24 * 24];
            memset(sol24, 7, sizeof(sol24));
            Sheet sol = {24, 24, sol24};
            SprProp oneprop[1] = {{4, 8}};
            Camera zcam = {0, 0};
            screen_clear_layer(&scr, LAYER_1);
            scroll_draw_sprite(&scr, LAYER_1, &sol, 0, 100, 100, &zcam,
                oneprop, 1, 0, 4);
            uint8_t *l1 = screen_layer_idx(&scr, LAYER_1);
            CHECK(l1[(size_t)96 * SCREEN_WIDTH + 96] == 7, "sprite net pos - off");
            CHECK(l1[(size_t)110 * SCREEN_WIDTH + 121] == 0, "offset not re-added");
        }
        scroll_draw_hotspots(&scr, &tiles, &prefs, sc.hots, HOTSPOT_PANTS, &sc.cam, LAYER_1);

        /* vtable */
        Engine e;
        memset(&e, 0, sizeof(e));
        scroll_engine_init(&e);
        CHECK(e.name && e.prepare && e.move_player && e.move_enems &&
              e.calc_frames && e.draw && e.loop, "scroll vtable filled");
        e.scr = &scr; e.prefs = &prefs; e.player = &pl;
        e.tileprops = tileprops; e.n_tileprops = ntp;
        e.sprprops = sprprops; e.n_sprprops = nspr;
        e.sprmap = sprmap;
        e.tiles = &tiles; e.sprs = &sprs;
        e.map = map;
        e.font = NULL;
        e.play_sfx = count_sfx; e.stop_sfx = NULL; e.sfx_ud = NULL;
        e.game_speed = 0.8;
        e.scroll = &sc;
        Keyboard ek;
        kbd_init(&ek);
        e.kbd = &ek;
        e.prepare(&e, 24);
        CHECK(sc.cam.x == 0 && sc.cam.y == 768, "prepare parks camera");
        e.move_player(&e);
        e.move_enems(&e);
        e.calc_frames(&e);
        e.mode = 1;
        e.draw(&e, 0); /* full scroll draw, no crash */

        /* world hotspot pickup: screen24 -> tile (11, 51) */
        pl.x = (11 * 16) << 6;
        pl.y = (51 * 16) << 6;
        pl.objects = 0;
        clear_sfx();
        scroll_collect(&e);
        CHECK(pl.objects == 1, "scroll hotspot pickup");
        CHECK(sfx_counts[6] == 1, "scroll pickup sfx");

        pcx_free(&tiles_img);
        pcx_free(&sprs_img);
        world_free(&sc.world);
        screen_quit(&scr);
    }

    world_free(&world);
    free(map);
    printf("phase5: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
