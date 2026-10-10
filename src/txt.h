#ifndef TXT_H
#define TXT_H

/* TXT data loaders. Mirrors lala-js engine/map.ts parseInts() +
 * engineLoadTileProperties/SpriteProperties/SpriteMapping/Enems/HotSpots.
 * All ints are decimal, split on commas/whitespace (QB INPUT semantics).
 * Structs are canonical (types.h); Raw aliases keep phase-1 test compat. */

#include "types.h"

#define TILEPROP_MAX 256
#define SPRPROP_MAX 256
#define SPRMAP_LEN 64          /* original REDIM spriteMapping%(63) */
#define ENEM_PANTS 30          /* mapW * (mapH-1) */
#define ENEM_PER_PANT 4        /* prefs.nEnems */
#define HOTSPOT_PANTS 30

typedef Enem EnemRaw;
typedef HotSpot HotSpotRaw;

/* Parse ints from NUL-terminated text into out[0..cap). Returns count.
 * Stops at cap; ignores non-numeric tokens like JS parseInt NaN skip. */
int parse_ints(const char *text, int *out, int cap);

/* Slurp file + parse. Returns count or -1 on I/O error. */
int txt_load_ints(const char *path, int *out, int cap);

int tileprops_load(const char *path, TileProp *out, int cap);
int sprprops_load(const char *path, SprProp *out, int cap);
/* Always writes 64 entries (pads with 0 like JS). Returns parsed count or -1. */
int sprmap_load(const char *path, int out[SPRMAP_LEN]);
/* pant index 0..29, slot 0..3. Returns 0 ok, -1 error (needs 1080 ints). */
int enems_load(const char *path, EnemRaw out[ENEM_PANTS][ENEM_PER_PANT]);
/* Returns hotspot count (30) or -1 on error. */
int hotspots_load(const char *path, HotSpotRaw out[HOTSPOT_PANTS]);

#endif
