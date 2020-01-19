#ifndef GAME_SPELL_H
#define GAME_SPELL_H

// casting:
// open spellbook
// select spell and press a to activate it
// press z to cast
// if non-targeted spell, cast immediately
//     this could be self-buffing spells or aoe spells
// if targeted spell, enter targeting mode
// press z again to cast spell, calling spell_cast() with the coordinates of the cursor
// activating a spell should be a input module thing, not an actor thing
//     it doesn't make much sense for an npc to activate a spell, their AI would just cast it

enum spell_type
{
    SPELL_TYPE_HEAL,

    NUM_SPELL_TYPES
};

struct spell_data
{
    const char *name;
};

#endif
