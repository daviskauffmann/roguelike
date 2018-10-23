#include <roguelike/roguelike.h>

struct item *item_create(enum item_type type, int level, int x, int y)
{
    struct item *item = calloc(1, sizeof(struct item));

    item->type = type;
    item->level = level;
    item->x = x;
    item->y = y;

    return item;
}

bool item_is_two_handed(struct item *item, struct actor *actor)
{
    if (base_item_info[item_info[item->type].base_item].equip_slot != EQUIP_SLOT_MAIN_HAND)
    {
        return false;
    }

    switch (race_info[actor->race].size)
    {
    case RACE_SIZE_MEDIUM:
    {
        if (base_item_info[item_info[item->type].base_item].weapon_size == WEAPON_SIZE_TINY ||
            base_item_info[item_info[item->type].base_item].weapon_size == WEAPON_SIZE_SMALL ||
            base_item_info[item_info[item->type].base_item].weapon_size == WEAPON_SIZE_MEDIUM)
        {
            return false;
        }
        else if (base_item_info[item_info[item->type].base_item].weapon_size == WEAPON_SIZE_LARGE)
        {
            return true;
        }
    }
    break;
    case RACE_SIZE_SMALL:
    {
        if (base_item_info[item_info[item->type].base_item].weapon_size == WEAPON_SIZE_TINY ||
            base_item_info[item_info[item->type].base_item].weapon_size == WEAPON_SIZE_SMALL)
        {
            return false;
        }
        else if (base_item_info[item_info[item->type].base_item].weapon_size == WEAPON_SIZE_LARGE ||
                 base_item_info[item_info[item->type].base_item].weapon_size == WEAPON_SIZE_MEDIUM)
        {
            return true;
        }
    }
    break;
    }

    return false;
}

void item_destroy(struct item *item)
{
    free(item);
}
