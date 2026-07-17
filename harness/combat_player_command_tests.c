#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_combat.h"

static u3_combat_state state;
static uint8_t experience[U3_COMBAT_EXPERIENCE_COUNT];

#define ASSERT_EQ_INT(expected, actual) assert_eq_int((expected), (actual), __FILE__, __LINE__)

static void assert_eq_int(int expected, int actual, const char *file, int line)
{
    if (expected != actual) {
        fprintf(stderr, "%s:%d: expected %d, got %d\n", file, line, expected, actual);
        exit(1);
    }
}

static void reset_state(void)
{
    memset(&state, 0, sizeof(state));
    memset(experience, 0, sizeof(experience));
    state.monster_type = 0x10;
    state.character_x[0] = 4;
    state.character_y[0] = 4;
    state.character_shape[0] = 0xF0;
    state.character_tile[0] = 2;
    for (int index = 0; index < U3_COMBAT_TILE_COUNT; index++)
        state.tile_array[index] = 2;
}

static u3_combat_player_command_input base_input(uint16_t command)
{
    u3_combat_player_command_input input;

    memset(&input, 0, sizeof(input));
    input.command = command;
    input.character = 0;
    input.weapon = 2;
    input.weapon_quantity = 1;
    input.strength = 20;
    input.dexterity = 50;
    input.projectile_monster = U3_COMBAT_NO_SLOT;
    input.exodus_castle_result = 0xFF;
    input.hit_chance_roll = 128;
    input.hit_dexterity_roll = 99;
    input.damage_roll = 5;
    return input;
}

static void place_monster(uint8_t x, uint8_t y, uint8_t hp)
{
    state.monster_x[2] = x;
    state.monster_y[2] = y;
    state.monster_tile[2] = 0x04;
    state.monster_hp[2] = hp;
}

static void test_pass_is_handled_without_redraw(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    input = base_input(U3_COMBAT_COMMAND_PASS);
    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.passed);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_PASSED, result.status);
    ASSERT_EQ_INT(0, result.redraw);
}

static void test_cardinal_move_updates_character_coordinate(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    input = base_input(U3_COMBAT_COMMAND_EAST);
    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.moved);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_MOVED, result.status);
    ASSERT_EQ_INT(5, state.character_x[0]);
    ASSERT_EQ_INT(4, state.character_y[0]);
    ASSERT_EQ_INT(1, result.redraw);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_STEP, result.sound_id);
}

static void test_blocked_move_preserves_character_coordinate(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    state.tile_array[(4 * 11) + 5] = 0;
    input = base_input(U3_COMBAT_COMMAND_EAST);
    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.blocked);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_BLOCKED, result.status);
    ASSERT_EQ_INT(4, state.character_x[0]);
    ASSERT_EQ_INT(4, state.character_y[0]);
    ASSERT_EQ_INT(0, result.redraw);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_BUMP, result.sound_id);
    ASSERT_EQ_INT(0, result.target_tile);
}

static void test_attack_without_direction_requests_direction(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    input = base_input(U3_COMBAT_COMMAND_ATTACK);
    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.attack_direction_required);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_ATTACK_DIRECTION_REQUIRED, result.status);
}

static void test_direction_supplied_melee_attack_hits_adjacent_monster(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    place_monster(5, 4, 30);
    input = base_input(U3_COMBAT_COMMAND_ATTACK);
    input.attack_direction_x = 1;

    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.attacked);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_ATTACK_HIT, result.status);
    ASSERT_EQ_INT(1, result.attack_result.hit);
    ASSERT_EQ_INT(5, state.monster_hp[2]);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_HIT, result.sound_id);
}

static void test_direction_supplied_melee_attack_misses_empty_tile(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    input = base_input(U3_COMBAT_COMMAND_ATTACK);
    input.attack_direction_x = 1;

    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.attacked);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_ATTACK_MISSED, result.status);
    ASSERT_EQ_INT(1, result.attack_result.miss);
}

static void test_projectile_weapon_reports_deferred_status(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    input = base_input(U3_COMBAT_COMMAND_ATTACK);
    input.attack_direction_x = 1;
    input.weapon = 3;

    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.unsupported);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_ATTACK_DEFERRED, result.status);
    ASSERT_EQ_INT(0, result.attacked);
}

static void test_dead_character_command_reports_unsupported_without_mutation(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    state.character_status[0] = U3_COMBAT_STATUS_DEAD;
    input = base_input(U3_COMBAT_COMMAND_EAST);

    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.unsupported);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_UNSUPPORTED, result.status);
    ASSERT_EQ_INT(4, state.character_x[0]);
    ASSERT_EQ_INT(4, state.character_y[0]);
}

int main(void)
{
    test_pass_is_handled_without_redraw();
    test_cardinal_move_updates_character_coordinate();
    test_blocked_move_preserves_character_coordinate();
    test_attack_without_direction_requests_direction();
    test_direction_supplied_melee_attack_hits_adjacent_monster();
    test_direction_supplied_melee_attack_misses_empty_tile();
    test_projectile_weapon_reports_deferred_status();
    test_dead_character_command_reports_unsupported_without_mutation();

    puts("combat player command characterization passed");
    return 0;
}
