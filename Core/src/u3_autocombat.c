#include "u3_autocombat.h"

#include <stdbool.h>

#include "u3_map_math.h"

static uint8_t u3_autocombat_get_tile(const u3_autocombat_state *state, int16_t x, int16_t y)
{
    /* Legacy reference: Sources/UltimaMisc.c GetXYTile. */
    if (x < 0 || x > 10 || y < 0 || y > 10)
        return 0;
    return state->tile_array[(y * 11) + x];
}

static bool u3_autocombat_combat_char_here(const u3_autocombat_state *state, int16_t x, int16_t y)
{
    /* Legacy reference: Sources/UltimaAutocombat.c CombatCharHere. */
    int16_t character;
    uint8_t value = u3_autocombat_get_tile(state, x, y);
    bool is_one_here = false;

    if (value != 2 && value != 4 && value != 6 && value != 0x10)
        is_one_here = true;
    for (character = 0; character < U3_COMBAT_CHARACTER_COUNT; character++) {
        if (state->combat.character_x[character] == x && state->combat.character_y[character] == y)
            is_one_here = true;
    }
    return is_one_here;
}

bool u3_autocombat_monster_can_attack(const u3_autocombat_state *state, int16_t x, int16_t y)
{
    /* Legacy reference: Sources/UltimaAutocombat.c MonsterCanAttack. */
    int16_t monster;
    bool result = false;

    result |= (u3_combat_monster_here(&state->combat, x - 1, y - 1) != U3_COMBAT_NO_SLOT);
    result |= (u3_combat_monster_here(&state->combat, x, y - 1) != U3_COMBAT_NO_SLOT);
    result |= (u3_combat_monster_here(&state->combat, x + 1, y - 1) != U3_COMBAT_NO_SLOT);
    result |= (u3_combat_monster_here(&state->combat, x - 1, y) != U3_COMBAT_NO_SLOT);
    result |= (u3_combat_monster_here(&state->combat, x + 1, y) != U3_COMBAT_NO_SLOT);
    result |= (u3_combat_monster_here(&state->combat, x - 1, y + 1) != U3_COMBAT_NO_SLOT);
    result |= (u3_combat_monster_here(&state->combat, x, y + 1) != U3_COMBAT_NO_SLOT);
    result |= (u3_combat_monster_here(&state->combat, x + 1, y + 1) != U3_COMBAT_NO_SLOT);

    if (state->combat.monster_type == 0x3A) {
        for (monster = 0; monster < U3_COMBAT_MONSTER_COUNT; monster++) {
            if (state->combat.monster_hp[monster]) {
                if (u3_map_math_absolute((int16_t)(state->combat.monster_x[monster] - x)) ==
                    u3_map_math_absolute((int16_t)(state->combat.monster_y[monster] - y)))
                    result = true;
            }
        }
    }
    return result;
}

bool u3_autocombat_nearly_dead(const u3_autocombat_state *state, int16_t who)
{
    /* Legacy reference: Sources/UltimaAutocombat.c NearlyDead. */
    int16_t character;
    bool nearly_dead = false;

    if (who > 0) {
        character = who - 1;
        if (state->character_hp[character] < 50)
            nearly_dead = true;
    } else {
        for (character = 0; character < U3_COMBAT_CHARACTER_COUNT; character++) {
            if (!state->character_alive[character]) {
                nearly_dead = true;
            } else if (state->character_hp[character] < 50) {
                nearly_dead = true;
            }
        }
    }
    return nearly_dead;
}

void u3_autocombat_setup_now(u3_autocombat_state *state)
{
    /* Legacy reference: Sources/UltimaAutocombat.c SetupNow. */
    int16_t monster;

    for (monster = 0; monster < U3_COMBAT_MONSTER_COUNT; monster++) {
        state->future_monster_x[monster] = state->combat.monster_x[monster];
        state->future_monster_y[monster] = state->combat.monster_y[monster];
    }
}

bool u3_autocombat_future_monster_here(const u3_autocombat_state *state, int16_t x, int16_t y)
{
    /* Legacy reference: Sources/UltimaAutocombat.c FutureMonsterHere. */
    int16_t slot;
    bool result = false;

    for (slot = U3_COMBAT_MONSTER_COUNT - 1; slot >= 0; slot--) {
        if (state->combat.monster_hp[slot] != 0) {
            if (state->future_monster_x[slot] == x && state->future_monster_y[slot] == y)
                result = true;
        }
    }
    for (slot = 0; slot < U3_COMBAT_CHARACTER_COUNT; slot++) {
        if (state->character_alive[slot]) {
            if (state->combat.character_x[slot] == x && state->combat.character_y[slot] == y)
                result = true;
        }
    }
    return result;
}

void u3_autocombat_setup_future(u3_autocombat_state *state)
{
    /* Legacy reference: Sources/UltimaAutocombat.c SetupFuture. */
    int16_t monster;
    int16_t character;
    int16_t distance;
    int16_t closest_character;
    int16_t closest_value;
    int16_t new_x;
    int16_t new_y;
    int16_t delta_x;
    int16_t delta_y;

    u3_autocombat_setup_now(state);
    for (monster = 0; monster < U3_COMBAT_MONSTER_COUNT; monster++) {
        if (state->combat.monster_hp[monster] != 0) {
            closest_character = U3_COMBAT_MONSTER_COUNT;
            closest_value = 128;
            for (character = 0; character < U3_COMBAT_CHARACTER_COUNT; character++) {
                distance = u3_map_math_absolute((int16_t)(state->future_monster_x[monster] - state->combat.character_x[character])) +
                           u3_map_math_absolute((int16_t)(state->future_monster_y[monster] - state->combat.character_y[character]));
                if (distance < closest_value) {
                    closest_value = distance;
                    closest_character = character;
                }
            }
            delta_x = u3_map_math_get_heading((int16_t)(state->combat.character_x[closest_character] - state->future_monster_x[monster]));
            delta_y = u3_map_math_get_heading((int16_t)(state->combat.character_y[closest_character] - state->future_monster_y[monster]));
            new_x = state->future_monster_x[monster] + delta_x;
            new_y = state->future_monster_y[monster] + delta_y;
            if (u3_autocombat_future_monster_here(state, new_x, new_y)) {
                new_x = state->future_monster_x[monster];
                new_y = state->future_monster_y[monster] + delta_y;
                if (u3_autocombat_future_monster_here(state, new_x, new_y)) {
                    new_x = state->future_monster_x[monster] + delta_x;
                    new_y = state->future_monster_y[monster];
                    if (u3_autocombat_future_monster_here(state, new_x, new_y)) {
                        new_x = state->future_monster_x[monster];
                        new_y = state->future_monster_y[monster];
                    }
                }
            }
            state->future_monster_x[monster] = (uint8_t)new_x;
            state->future_monster_y[monster] = (uint8_t)new_y;
        }
    }
}

char u3_autocombat_dir_to_nearest_monster(const u3_autocombat_state *state, int16_t character)
{
    /* Legacy reference: Sources/UltimaAutocombat.c DirToNearestMonster. */
    int16_t monster;
    int16_t distance;
    int16_t closest_monster = U3_COMBAT_MONSTER_COUNT;
    int16_t closest_value = 128;
    int16_t delta_x;
    int16_t delta_y;

    for (monster = 0; monster < U3_COMBAT_MONSTER_COUNT; monster++) {
        if (state->combat.monster_hp[monster] != 0) {
            distance = u3_map_math_absolute((int16_t)(state->future_monster_x[monster] - state->combat.character_x[character])) +
                       u3_map_math_absolute((int16_t)(state->future_monster_y[monster] - state->combat.character_y[character]));
            if (distance < closest_value) {
                closest_value = distance;
                closest_monster = monster;
            }
        }
    }
    delta_x = u3_map_math_get_heading((int16_t)(state->future_monster_x[closest_monster] - state->combat.character_x[character]));
    delta_y = u3_map_math_get_heading((int16_t)(state->future_monster_y[closest_monster] - state->combat.character_y[character]));
    return u3_autocombat_auto_move_char(state, character, delta_x, delta_y);
}

char u3_autocombat_line_up_to_monster(const u3_autocombat_state *state, int16_t character)
{
    /* Legacy reference: Sources/UltimaAutocombat.c LineUpToMonster. */
    char direction;

    if (state->combat.character_x[character] < 6) {
        direction = u3_autocombat_monster_lined_up(state, character, state->combat.character_x[character] + 1, state->combat.character_y[character]);
        if (direction)
            return u3_autocombat_auto_move_char(state, character, 1, 0);
        direction = u3_autocombat_monster_lined_up(state, character, state->combat.character_x[character] - 1, state->combat.character_y[character]);
        if (direction)
            return u3_autocombat_auto_move_char(state, character, -1, 0);
        direction = u3_autocombat_monster_lined_up(state, character, state->combat.character_x[character], state->combat.character_y[character] - 1);
        if (direction)
            return u3_autocombat_auto_move_char(state, character, 0, -1);
        direction = u3_autocombat_monster_lined_up(state, character, state->combat.character_x[character], state->combat.character_y[character] + 1);
        if (direction)
            return u3_autocombat_auto_move_char(state, character, 0, 1);
        if (!state->allow_diagonal)
            return u3_autocombat_dir_to_nearest_monster(state, character);
        direction = u3_autocombat_monster_lined_up(state, character, state->combat.character_x[character] + 1, state->combat.character_y[character] - 1);
        if (direction)
            return direction;
        direction = u3_autocombat_monster_lined_up(state, character, state->combat.character_x[character] + 1, state->combat.character_y[character] + 1);
        if (direction)
            return direction;
        direction = u3_autocombat_monster_lined_up(state, character, state->combat.character_x[character] - 1, state->combat.character_y[character] - 1);
        if (direction)
            return direction;
        direction = u3_autocombat_monster_lined_up(state, character, state->combat.character_x[character] - 1, state->combat.character_y[character] + 1);
        if (direction)
            return direction;
        return u3_autocombat_dir_to_nearest_monster(state, character);
    } else {
        direction = u3_autocombat_monster_lined_up(state, character, state->combat.character_x[character] - 1, state->combat.character_y[character]);
        if (direction)
            return u3_autocombat_auto_move_char(state, character, -1, 0);
        direction = u3_autocombat_monster_lined_up(state, character, state->combat.character_x[character] + 1, state->combat.character_y[character]);
        if (direction)
            return u3_autocombat_auto_move_char(state, character, 1, 0);
        direction = u3_autocombat_monster_lined_up(state, character, state->combat.character_x[character], state->combat.character_y[character] - 1);
        if (direction)
            return u3_autocombat_auto_move_char(state, character, 0, -1);
        direction = u3_autocombat_monster_lined_up(state, character, state->combat.character_x[character], state->combat.character_y[character] + 1);
        if (direction)
            return u3_autocombat_auto_move_char(state, character, 0, 1);
        if (!state->allow_diagonal)
            return u3_autocombat_dir_to_nearest_monster(state, character);
        direction = u3_autocombat_monster_lined_up(state, character, state->combat.character_x[character] - 1, state->combat.character_y[character] - 1);
        if (direction)
            return direction;
        direction = u3_autocombat_monster_lined_up(state, character, state->combat.character_x[character] - 1, state->combat.character_y[character] + 1);
        if (direction)
            return direction;
        direction = u3_autocombat_monster_lined_up(state, character, state->combat.character_x[character] + 1, state->combat.character_y[character] - 1);
        if (direction)
            return direction;
        direction = u3_autocombat_monster_lined_up(state, character, state->combat.character_x[character] + 1, state->combat.character_y[character] + 1);
        if (direction)
            return direction;
        return u3_autocombat_dir_to_nearest_monster(state, character);
    }
}

int16_t u3_autocombat_threat_value(const u3_autocombat_state *state)
{
    /* Legacy reference: Sources/UltimaAutocombat.c ThreatValue. */
    int16_t monster;
    int16_t exp_value = state->experience[(state->combat.monster_type / 2) & 0x0F];
    int16_t total = 0;

    for (monster = 0; monster < U3_COMBAT_MONSTER_COUNT; monster++) {
        if (state->combat.monster_hp[monster] != 0)
            total += exp_value;
    }
    if (state->combat.monster_type == 0x1C || state->combat.monster_type == 0x3C || state->combat.monster_type == 0x38)
        total *= 2;
    return total;
}

char u3_autocombat_monster_nearby(const u3_autocombat_state *state, int16_t character)
{
    /* Legacy reference: Sources/UltimaAutocombat.c MonsterNearby. */
    int16_t x = state->combat.character_x[character];
    int16_t y = state->combat.character_y[character];

    if (u3_combat_monster_here(&state->combat, x, y - 1) != U3_COMBAT_NO_SLOT)
        return '8';
    if (u3_combat_monster_here(&state->combat, x - 1, y) != U3_COMBAT_NO_SLOT)
        return '4';
    if (u3_combat_monster_here(&state->combat, x + 1, y) != U3_COMBAT_NO_SLOT)
        return '6';
    if (u3_combat_monster_here(&state->combat, x, y + 1) != U3_COMBAT_NO_SLOT)
        return '2';
    if (!state->allow_diagonal)
        return 0;
    if (u3_combat_monster_here(&state->combat, x - 1, y - 1) != U3_COMBAT_NO_SLOT)
        return '7';
    if (u3_combat_monster_here(&state->combat, x - 1, y + 1) != U3_COMBAT_NO_SLOT)
        return '1';
    if (u3_combat_monster_here(&state->combat, x + 1, y - 1) != U3_COMBAT_NO_SLOT)
        return '9';
    if (u3_combat_monster_here(&state->combat, x + 1, y + 1) != U3_COMBAT_NO_SLOT)
        return '3';
    return 0;
}

char u3_autocombat_monster_lined_up(const u3_autocombat_state *state, int16_t character, int16_t x, int16_t y)
{
    /* Legacy reference: Sources/UltimaAutocombat.c MonsterLinedUp. */
    int16_t monster;
    int16_t distance;
    int16_t closest_monster = U3_COMBAT_MONSTER_COUNT;
    int16_t closest_value = 128;
    int16_t delta_x;
    int16_t delta_y;
    char key_to_press = 0;
    bool this_one;

    for (monster = 0; monster < U3_COMBAT_MONSTER_COUNT; monster++) {
        this_one = false;
        if (state->combat.monster_hp[monster] != 0) {
            if (state->future_monster_x[monster] == x || state->future_monster_y[monster] == y)
                this_one = true;
            if (state->allow_diagonal) {
                this_one |= (u3_map_math_absolute((int16_t)(x - state->future_monster_x[monster])) ==
                             u3_map_math_absolute((int16_t)(y - state->future_monster_y[monster])));
            }
        }
        if (this_one) {
            distance = u3_map_math_absolute((int16_t)(state->future_monster_x[monster] - x)) +
                       u3_map_math_absolute((int16_t)(state->future_monster_y[monster] - y));
            if (distance < closest_value) {
                closest_value = distance;
                closest_monster = monster;
            }
        }
    }
    if (closest_monster == U3_COMBAT_MONSTER_COUNT)
        return 0;
    delta_x = u3_map_math_get_heading((int16_t)(state->future_monster_x[closest_monster] - x));
    delta_y = u3_map_math_get_heading((int16_t)(state->future_monster_y[closest_monster] - y));

    if (delta_x == -1 && delta_y == 1)
        key_to_press = '1';
    if (delta_x == 0 && delta_y == 1)
        key_to_press = '2';
    if (delta_x == 1 && delta_y == 1)
        key_to_press = '3';
    if (delta_x == -1 && delta_y == 0)
        key_to_press = '4';
    if (delta_x == 1 && delta_y == 0)
        key_to_press = '6';
    if (delta_x == -1 && delta_y == -1)
        key_to_press = '7';
    if (delta_x == 0 && delta_y == -1)
        key_to_press = '8';
    if (delta_x == 1 && delta_y == -1)
        key_to_press = '9';
    if (character < U3_COMBAT_CHARACTER_COUNT)
        return u3_autocombat_auto_move_char(state, character, delta_x, delta_y);
    return key_to_press;
}

char u3_autocombat_auto_move_char(const u3_autocombat_state *state, int16_t character, int16_t delta_x, int16_t delta_y)
{
    /* Legacy reference: Sources/UltimaAutocombat.c AutoMoveChar. */
    char key_to_press;
    int16_t save_delta_x;

    if (!state->allow_diagonal && delta_x != 0 && delta_y != 0)
        delta_x = 0;
    if (!u3_autocombat_combat_char_here(state,
                                        state->combat.character_x[character] + delta_x,
                                        state->combat.character_y[character] + delta_y))
        goto do_key_now;
    if (delta_x == 0) {
        if (state->allow_diagonal) {
            delta_x = 1;
            if (!u3_autocombat_combat_char_here(state,
                                                state->combat.character_x[character] + delta_x,
                                                state->combat.character_y[character] + delta_y))
                goto do_key_now;
            delta_x = -1;
            if (!u3_autocombat_combat_char_here(state,
                                                state->combat.character_x[character] + delta_x,
                                                state->combat.character_y[character] + delta_y))
                goto do_key_now;
        }
        delta_y = 0;
        delta_x = 1;
        if (!u3_autocombat_combat_char_here(state,
                                            state->combat.character_x[character] + delta_x,
                                            state->combat.character_y[character] + delta_y))
            goto do_key_now;
        delta_x = -1;
        if (!u3_autocombat_combat_char_here(state,
                                            state->combat.character_x[character] + delta_x,
                                            state->combat.character_y[character] + delta_y))
            goto do_key_now;
        delta_x = 2;
        goto do_key_now;
    }
    if (delta_y == 0) {
        if (state->allow_diagonal) {
            delta_y = -1;
            if (!u3_autocombat_combat_char_here(state,
                                                state->combat.character_x[character] + delta_x,
                                                state->combat.character_y[character] + delta_y))
                goto do_key_now;
            delta_y = 1;
            if (!u3_autocombat_combat_char_here(state,
                                                state->combat.character_x[character] + delta_x,
                                                state->combat.character_y[character] + delta_y))
                goto do_key_now;
        }
        delta_x = 0;
        delta_y = -1;
        if (!u3_autocombat_combat_char_here(state,
                                            state->combat.character_x[character] + delta_x,
                                            state->combat.character_y[character] + delta_y))
            goto do_key_now;
        delta_y = 1;
        if (!u3_autocombat_combat_char_here(state,
                                            state->combat.character_x[character] + delta_x,
                                            state->combat.character_y[character] + delta_y))
            goto do_key_now;
        delta_y = 2;
        goto do_key_now;
    }
    save_delta_x = delta_x;
    delta_x = 0;
    if (!u3_autocombat_combat_char_here(state,
                                        state->combat.character_x[character] + delta_x,
                                        state->combat.character_y[character] + delta_y))
        goto do_key_now;
    delta_x = save_delta_x;
    delta_y = 0;
    if (!u3_autocombat_combat_char_here(state,
                                        state->combat.character_x[character] + delta_x,
                                        state->combat.character_y[character] + delta_y))
        goto do_key_now;
    delta_x = 2;
do_key_now:
    key_to_press = ' ';
    if (delta_x == -1 && delta_y == 1)
        key_to_press = '1';
    if (delta_x == 0 && delta_y == 1)
        key_to_press = '2';
    if (delta_x == 1 && delta_y == 1)
        key_to_press = '3';
    if (delta_x == -1 && delta_y == 0)
        key_to_press = '4';
    if (delta_x == 1 && delta_y == 0)
        key_to_press = '6';
    if (delta_x == -1 && delta_y == -1)
        key_to_press = '7';
    if (delta_x == 0 && delta_y == -1)
        key_to_press = '8';
    if (delta_x == 1 && delta_y == -1)
        key_to_press = '9';
    return key_to_press;
}
