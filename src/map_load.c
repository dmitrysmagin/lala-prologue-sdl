#include "map_load.h"
#include "assets.h"

#include <stdio.h>
#include <stdlib.h>

int map_load(uint8_t **out, size_t *len_out) {
    char path[512];
    if (!resolve_asset("MAP/LALA.MAP", path, sizeof(path))) {
        fprintf(stderr, "map_load: LALA.MAP not found (hint: run `make assets`)\n");
        return -1;
    }
    uint8_t *buf = NULL;
    size_t len = 0;
    if (file_slurp(path, &buf, &len) != 0) return -1;
    if (len < MAP_EXPECTED_LEN) {
        fprintf(stderr, "map_load: expected %d bytes, got %zu\n", MAP_EXPECTED_LEN, len);
        free(buf);
        return -1;
    }
    *out = buf;
    if (len_out) *len_out = len;
    return 0;
}
