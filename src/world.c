#include <stdlib.h>
#include <math.h>
#include <libtcod.h>

#include "world.h"
#include "config.h"
#include "game.h"

void world_init(void)
{
    maps = TCOD_list_new();
}

void world_turn(void)
{
#if SIMULATE_ALL_MAPS
    for (map_t **iterator = (map_t **)TCOD_list_begin(maps);
         iterator != (map_t **)TCOD_list_end(maps);
         iterator++)
    {
        map_t *map = *iterator;

        map_update(map);
    }
#else
    map_update(current_map);
#endif
}

void world_tick(void)
{
}

void world_draw_turn(void)
{
    TCOD_map_t fov_map = map_calc_fov(current_map, player->x, player->y, actorinfo[player->type].sight_radius);

    float r2 = pow(actorinfo[player->type].sight_radius, 2);

    for (int x = view_left; x < view_left + view_right; x++)
    {
        for (int y = view_top; y < view_top + view_bottom; y++)
        {
            if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
            {
                continue;
            }

            for (actor_t **iterator = (actor_t **)TCOD_list_begin(current_map->actors);
                 iterator != (actor_t **)TCOD_list_end(current_map->actors);
                 iterator++)
            {
                actor_t *actor = *iterator;

                if (!TCOD_map_is_in_fov(fov_map, actor->x, actor->y))
                {
                    continue;
                }

                if (actor->x == x && actor->y == y)
                {
                    goto out;
                }
            }

            tile_t *tile = &current_map->tiles[x][y];

            TCOD_color_t color;
            if (TCOD_map_is_in_fov(fov_map, x, y))
            {
                tile->seen = true;

                float d = pow(x - player->x, 2) + pow(y - player->y, 2);
                float l = CLAMP(0.0f, 1.0f, (r2 - d) / r2);
                if (torch)
                {
                    color = TCOD_color_lerp(tileinfo[tile->type].dark_color, TCOD_color_lerp(tileinfo[tile->type].light_color, torch_color, l), l);
                }
                else
                {
                    color = TCOD_color_lerp(tileinfo[tile->type].dark_color, tileinfo[tile->type].light_color, l);
                }
            }
            else
            {
                if (tile->seen)
                {
                    color = tileinfo[tile->type].dark_color;
                }
                else
                {
                    continue;
                }
            }

            TCOD_console_set_char_foreground(NULL, x - view_left, y - view_top, color);
            TCOD_console_set_char(NULL, x - view_left, y - view_top, tileinfo[tile->type].glyph);

        out:;
        }
    }

    for (actor_t **iterator = (actor_t **)TCOD_list_begin(current_map->actors);
         iterator != (actor_t **)TCOD_list_end(current_map->actors);
         iterator++)
    {
        actor_t *actor = *iterator;

        if (!TCOD_map_is_in_fov(fov_map, actor->x, actor->y))
        {
            continue;
        }

        TCOD_console_set_char_foreground(NULL, actor->x - view_left, actor->y - view_top, actorinfo[actor->type].color);
        TCOD_console_set_char(NULL, actor->x - view_left, actor->y - view_top, actorinfo[actor->type].glyph);
    }

    TCOD_map_delete(fov_map);
}

void world_draw_tick(void)
{
    if (sfx)
    {
        if (torch)
        {
            TCOD_map_t fov_map = map_calc_fov(current_map, player->x, player->y, actorinfo[player->type].sight_radius);

            static TCOD_noise_t noise = NULL;
            if (noise == NULL)
            {
                noise = TCOD_noise_new(1, TCOD_NOISE_DEFAULT_HURST, TCOD_NOISE_DEFAULT_LACUNARITY, NULL);
            }

            static float torchx = 0.0f;
            float tdx;
            float dx;
            float dy;
            float di;

            torchx += 0.2f;
            tdx = torchx + 20.0f;
            dx = TCOD_noise_get(noise, &tdx) * 0.5f;
            tdx += 30.0f;
            dy = TCOD_noise_get(noise, &tdx) * 0.5f;
            di = 0.2f * TCOD_noise_get(noise, &torchx);

            float r2 = pow(actorinfo[player->type].sight_radius, 2);

            for (int x = view_left; x < view_left + view_right; x++)
            {
                for (int y = view_top; y < view_top + view_bottom; y++)
                {
                    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
                    {
                        continue;
                    }

                    if (!TCOD_map_is_in_fov(fov_map, x, y))
                    {
                        continue;
                    }

                    for (actor_t **iterator = (actor_t **)TCOD_list_begin(current_map->actors);
                         iterator != (actor_t **)TCOD_list_end(current_map->actors);
                         iterator++)
                    {
                        actor_t *actor = *iterator;

                        if (!TCOD_map_is_in_fov(fov_map, actor->x, actor->y))
                        {
                            continue;
                        }

                        if (actor->x == x && actor->y == y)
                        {
                            goto out;
                        }
                    }

                    tile_t *tile = &current_map->tiles[x][y];

                    float d = pow(x - player->x + dx, 2) + pow(y - player->y + dy, 2);
                    float l = CLAMP(0.0f, 1.0f, (r2 - d) / r2 + di);
                    TCOD_color_t color = TCOD_color_lerp(tileinfo[tile->type].dark_color, TCOD_color_lerp(tileinfo[tile->type].light_color, torch_color, l), l);

                    TCOD_console_set_char_foreground(NULL, x - view_left, y - view_top, color);

                out:;
                }
            }

            TCOD_map_delete(fov_map);
        }
    }
}

void world_destroy(void)
{
    for (map_t **iterator = (map_t **)TCOD_list_begin(maps);
         iterator != (map_t **)TCOD_list_end(maps);
         iterator++)
    {
        map_t *map = *iterator;

        TCOD_list_clear_and_delete(map->actors);
    }

    TCOD_list_clear_and_delete(maps);
}

map_t *map_create(void)
{
    map_t *map = (map_t *)malloc(sizeof(map_t));

    for (int x = 0; x < MAP_WIDTH; x++)
    {
        for (int y = 0; y < MAP_HEIGHT; y++)
        {
            tile_t *tile = &map->tiles[x][y];
            tile->type = TILETYPE_WALL;
            tile->seen = false;
        }
    }

    map->rooms = TCOD_list_new();
    TCOD_bsp_t *bsp = TCOD_bsp_new_with_size(0, 0, MAP_WIDTH, MAP_HEIGHT);
    TCOD_bsp_split_recursive(bsp, NULL, BSP_DEPTH, MIN_ROOM_SIZE + 1, MIN_ROOM_SIZE + 1, 1.5f, 1.5f);
    TCOD_bsp_traverse_inverted_level_order(bsp, traverse_node, map);
    TCOD_bsp_delete(bsp);

    room_t *stair_down_room = map_get_random_room(map);
    room_get_random_pos(stair_down_room, &map->stair_down_x, &map->stair_down_y);
    map->tiles[map->stair_down_x][map->stair_down_y].type = TILETYPE_STAIR_DOWN;

    room_t *stair_up_room = map_get_random_room(map);
    room_get_random_pos(stair_up_room, &map->stair_up_x, &map->stair_up_y);
    map->tiles[map->stair_up_x][map->stair_up_y].type = TILETYPE_STAIR_UP;

    map->actors = TCOD_list_new();
    for (int i = 0; i < NUM_ACTORS; i++)
    {
        room_t *actor_room = map_get_random_room(map);

        if (actor_room == stair_up_room)
        {
            continue;
        }

        actor_t *actor = actor_create(map, ACTORTYPE_MONSTER, 0, 0);
        room_get_random_pos(actor_room, &actor->x, &actor->y);
    }

    TCOD_list_push(maps, map);

    return map;
}

bool traverse_node(TCOD_bsp_t *node, map_t *map)
{
    if (TCOD_bsp_is_leaf(node))
    {
        int min_x = node->x + 1;
        int max_x = node->x + node->w - 1;
        int min_y = node->y + 1;
        int max_y = node->y + node->h - 1;

        if (max_x == MAP_WIDTH - 1)
        {
            max_x--;
        }
        if (max_y == MAP_HEIGHT - 1)
        {
            max_y--;
        }

#if FULL_ROOMS
        min_x = TCOD_random_get_int(NULL, min_x, max_x - MIN_ROOM_SIZE + 1);
        min_y = TCOD_random_get_int(NULL, min_y, max_y - MIN_ROOM_SIZE + 1);
        max_x = TCOD_random_get_int(NULL, min_x + MIN_ROOM_SIZE - 2, max_x);
        max_y = TCOD_random_get_int(NULL, min_y + MIN_ROOM_SIZE - 2, max_y);
#endif

        node->x = min_x;
        node->y = min_y;
        node->w = max_x - min_x + 1;
        node->h = max_y - min_y + 1;

        for (int x = min_x; x < max_x + 1; x++)
        {
            for (int y = min_y; y < max_y + 1; y++)
            {
                tile_t *tile = &map->tiles[x][y];
                tile->type = TILETYPE_FLOOR;
            }
        }

        room_t *room = (room_t *)malloc(sizeof(room_t));

        room->x = node->x;
        room->y = node->y;
        room->w = node->w;
        room->h = node->h;

        TCOD_list_push(map->rooms, room);
    }
    else
    {
        TCOD_bsp_t *left = TCOD_bsp_left(node);
        TCOD_bsp_t *right = TCOD_bsp_right(node);

        node->x = min(left->x, right->x);
        node->y = min(left->y, right->y);
        node->w = max(left->x + left->w, right->x + right->w) - node->x;
        node->h = max(left->y + left->h, right->y + right->h) - node->y;

        if (node->horizontal)
        {
            if (left->x + left->w - 1 < right->x || right->x + right->w - 1 < left->x)
            {
                int x1 = TCOD_random_get_int(NULL, left->x, left->x + left->w - 1);
                int x2 = TCOD_random_get_int(NULL, right->x, right->x + right->w - 1);
                int y = TCOD_random_get_int(NULL, left->y + left->h, right->y);

                vline_up(map, x1, y - 1);
                hline(map, x1, y, x2);
                vline_down(map, x2, y + 1);
            }
            else
            {
                int min_x = max(left->x, right->x);
                int max_x = min(left->x + left->w - 1, right->x + right->w - 1);
                int x = TCOD_random_get_int(NULL, min_x, max_x);

                while (x > MAP_WIDTH - 1)
                {
                    x--;
                }

                vline_down(map, x, right->y);
                vline_up(map, x, right->y - 1);
            }
        }
        else
        {
            if (left->y + left->h - 1 < right->y || right->y + right->h - 1 < left->y)
            {
                int y1 = TCOD_random_get_int(NULL, left->y, left->y + left->h - 1);
                int y2 = TCOD_random_get_int(NULL, right->y, right->y + right->h - 1);
                int x = TCOD_random_get_int(NULL, left->x + left->w, right->x);

                hline_left(map, x - 1, y1);
                vline(map, x, y1, y2);
                hline_right(map, x + 1, y2);
            }
            else
            {
                int min_y = max(left->y, right->y);
                int max_y = min(left->y + left->h - 1, right->y + right->h - 1);
                int y = TCOD_random_get_int(NULL, min_y, max_y);

                while (y > MAP_HEIGHT - 1)
                {
                    y--;
                }

                hline_left(map, right->x - 1, y);
                hline_right(map, right->x, y);
            }
        }
    }

    return true;
}

void vline(map_t *map, int x, int y1, int y2)
{
    if (y1 > y2)
    {
        int t = y1;
        y1 = y2;
        y2 = t;
    }

    for (int y = y1; y < y2 + 1; y++)
    {
        tile_t *tile = &map->tiles[x][y];
        tile->type = TILETYPE_FLOOR;
    }
}

void vline_up(map_t *map, int x, int y)
{
    tile_t *tile = &map->tiles[x][y];

    while (y >= 0 && tile->type != TILETYPE_FLOOR)
    {
        tile->type = TILETYPE_FLOOR;
        y--;
    }
}

void vline_down(map_t *map, int x, int y)
{
    tile_t *tile = &map->tiles[x][y];

    while (y < MAP_HEIGHT && tile->type != TILETYPE_FLOOR)
    {
        tile->type = TILETYPE_FLOOR;
        y++;
    }
}

void hline(map_t *map, int x1, int y, int x2)
{
    if (x1 > x2)
    {
        int t = x1;
        x1 = x2;
        x2 = t;
    }

    for (int x = x1; x < x2 + 1; x++)
    {
        tile_t *tile = &map->tiles[x][y];
        tile->type = TILETYPE_FLOOR;
    }
}

void hline_left(map_t *map, int x, int y)
{
    tile_t *tile = &map->tiles[x][y];
    while (x >= 0 && tile->type != TILETYPE_FLOOR)
    {
        tile->type = TILETYPE_FLOOR;
        x--;
    }
}

void hline_right(map_t *map, int x, int y)
{
    tile_t *tile = &map->tiles[x][y];
    while (x < MAP_WIDTH && tile->type != TILETYPE_FLOOR)
    {
        tile->type = TILETYPE_FLOOR;
        x++;
    }
}

void map_update(map_t *map)
{
    for (actor_t **iterator = (actor_t **)TCOD_list_begin(map->actors);
         iterator != (actor_t **)TCOD_list_end(map->actors);
         iterator++)
    {
        actor_t *actor = *iterator;

        actor_update(map, actor);
    }

    for (actor_t **iterator = (actor_t **)TCOD_list_begin(map->actors);
         iterator != (actor_t **)TCOD_list_end(map->actors);
         iterator++)
    {
        actor_t *actor = *iterator;

        if (actor->mark_for_delete)
        {
            iterator = (actor_t **)TCOD_list_remove_iterator_fast(map->actors, (void **)iterator);

            free(actor);
        }
    }
}

TCOD_map_t map_to_TCOD_map(map_t *map)
{
    TCOD_map_t TCOD_map = TCOD_map_new(MAP_WIDTH, MAP_HEIGHT);

    for (int x = 0; x < MAP_WIDTH; x++)
    {
        for (int y = 0; y < MAP_HEIGHT; y++)
        {
            tile_t *tile = &map->tiles[x][y];

            TCOD_map_set_properties(TCOD_map, x, y, tileinfo[tile->type].is_transparent, tileinfo[tile->type].is_walkable);
        }
    }

    for (actor_t **iterator = (actor_t **)TCOD_list_begin(map->actors);
         iterator != (actor_t **)TCOD_list_end(map->actors);
         iterator++)
    {
        actor_t *actor = *iterator;

        TCOD_map_set_properties(TCOD_map, actor->x, actor->y, TCOD_map_is_transparent(TCOD_map, actor->x, actor->y), false);
    }

    return TCOD_map;
}

room_t *map_get_random_room(map_t *map)
{
    return TCOD_list_get(map->rooms, TCOD_random_get_int(NULL, 0, TCOD_list_size(map->rooms) - 1));
}

TCOD_map_t map_calc_fov(map_t *map, int x, int y, int radius)
{
    TCOD_map_t fov_map = map_to_TCOD_map(map);

    TCOD_map_compute_fov(fov_map, x, y, radius, true, FOV_DIAMOND);

#if LIT_ROOMS
    for (room_t **iterator = (room_t **)TCOD_list_begin(map->rooms);
         iterator != (room_t **)TCOD_list_end(map->rooms);
         iterator++)
    {
        room_t *room = *iterator;

        if (!room_is_inside(room, x, y))
        {
            continue;
        }

        for (int x = room->x - 1; x <= room->x + room->w; x++)
        {
            for (int y = room->y - 1; y <= room->y + room->h; y++)
            {
                TCOD_map_set_in_fov(fov_map, x, y, true);
            }
        }
    }
#endif

    return fov_map;
}

TCOD_path_t map_calc_path(map_t *map, int ox, int oy, int dx, int dy)
{
    TCOD_map_t path_map = map_to_TCOD_map(map);

    for (actor_t **iterator = (actor_t **)TCOD_list_begin(map->actors);
         iterator != (actor_t **)TCOD_list_end(map->actors);
         iterator++)
    {
        actor_t *actor = *iterator;

        if (actor->x == ox && actor->y == oy)
        {
            TCOD_map_set_properties(path_map, dx, dy, TCOD_map_is_transparent(path_map, dx, dy), true);
        }
    }

    TCOD_path_t path = TCOD_path_new_using_map(path_map, 1.0f);
    TCOD_path_compute(path, ox, oy, dx, dy);

    TCOD_map_delete(path_map);

    return path;
}

void map_destroy(map_t *map)
{
    TCOD_list_remove(maps, map);

    free(map);
}

void room_get_random_pos(room_t *room, int *x, int *y)
{
    *x = TCOD_random_get_int(NULL, room->x, room->x + room->w - 1);
    *y = TCOD_random_get_int(NULL, room->y, room->y + room->h - 1);
}

bool room_is_inside(room_t *room, int x, int y)
{
    return min(room->x, room->x + room->w) <= x && x < max(room->x, room->x + room->w) && min(room->y, room->y + room->h) <= y && y < max(room->y, room->y + room->h);
}

actor_t *actor_create(map_t *map, actortype_t type, int x, int y)
{
    actor_t *actor = (actor_t *)malloc(sizeof(actor_t));

    actor->type = type;
    actor->x = x;
    actor->y = y;
    actor->target_x = -1;
    actor->target_y = -1;
    actor->mark_for_delete = false;

    TCOD_list_push(map->actors, actor);

    return actor;
}

void actor_update(map_t *map, actor_t *actor)
{
    if (actor == player)
    {
        return;
    }

    if (TCOD_random_get_int(NULL, 0, 1) == 0)
    {
        TCOD_map_t fov_map = map_calc_fov(map, actor->x, actor->y, actorinfo[actor->type].sight_radius);

        for (actor_t **iterator = (actor_t **)TCOD_list_begin(map->actors);
             iterator != (actor_t **)TCOD_list_end(map->actors);
             iterator++)
        {
            actor_t *other = *iterator;

            if (other == actor)
            {
                continue;
            }

            if (TCOD_map_is_in_fov(fov_map, other->x, other->y))
            {
                actor_target_set(actor, other->x, other->y);
                actor_target_moveto(map, actor);
            }
        }

        TCOD_map_delete(fov_map);
    }
    else
    {
        int dir = TCOD_random_get_int(NULL, 0, 8);
        switch (dir)
        {
        case 0:
            actor_move(map, actor, actor->x, actor->y - 1);
            break;
        case 1:
            actor_move(map, actor, actor->x, actor->y + 1);
            break;
        case 2:
            actor_move(map, actor, actor->x - 1, actor->y);
            break;
        case 3:
            actor_move(map, actor, actor->x + 1, actor->y);
            break;
        }
    }
}

void actor_move(map_t *map, actor_t *actor, int x, int y)
{
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
    {
        return;
    }

    tile_t *tile = &map->tiles[x][y];
    if (!tileinfo[tile->type].is_walkable)
    {
        return;
    }

    for (actor_t **iterator = (actor_t **)TCOD_list_begin(map->actors);
         iterator != (actor_t **)TCOD_list_end(map->actors);
         iterator++)
    {
        actor_t *other = *iterator;

        if (other->x != x || other->y != y)
        {
            continue;
        }

        // TODO: damage and health
        // TODO: player death
        // TODO: dealing with corpses, is_dead flag or separate object altogether?
        // if corpses can be resurrected, they will need to store information about the actor
        // if corpses can be picked up, they will need to act like items
        if (other != player)
        {
            other->mark_for_delete = true;
        }

        return;
    }

    actor->x = x;
    actor->y = y;
}

void actor_target_set(actor_t *actor, int x, int y)
{
    actor->target_x = x;
    actor->target_y = y;
}

bool actor_target_moveto(map_t *map, actor_t *actor)
{
    if (actor->target_x == -1 || actor->target_y == -1)
    {
        return false;
    }

    TCOD_path_t path = map_calc_path(map, actor->x, actor->y, actor->target_x, actor->target_y);

    int x, y;
    bool valid_path = !TCOD_path_is_empty(path) && TCOD_path_walk(path, &x, &y, false);

    if (valid_path)
    {
        actor_move(map, actor, x, y);
    }
    else
    {
        actor_target_set(actor, -1, -1);
    }

    TCOD_path_delete(path);

    return valid_path;
}

void actor_destroy(map_t *map, actor_t *actor)
{
    TCOD_list_remove(map->actors, actor);

    free(actor);
}