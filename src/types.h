#ifndef TYPES_H
#define TYPES_H

/* Canonical game structs. Mirrors lala-js engine/types.ts (QB TYPEs from
 * ENGINE.BI). Phase 2 introduces the subset needed by font/render/screen;
 * prefs.c (later phase) fills Prefs values, engine phases fill behaviour. */
#include <stdint.h>

/* QB constants from ENGINE.BI */
#define MAXPREFS 1
#define FRONT 1
#define BACK 0
#define ANIMATED 2

#define DLEFT 1
#define DRIGHT 2
#define DUP 3
#define DDOWN 4

#define STATENORMAL 0
#define STATEFLICKER 1

/* Layer IDs — mirrors DirectQB layer concept (screen.ts) */
#define VIDEO 0
#define LAYER_1 1
#define LAYER_2 2
#define LAYER_3 3
#define NUM_LAYERS 4

typedef struct { int32_t x, y; } IVec2;

typedef struct {
    int32_t x, y;         /* fixed-point 1/64 px */
    int32_t vx, vy;
    int32_t frame;
    int32_t facing;       /* 0 = right, 6 = left (spriteMapping offset) */
    int32_t subFrame;
    int32_t sprId;
    int32_t gotten;       /* on-platform flag */
    int32_t jumping;
    int32_t ctJump;
    int32_t lives;
    int32_t objects;
    int32_t score;
    int32_t keys;
    int32_t attempt;      /* DLEFT/DRIGHT/DUP/DDOWN or 0 */
    int32_t state;        /* STATENORMAL / STATEFLICKER */
    int32_t ctState;
    int32_t gameOver;
} Player;

typedef struct {
    int32_t mapW, mapH;
    int32_t screenW, screenH;
    IVec2 screenPos;      /* {0,4}: VGA shows pixels starting 4 rows down */
    int32_t numTiles;
    int32_t numSprites;
    int32_t gMaxVy, g;
    int32_t jumpVyInitial, jumpVyMax, jumpIncr;
    int32_t walkVxMax, walkAx, walkFr;
    int32_t iniPant, iniTX, iniTY;
    int32_t nEnems, totalEnems, enemPlat;
    int32_t boltTile, lifeTile, objectTile, keyTile;
    int32_t initialLives, maxObjs, refill;
    int32_t hotSpotX, hotSpotY;
    int32_t bgL1, bgL2;
} Prefs;

typedef struct { int location; int flags; } TileProp;
typedef struct { int off_x; int off_y; } SprProp;

typedef struct {
    int32_t x, y, x1, y1, x2, y2, mx, my, t;
    int32_t facing, frame, subFrame, sprId;
} Enem;

typedef struct { int32_t x, y, t, s; } HotSpot;

/* Flip-screen tile buffer cell (mirrors TypeTileLayers). Letterboxed
 * 22x14 = 308 entries; visible 20x12 (1-tile border simplifies collision). */
typedef struct {
    uint8_t layer1, layer2, behaviour, anim;
    int32_t realMapIndex;
} TileCell;

#define CUR_BUFF_W 22
#define CUR_BUFF_H 14
#define CUR_BUFF_SIZE (CUR_BUFF_W * CUR_BUFF_H) /* 308 */

/* Indexed pixel sheet (tileset 16px tiles / spriteset 24px sprites /
 * backdrop). Palette lives in Screen. */
typedef struct {
    int width, height;
    const uint8_t *data;  /* width*height palette indices */
} Sheet;

typedef struct { int32_t x, y; } Camera;

#endif
