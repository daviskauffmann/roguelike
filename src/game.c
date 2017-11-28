#include <libtcod.h>
#include <math.h>
#include <stdio.h>

#include "CMemLeak.h"
#include "config.h"
#include "game.h"
#include "utils.h"

void game_init(game_t *game)
{
    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        entity_t *entity = &game->state.entities[i];

        entity->id = ID_UNUSED;
        entity->game = game;

        for (int j = 0; j < NUM_COMPONENTS; j++)
        {
            component_t *component = &game->state.components[j][i];

            component->id = ID_UNUSED;
        }
    }

    for (int i = 0; i < NUM_MAPS; i++)
    {
        map_t *map = &game->state.maps[i];

        map_init(map, game, i);
    }

    game->tile_common = (tile_common_t){
        .shadow_color = TCOD_color_RGB(16, 16, 32)};

    game->tile_info[TILE_FLOOR] = (tile_info_t){
        .glyph = '.',
        .color = TCOD_white,
        .is_transparent = true,
        .is_walkable = true};
    game->tile_info[TILE_WALL] = (tile_info_t){
        .glyph = '#',
        .color = TCOD_white,
        .is_transparent = false,
        .is_walkable = false};
    game->tile_info[TILE_STAIR_DOWN] = (tile_info_t){
        .glyph = '>',
        .color = TCOD_white,
        .is_transparent = true,
        .is_walkable = true};
    game->tile_info[TILE_STAIR_UP] = (tile_info_t){
        .glyph = '<',
        .color = TCOD_white,
        .is_transparent = true,
        .is_walkable = true};

    game->player = NULL;

    game->turn_available = true;
    game->should_update = false;
    game->should_restart = false;
    game->should_quit = false;
    game->game_over = false;

    game->state.messages = TCOD_list_new();

    game->msg_visible = true;
    game->panel_visible = false;
}

void game_new(game_t *game)
{
    game->state.turn = 0;
    game->should_update = true;

    map_t *map = &game->state.maps[0];

    // TODO: why assign the player here?
    entity_t *player = entity_create(game);
    game->player = player;
    position_t *player_position = (position_t *)component_add(player, COMPONENT_POSITION);
    player_position->map = map;
    player_position->x = map->stair_up_x;
    player_position->y = map->stair_up_y;
    TCOD_list_push(map->tiles[player_position->x][player_position->y].entities, player);
    TCOD_list_push(map->entities, player);
    physics_t *player_physics = (physics_t *)component_add(player, COMPONENT_PHYSICS);
    player_physics->is_walkable = false;
    player_physics->is_transparent = true;
    light_t *player_light = (light_t *)component_add(player, COMPONENT_LIGHT);
    player_light->radius = 5;
    player_light->color = TCOD_white;
    player_light->flicker = false;
    player_light->priority = LIGHT_PRIORITY_0;
    if (player_light->fov_map != NULL)
    {
        TCOD_map_delete(player_light->fov_map);
    }
    player_light->fov_map = NULL;
    fov_t *player_fov = (fov_t *)component_add(player, COMPONENT_FOV);
    player_fov->radius = 1;
    if (player_fov->fov_map != NULL)
    {
        TCOD_map_delete(player_fov->fov_map);
    }
    player_fov->fov_map = NULL;
    appearance_t *player_appearance = (appearance_t *)component_add(player, COMPONENT_APPEARANCE);
    player_appearance->name = "Blinky";
    player_appearance->glyph = '@';
    player_appearance->color = TCOD_white;
    player_appearance->layer = LAYER_1;
    health_t *player_health = (health_t *)component_add(player, COMPONENT_HEALTH);
    player_health->max = 20;
    player_health->current = player_health->max;
    alignment_t *player_alignment = (alignment_t *)component_add(player, COMPONENT_ALIGNMENT);
    player_alignment->type = ALIGNMENT_GOOD;
    targeting_t *player_targeting = (targeting_t *)component_add(player, COMPONENT_TARGETING);
    player_targeting->type = TARGETING_NONE;
    player_targeting->x = -1;
    player_targeting->y = -1;
    inventory_t *player_inventory = (inventory_t *)component_add(player, COMPONENT_INVENTORY);
    player_inventory->items = TCOD_list_new();

    entity_t *pet = entity_create(game);
    position_t *pet_position = (position_t *)component_add(pet, COMPONENT_POSITION);
    pet_position->map = map;
    pet_position->x = map->stair_up_x + 1;
    pet_position->y = map->stair_up_y;
    TCOD_list_push(map->tiles[pet_position->x][pet_position->y].entities, pet);
    TCOD_list_push(map->entities, pet);
    physics_t *pet_physics = (physics_t *)component_add(pet, COMPONENT_PHYSICS);
    pet_physics->is_walkable = false;
    pet_physics->is_transparent = true;
    light_t *pet_light = (light_t *)component_add(pet, COMPONENT_LIGHT);
    pet_light->radius = 5;
    pet_light->color = TCOD_white;
    pet_light->flicker = false;
    pet_light->priority = LIGHT_PRIORITY_0;
    if (pet_light->fov_map != NULL)
    {
        TCOD_map_delete(pet_light->fov_map);
    }
    pet_light->fov_map = NULL;
    fov_t *pet_fov = (fov_t *)component_add(pet, COMPONENT_FOV);
    pet_fov->radius = 1;
    if (pet_fov->fov_map != NULL)
    {
        TCOD_map_delete(pet_fov->fov_map);
    }
    pet_fov->fov_map = NULL;
    appearance_t *pet_appearance = (appearance_t *)component_add(pet, COMPONENT_APPEARANCE);
    pet_appearance->name = "Spot";
    pet_appearance->glyph = 'd';
    pet_appearance->color = TCOD_white;
    pet_appearance->layer = LAYER_1;
    ai_t *pet_ai = (ai_t *)component_add(pet, COMPONENT_AI);
    pet_ai->energy = 1.0f;
    pet_ai->energy_per_turn = 0.5f;
    pet_ai->follow_target = player;
    health_t *pet_health = (health_t *)component_add(pet, COMPONENT_HEALTH);
    pet_health->max = 20;
    pet_health->current = pet_health->max;
    alignment_t *pet_alignment = (alignment_t *)component_add(pet, COMPONENT_ALIGNMENT);
    pet_alignment->type = ALIGNMENT_GOOD;

    msg_log(player->game, NULL, TCOD_white, "Hail, %s!", player_appearance->name);
}

void game_input(game_t *game)
{
    TCOD_key_t key;
    TCOD_mouse_t mouse;
    TCOD_event_t ev = TCOD_sys_check_for_event(TCOD_EVENT_ANY, &key, &mouse);

    switch (ev)
    {
    case TCOD_EVENT_KEY_PRESS:
    {
        switch (key.vk)
        {
        case TCODK_ESCAPE:
        {
            game->should_quit = true;

            break;
        }
        case TCODK_ENTER:
        {
            if (key.lalt)
            {
                fullscreen = !fullscreen;

                TCOD_console_set_fullscreen(fullscreen);
            }

            break;
        }
        case TCODK_CHAR:
        {
            switch (key.c)
            {
            case 'r':
            {
                game->should_restart = true;

                break;
            }
            }

            break;
        }
        }

        break;
    }
    }

    if (game->turn_available)
    {
        game->turn_available = false;

        switch (ev)
        {
        case TCOD_EVENT_KEY_PRESS:
        {
            switch (key.vk)
            {
            case TCODK_KP5:
            {
                game->should_update = true;

                break;
            }
            }

            break;
        }
        }

        if (!game->game_over)
        {
            entity_t *entity = game->player;

            position_t *position = (position_t *)component_get(entity, COMPONENT_POSITION);
            fov_t *fov = (fov_t *)component_get(entity, COMPONENT_FOV);
            alignment_t *alignment = (alignment_t *)component_get(entity, COMPONENT_ALIGNMENT);
            targeting_t *targeting = (targeting_t *)component_get(entity, COMPONENT_TARGETING);

            switch (ev)
            {
            case TCOD_EVENT_KEY_PRESS:
            {
                switch (key.vk)
                {
                case TCODK_KP1:
                {
                    if (targeting != NULL && targeting->type != TARGETING_NONE)
                    {
                        targeting->x--;
                        targeting->y++;
                    }
                    else if (position != NULL)
                    {
                        game->should_update = true;

                        int x = position->x - 1;
                        int y = position->y + 1;

                        if (key.lctrl)
                        {
                            entity_swing(entity, x, y);
                        }
                        else
                        {
                            entity_move(entity, x, y);
                        }
                    }

                    break;
                }
                case TCODK_KP2:
                {
                    if (targeting != NULL && targeting->type != TARGETING_NONE)
                    {
                        targeting->y++;
                    }
                    else if (position != NULL)
                    {
                        game->should_update = true;

                        int x = position->x;
                        int y = position->y + 1;

                        if (key.lctrl)
                        {
                            entity_swing(entity, x, y);
                        }
                        else
                        {
                            entity_move(entity, x, y);
                        }
                    }

                    break;
                }
                case TCODK_KP3:
                {
                    if (targeting != NULL && targeting->type != TARGETING_NONE)
                    {
                        targeting->x++;
                        targeting->y++;
                    }
                    else if (position != NULL)
                    {
                        game->should_update = true;

                        int x = position->x + 1;
                        int y = position->y + 1;

                        if (key.lctrl)
                        {
                            entity_swing(entity, x, y);
                        }
                        else
                        {
                            entity_move(entity, x, y);
                        }
                    }

                    break;
                }
                case TCODK_KP4:
                {
                    if (targeting != NULL && targeting->type != TARGETING_NONE)
                    {
                        targeting->x--;
                        targeting->y;
                    }
                    else if (position != NULL)
                    {
                        game->should_update = true;

                        int x = position->x - 1;
                        int y = position->y;

                        if (key.lctrl)
                        {
                            entity_swing(entity, x, y);
                        }
                        else
                        {
                            entity_move(entity, x, y);
                        }
                    }

                    break;
                }
                case TCODK_KP6:
                {
                    if (targeting != NULL && targeting->type != TARGETING_NONE)
                    {
                        targeting->x++;
                        targeting->y;
                    }
                    else if (position != NULL)
                    {
                        game->should_update = true;

                        int x = position->x + 1;
                        int y = position->y;

                        if (key.lctrl)
                        {
                            entity_swing(entity, x, y);
                        }
                        else
                        {
                            entity_move(entity, x, y);
                        }
                    }

                    break;
                }
                case TCODK_KP7:
                {
                    if (targeting != NULL && targeting->type != TARGETING_NONE)
                    {
                        targeting->x--;
                        targeting->y--;
                    }
                    else if (position != NULL)
                    {
                        game->should_update = true;

                        int x = position->x - 1;
                        int y = position->y - 1;

                        if (key.lctrl)
                        {
                            entity_swing(entity, x, y);
                        }
                        else
                        {
                            entity_move(entity, x, y);
                        }
                    }
                    break;
                }
                case TCODK_KP8:
                {
                    if (targeting != NULL && targeting->type != TARGETING_NONE)
                    {
                        targeting->x;
                        targeting->y--;
                    }
                    else if (position != NULL)
                    {
                        game->should_update = true;

                        int x = position->x;
                        int y = position->y - 1;

                        if (key.lctrl)
                        {
                            entity_swing(entity, x, y);
                        }
                        else
                        {
                            entity_move(entity, x, y);
                        }
                    }

                    break;
                }
                case TCODK_KP9:
                {
                    if (targeting != NULL && targeting->type != TARGETING_NONE)
                    {
                        targeting->x++;
                        targeting->y--;
                    }
                    else if (position != NULL)
                    {
                        game->should_update = true;

                        int x = position->x + 1;
                        int y = position->y - 1;

                        if (key.lctrl)
                        {
                            entity_swing(entity, x, y);
                        }
                        else
                        {
                            entity_move(entity, x, y);
                        }
                    }

                    break;
                }
                case TCODK_CHAR:
                {
                    switch (key.c)
                    {
                    case 'c':
                    {
                        game->panel_visible = !game->panel_visible;

                        break;
                    }
                    case 'f':
                    {
                        if (position != NULL && fov != NULL && targeting != NULL)
                        {
                            if (targeting->type == TARGETING_SHOOT)
                            {
                                game->should_update = true;

                                targeting->type = TARGETING_NONE;

                                entity_shoot(entity, targeting->x, targeting->y);
                            }
                            else
                            {
                                targeting->type = TARGETING_SHOOT;

                                bool target_found = false;

                                if (alignment != NULL)
                                {
                                    for (void **iterator = TCOD_list_begin(position->map->entities); iterator != TCOD_list_end(position->map->entities); iterator++)
                                    {
                                        entity_t *other = *iterator;

                                        position_t *other_position = (position_t *)component_get(other, COMPONENT_POSITION);
                                        alignment_t *other_alignment = (alignment_t *)component_get(other, COMPONENT_ALIGNMENT);

                                        if (other_position != NULL && other_alignment != NULL)
                                        {
                                            if (TCOD_map_is_in_fov(fov->fov_map, other_position->x, other_position->y) &&
                                                other_alignment->type != alignment->type)
                                            {
                                                target_found = true;

                                                targeting->x = other_position->x;
                                                targeting->y = other_position->y;
                                            }
                                        }
                                    }
                                }

                                if (!target_found)
                                {
                                    targeting->x = position->x;
                                    targeting->y = position->y;
                                }
                            }
                        }

                        break;
                    }
                    case 'g':
                    {
                        inventory_t *inventory = (inventory_t *)component_get(entity, COMPONENT_INVENTORY);

                        if (position != NULL && inventory != NULL)
                        {
                            tile_t *tile = &position->map->tiles[position->x][position->y];

                            bool item_found = false;

                            for (void **iterator = TCOD_list_begin(tile->entities); iterator != TCOD_list_end(tile->entities); iterator++)
                            {
                                entity_t *other = *iterator;

                                pickable_t *pickable = (pickable_t *)component_get(other, COMPONENT_PICKABLE);

                                if (pickable != NULL)
                                {
                                    game->should_update = true;

                                    item_found = true;

                                    entity_pick(entity, other);

                                    break;
                                }
                            }

                            if (!item_found)
                            {
                                msg_log(entity->game, position, TCOD_white, "There is nothing here!");
                            }
                        }

                        break;
                    }
                    case 'i':
                    {
                        game->panel_visible = !game->panel_visible;

                        break;
                    }
                    case 'l':
                    {
                        if (position != NULL && fov != NULL && targeting != NULL)
                        {
                            if (targeting->type == TARGETING_LOOK)
                            {
                                targeting->type = TARGETING_NONE;

                                msg_log(entity->game, position, TCOD_white, "Look!");
                            }
                            else
                            {
                                targeting->type = TARGETING_LOOK;
                                targeting->x = position->x;
                                targeting->y = position->y;
                            }
                        }

                        break;
                    }
                    case 'm':
                    {
                        game->msg_visible = !game->msg_visible;

                        break;
                    }
                    case 't':
                    {
                        light_t *light = (light_t *)component_get(entity, COMPONENT_LIGHT);

                        if (light != NULL)
                        {
                            game->should_update = true;

                            static bool torch = false;

                            torch = !torch;

                            if (torch)
                            {
                                light->radius = 10;
                                light->color = TCOD_light_amber;
                                light->flicker = true;
                                light->priority = LIGHT_PRIORITY_2;
                            }
                            else
                            {
                                light->radius = 5;
                                light->color = TCOD_white;
                                light->flicker = false;
                                light->priority = LIGHT_PRIORITY_0;
                            }
                        }

                        break;
                    }
                    case 'z':
                    {
                        if (position != NULL && fov != NULL && targeting != NULL)
                        {
                            if (targeting->type == TARGETING_ZAP)
                            {
                                game->should_update = true;

                                targeting->type = TARGETING_NONE;

                                msg_log(entity->game, position, TCOD_white, "Zap!");
                            }
                            else
                            {
                                targeting->type = TARGETING_ZAP;

                                bool target_found = false;

                                if (alignment != NULL)
                                {
                                    for (void **iterator = TCOD_list_begin(position->map->entities); iterator != TCOD_list_end(position->map->entities); iterator++)
                                    {
                                        entity_t *other = *iterator;

                                        position_t *other_position = (position_t *)component_get(other, COMPONENT_POSITION);
                                        alignment_t *other_alignment = (alignment_t *)component_get(other, COMPONENT_ALIGNMENT);

                                        if (other_position != NULL && other_alignment != NULL)
                                        {
                                            if (TCOD_map_is_in_fov(fov->fov_map, other_position->x, other_position->y) &&
                                                other_alignment->type != alignment->type)
                                            {
                                                target_found = true;

                                                targeting->x = other_position->x;
                                                targeting->y = other_position->y;
                                            }
                                        }
                                    }
                                }

                                if (!target_found)
                                {
                                    targeting->x = position->x;
                                    targeting->y = position->y;
                                }
                            }
                        }

                        break;
                    }
                    }

                    break;
                }
                }
            }
            }
        }
    }
}

void game_update(game_t *game)
{
    game->turn_available = true;

    position_t *player_position = (position_t *)component_get(game->player, COMPONENT_POSITION);

    if (player_position != NULL)
    {
        map_t *map = player_position->map;

        for (void **iterator = TCOD_list_begin(map->entities); iterator != TCOD_list_end(map->entities); iterator++)
        {
            entity_t *entity = *iterator;

            took_damage_t *took_damage = (took_damage_t *)component_get(entity, COMPONENT_TOOK_DAMAGE);

            if (took_damage != NULL)
            {
                took_damage->fade -= (1.0f / FPS) / 0.25f;

                if (took_damage->fade <= 0)
                {
                    component_remove(entity, COMPONENT_TOOK_DAMAGE);
                }

                fov_t *player_fov = (fov_t *)component_get(game->player, COMPONENT_FOV);

                position_t *position = (position_t *)component_get(entity, COMPONENT_POSITION);

                if (player_fov != NULL &&
                    position != NULL)
                {
                    if (TCOD_map_is_in_fov(player_fov->fov_map, position->x, position->y))
                    {
                        game->turn_available = false;
                    }
                }
            }
        }

        if (game->should_update)
        {
            game->should_update = false;
            game->state.turn++;

            TCOD_list_t lights = TCOD_list_new();

            for (void **iterator = TCOD_list_begin(map->entities); iterator != TCOD_list_end(map->entities); iterator++)
            {
                entity_t *entity = *iterator;

                position_t *position = (position_t *)component_get(entity, COMPONENT_POSITION);
                light_t *light = (light_t *)component_get(entity, COMPONENT_LIGHT);

                if (position != NULL && light != NULL)
                {
                    if (light->fov_map != NULL)
                    {
                        TCOD_map_delete(light->fov_map);
                    }

                    light->fov_map = map_to_fov_map(position->map, position->x, position->y, light->radius);

                    TCOD_list_push(lights, entity);
                }
            }

            for (void **iterator = TCOD_list_begin(map->entities); iterator != TCOD_list_end(map->entities); iterator++)
            {
                entity_t *entity = *iterator;

                position_t *position = (position_t *)component_get(entity, COMPONENT_POSITION);

                if (position != NULL)
                {
                    fov_t *fov = (fov_t *)component_get(entity, COMPONENT_FOV);

                    if (fov != NULL)
                    {
                        if (fov->fov_map != NULL)
                        {
                            TCOD_map_delete(fov->fov_map);
                        }

                        fov->fov_map = map_to_fov_map(position->map, position->x, position->y, fov->radius);

                        TCOD_map_t los_map = map_to_fov_map(position->map, position->x, position->y, 0);

                        for (int x = 0; x < MAP_WIDTH; x++)
                        {
                            for (int y = 0; y < MAP_HEIGHT; y++)
                            {
                                if (TCOD_map_is_in_fov(los_map, x, y))
                                {
                                    tile_t *tile = &position->map->tiles[x][y];

                                    for (void **iterator = TCOD_list_begin(lights); iterator != TCOD_list_end(lights); iterator++)
                                    {
                                        entity_t *other = *iterator;

                                        position_t *other_position = (position_t *)component_get(other, COMPONENT_POSITION);
                                        light_t *other_light = (light_t *)component_get(other, COMPONENT_LIGHT);

                                        if (other_position != NULL && other_light != NULL)
                                        {
                                            if (TCOD_map_is_in_fov(other_light->fov_map, x, y))
                                            {
                                                TCOD_map_set_in_fov(fov->fov_map, x, y, true);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        TCOD_map_delete(los_map);
                    }

                    ai_t *ai = (ai_t *)component_get(entity, COMPONENT_AI);

                    if (ai != NULL)
                    {
                        ai->energy += ai->energy_per_turn;

                        while (ai->energy >= 1.0f)
                        {
                            ai->energy -= 1.0f;

                            bool target_found = false;

                            fov_t *fov = (fov_t *)component_get(entity, COMPONENT_FOV);
                            alignment_t *alignment = (alignment_t *)component_get(entity, COMPONENT_ALIGNMENT);

                            if (fov != NULL && alignment != NULL)
                            {
                                for (void **iterator = TCOD_list_begin(position->map->entities); iterator != TCOD_list_end(position->map->entities); iterator++)
                                {
                                    entity_t *other = *iterator;

                                    position_t *other_position = (position_t *)component_get(other, COMPONENT_POSITION);
                                    alignment_t *other_alignment = (alignment_t *)component_get(other, COMPONENT_ALIGNMENT);

                                    if (other_position != NULL && other_alignment != NULL)
                                    {
                                        if (TCOD_map_is_in_fov(fov->fov_map, other_position->x, other_position->y) &&
                                            other_alignment->type != alignment->type)
                                        {
                                            target_found = true;

                                            if (distance(position->x, position->y, other_position->x, other_position->y) < 2.0f)
                                            {
                                                entity_attack(entity, other);
                                            }
                                            else
                                            {
                                                entity_path_towards(entity, other_position->x, other_position->y);
                                            }

                                            break;
                                        }
                                    }
                                }
                            }

                            if (ai->follow_target != NULL)
                            {
                                position_t *follow_position = (position_t *)component_get(ai->follow_target, COMPONENT_POSITION);

                                if (follow_position != NULL)
                                {
                                    if (!TCOD_map_is_in_fov(fov->fov_map, follow_position->x, follow_position->y) ||
                                        distance(position->x, position->y, follow_position->x, follow_position->y) > 5.0f)
                                    {
                                        target_found = true;

                                        entity_path_towards(entity, follow_position->x, follow_position->y);
                                    }
                                }
                            }

                            if (!target_found)
                            {
                                entity_move_random(entity);
                            }
                        }
                    }
                }
            }

            TCOD_list_delete(lights);
        }
    }
}

#define CONSTRAIN_VIEW 1

void game_render(game_t *game)
{
    TCOD_console_set_default_background(NULL, TCOD_black);
    TCOD_console_set_default_foreground(NULL, TCOD_white);
    TCOD_console_clear(NULL);

    int msg_x = 0;
    int msg_height = console_height / 4;
    int msg_y = console_height - msg_height;
    int msg_width = console_width;

    int panel_width = console_width / 2;
    int panel_x = console_width - panel_width;
    int panel_y = 0;
    int panel_height = console_height - (game->msg_visible ? msg_height : 0);

    if (game->player != NULL)
    {
        position_t *player_position = (position_t *)component_get(game->player, COMPONENT_POSITION);

        if (player_position != NULL)
        {
            int view_width = console_width - (game->panel_visible ? panel_width : 0);
            int view_height = console_height - (game->msg_visible ? msg_height : 0);
            int view_x = player_position->x - view_width / 2;
            int view_y = player_position->y - view_height / 2;

#if CONSTRAIN_VIEW
            view_x = view_x < 0
                         ? 0
                         : view_x + view_width > MAP_WIDTH
                               ? MAP_WIDTH - view_width
                               : view_x;
            view_y = view_y < 0
                         ? 0
                         : view_y + view_height > MAP_HEIGHT
                               ? MAP_HEIGHT - view_height
                               : view_y;
#endif

            static TCOD_noise_t noise = NULL;
            if (noise == NULL)
            {
                noise = TCOD_noise_new(1, TCOD_NOISE_DEFAULT_HURST, TCOD_NOISE_DEFAULT_LACUNARITY, NULL);
            }

            static float noise_x = 0.0f;

            noise_x += 0.2f;
            float noise_dx = noise_x + 20.0f;
            float dx = TCOD_noise_get(noise, &noise_dx) * 0.5f;
            noise_dx += 30.0f;
            float dy = TCOD_noise_get(noise, &noise_dx) * 0.5f;
            float di = 0.2f * TCOD_noise_get(noise, &noise_x);

            TCOD_list_t entities_by_layer[NUM_LAYERS];
            TCOD_list_t lights_by_priority[NUM_LIGHT_PRIORITIES];

            for (int i = 0; i < NUM_LAYERS; i++)
            {
                entities_by_layer[i] = TCOD_list_new();
            }

            for (int i = 0; i < NUM_LIGHT_PRIORITIES; i++)
            {
                lights_by_priority[i] = TCOD_list_new();
            }

            for (void **iterator = TCOD_list_begin(player_position->map->entities); iterator != TCOD_list_end(player_position->map->entities); iterator++)
            {
                entity_t *entity = *iterator;

                appearance_t *appearance = (appearance_t *)component_get(entity, COMPONENT_APPEARANCE);

                if (appearance != NULL)
                {
                    TCOD_list_push(entities_by_layer[appearance->layer], entity);
                }

                position_t *position = (position_t *)component_get(entity, COMPONENT_POSITION);
                light_t *light = (light_t *)component_get(entity, COMPONENT_LIGHT);

                if (position != NULL && light != NULL)
                {
                    TCOD_list_push(lights_by_priority[light->priority], entity);
                }
            }

            fov_t *player_fov = (fov_t *)component_get(game->player, COMPONENT_FOV);

            if (player_fov != NULL)
            {
                for (int x = view_x; x < view_x + view_width; x++)
                {
                    for (int y = view_y; y < view_y + view_height; y++)
                    {
                        if (map_is_inside(x, y))
                        {
                            tile_t *tile = &player_position->map->tiles[x][y];

                            if (TCOD_map_is_in_fov(player_fov->fov_map, x, y))
                            {
                                tile->seen = true;
                            }

                            for (int i = 0; i < NUM_LIGHT_PRIORITIES; i++)
                            {
                                for (void **iterator = TCOD_list_begin(lights_by_priority[i]); iterator != TCOD_list_end(lights_by_priority[i]); iterator++)
                                {
                                    entity_t *entity = *iterator;

                                    light_t *light = (light_t *)component_get(entity, COMPONENT_LIGHT);

                                    if (TCOD_map_is_in_fov(light->fov_map, x, y))
                                    {
                                        tile->seen = true;
                                    }
                                }
                            }

                            TCOD_color_t color = game->tile_common.shadow_color;

                            if (TCOD_map_is_in_fov(player_fov->fov_map, x, y) || tile->seen)
                            {
                                for (int i = 0; i < NUM_LIGHT_PRIORITIES; i++)
                                {
                                    for (void **iterator = TCOD_list_begin(lights_by_priority[i]); iterator != TCOD_list_end(lights_by_priority[i]); iterator++)
                                    {
                                        entity_t *entity = *iterator;

                                        position_t *position = (position_t *)component_get(entity, COMPONENT_POSITION);
                                        light_t *light = (light_t *)component_get(entity, COMPONENT_LIGHT);

                                        if (TCOD_map_is_in_fov(light->fov_map, x, y))
                                        {
                                            float r2 = pow(light->radius, 2);
                                            float d = pow(x - position->x + (light->flicker ? dx : 0), 2) + pow(y - position->y + (light->flicker ? dy : 0), 2);
                                            float l = CLAMP(0.0f, 1.0f, (r2 - d) / r2 + (light->flicker ? di : 0));

                                            color = TCOD_color_lerp(color, TCOD_color_lerp(game->tile_info[tile->type].color, light->color, l), l);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                if (!tile->seen)
                                {
                                    continue;
                                }
                            }

                            TCOD_console_set_char_foreground(NULL, x - view_x, y - view_y, color);
                            TCOD_console_set_char(NULL, x - view_x, y - view_y, game->tile_info[tile->type].glyph);
                        }
                    }
                }

                for (int i = 0; i < NUM_LAYERS; i++)
                {
                    for (void **iterator = TCOD_list_begin(entities_by_layer[i]); iterator != TCOD_list_end(entities_by_layer[i]); iterator++)
                    {
                        entity_t *entity = *iterator;

                        position_t *position = (position_t *)component_get(entity, COMPONENT_POSITION);
                        appearance_t *appearance = (appearance_t *)component_get(entity, COMPONENT_APPEARANCE);

                        if (position != NULL && appearance != NULL)
                        {
                            if (position->map == player_position->map && TCOD_map_is_in_fov(player_fov->fov_map, position->x, position->y))
                            {
                                TCOD_color_t color = appearance->color;

                                took_damage_t *took_damage = (took_damage_t *)component_get(entity, COMPONENT_TOOK_DAMAGE);

                                if (took_damage != NULL)
                                {
                                    color = TCOD_color_lerp(appearance->color, TCOD_red, took_damage->fade);
                                }

                                TCOD_console_set_char_foreground(NULL, position->x - view_x, position->y - view_y, color);
                                TCOD_console_set_char(NULL, position->x - view_x, position->y - view_y, appearance->glyph);
                            }
                        }
                    }
                }
            }

            for (int i = 0; i < NUM_LIGHT_PRIORITIES; i++)
            {
                TCOD_list_delete(lights_by_priority[i]);
            }

            for (int i = 0; i < NUM_LAYERS; i++)
            {
                TCOD_list_delete(entities_by_layer[i]);
            }

            targeting_t *player_targeting = (targeting_t *)component_get(game->player, COMPONENT_TARGETING);

            if (player_targeting != NULL && player_targeting->type != TARGETING_NONE)
            {
                TCOD_console_set_char_foreground(NULL, player_targeting->x - view_x, player_targeting->y - view_y, TCOD_red);
                TCOD_console_set_char(NULL, player_targeting->x - view_x, player_targeting->y - view_y, 'X');
            }
        }
    }

    if (game->msg_visible)
    {
        static TCOD_console_t msg = NULL;

        if (msg == NULL)
        {
            msg = TCOD_console_new(console_width, console_height);
        }

        TCOD_console_set_default_background(msg, TCOD_black);
        TCOD_console_set_default_foreground(msg, TCOD_white);
        TCOD_console_clear(msg);

        int y = 1;
        for (void **i = TCOD_list_begin(game->state.messages); i != TCOD_list_end(game->state.messages); i++)
        {
            message_t *message = *i;

            TCOD_console_set_default_foreground(msg, message->color);
            TCOD_console_print(msg, msg_x + 1, y, message->text);

            y++;
        }

        TCOD_console_set_default_foreground(msg, TCOD_white);
        TCOD_console_print_frame(msg, 0, 0, msg_width, msg_height, false, TCOD_BKGND_SET, "Log");

        TCOD_console_blit(msg, 0, 0, msg_width, msg_height, NULL, msg_x, msg_y, 1, 1);
    }

    if (game->panel_visible)
    {
        static TCOD_console_t panel = NULL;

        if (panel == NULL)
        {
            panel = TCOD_console_new(console_height, console_width);
        }

        TCOD_console_print_frame(panel, 0, 0, panel_width, panel_height, false, TCOD_BKGND_SET, "Panel");

        TCOD_console_blit(panel, 0, 0, panel_width, panel_height, NULL, panel_x, panel_y, 1, 1);
    }

    TCOD_console_print(NULL, 0, 0, "Turn: %d", game->state.turn);

    TCOD_console_flush();
}

void game_reset(game_t *game)
{
    for (int i = 0; i < NUM_MAPS; i++)
    {
        map_t *map = &game->state.maps[i];

        map_reset(map);
    }

    for (void **iterator = TCOD_list_begin(game->state.messages); iterator != TCOD_list_end(game->state.messages); iterator++)
    {
        message_t *message = *iterator;

        free(message->text);
        free(message);
    }

    TCOD_list_delete(game->state.messages);
}