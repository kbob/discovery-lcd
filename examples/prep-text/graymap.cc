#include "graymap.h"

#include <stdlib.h>

graymap *alloc_graymap(size_t w, size_t h)
{
    graymap *map = (graymap *)calloc(1, sizeof *map);
    map->pixels = (uint8_t *)calloc(w * h, sizeof *map->pixels);
    map->w = w;
    map->h = h;
    map->pitch = w * sizeof *map->pixels;
    return map;
}

void free_graymap(graymap *map)
{
    free(map->pixels);
    free(map);
}
