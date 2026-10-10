#ifndef ASSETS_H
#define ASSETS_H

#include <stddef.h>
#include <stdint.h>

/* Asset path resolution + file slurp.
 * resolve_asset("GFX/TITLE.PCX") tries, in order:
 *   dist/assets/<rel>   (cwd = lala-sdl, e.g. `make run`)
 *   assets/<rel>         (cwd = dist)
 *   ../lala-js/public/<rel> (fallback to source checkout)
 *   <rel>                (as given)
 * Returns buf on success, NULL if none exists. */
const char *resolve_asset(const char *rel, char *buf, size_t cap);

/* Read whole file into malloc'd buffer. Returns 0 on success. */
int file_slurp(const char *path, uint8_t **out, size_t *len_out);

#endif
