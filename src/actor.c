#include <libtcod.h>

#include "CMemLeak.h"
#include "game.h"

#define LIT_ROOMS 0

actor_t *actor_create(map_t *map, int x, int y, unsigned char glyph, TCOD_color_t color, int fov_radius)
{
    actor_t *actor = (actor_t *)malloc(sizeof(actor_t));

    actor->map = map;
    actor->x = x;
    actor->y = y;
    actor->glyph = glyph;
    actor->color = color;
    actor->torch = false;
    actor->items = TCOD_list_new();
    actor->fov_radius = fov_radius;
    actor->fov_map = NULL;
    actor->mark_for_delete = false;

    actor_calc_fov(actor);

    return actor;
}

void actor_turn(actor_t *actor)
{
    actor_calc_fov(actor);

    for (void **i = TCOD_list_begin(actor->items); i != TCOD_list_end(actor->items); i++)
    {
        item_t *item = *i;

        item_turn(item);
    }

    if (actor == player)
    {
        return;
    }

    if (TCOD_random_get_int(NULL, 0, 1) == 0)
    {
        for (void **i = TCOD_list_begin(actor->map->actors); i != TCOD_list_end(actor->map->actors); i++)
        {
            actor_t *other = *i;

            if (other == actor)
            {
                continue;
            }

            if (TCOD_map_is_in_fov(actor->fov_map, other->x, other->y))
            {
                msg_log("{name} spots {name}", actor->map, actor->x, actor->y);

                actor_move_towards(actor, other->x, other->y, true, false);
            }
        }
    }
    else
    {
        switch (TCOD_random_get_int(NULL, 0, 8))
        {
        case 0:
            actor_move(actor, actor->x, actor->y - 1, false, false);

            break;

        case 1:
            actor_move(actor, actor->x, actor->y + 1, false, false);

            break;

        case 2:
            actor_move(actor, actor->x - 1, actor->y, false, false);

            break;

        case 3:
            actor_move(actor, actor->x + 1, actor->y, false, false);

            break;
        }
    }
}

void actor_tick(actor_t *actor)
{
    for (void **i = TCOD_list_begin(actor->items); i != TCOD_list_end(actor->items); i++)
    {
        item_t *item = *i;

        item_tick(item);
    }
}

void actor_calc_fov(actor_t *actor)
{
    if (actor->fov_map != NULL)
    {
        TCOD_map_delete(actor->fov_map);
    }

    actor->fov_map = map_to_TCOD_map(actor->map);

    TCOD_map_compute_fov(actor->fov_map, actor->x, actor->y, actor->fov_radius, true, FOV_DIAMOND);

#if LIT_ROOMS
    for (void **i = TCOD_list_begin(actor->map->rooms); i != TCOD_list_end(actor->map->rooms); i++)
    {
        room_t *room = *i;

        if (!room_is_inside(room, x, y))
        {
            continue;
        }

        for (int x = room->x - 1; x <= room->x + room->w; x++)
        {
            for (int y = room->y - 1; y <= room->y + room->h; y++)
            {
                TCOD_map_set_in_fov(actor->fov_map, x, y, true);
            }
        }
    }
#endif
}

bool actor_move_towards(actor_t *actor, int x, int y, bool attack, bool take_items)
{
    TCOD_map_set_properties(actor->fov_map, x, y, TCOD_map_is_transparent(actor->fov_map, x, y), true);

    TCOD_path_t path = TCOD_path_new_using_map(actor->fov_map, 1.0f);
    TCOD_path_compute(path, actor->x, actor->y, x, y);

    bool success = false;

    if (!TCOD_path_is_empty(path))
    {
        int next_x, next_y;
        if (TCOD_path_walk(path, &next_x, &next_y, false))
        {
            if (next_x != x || next_y != y)
            {
                attack = false;
                take_items = false;
            }

            success = actor_move(actor, next_x, next_y, attack, take_items);
        }
    }

    TCOD_path_delete(path);

    return success;
}

bool actor_move(actor_t *actor, int x, int y, bool attack, bool take_items)
{
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
    {
        return false;
    }

    tile_t *tile = &actor->map->tiles[x][y];
    actor_t *other = tile->actor;

    if (!tile_walkable[tile->type])
    {
        return false;
    }

    if (other != NULL)
    {
        // TODO: damage and health
        // TODO: player death
        // TODO: dealing with corpses, is_dead flag or separate object altogether?
        // if corpses can be resurrected, they will need to store information about the actor
        // if corpses can be picked up, they will need to act like items
        if (attack && other != player)
        {
            msg_log("{name} hits {name} for {damage}", actor->map, actor->x, actor->y);

            other->mark_for_delete = true;
        }

        return true;
    }

    if (take_items && TCOD_list_size(tile->items) > 0)
    {
        for (void **i = TCOD_list_begin(tile->items); i != TCOD_list_end(tile->items); i++)
        {
            item_t *item = *i;

            i = TCOD_list_remove_iterator(tile->items, i);

            TCOD_list_push(actor->items, item);
        }
    }

    actor->map->tiles[actor->x][actor->y].actor = NULL;
    actor->map->tiles[x][y].actor = actor;

    actor->x = x;
    actor->y = y;

    return true;
}

void actor_draw_turn(actor_t *actor)
{
    if (!TCOD_map_is_in_fov(player->fov_map, actor->x, actor->y))
    {
        return;
    }

    TCOD_console_set_char_foreground(NULL, actor->x - view_x, actor->y - view_y, actor->color);
    TCOD_console_set_char(NULL, actor->x - view_x, actor->y - view_y, actor->glyph);
}

void actor_draw_tick(actor_t *actor)
{
}

void actor_destroy(actor_t *actor)
{
    for (void **i = TCOD_list_begin(actor->items); i != TCOD_list_end(actor->items); i++)
    {
        item_t *item = *i;

        item_destroy(item);
    }

    TCOD_list_delete(actor->items);

    if (actor->fov_map != NULL)
    {
        TCOD_map_delete(actor->fov_map);
    }

    free(actor);
}