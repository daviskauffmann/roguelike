#include "tile.h"

void tile_setup(struct tile *tile, enum tile_type type, bool seen)
{
    tile->type = type;
    tile->seen = seen;
    tile->object = NULL;
    tile->actor = NULL;
    tile->corpses = TCOD_list_new();
    tile->items = TCOD_list_new();
}

void tile_cleanup(struct tile *tile)
{
    TCOD_list_delete(tile->corpses);
    TCOD_list_delete(tile->items);
}
