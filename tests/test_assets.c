/* Phase 1 parity test: C loaders vs lala-js expectations.
 * Ground truth established from dist/assets + JS semantics:
 *   MAP 8640 B | TILEPROP 41 pairs | SPRPROP 21 pairs | SPRMAP 44 ints (pad 64)
 *   ENEMS 30x4 (1080 ints) | HOTSPOTS 30 | FNT 2305 B | 5x PCX dims | 10x WAV header
 * Exit 0 = all pass, 1 = any failure. No SDL needed (pure C + stdio). */
#include "assets.h"
#include "fnt.h"
#include "map_load.h"
#include "pcx.h"
#include "txt.h"
#include "wav.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail = 0;
static int g_pass = 0;

#define CHECK(cond, fmt, ...) do { \
    if (cond) { g_pass++; } \
    else { g_fail++; printf("FAIL %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); } \
} while (0)

int main(void) {
    /* ---- LALA.MAP ---- */
    {
        uint8_t *map = NULL;
        size_t len = 0;
        CHECK(map_load(&map, &len) == 0, "map_load ok");
        CHECK(len == MAP_EXPECTED_LEN, "MAP len %zu == %d", len, MAP_EXPECTED_LEN);
        free(map);
    }
    /* ---- TILEPROP.TXT ---- */
    {
        char path[512];
        CHECK(resolve_asset("MAP/TILEPROP.TXT", path, sizeof(path)) != NULL, "tileprop resolves");
        TileProp tp[TILEPROP_MAX];
        int n = tileprops_load(path, tp, TILEPROP_MAX);
        CHECK(n == 41, "tileprops n=%d == 41", n);
        CHECK(tp[0].location == 0 && tp[0].flags == 0, "tileprop[0]=(%d,%d)==(0,0)",
            tp[0].location, tp[0].flags);
        CHECK(tp[1].location == 1 && tp[1].flags == 8, "tileprop[1]=(%d,%d)==(1,8)",
            tp[1].location, tp[1].flags);
    }
    /* ---- SPRPROP ---- */
    {
        char path[512];
        CHECK(resolve_asset("GFX/SPRPROP.TXT", path, sizeof(path)) != NULL, "sprprop resolves");
        SprProp sp[SPRPROP_MAX];
        int n = sprprops_load(path, sp, SPRPROP_MAX);
        CHECK(n == 21, "sprprops n=%d == 21", n);
        CHECK(sp[0].off_x == 4 && sp[0].off_y == 8, "sprprop[0]=(%d,%d)==(4,8)",
            sp[0].off_x, sp[0].off_y);
    }
    /* ---- SPRMAP (44 ints, padded to 64) ---- */
    {
        char path[512];
        CHECK(resolve_asset("GFX/SPRMAP.TXT", path, sizeof(path)) != NULL, "sprmap resolves");
        int sm[SPRMAP_LEN];
        int n = sprmap_load(path, sm);
        int head[] = {0, 1, 0, 2, 1, 2, 3, 4};
        CHECK(n == 44, "sprmap parsed=%d == 44", n);
        CHECK(memcmp(sm, head, sizeof(head)) == 0, "sprmap head matches JS");
        CHECK(sm[SPRMAP_LEN - 1] == 0, "sprmap tail padded with 0");
    }
    /* ---- ENEMS ---- */
    {
        char path[512];
        CHECK(resolve_asset("MAP/ENEMS.TXT", path, sizeof(path)) != NULL, "enems resolves");
        static EnemRaw en[ENEM_PANTS][ENEM_PER_PANT];
        CHECK(enems_load(path, en) == 0, "enems_load consumes 1080 ints");
        EnemRaw *e = &en[0][0];
        CHECK(e->x == 96 && e->y == 32 && e->x1 == 96 && e->y1 == 32 &&
              e->x2 == 112 && e->y2 == 160 && e->mx == 1 && e->my == 1 && e->t == 3,
            "enem[0][0]=(%d,%d,%d,%d,%d,%d,%d,%d,%d)",
            e->x, e->y, e->x1, e->y1, e->x2, e->y2, e->mx, e->my, e->t);
    }
    /* ---- HOTSPOTS ---- */
    {
        char path[512];
        CHECK(resolve_asset("MAP/HOTSPOTS.TXT", path, sizeof(path)) != NULL, "hotspots resolves");
        HotSpotRaw hs[HOTSPOT_PANTS];
        CHECK(hotspots_load(path, hs) == HOTSPOT_PANTS, "hotspots n=30");
        CHECK(hs[0].x == 14 && hs[0].y == 1 && hs[0].t == 2,
            "hotspot[0]=(%d,%d,%d)==(14,1,2)", hs[0].x, hs[0].y, hs[0].t);
    }
    /* ---- FNT ---- */
    {
        char path[512];
        CHECK(resolve_asset("GFX/lala.fnt", path, sizeof(path)) != NULL, "fnt resolves");
        Font f;
        CHECK(fnt_load_file(path, &f) == 0, "fnt_load ok (2305 B)");
        CHECK(f.widths[32] == 8, "fnt widths[space]==8");
        uint8_t expectA[8] = {0, 60, 118, 118, 252, 252, 236, 0};
        CHECK(memcmp(f.glyph[65].rows, expectA, 8) == 0, "fnt glyph 'A' matches");
        uint8_t expectSp[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        CHECK(memcmp(f.glyph[32].rows, expectSp, 8) == 0, "fnt glyph space empty");
    }
    /* ---- PCX dims + palette ---- */
    {
        struct { const char *rel; int w, h; } cases[] = {
            {"GFX/TILESET.PCX", 320, 48},
            {"GFX/SPRSET.PCX", 192, 192},
            {"GFX/BACKDROP.PCX", 320, 192},
            {"GFX/TITLE.PCX", 320, 200},
            {"GFX/ENDING.PCX", 320, 200},
        };
        for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
            char path[512];
            CHECK(resolve_asset(cases[i].rel, path, sizeof(path)) != NULL,
                "%s resolves", cases[i].rel);
            PcxImage img;
            memset(&img, 0, sizeof(img));
            CHECK(pcx_load_file(path, &img) == 0, "%s decodes", cases[i].rel);
            CHECK(img.width == cases[i].w && img.height == cases[i].h,
                "%s %dx%d == %dx%d", cases[i].rel, img.width, img.height,
                cases[i].w, cases[i].h);
            CHECK(img.palette != NULL, "%s has 768B palette", cases[i].rel);
            pcx_free(&img);
        }
    }
    /* ---- WAV headers: PCM/mono/11025/8-bit (JS WAVLoader wrongly assumed 16-bit) ---- */
    {
        const char *names[] = {"AH", "AMBIENT1", "AMBIENT2", "BOLT", "HIT",
                               "JUMP", "KEY", "LIFE", "OBJECT", "PINCHE"};
        for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
            char rel[64];
            snprintf(rel, sizeof(rel), "SFX/%s.WAV", names[i]);
            char path[512];
            CHECK(resolve_asset(rel, path, sizeof(path)) != NULL, "%s resolves", rel);
            WavInfo wi;
            CHECK(wav_probe_file(path, &wi) == 0, "%s probes", rel);
            CHECK(wi.audio_format == 1 && wi.channels == 1 &&
                  wi.sample_rate == 11025 && wi.bits_per_sample == 8,
                "%s pcm/mono/11025/8 (got %d/%d/%d/%d)", rel,
                wi.audio_format, wi.channels, wi.sample_rate, wi.bits_per_sample);
            CHECK(wi.data_len > 0, "%s data_len=%u > 0", rel, wi.data_len);
        }
    }

    printf("phase1: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
