#ifndef MAP_LOAD_H
#define MAP_LOAD_H

#include <stddef.h>
#include <stdint.h>

/* Raw level map. Mirrors engineMapLoad(): LALA.MAP is a flat byte array of
 * tile indices, mapW*mapH*screenW*screenH = 6*6*20*12 = 8640 bytes.
 * (lala-js MAPLoader.ts 'LALA'-signature format is unrelated dead code —
 * the real file has no header.) */
#define MAP_EXPECTED_LEN 8640

/* Load via resolve_asset("MAP/LALA.MAP"). Mallocs *out. 0 on success. */
int map_load(uint8_t **out, size_t *len_out);

#endif
