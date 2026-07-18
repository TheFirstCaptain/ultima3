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
    input.spell = U3_COMBAT_SPELL_MITTAR;
    input.weapon = 2;
    input.weapon_quantity = 1;
    input.character_class = 'W';
    input.magic = 25;
    input.strength = 20;
    input.dexterity = 50;
    input.projectile_monster = U3_COMBAT_NO_SLOT;
    input.exodus_castle_result = 0xFF;
    input.hit_chance_roll = 128;
    input.hit_dexterity_roll = 99;
    input.damage_roll = 5;
    input.spell_damage_roll = 5;
    input.flee_roll = 255;
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

static void test_flee_success_reports_escape_without_mutation(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    input = base_input(U3_COMBAT_COMMAND_FLEE);
    input.flee_roll = U3_COMBAT_FLEE_SUCCESS_THRESHOLD;
    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_FLEE_SUCCESS, result.status);
    ASSERT_EQ_INT(1, result.flee_result.attempted);
    ASSERT_EQ_INT(1, result.flee_result.succeeded);
    ASSERT_EQ_INT(0, result.flee_result.failed);
    ASSERT_EQ_INT(U3_COMBAT_FLEE_SUCCESS_THRESHOLD, result.flee_result.roll);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_STEP, result.sound_id);
    ASSERT_EQ_INT(4, state.character_x[0]);
    ASSERT_EQ_INT(4, state.character_y[0]);
}

static void test_flee_failure_consumes_turn_without_mutation(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    input = base_input(U3_COMBAT_COMMAND_FLEE);
    input.flee_roll = U3_COMBAT_FLEE_SUCCESS_THRESHOLD - 1;
    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_FLEE_FAILED, result.status);
    ASSERT_EQ_INT(1, result.flee_result.attempted);
    ASSERT_EQ_INT(0, result.flee_result.succeeded);
    ASSERT_EQ_INT(1, result.flee_result.failed);
    ASSERT_EQ_INT(U3_COMBAT_FLEE_SUCCESS_THRESHOLD - 1, result.flee_result.roll);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_BUMP, result.sound_id);
    ASSERT_EQ_INT(4, state.character_x[0]);
    ASSERT_EQ_INT(4, state.character_y[0]);
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

static void test_projectile_weapon_hits_first_monster_in_line(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    place_monster(8, 4, 40);
    input = base_input(U3_COMBAT_COMMAND_ATTACK);
    input.attack_direction_x = 1;
    input.weapon = 3;

    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.attacked);
    ASSERT_EQ_INT(1, result.attack_result.used_projectile);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_ATTACK_HIT, result.status);
    ASSERT_EQ_INT(2, result.attack_result.target_monster);
    ASSERT_EQ_INT(12, state.monster_hp[2]);
}

static void test_projectile_weapon_hits_nearest_monster_in_line(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    place_monster(8, 4, 40);
    state.monster_x[1] = 6;
    state.monster_y[1] = 4;
    state.monster_hp[1] = 40;
    input = base_input(U3_COMBAT_COMMAND_ATTACK);
    input.attack_direction_x = 1;
    input.weapon = 3;

    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.attacked);
    ASSERT_EQ_INT(1, result.attack_result.used_projectile);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_ATTACK_HIT, result.status);
    ASSERT_EQ_INT(1, result.attack_result.target_monster);
    ASSERT_EQ_INT(12, state.monster_hp[1]);
    ASSERT_EQ_INT(40, state.monster_hp[2]);
}

static void test_projectile_weapon_line_ignores_terrain_and_characters(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    place_monster(8, 4, 40);
    state.tile_array[(4 * 11) + 5] = 0x10;
    state.character_x[1] = 6;
    state.character_y[1] = 4;
    input = base_input(U3_COMBAT_COMMAND_ATTACK);
    input.attack_direction_x = 1;
    input.weapon = 3;

    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.attacked);
    ASSERT_EQ_INT(1, result.attack_result.used_projectile);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_ATTACK_HIT, result.status);
    ASSERT_EQ_INT(2, result.attack_result.target_monster);
    ASSERT_EQ_INT(12, state.monster_hp[2]);
}

static void test_projectile_weapon_misses_when_line_leaves_arena(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    input = base_input(U3_COMBAT_COMMAND_ATTACK);
    input.attack_direction_x = 1;
    input.weapon = 3;

    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.attacked);
    ASSERT_EQ_INT(1, result.attack_result.used_projectile);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_ATTACK_MISSED, result.status);
    ASSERT_EQ_INT(U3_COMBAT_NO_SLOT, result.attack_result.target_monster);
}

static void test_thrown_dagger_decrements_inventory_and_uses_projectile_line(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    place_monster(8, 4, 40);
    input = base_input(U3_COMBAT_COMMAND_ATTACK);
    input.attack_direction_x = 1;
    input.weapon = 1;
    input.weapon_quantity = 2;
    state.character_weapon[0] = 1;
    state.character_weapon_inventory[0][1] = 2;

    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.attacked);
    ASSERT_EQ_INT(1, result.attack_result.used_thrown_dagger);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_ATTACK_HIT, result.status);
    ASSERT_EQ_INT(1, state.character_weapon[0]);
    ASSERT_EQ_INT(1, state.character_weapon_inventory[0][1]);
}

static void test_last_thrown_dagger_unequips_on_miss(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    input = base_input(U3_COMBAT_COMMAND_ATTACK);
    input.attack_direction_x = 1;
    input.weapon = 1;
    input.weapon_quantity = 1;
    state.character_weapon[0] = 1;
    state.character_weapon_inventory[0][1] = 1;

    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.attacked);
    ASSERT_EQ_INT(1, result.attack_result.used_thrown_dagger);
    ASSERT_EQ_INT(1, result.attack_result.unequipped_weapon);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_ATTACK_MISSED, result.status);
    ASSERT_EQ_INT(0, state.character_weapon[0]);
    ASSERT_EQ_INT(0, state.character_weapon_inventory[0][1]);
}

static void test_spell_without_direction_requests_direction(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    input = base_input(U3_COMBAT_COMMAND_SPELL);

    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.attack_direction_required);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_SPELL_DIRECTION_REQUIRED, result.status);
    ASSERT_EQ_INT(25, input.magic);
    ASSERT_EQ_INT(0, state.character_magic[0]);
}

static void test_mittar_hits_first_monster_in_line_and_spends_magic(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    place_monster(8, 4, 40);
    input = base_input(U3_COMBAT_COMMAND_SPELL);
    input.attack_direction_x = 1;
    state.character_class[0] = input.character_class;
    state.character_magic[0] = input.magic;

    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_SPELL_HIT, result.status);
    ASSERT_EQ_INT(1, result.spell_result.hit);
    ASSERT_EQ_INT(1, result.spell_result.spent_magic);
    ASSERT_EQ_INT(25, result.spell_result.magic_before);
    ASSERT_EQ_INT(20, result.spell_result.magic_after);
    ASSERT_EQ_INT(20, state.character_magic[0]);
    ASSERT_EQ_INT(21, result.spell_result.damage_amount);
    ASSERT_EQ_INT(19, state.monster_hp[2]);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_IMMOLATE, result.sound_id);
}

static void test_mittar_misses_empty_line_but_spends_magic(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    input = base_input(U3_COMBAT_COMMAND_SPELL);
    input.attack_direction_x = 1;
    state.character_class[0] = input.character_class;
    state.character_magic[0] = input.magic;

    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_SPELL_MISSED, result.status);
    ASSERT_EQ_INT(1, result.spell_result.miss);
    ASSERT_EQ_INT(1, result.spell_result.spent_magic);
    ASSERT_EQ_INT(20, result.spell_result.magic_after);
    ASSERT_EQ_INT(20, state.character_magic[0]);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_FAILED_SPELL, result.sound_id);
}

static void test_mittar_rejects_invalid_caster_without_spending_magic(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    place_monster(8, 4, 40);
    input = base_input(U3_COMBAT_COMMAND_SPELL);
    input.attack_direction_x = 1;
    input.character_class = 'F';
    state.character_class[0] = input.character_class;
    state.character_magic[0] = input.magic;

    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_SPELL_INVALID_CASTER, result.status);
    ASSERT_EQ_INT(1, result.spell_result.invalid_caster);
    ASSERT_EQ_INT(0, result.spell_result.spent_magic);
    ASSERT_EQ_INT(25, result.spell_result.magic_after);
    ASSERT_EQ_INT(25, state.character_magic[0]);
    ASSERT_EQ_INT(40, state.monster_hp[2]);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_FAILED_SPELL, result.sound_id);
}

static void test_mittar_rejects_insufficient_magic_without_mutation(void)
{
    u3_combat_player_command_input input;
    u3_combat_player_command_result result;

    reset_state();
    place_monster(8, 4, 40);
    input = base_input(U3_COMBAT_COMMAND_SPELL);
    input.attack_direction_x = 1;
    input.magic = 4;
    state.character_class[0] = input.character_class;
    state.character_magic[0] = input.magic;

    result = u3_combat_player_command(&state, experience, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(U3_COMBAT_PLAYER_STATUS_SPELL_INSUFFICIENT_MAGIC, result.status);
    ASSERT_EQ_INT(1, result.spell_result.insufficient_magic);
    ASSERT_EQ_INT(0, result.spell_result.spent_magic);
    ASSERT_EQ_INT(4, result.spell_result.magic_after);
    ASSERT_EQ_INT(4, state.character_magic[0]);
    ASSERT_EQ_INT(40, state.monster_hp[2]);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_FAILED_SPELL, result.sound_id);
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

static void test_party_defeat_reports_no_active_party_members(void)
{
    u3_combat_party_defeat_result result;

    reset_state();
    state.character_status[0] = U3_COMBAT_STATUS_DEAD;
    state.character_x[0] = 0xFF;
    state.character_y[0] = 0xFF;
    state.character_status[1] = U3_COMBAT_STATUS_ASH;
    state.character_x[1] = 0xFF;
    state.character_y[1] = 0xFF;

    result = u3_combat_check_party_defeat(&state, 2);

    ASSERT_EQ_INT(1, result.checked);
    ASSERT_EQ_INT(1, result.defeated);
    ASSERT_EQ_INT(2, result.party_size);
    ASSERT_EQ_INT(0, result.active_characters);
    ASSERT_EQ_INT(2, result.defeated_characters);
}

static void test_party_defeat_ignores_inactive_slots(void)
{
    u3_combat_party_defeat_result result;

    reset_state();
    state.character_status[0] = U3_COMBAT_STATUS_DEAD;
    state.character_x[0] = 0xFF;
    state.character_y[0] = 0xFF;
    state.character_status[1] = U3_COMBAT_STATUS_GOOD;
    state.character_x[1] = 4;
    state.character_y[1] = 5;

    result = u3_combat_check_party_defeat(&state, 1);

    ASSERT_EQ_INT(1, result.checked);
    ASSERT_EQ_INT(1, result.defeated);
    ASSERT_EQ_INT(1, result.party_size);
    ASSERT_EQ_INT(0, result.active_characters);
    ASSERT_EQ_INT(1, result.defeated_characters);
}

static void test_party_defeat_reports_surviving_poisoned_character(void)
{
    u3_combat_party_defeat_result result;

    reset_state();
    state.character_status[0] = U3_COMBAT_STATUS_POISONED;

    result = u3_combat_check_party_defeat(&state, 1);

    ASSERT_EQ_INT(1, result.checked);
    ASSERT_EQ_INT(0, result.defeated);
    ASSERT_EQ_INT(1, result.party_size);
    ASSERT_EQ_INT(1, result.active_characters);
    ASSERT_EQ_INT(0, result.defeated_characters);
}

int main(void)
{
    test_pass_is_handled_without_redraw();
    test_flee_success_reports_escape_without_mutation();
    test_flee_failure_consumes_turn_without_mutation();
    test_cardinal_move_updates_character_coordinate();
    test_blocked_move_preserves_character_coordinate();
    test_attack_without_direction_requests_direction();
    test_direction_supplied_melee_attack_hits_adjacent_monster();
    test_direction_supplied_melee_attack_misses_empty_tile();
    test_projectile_weapon_hits_first_monster_in_line();
    test_projectile_weapon_hits_nearest_monster_in_line();
    test_projectile_weapon_line_ignores_terrain_and_characters();
    test_projectile_weapon_misses_when_line_leaves_arena();
    test_thrown_dagger_decrements_inventory_and_uses_projectile_line();
    test_last_thrown_dagger_unequips_on_miss();
    test_spell_without_direction_requests_direction();
    test_mittar_hits_first_monster_in_line_and_spends_magic();
    test_mittar_misses_empty_line_but_spends_magic();
    test_mittar_rejects_invalid_caster_without_spending_magic();
    test_mittar_rejects_insufficient_magic_without_mutation();
    test_dead_character_command_reports_unsupported_without_mutation();
    test_party_defeat_reports_no_active_party_members();
    test_party_defeat_ignores_inactive_slots();
    test_party_defeat_reports_surviving_poisoned_character();

    puts("combat player command characterization passed");
    return 0;
}
