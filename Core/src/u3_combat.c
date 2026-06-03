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

static void u3_combat_put_tile(u3_combat_state *state, int16_t value, int16_t x, int16_t y)
{
    /* Legacy reference: Sources/UltimaMisc.c PutXYTile. */
    if (x < 0 || x > 10 || y < 0 || y > 10)
        return;
    state->tile_array[(y * 11) + x] = (uint8_t)value;
}

u3_combat_damage_result u3_combat_damage_monster(u3_combat_state *state,
                                                  const uint8_t experience[U3_COMBAT_EXPERIENCE_COUNT],
                                                  int16_t monster,
                                                  int16_t damage,
                                                  int16_t character)
{
    /* Legacy reference: Sources/UltimaSpellCombat.c DamageMonster, $84C7. */
    u3_combat_damage_result result = {0};

    result.character = (uint8_t)character;

    if (state->monster_type == U3_COMBAT_LORD_BRITISH_TYPE) {
        result.ignored_lord_british = 1;
        return result;
    }

    if ((state->monster_hp[monster] - damage) < 1) {
        uint8_t expnum = (uint8_t)((state->monster_type / 2) & 0x0F);
        uint8_t x = state->monster_x[monster];
        uint8_t y = state->monster_y[monster];
        uint8_t restored_tile = state->monster_tile[monster];

        result.defeated = 1;
        result.award_experience = 1;
        result.redraw_tiles = 1;
        result.restore_tile = 1;
        result.message_id = U3_COMBAT_MONSTER_DEFEATED_MESSAGE;
        result.experience_awarded = experience[expnum];
        result.monster_x = x;
        result.monster_y = y;
        result.restored_tile = restored_tile;

        u3_combat_put_tile(state, restored_tile, x, y);
        state->monster_hp[monster] = 0;
    } else {
        result.damaged = 1;
        state->monster_hp[monster] = (uint8_t)(state->monster_hp[monster] - damage);
    }

    return result;
}
