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
}

static u3_combat_attack_input base_attack(void)
{
    u3_combat_attack_input input;

    memset(&input, 0, sizeof(input));
    input.character = 0;
    input.direction_x = 1;
    input.direction_y = 0;
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

static void place_character_and_monster(uint8_t monster_hp)
{
    state.character_x[0] = 4;
    state.character_y[0] = 4;
    state.monster_x[2] = 5;
    state.monster_y[2] = 4;
    state.monster_tile[2] = 0x06;
    state.monster_hp[2] = monster_hp;
    state.tile_array[(4 * 11) + 5] = state.monster_type;
}

static void test_zero_direction_cancels_without_swing(void)
{
    u3_combat_attack_input input;
    u3_combat_attack_result result;

    reset_state();
    input = base_attack();
    input.direction_x = 0;
    input.direction_y = 0;

    result = u3_combat_attack(&state, experience, &input);

    ASSERT_EQ_INT(1, result.cancelled);
    ASSERT_EQ_INT(0, result.swish_sound);
    ASSERT_EQ_INT(0, result.miss);
    ASSERT_EQ_INT(0, result.hit);
}

static void test_melee_hit_applies_damage_and_restores_monster_type_when_alive(void)
{
    u3_combat_attack_input input;
    u3_combat_attack_result result;

    reset_state();
    place_character_and_monster(30);
    input = base_attack();

    result = u3_combat_attack(&state, experience, &input);

    ASSERT_EQ_INT(1, result.hit);
    ASSERT_EQ_INT(0, result.miss);
    ASSERT_EQ_INT(1, result.print_hit_message);
    ASSERT_EQ_INT(1, result.play_hit_sound);
    ASSERT_EQ_INT(1, result.redraw_tiles);
    ASSERT_EQ_INT(1, result.pause_after_hit);
    ASSERT_EQ_INT(U3_COMBAT_ATTACK_HIT_MESSAGE, result.message_id);
    ASSERT_EQ_INT(2, result.target_monster);
    ASSERT_EQ_INT(5, result.hit_x);
    ASSERT_EQ_INT(4, result.hit_y);
    ASSERT_EQ_INT(U3_COMBAT_ATTACK_HIT_TILE, result.hit_tile);
    ASSERT_EQ_INT(25, result.damage_amount);
    ASSERT_EQ_INT(5, state.monster_hp[2]);
    ASSERT_EQ_INT(state.monster_type, state.tile_array[(4 * 11) + 5]);
    ASSERT_EQ_INT(1, result.damage_result.damaged);
    ASSERT_EQ_INT(1, result.damage_result.character);
}

static void test_melee_miss_when_no_adjacent_monster(void)
{
    u3_combat_attack_input input;
    u3_combat_attack_result result;

    reset_state();
    state.character_x[0] = 4;
    state.character_y[0] = 4;
    input = base_attack();

    result = u3_combat_attack(&state, experience, &input);

    ASSERT_EQ_INT(1, result.miss);
    ASSERT_EQ_INT(0, result.hit);
    ASSERT_EQ_INT(U3_COMBAT_ATTACK_MISS_MESSAGE, result.message_id);
    ASSERT_EQ_INT(1, result.redraw_tiles);
    ASSERT_EQ_INT(U3_COMBAT_NO_SLOT, result.target_monster);
}

static void test_projectile_weapon_uses_supplied_projectile_target(void)
{
    u3_combat_attack_input input;
    u3_combat_attack_result result;

    reset_state();
    place_character_and_monster(40);
    input = base_attack();
    input.weapon = 3;
    input.projectile_monster = 2;

    result = u3_combat_attack(&state, experience, &input);

    ASSERT_EQ_INT(1, result.used_projectile);
    ASSERT_EQ_INT(1, result.hit);
    ASSERT_EQ_INT(28, result.damage_amount);
    ASSERT_EQ_INT(12, state.monster_hp[2]);
}

static void test_projectile_weapon_misses_when_supplied_projectile_target_misses(void)
{
    u3_combat_attack_input input;
    u3_combat_attack_result result;

    reset_state();
    state.character_x[0] = 4;
    state.character_y[0] = 4;
    input = base_attack();
    input.weapon = 5;
    input.projectile_monster = U3_COMBAT_NO_SLOT;

    result = u3_combat_attack(&state, experience, &input);

    ASSERT_EQ_INT(1, result.used_projectile);
    ASSERT_EQ_INT(1, result.miss);
    ASSERT_EQ_INT(0, result.hit);
}

static void test_projectile_weapon_misses_when_supplied_projectile_target_is_dead(void)
{
    u3_combat_attack_input input;
    u3_combat_attack_result result;

    reset_state();
    place_character_and_monster(0);
    input = base_attack();
    input.weapon = 3;
    input.projectile_monster = 2;

    result = u3_combat_attack(&state, experience, &input);

    ASSERT_EQ_INT(1, result.used_projectile);
    ASSERT_EQ_INT(1, result.miss);
    ASSERT_EQ_INT(0, result.hit);
    ASSERT_EQ_INT(0, state.monster_hp[2]);
}

static void test_thrown_dagger_decrements_inventory_before_projectile_hit(void)
{
    u3_combat_attack_input input;
    u3_combat_attack_result result;

    reset_state();
    place_character_and_monster(30);
    input = base_attack();
    input.weapon = 1;
    input.weapon_quantity = 2;
    input.direction_x = 0;
    input.direction_y = 1;
    input.projectile_monster = 2;

    result = u3_combat_attack(&state, experience, &input);

    ASSERT_EQ_INT(1, result.used_thrown_dagger);
    ASSERT_EQ_INT(1, result.mutated_inventory);
    ASSERT_EQ_INT(1, result.weapon_after);
    ASSERT_EQ_INT(1, result.weapon_quantity_after);
    ASSERT_EQ_INT(22, result.damage_amount);
    ASSERT_EQ_INT(8, state.monster_hp[2]);
}

static void test_last_thrown_dagger_unequips_before_damage_bonus(void)
{
    u3_combat_attack_input input;
    u3_combat_attack_result result;

    reset_state();
    place_character_and_monster(30);
    input = base_attack();
    input.weapon = 1;
    input.weapon_quantity = 1;
    input.direction_x = 0;
    input.direction_y = 1;
    input.projectile_monster = 2;

    result = u3_combat_attack(&state, experience, &input);

    ASSERT_EQ_INT(1, result.used_thrown_dagger);
    ASSERT_EQ_INT(1, result.unequipped_weapon);
    ASSERT_EQ_INT(0, result.weapon_after);
    ASSERT_EQ_INT(0, result.weapon_quantity_after);
    ASSERT_EQ_INT(19, result.damage_amount);
    ASSERT_EQ_INT(11, state.monster_hp[2]);
}

static void test_exodus_castle_restricts_non_exotic_weapon_after_targeting(void)
{
    u3_combat_attack_input input;
    u3_combat_attack_result result;

    reset_state();
    place_character_and_monster(30);
    input = base_attack();
    input.exodus_castle_result = 0;

    result = u3_combat_attack(&state, experience, &input);

    ASSERT_EQ_INT(1, result.miss);
    ASSERT_EQ_INT(0, result.hit);
    ASSERT_EQ_INT(2, result.target_monster);
    ASSERT_EQ_INT(30, state.monster_hp[2]);
}

static void test_exodus_castle_allows_weapon_15(void)
{
    u3_combat_attack_input input;
    u3_combat_attack_result result;

    reset_state();
    place_character_and_monster(100);
    input = base_attack();
    input.weapon = 15;
    input.exodus_castle_result = 0;

    result = u3_combat_attack(&state, experience, &input);

    ASSERT_EQ_INT(1, result.hit);
    ASSERT_EQ_INT(64, result.damage_amount);
    ASSERT_EQ_INT(36, state.monster_hp[2]);
}

static void test_low_hit_roll_uses_dexterity_check_for_miss(void)
{
    u3_combat_attack_input input;
    u3_combat_attack_result result;

    reset_state();
    place_character_and_monster(30);
    input = base_attack();
    input.hit_chance_roll = 127;
    input.dexterity = 49;
    input.hit_dexterity_roll = 50;

    result = u3_combat_attack(&state, experience, &input);

    ASSERT_EQ_INT(1, result.miss);
    ASSERT_EQ_INT(0, result.hit);
    ASSERT_EQ_INT(30, state.monster_hp[2]);
}

static void test_low_hit_roll_hits_when_dexterity_matches_roll(void)
{
    u3_combat_attack_input input;
    u3_combat_attack_result result;

    reset_state();
    place_character_and_monster(30);
    input = base_attack();
    input.hit_chance_roll = 0;
    input.dexterity = 50;
    input.hit_dexterity_roll = 50;

    result = u3_combat_attack(&state, experience, &input);

    ASSERT_EQ_INT(1, result.hit);
    ASSERT_EQ_INT(5, state.monster_hp[2]);
}

static void test_hit_that_defeats_monster_uses_damage_core(void)
{
    u3_combat_attack_input input;
    u3_combat_attack_result result;

    reset_state();
    place_character_and_monster(21);
    state.monster_tile[2] = 0x04;
    experience[(0x10 / 2) & 0x0F] = 33;
    input = base_attack();

    result = u3_combat_attack(&state, experience, &input);

    ASSERT_EQ_INT(1, result.hit);
    ASSERT_EQ_INT(1, result.damage_result.defeated);
    ASSERT_EQ_INT(33, result.damage_result.experience_awarded);
    ASSERT_EQ_INT(0, state.monster_hp[2]);
    ASSERT_EQ_INT(0x04, state.tile_array[(4 * 11) + 5]);
}

int main(void)
{
    test_zero_direction_cancels_without_swing();
    test_melee_hit_applies_damage_and_restores_monster_type_when_alive();
    test_melee_miss_when_no_adjacent_monster();
    test_projectile_weapon_uses_supplied_projectile_target();
    test_projectile_weapon_misses_when_supplied_projectile_target_misses();
    test_projectile_weapon_misses_when_supplied_projectile_target_is_dead();
    test_thrown_dagger_decrements_inventory_before_projectile_hit();
    test_last_thrown_dagger_unequips_before_damage_bonus();
    test_exodus_castle_restricts_non_exotic_weapon_after_targeting();
    test_exodus_castle_allows_weapon_15();
    test_low_hit_roll_uses_dexterity_check_for_miss();
    test_low_hit_roll_hits_when_dexterity_matches_roll();
    test_hit_that_defeats_monster_uses_damage_core();

    puts("combat attack characterization passed");
    return 0;
}
