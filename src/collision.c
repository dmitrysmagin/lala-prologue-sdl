#include "collision.h"
#include "map.h"

int detect_collision(int i, int32_t nPant, const Enem *enems, int n_pants,
    int per_pant, const Player *pl) {
    if (pl->state != STATENORMAL) return 0;
    if (nPant < 0 || nPant >= n_pants || i < 0 || i >= per_pant) return 0;
    int32_t px = pl->x >> 6;
    int32_t py = pl->y >> 6;
    const Enem *e = &enems[(size_t)nPant * per_pant + i];
    return (e->x >= px - 14 && e->x <= px + 14 &&
            e->y >= py - 14 && e->y <= py + 14);
}

int detect_keyhole(TileCell *buff, uint8_t *map, int xx, int yy) {
    int idx = c_to_idx(xx, yy);
    if (idx < 0 || idx >= CUR_BUFF_SIZE) return 0;
    if (buff[idx].behaviour == 10) {
        buff[idx].layer1 = 0;
        buff[idx].layer2 = 0;
        buff[idx].behaviour = 0;
        if (buff[idx].realMapIndex >= 0) map[buff[idx].realMapIndex] = 0;
        return 1;
    }
    return 0;
}
