#include "u3_combat.h"

uint8_t u3_combat_valid_move(const u3_combat_state *state, int16_t tile)
{
    /* Legacy reference: Sources/UltimaSpellCombat.c CombatValidMove, $7E31. */
    if (state->monster_type < 0x16 || state->monster_type >= 0x20) {
        if (tile == 2)
            return 0;
        if (tile == 4)
            return 0;
        if (tile == 6)
            return 0;
        if (tile == 0x10)
            return 0;
        return 0xFF;
    } else {
        if (tile == 0)
            return 0;
        return 0xFF;
    }
}

uint8_t u3_combat_monster_here(const u3_combat_state *state, int16_t x, int16_t y)
{
    /* Legacy reference: Sources/UltimaSpellCombat.c CombatMonsterHere, $7D24. */
    int16_t mon;
    uint8_t result = U3_COMBAT_NO_SLOT;

    for (mon = U3_COMBAT_MONSTER_COUNT - 1; mon >= 0; mon--) {
        if (state->monster_hp[mon] != 0) {
            if (state->monster_x[mon] == x && state->monster_y[mon] == y)
                result = (uint8_t)mon;
        }
    }
    return result;
}

uint8_t u3_combat_character_here(const u3_combat_state *state, int16_t x, int16_t y)
{
    /* Legacy reference: Sources/UltimaSpellCombat.c CombatCharacterHere. */
    int16_t character;

    for (character = U3_COMBAT_CHARACTER_COUNT - 1; character >= 0; character--) {
        if (state->character_x[character] == x && state->character_y[character] == y)
            return (uint8_t)character;
    }
    return U3_COMBAT_NO_SLOT;
}
