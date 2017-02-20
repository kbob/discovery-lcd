#include "graymap.h"

#include <stdlib.h>

graymap *alloc_graymap(size_t w, int ascent, int descent, int line_height)
{
    size_t height = ascent + descent;
    graymap *map = (graymap *)calloc(1, sizeof *map);
    map->pixels = (uint8_t *)calloc(w * height, sizeof *map->pixels);
    map->w = w;
    map->h = height;
    map->pitch = w * sizeof *map->pixels;
    map->ascent = ascent;
    map->descent = descent;
    map->line_height = line_height;
    return map;
}

void free_graymap(graymap *map)
{
    free(map->pixels);
    free(map);
}
