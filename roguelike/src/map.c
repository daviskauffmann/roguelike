#include <roguelike/roguelike.h>

#define MAP_ALGORITHM_CUSTOM

#define CUSTOM_NUM_ROOM_ATTEMPTS 20
#define CUSTOM_MIN_ROOM_SIZE 5
#define CUSTOM_MAX_ROOM_SIZE 15
#define CUSTOM_PREVENT_OVERLAP_CHANCE 0.5f

#define BSP_MIN_ROOM_SIZE 4
#define BSP_DEPTH 8
#define BSP_RANDOM_ROOMS 0
#define BSP_ROOM_WALLS 1

#define DOOR_CHANCE 0.5f
#define SPAWN_OBJECTS 10
#define SPAWN_ADVENTURERS 5
#define SPAWN_MONSTERS 10
#define SPAWN_ITEMS 20

static void hline(struct map *map, int x1, int y, int x2);
static void hline_left(struct map *map, int x, int y);
static void hline_right(struct map *map, int x, int y);
static void vline(struct map *map, int x, int y1, int y2);
static void vline_up(struct map *map, int x, int y);
static void vline_down(struct map *map, int x, int y);
static bool traverse_node(TCOD_bsp_t *node, struct map *map);

void map_init(struct map *map, int floor)
{
    map->floor = floor;

    for (int x = 0; x < MAP_WIDTH; x++)
    {
        for (int y = 0; y < MAP_HEIGHT; y++)
        {
            struct tile *tile = &map->tiles[x][y];

            tile_init(tile, TILE_TYPE_EMPTY, false);
        }
    }

    map->rooms = TCOD_list_new();
    map->objects = TCOD_list_new();
    map->actors = TCOD_list_new();
    map->items = TCOD_list_new();
    map->projectiles = TCOD_list_new();
}

void map_generate(struct map *map)
{
    for (int x = 0; x < MAP_WIDTH; x++)
    {
        for (int y = 0; y < MAP_HEIGHT; y++)
        {
            struct tile *tile = &map->tiles[x][y];

            tile->type = TILE_TYPE_WALL;
        }
    }

#if defined(MAP_ALGORITHM_CUSTOM)
    for (int i = 0; i < CUSTOM_NUM_ROOM_ATTEMPTS; i++)
    {
        int room_x = TCOD_random_get_int(NULL, 0, MAP_WIDTH);
        int room_y = TCOD_random_get_int(NULL, 0, MAP_HEIGHT);
        int room_w = TCOD_random_get_int(NULL, CUSTOM_MIN_ROOM_SIZE, CUSTOM_MAX_ROOM_SIZE);
        int room_h = TCOD_random_get_int(NULL, CUSTOM_MIN_ROOM_SIZE, CUSTOM_MAX_ROOM_SIZE);

        if (room_x < 2 ||
            room_x + room_w > MAP_WIDTH - 2 ||
            room_y < 2 ||
            room_y + room_h > MAP_HEIGHT - 2)
        {
            continue;
        }

        if (TCOD_random_get_float(NULL, 0, 1) < CUSTOM_PREVENT_OVERLAP_CHANCE)
        {
            bool overlap = false;

            for (int x = room_x - 2; x < room_x + room_w + 2; x++)
            {
                for (int y = room_y - 2; y < room_y + room_h + 2; y++)
                {
                    struct tile *tile = &map->tiles[x][y];

                    if (tile->type == TILE_TYPE_FLOOR)
                    {
                        overlap = true;
                    }
                }
            }

            if (overlap)
            {
                continue;
            }
        }

        for (int x = room_x; x < room_x + room_w; x++)
        {
            for (int y = room_y; y < room_y + room_h; y++)
            {
                struct tile *tile = &map->tiles[x][y];

                tile->type = TILE_TYPE_FLOOR;
            }
        }

        struct room *room = room_create(room_x, room_y, room_w, room_h);

        TCOD_list_push(map->rooms, room);
    }

    for (int i = 0; i < TCOD_list_size(map->rooms) - 1; i++)
    {
        struct room *room = TCOD_list_get(map->rooms, i);
        struct room *next_room = TCOD_list_get(map->rooms, i + 1);

        int x1, y1, x2, y2;
        room_get_random_pos(room, &x1, &y1);
        room_get_random_pos(next_room, &x2, &y2);

        if (TCOD_random_get_int(NULL, 0, 1) == 0)
        {
            vline(map, x1, y1, y2);
            hline(map, x1, y2, x2);
        }
        else
        {
            hline(map, x1, y1, x2);
            vline(map, x2, y1, y2);
        }
    }

    for (int x = 0; x < MAP_WIDTH; x++)
    {
        for (int y = 0; y < MAP_HEIGHT; y++)
        {
            struct tile *tile = &map->tiles[x][y];

            if (tile->type == TILE_TYPE_FLOOR)
            {
                struct tile *neighbors[8];

                neighbors[0] = &map->tiles[x + 0][y - 1];
                neighbors[1] = &map->tiles[x + 1][y - 1];
                neighbors[2] = &map->tiles[x + 1][y + 0];
                neighbors[3] = &map->tiles[x + 1][y + 1];
                neighbors[4] = &map->tiles[x + 0][y + 1];
                neighbors[5] = &map->tiles[x - 1][y + 1];
                neighbors[6] = &map->tiles[x - 1][y + 0];
                neighbors[7] = &map->tiles[x - 1][y - 1];

                for (int i = 0; i < 8; i++)
                {
                    struct tile *neighbor = neighbors[i];

                    if (neighbor->type == TILE_TYPE_EMPTY)
                    {
                        neighbor->type = TILE_TYPE_WALL;
                    }
                }
            }
        }
    }
#elif defined(MAP_ALGORITHM_BSP)
    TCOD_bsp_t *bsp = TCOD_bsp_new_with_size(0, 0, MAP_WIDTH, MAP_HEIGHT);
    TCOD_bsp_split_recursive(
        bsp,
        NULL,
        BSP_DEPTH,
        BSP_MIN_ROOM_SIZE + (BSP_ROOM_WALLS ? 1 : 0),
        BSP_MIN_ROOM_SIZE + (BSP_ROOM_WALLS ? 1 : 0),
        1.5f,
        1.5f);
    TCOD_bsp_traverse_inverted_level_order(bsp, traverse_node, map);
    TCOD_bsp_delete(bsp);
#endif

    for (int x = 0; x < MAP_WIDTH; x++)
    {
        for (int y = 0; y < MAP_HEIGHT; y++)
        {
            struct tile *tile = &map->tiles[x][y];

            bool put_door = false;

            if (tile->type == TILE_TYPE_FLOOR && TCOD_random_get_float(NULL, 0, 1) < DOOR_CHANCE)
            {
                if (map->tiles[x][y - 1].type == TILE_TYPE_FLOOR &&
                    map->tiles[x + 1][y - 1].type == TILE_TYPE_FLOOR &&
                    map->tiles[x - 1][y - 1].type == TILE_TYPE_FLOOR &&
                    map->tiles[x - 1][y].type == TILE_TYPE_WALL &&
                    map->tiles[x + 1][y].type == TILE_TYPE_WALL)
                {
                    put_door = true;
                }

                if (map->tiles[x + 1][y].type == TILE_TYPE_FLOOR &&
                    map->tiles[x + 1][y - 1].type == TILE_TYPE_FLOOR &&
                    map->tiles[x + 1][y + 1].type == TILE_TYPE_FLOOR &&
                    map->tiles[x][y + 1].type == TILE_TYPE_WALL &&
                    map->tiles[x][y - 1].type == TILE_TYPE_WALL)
                {
                    put_door = true;
                }

                if (map->tiles[x][y + 1].type == TILE_TYPE_FLOOR &&
                    map->tiles[x + 1][y + 1].type == TILE_TYPE_FLOOR &&
                    map->tiles[x - 1][y + 1].type == TILE_TYPE_FLOOR &&
                    map->tiles[x - 1][y].type == TILE_TYPE_WALL &&
                    map->tiles[x + 1][y].type == TILE_TYPE_WALL)
                {
                    put_door = true;
                }

                if (map->tiles[x - 1][y].type == TILE_TYPE_FLOOR &&
                    map->tiles[x - 1][y - 1].type == TILE_TYPE_FLOOR &&
                    map->tiles[x - 1][y + 1].type == TILE_TYPE_FLOOR &&
                    map->tiles[x][y + 1].type == TILE_TYPE_WALL &&
                    map->tiles[x][y - 1].type == TILE_TYPE_WALL)
                {
                    put_door = true;
                }
            }

            if (put_door)
            {
                struct object *object = object_create(
                    OBJECT_TYPE_DOOR_CLOSED,
                    map->floor,
                    x,
                    y,
                    TCOD_white,
                    -1,
                    TCOD_white,
                    false);

                TCOD_list_push(map->objects, object);
                tile->object = object;
            }
        }
    }

    {
        do
        {
            struct room *stair_down_room = map_get_random_room(map);
            room_get_random_pos(stair_down_room, &map->stair_down_x, &map->stair_down_y);
        }
        while (map->tiles[map->stair_down_x][map->stair_down_y].object != NULL);

        struct object *object = object_create(
            OBJECT_TYPE_STAIR_DOWN,
            map->floor,
            map->stair_down_x,
            map->stair_down_y,
            TCOD_white,
            -1,
            TCOD_white,
            false);

        struct tile *tile = &map->tiles[map->stair_down_x][map->stair_down_y];

        TCOD_list_push(map->objects, object);
        tile->object = object;
    }

    {
        do
        {
            struct room *stair_up_room = map_get_random_room(map);
            room_get_random_pos(stair_up_room, &map->stair_up_x, &map->stair_up_y);
        }
        while (map->tiles[map->stair_up_x][map->stair_up_y].object != NULL);

        struct object *object = object_create(
            OBJECT_TYPE_STAIR_UP,
            map->floor,
            map->stair_up_x,
            map->stair_up_y,
            TCOD_white,
            -1,
            TCOD_white,
            false);

        struct tile *tile = &map->tiles[map->stair_up_x][map->stair_up_y];

        TCOD_list_push(map->objects, object);
        tile->object = object;
    }

    for (int i = 0; i < SPAWN_OBJECTS; i++)
    {
        struct room *room = map_get_random_room(map);

        int x, y;
        do
        {
            room_get_random_pos(room, &x, &y);
        }
        while (map->tiles[x][y].object != NULL);

        enum object_type type = 0;
        TCOD_color_t color = TCOD_white;
        int light_radius = -1;
        TCOD_color_t light_color = TCOD_white;
        bool light_flicker = false;

        switch (TCOD_random_get_int(NULL, 0, 3))
        {
        case 0:
        {
            type = OBJECT_TYPE_ALTAR;
            light_radius = 3;
            light_color = TCOD_white;
            light_flicker = false;
        }
        break;
        case 1:
        {
            TCOD_color_t random_color = TCOD_color_RGB(
                (unsigned char)TCOD_random_get_int(NULL, 0, 255),
                (unsigned char)TCOD_random_get_int(NULL, 0, 255),
                (unsigned char)TCOD_random_get_int(NULL, 0, 255));
            type = OBJECT_TYPE_BRAZIER;
            color = random_color;
            light_radius = TCOD_random_get_int(NULL, 10, 20);
            light_color = random_color;
            light_flicker = TCOD_random_get_int(NULL, 0, 1) == 0 ? true : false;
        }
        break;
        case 2:
        {
            type = OBJECT_TYPE_FOUNTAIN;
            color = TCOD_blue;
        }
        break;
        case 3:
        {
            type = OBJECT_TYPE_THRONE;
            color = TCOD_yellow;
        }
        break;
        }

        struct object *object = object_create(
            type,
            map->floor,
            x,
            y,
            color,
            light_radius,
            light_color,
            light_flicker);

        struct tile *tile = &map->tiles[x][y];

        TCOD_list_push(map->objects, object);
        tile->object = object;
    }

    for (int i = 0; i < SPAWN_ADVENTURERS; i++)
    {
        struct room *room = map_get_random_room(map);

        int x, y;
        do
        {
            room_get_random_pos(room, &x, &y);
        }
        while (map->tiles[x][y].actor != NULL || map->tiles[x][y].object != NULL);

        enum race race = TCOD_random_get_int(NULL, RACE_DWARF, RACE_HUMAN);
        enum class class = TCOD_random_get_int(NULL, CLASS_BARBARIAN, CLASS_WIZARD);

        const char *filename;
        char *sex;

        switch (race)
        {
        case RACE_DWARF:
        {
            filename = "assets/namegen/mingos_dwarf.cfg";

            if (TCOD_random_get_int(NULL, 0, 1) == 0)
            {
                sex = "dwarf male";
            }
            else
            {
                sex = "dwarf female";
            }
        }
        break;
        default:
        {
            filename = "assets/namegen/mingos_standard.cfg";

            if (TCOD_random_get_int(NULL, 0, 1) == 0)
            {
                sex = "male";
            }
            else
            {
                sex = "female";
            }
        }
        break;
        }

        TCOD_namegen_parse(filename, NULL);
        char *name = TCOD_namegen_generate(sex, false);
        TCOD_namegen_destroy();

        struct actor *actor = actor_create(
            name,
            race,
            class,
            FACTION_GOOD,
            map->floor + 1,
            map->floor,
            x,
            y);

        struct tile *tile = &map->tiles[x][y];

        TCOD_list_push(map->actors, actor);
        tile->actor = actor;
    }

    for (int i = 0; i < SPAWN_MONSTERS; i++)
    {
        struct room *room = map_get_random_room(map);

        int x, y;
        do
        {
            room_get_random_pos(room, &x, &y);
        }
        while (map->tiles[x][y].actor != NULL || map->tiles[x][y].object != NULL);

        enum monster monster = TCOD_random_get_int(NULL, 0, NUM_MONSTERS - 1);
        struct prototype *prototype = &monster_prototype[monster];

        struct actor *actor = actor_create(
            prototype->name,
            prototype->race,
            prototype->class,
            FACTION_EVIL,
            map->floor + 1,
            map->floor,
            x,
            y);

        struct tile *tile = &map->tiles[x][y];

        TCOD_list_push(map->actors, actor);
        tile->actor = actor;
    }

    for (int i = 0; i < SPAWN_ITEMS; i++)
    {
        struct room *room = map_get_random_room(map);

        int x, y;
        room_get_random_pos(room, &x, &y);

        struct item *item = item_create(
            TCOD_random_get_int(NULL, 0, NUM_ITEM_TYPES - 1),
            map->floor,
            x,
            y);

        struct tile *tile = &map->tiles[x][y];

        TCOD_list_push(map->items, item);
        TCOD_list_push(tile->items, item);
    }
}

bool map_is_inside(int x, int y)
{
    return x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT;
}

struct room *map_get_random_room(struct map *map)
{
    return TCOD_list_get(map->rooms, TCOD_random_get_int(NULL, 0, TCOD_list_size(map->rooms) - 1));
}

bool map_is_transparent(struct map *map, int x, int y)
{
    struct tile *tile = &map->tiles[x][y];

    if (tile->object && !object_info[tile->object->type].is_transparent)
    {
        return false;
    }

    return tile_info[tile->type].is_transparent;
}

bool map_is_walkable(struct map *map, int x, int y)
{
    struct tile *tile = &map->tiles[x][y];

    if (tile->object && !object_info[tile->object->type].is_walkable && tile->object->type != OBJECT_TYPE_DOOR_CLOSED)
    {
        return false;
    }

    if (tile->actor && !tile->actor->dead)
    {
        return false;
    }

    return tile_info[tile->type].is_walkable;
}

TCOD_map_t map_to_TCOD_map(struct map *map)
{
    TCOD_map_t TCOD_map = TCOD_map_new(MAP_WIDTH, MAP_HEIGHT);

    for (int x = 0; x < MAP_WIDTH; x++)
    {
        for (int y = 0; y < MAP_HEIGHT; y++)
        {
            TCOD_map_set_properties(TCOD_map, x, y, map_is_transparent(map, x, y), map_is_walkable(map, x, y));
        }
    }

    return TCOD_map;
}

TCOD_map_t map_to_fov_map(struct map *map, int x, int y, int radius)
{
    TCOD_map_t fov_map = map_to_TCOD_map(map);

    TCOD_map_compute_fov(fov_map, x, y, radius, true, FOV_RESTRICTIVE);

    return fov_map;
}

void map_reset(struct map *map)
{
    for (int x = 0; x < MAP_WIDTH; x++)
    {
        for (int y = 0; y < MAP_HEIGHT; y++)
        {
            struct tile *tile = &map->tiles[x][y];

            tile_reset(tile);
        }
    }

    for (void **iterator = TCOD_list_begin(map->rooms); iterator != TCOD_list_end(map->rooms); iterator++)
    {
        struct room *room = *iterator;

        room_destroy(room);
    }

    TCOD_list_delete(map->rooms);

    for (void **iterator = TCOD_list_begin(map->objects); iterator != TCOD_list_end(map->objects); iterator++)
    {
        struct object *object = *iterator;

        object_destroy(object);
    }

    TCOD_list_delete(map->objects);

    for (void **iterator = TCOD_list_begin(map->actors); iterator != TCOD_list_end(map->actors); iterator++)
    {
        struct actor *actor = *iterator;

        actor_destroy(actor);
    }

    TCOD_list_delete(map->actors);

    for (void **iterator = TCOD_list_begin(map->items); iterator != TCOD_list_end(map->items); iterator++)
    {
        struct item *item = *iterator;

        item_destroy(item);
    }

    TCOD_list_delete(map->items);

    for (void **iterator = TCOD_list_begin(map->projectiles); iterator != TCOD_list_end(map->projectiles); iterator++)
    {
        struct projectile *projectile = *iterator;

        projectile_destroy(projectile);
    }

    TCOD_list_delete(map->projectiles);
}

static void hline(struct map *map, int x1, int y, int x2)
{
    int x = x1;
    int dx = (x1 > x2 ? -1 : 1);

    map->tiles[x][y].type = TILE_TYPE_FLOOR;

    if (x1 != x2)
    {
        do
        {
            x += dx;

            map->tiles[x][y].type = TILE_TYPE_FLOOR;
        }
        while (x != x2);
    }
}

static void hline_left(struct map *map, int x, int y)
{
    while (x >= 0 && map->tiles[x][y].type != TILE_TYPE_FLOOR)
    {
        map->tiles[x][y].type = TILE_TYPE_FLOOR;

        x--;
    }
}

static void hline_right(struct map *map, int x, int y)
{
    while (x < MAP_WIDTH && map->tiles[x][y].type != TILE_TYPE_FLOOR)
    {
        map->tiles[x][y].type = TILE_TYPE_FLOOR;

        x++;
    }
}

static void vline(struct map *map, int x, int y1, int y2)
{
    int y = y1;
    int dy = (y1 > y2 ? -1 : 1);

    map->tiles[x][y].type = TILE_TYPE_FLOOR;

    if (y1 != y2)
    {
        do
        {
            y += dy;

            map->tiles[x][y].type = TILE_TYPE_FLOOR;
        }
        while (y != y2);
    }
}

static void vline_up(struct map *map, int x, int y)
{
    while (y >= 0 && map->tiles[x][y].type != TILE_TYPE_FLOOR)
    {
        map->tiles[x][y].type = TILE_TYPE_FLOOR;

        y--;
    }
}

static void vline_down(struct map *map, int x, int y)
{
    while (y < MAP_HEIGHT && map->tiles[x][y].type != TILE_TYPE_FLOOR)
    {
        map->tiles[x][y].type = TILE_TYPE_FLOOR;

        y++;
    }
}

static bool traverse_node(TCOD_bsp_t *node, struct map *map)
{
    if (TCOD_bsp_is_leaf(node))
    {
        int minx = node->x + 1;
        int maxx = node->x + node->w - 1;
        int miny = node->y + 1;
        int maxy = node->y + node->h - 1;

#if !BSP_ROOM_WALLS
        if (minx > 1)
        {
            minx--;
        }

        if (miny > 1)
        {
            miny--;
        }
#endif

        if (maxx == MAP_WIDTH - 1)
        {
            maxx--;
        }

        if (maxy == MAP_HEIGHT - 1)
        {
            maxy--;
        }

#if BSP_RANDOM_ROOMS
        minx = TCOD_random_get_int(NULL, minx, maxx - BSP_MIN_ROOM_SIZE + 1);
        miny = TCOD_random_get_int(NULL, miny, maxy - BSP_MIN_ROOM_SIZE + 1);
        maxx = TCOD_random_get_int(NULL, minx + BSP_MIN_ROOM_SIZE - 1, maxx);
        maxy = TCOD_random_get_int(NULL, miny + BSP_MIN_ROOM_SIZE - 1, maxy);
#endif

        node->x = minx;
        node->y = miny;
        node->w = maxx - minx + 1;
        node->h = maxy - miny + 1;

        for (int x = minx; x <= maxx; x++)
        {
            for (int y = miny; y <= maxy; y++)
            {
                map->tiles[x][y].type = TILE_TYPE_FLOOR;
            }
        }

        struct room *room = room_create(node->x, node->y, node->w, node->h);

        TCOD_list_push(map->rooms, room);
    }
    else
    {
        TCOD_bsp_t *left = TCOD_bsp_left(node);
        TCOD_bsp_t *right = TCOD_bsp_right(node);

        node->x = MIN(left->x, right->x);
        node->y = MIN(left->y, right->y);
        node->w = MAX(left->x + left->w, right->x + right->w) - node->x;
        node->h = MAX(left->y + left->h, right->y + right->h) - node->y;

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
                int minx = MAX(left->x, right->x);
                int maxx = MIN(left->x + left->w - 1, right->x + right->w - 1);
                int x = TCOD_random_get_int(NULL, minx, maxx);

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
                int miny = MAX(left->y, right->y);
                int maxy = MIN(left->y + left->h - 1, right->y + right->h - 1);
                int y = TCOD_random_get_int(NULL, miny, maxy);

                hline_left(map, right->x - 1, y);
                hline_right(map, right->x, y);
            }
        }
    }

    return true;
}