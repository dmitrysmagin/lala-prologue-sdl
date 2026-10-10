# lala-js → lala-sdl (pure C + SDL2 + mingw32-gcc) — Plan

Port of LaLa Prologue (Mojon Twins, QB 4.5) from `lala-js` (TypeScript/Canvas)
to pure C99 + SDL2, built with mingw32-gcc. Keeps both engines.

## 1. Source survey (lala-js, ~4843 LOC)

| JS module | LOC | C target |
|---|---|---|
| `screen.ts` (4×320×200 layers, palette LUT, blitters, fades) | 550 | `screen.c` |
| `engine/player.ts` (fixed 1/64px physics, gravity/jump/friction, tile collide, keyholes) | 236 | shared `player.c` + 2 wrappers |
| `engine/game-loop.ts` (flip-screen loop, transitions, HUD) | 348 | `engine_flip.c` |
| `engine/scroll-game-loop.ts` + `scroll-player/enemies/render/camera` | 310+222+151+90+35 | `engine_scroll.c` |
| `engine/map.ts`, `scroll-types.ts` (screen prepare, world build, enemy/hotspot convert) | 275+137 | `map.c`, `world.c` |
| `assets/PCX,FNT,TXT,MAP,WAV` | ~350 | `pcx.c fnt.c txt.c map_load.c` |
| `keyboard.ts` (QB scancodes) | 216 | `keyboard.c` (SDL→QB map) |
| `audio/music-player,sound-effects.ts` (OGG music + 10×WAV) | 336 | `audio.c` via `SDL2_mixer` |
| `states/title.ts,ending.ts`, `render.ts` (FNT text, stats) | 208+159+187 | `state_title.c state_ending.c render.c font.c` |
| `main.ts,prefs.ts,types.ts,config.ts` | 279+114+145+12 | `main.c prefs.c types.h config.h` |

Assets in `lala-js/public/`: `GFX/` (TILESET/SPRSET/BACKDROP/TITLE/ENDING.PCX,
`lala.fnt`, `SPRPROP/SPRMAP.TXT`, `LALA.BMA` — blender map, optional),
`MAP/` (`LALA.MAP` 8640 B, `TILEPROP/ENEMS/HOTSPOTS.TXT`),
`MUSIC/*.ogg` (DESORUIN/G66A/MC12), `SFX/*.WAV` (10× 8-bit 11025 Hz mono).
Toolchain verified: `gcc 15.2.0 x86_64-w64-mingw32`, `SDL2 2.32.10 +
SDL2_mixer 2.8.1 + SDL_image headers present` via `pkg-config`.

## 2. Destination layout (`lala-sdl/`)

```
lala-sdl/
├── PLAN-SDL.md
├── Makefile            # CC=gcc -std=c99 -O2 -Wall; pkg-config sdl2 SDL2_mixer; local .tmp (§6)
├── build.bat           # cmd fallback, also redirects TEMP/TMP to .tmp
├── .tmp/               # local temp for gcc/as/ld (gitignored, auto-created)
├── src/
│   ├── main.c          # init, title→play→ending→title outer loop
│   ├── types.h         # ALL structs (§3), FRONT/BACK/ANIM, DLEFT.., STATENORMAL..
│   ├── prefs.h/.c      # engineInitVals(), engineInitPlayer(), createPlayer()
│   ├── config.h        # {double game_speed=0.8; int scroll_enabled;}
│   ├── pcx.h/.c        # PCX RLE decode + 0x0C+768 palette
│   ├── fnt.h/.c        # DirectQB FNT: 2048 B glyphs + 257 B widths; font_blit()
│   ├── txt.h/.c        # parseInts() → tileprops/sprprops/sprmap/enems/hotspots
│   ├── map_load.h/.c   # fopen reads from dist/assets/ (no fetch)
│   ├── map.h/.c        # engineScreenPrepare(), 22×14 letterbox, cToIdx()
│   ├── world.h/.c      # buildWorldBuff() 120×72 + convertEnemies/HotSpotsToWorld()
│   ├── screen.h/.c     # Virtual VGA (§4)
│   ├── keyboard.h/.c   # QB scancodes 0x1D/1C/4B/48/4D/50/11/12/13/01/39
│   ├── font.h/.c + render.h/.c  # dqbPrint/engineRprint/enginePrintStats/drawHotSpots
│   ├── audio.h/.c      # Mix_Music OGG + 10×Mix_Chunk WAV, playSfx/stopSfx
│   ├── player.h/.c     # shared fixed-point physics helpers
│   ├── engine_if.h     # Engine interface (§3)
│   ├── engine_flip.c   # flip-screen impl
│   ├── engine_scroll.c # scrolling impl
│   ├── state_title.c / state_ending.c
│   └── game.h/.c       # engineDoGame()/scrollDoGame(), backdrop blit
└── dist/
    ├── lala.exe
    └── assets/GFX/ MAP/ MUSIC/ SFX/   # direct copy of lala-js/public/
```

## 3. Pure-C interfaces (structs + function pointers)

```c
/* types.h — all int32_t, no malloc in tick path */
typedef struct { int32_t x,y,vx,vy,frame,facing,subFrame,sprId;
  int32_t gotten,jumping,ctJump,lives,objects,score,keys;
  int32_t attempt,state,ctState,gameOver; } Player;
typedef struct { uint8_t layer1,layer2,behaviour,anim; int32_t realMapIndex; } TileCell;
typedef struct { uint8_t *layer1,*layer2,*behaviour,*anim; uint32_t *realMapIndex;
  int32_t worldW,worldH; } WorldBuff;   /* 120*72 = 8640 */
typedef struct { int32_t x,y,x1,y1,x2,y2,mx,my,t,facing,frame,subFrame,sprId; } Enem;
typedef struct { int32_t x,y,t; int32_t s; } HotSpot;
typedef struct { int32_t x,y; } Camera;

/* engine_if.h — the dual-engine contract */
typedef struct Engine Engine;
struct Engine {
  const char *name;                       /* "flip" | "scroll" */
  void (*prepare)(Engine *e, int32_t nPant);
  void (*movePlayer)(Engine *e);
  void (*moveEnems)(Engine *e);
  void (*draw)(Engine *e, int32_t frame);
  int32_t (*loop)(Engine *e, int32_t flag); /* 0/-1/-2/-3 like JS */
  Screen *scr; Keyboard *kbd; Prefs *prefs; Player *player;
  TileCell *curBuff;      /* flip only (308) */
  WorldBuff *world; Camera *cam;  /* scroll only */
  uint8_t *map; Enem *enems; HotSpot *hots;
  Tileset *tiles; Spriteset *sprs; Backdrop *back;
  Audio *audio;
};
```

Both `engine_flip.c` / `engine_scroll.c` fill this vtable; `main.c`
selects via `config.scroll_enabled` (title-screen ←/→ toggle, same as JS).
Shared `player.c` holds gravity/jump/collide code parametrized by a
`tileBehaviourAt(x,y)` function pointer so flip (letterbox `cToIdx`)
vs scroll (world index) differ in one line.

## 4. Key porting decisions

* **Screen:** `uint8_t layers[4][320*200]` indexed + `uint8_t pal[768]` +
  `SDL_Texture *tex` (ARGB8888). Blits (`blitTile16`, `blitSprite24` skip
  idx 0, `blitFromLayer`, `fillRect`, `putPixel`) write indices;
  `screen_present()` expands via palette to texture,
  `SDL_RenderSetLogicalSize(320,200)`, integer-scale ×3 window 960×600.
  Fades: CPU lerp of `pal[768]` over N frames (same as `fadeTo/fadeIn`).
* **Fixed-point:** keep `x,y,vx,vy` as `int32_t` in 1/64 px units, identical
  constants (`g=12,gMaxVy=192,jumpVyInitial=32,jumpIncr=24,walkAx=16,
  walkVxMax=128,walkFr=24`).
* **Timing:** `SDL_GetTicks64` accumulator: `ticks/sec = 60*game_speed`
  (0.8→48 tps); render every present, physics catch-up loop.
* **Input:** `SDL_PollEvent` → QB scancode set (`LEFT→0x4B, UP→0x48,
  RIGHT→0x4D, DOWN→0x50, LCTRL/RCTRL→0x1D, RETURN→0x1C, W/E/R→0x11/12/13,
  ESC→0x01`). `isDown/justPressed/clear` mirror JS.
* **Audio:** `SDL2_mixer`: `Mix_LoadMUS("assets/MUSIC/*.ogg")`,
  `Mix_LoadWAV("assets/SFX/*.WAV")`, 10 slots.
* **Text:** FNT 8×8 MSB-first blit, `engineRprint` = shadow col 255 + fg 254;
  force pal 254=white, 255=black as `main.ts` does.
* **Map quirk:** mirror JS exactly: `pants = mapW*(mapH-1) = 30`,
  `iniPant=24, iniTX=2, iniTY=1`.

## 5. Build

```make
CC = gcc -std=c99 -O2 -Wall -Wextra
CFLAGS += $(shell pkg-config --cflags sdl2 SDL2_mixer)
LIBS += $(shell pkg-config --libs sdl2 SDL2_mixer)
src → build/*.o → dist/lala.exe
dist/assets ← ../lala-js/public/*
```

## 6. Local `.tmp` temp redirect (Makefile bug workaround)

`gcc`/`as`/`ld` and `pkg-config` shims write temp files to `%TEMP%/%TMP%/%TMPDIR%`.
On this machine the system temp path is unreliable (spaces/AV locks/missing dir
under the opencode sandbox) and breaks parallel builds. Every build therefore
redirects all temp vars to a local, gitignored `.tmp/` inside `lala-sdl/`.

Makefile snippet (authoritative):

```make
TMPDIR_LOCAL := .tmp
$(shell mkdir -p $(TMPDIR_LOCAL))
export TMPDIR := $(abspath $(TMPDIR_LOCAL))
export TEMP   := $(abspath $(TMPDIR_LOCAL))
export TMP    := $(abspath $(TMPDIR_LOCAL))
# also cover GCC/collect2 temp strippings:
export GCC_TMPDIR := $(abspath $(TMPDIR_LOCAL))
```

`build.bat` equivalent:

```bat
mkdir .tmp 2>nul
set TMPDIR=%CD%\.tmp
set TEMP=%CD%\.tmp
set TMP=%CD%\.tmp
set GCC_TMPDIR=%CD%\.tmp
```

Rules:
* `.tmp/` is never committed (`.gitignore` entry).
* All tool invocations (compile, link, `windres` if added later) inherit the
  exported vars — no per-rule overrides.
* `make clean` removes `build/*.o`, `dist/lala.exe`, but keeps `.tmp/`
  (with `make distclean` removing it too).

## 7. Phases (after approval)

0. Skeleton: window+texture+palette, `make run` shows TITLE.PCX.
1. Loaders: PCX/FNT/TXT/MAP unit-tested vs JS output.
2. Screen+font+keyboard.
3. Audio (OGG+WAV).
4. Flip engine → playable.
5. Scroll engine → parity (same physics, camera clamp).
6. Title/ending states, `dist/` packaging, speed toggle.

Risks: OGG total ~7 MB (fine); WAV pitch-shift semantic loss (minor);
`ENEMS.TXT` inverted bounds (`x1>x2`) — keep `===` bounce verbatim.
