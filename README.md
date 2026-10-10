# LaLa SDL

A pure-C (C99) SDL2 port of **LaLa Prologue**, the 2012 DOS platformer by
[The Mojon Twins](https://www.mojontwins.com/juegos_mojonos/lala-prologue-ms-dos),
built with `mingw32-gcc`. No C++, no engine framework — structs, function
pointers, and SDL2/SDL2_mixer.

## About the original game

**LaLa Prologue** (2012, freeware) was written in Microsoft QuickBasic 4.5
using the DirectQB library and Bisqwit's fmEngine for Adlib music. You play
Lala, a witch-in-training who scattered Mistress Morgana's magical potions
across the decrepit surroundings of the academy. Collect all **15 potions**,
use keys to unlock gates, and avoid patrolling enemies and spike traps.

- 320×200, 256 colors (VGA Mode 13h)
- 6×6 grid of 20×12-tile screens
- Fixed-point (1/64 px) physics, tile collision, platform riding

Also playable in DOSBox via
[archive.org](https://archive.org/details/MSDOS-LalaPrologue) or
[DOSGames.com](https://dosgames.com/game/lala-the-magical-prologue).

## Features of this port

- **Dual engines** behind one vtable (`Engine` struct + function pointers):
  original **flip-screen** and smooth-**scrolling** (120×72 world buffer,
  camera follow). Toggle with ←/→ on the title screen.
- **Virtual VGA**: 4 indexed layers, 768-byte palette with fades, tile/sprite
  blitters, nearest-neighbour ×3 upscale (960×600).
- **Original asset formats**, no conversion step: PCX images, DirectQB FNT
  font, raw `LALA.MAP`, comma/whitespace TXT data.
- **Audio via SDL2_mixer**: OGG music (title/game/ending, looped, faded) and
  the 10 original 11025 Hz WAVs in DQB slots 1–10 across 6 voices.
- **Full game flow**: title → play → typewriter ending → title, HUD stats,
  hotspot pickups (potions/keys/lives), keyhole gates, evil tiles, W+E+R cheat.
- **Live speed control** (`-`/`=`): physics tick rate 0.3×–2.0×, wall-clock
  anchored so gameplay is identical on any refresh rate.
- **~319 unit checks** (`make test`) covering loaders, physics, and both engines.

## Requirements

- Windows, MSYS2 UCRT64 with: `gcc`, `pkg-config`, `SDL2`, `SDL2_mixer`
  (verified: gcc 15.2, SDL2 2.32, mixer 2.8)
- Game assets in `../lala-js/public/` (copied into `dist/assets/` by the build)

## Build & run

```bat
make          # dist/lala.exe + dist/assets from a clean checkout
make run      # build and play
make test     # 6 test suites (uses SDL dummy video/audio drivers)
make clean    # drop objects/exe (keeps .tmp); make distclean drops more
build.bat     # cmd.exe fallback without make
```

```bat
dist\lala.exe [flip|scroll]   # engine seed; title screen toggles live
```

`gcc`/`as`/`ld` temp files are redirected to a local gitignored `.tmp/`
(see `PLAN-SDL.md` §6) because the system temp path is unreliable here.

## Controls

| Key | Action |
|---|---|
| ←/→ | Walk (title: toggle flip/scroll engine) |
| Ctrl / ↑ | Jump |
| W+E+R | Cheat (+1 potion) |
| −/= | Game speed down/up (live) |
| D | Debug overlay: enemy + patrol rects (scroll engine) |
| Enter / Space | Start / confirm |
| Esc | Quit to title / exit |

## Layout

```
lala-sdl/
├── src/                  # pure C99, one module per file (+ headers)
│   ├── main.c            # outer loop: title → play → ending
│   ├── types.h           # Player, Prefs, TileCell, Enem, HotSpot, Sheet…
│   ├── prefs.c           # game constants (gravity, speeds, tile ids…)
│   ├── config.c          # game_speed, engine toggle
│   ├── assets.c          # path resolution (dist/assets → lala-js/public)
│   ├── pcx.c fnt.c txt.c wav.c map_load.c   # loaders / probes
│   ├── screen.c          # 4 indexed layers, palette, blitters, fades
│   ├── font.c render.c   # FNT text, HUD stats, hotspot tiles
│   ├── keyboard.c        # SDL → QB scancodes, edge detection
│   ├── audio.c           # OGG music + 10 WAV slots (SDL2_mixer)
│   ├── map.c world.c     # flip letterbox buffer / scroll world buffer
│   ├── collision.c player.c enemies.c        # flip physics + AI
│   ├── scroll_player.c scroll_enemies.c scroll_render.c  # scroll variants
│   ├── engine_if.h       # Engine vtable contract
│   ├── engine_flip.c engine_scroll.c        # the two engines
│   ├── state_title.c state_ending.c         # title + ending screens
│   └── game.c            # asset ownership, run setup, per-run reset
├── tests/                # test_assets/phase2/audio/engine/scroll/states
├── dist/                 # lala.exe + assets/ (GFX, MAP, MUSIC, SFX)
├── Makefile build.bat PLAN-SDL.md
└── README.md
```

## Porting notes (parity decisions)

- Integer physics kept bit-faithful: 1/64 px fixed point, `cToIdx(x,y) =
  1+x+(y+1)*22`, ceiling/floor/wall thresholds (`>7`/`>3`), enemy bounce on
  exact `==` bounds (including inverted `x1 > x2` ranges).
- Sprite `SPRPROP` offsets apply exactly once (net `pos − off`), matching all
  five JS `blitSprite` call sites; the blitter itself still supports offsets.
- Keyholes open on the wall-bump tick (collision snap realigns the player).
- Maps hold 36 screens but enemy/hotspot tables cover 30 (screens 30–35 are
  empty); guarded, not assumed.
- Scroll enemies beyond 400 px are culled; SFX `freq` pitch shift is accepted
  but ignored (mixer has no per-voice resample — only ±9% jump/pain wobble).
- JS `MAPLoader`/`BMALoader`/`WAVLoader(16-bit)` don't match the real files
  (flat 8640 B map, `BMap` blender, 8-bit WAVs) and were intentionally not ported.

## Credits

| Role | Person |
|---|---|
| Game | [The Mojon Twins](https://www.mojontwins.com) |
| fmEngineQB | [Joel Yliluoma (Bisqwit)](https://bisqwit.iki.fi/source/fmengineqb.html) |
| DirectQB | Angelo Mottola (Enhanced Creations) |
| SDL / SDL_mixer | [libsdl.org](https://www.libsdl.org) |
| SDL C port | See git history |

## License

Original game: freeware (GPLv3 per source). This port preserves the original
assets under their original license.
