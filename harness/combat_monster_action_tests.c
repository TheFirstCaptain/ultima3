#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_combat.h"

static u3_combat_state state;

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
    state.monster_type = 0x10;
}

static u3_combat_monster_action_input base_action(void)
{
    u3_combat_monster_action_input input;

    memset(&input, 0, sizeof(input));
    input.monster = 1;
    input.party_size = 4;
    input.target_character = 0;
    input.target_distance = 0;
    input.move_x = 5;
    input.move_y = 5;
    input.projectile_hit_character = U3_COMBAT_NO_SLOT;
    input.magic_target_character = 0;
    input.monster_hp_start = 0x20;
    input.monster_tile_value = 0;
    input.poison_roll = 0xFF;
    input.pilfer_branch_roll = 0;
    input.pilfer_item_roll = 0;
    input.exodus_castle_active = 0;
    input.exodus_damage_flags = 0;
    input.armour_hit_roll = 0;
    input.damage_roll = 4;
    return input;
}

static void place_live_monster(void)
{
    state.monster_x[1] = 4;
    state.monster_y[1] = 4;
    state.monster_tile[1] = 0x02;
    state.monster_hp[1] = 10;
    state.tile_array[(4 * 11) + 4] = (uint8_t)state.monster_type;
}

static void place_live_character(uint8_t character)
{
    state.character_x[character] = 4;
    state.character_y[character] = 5;
    state.character_tile[character] = 0x03;
    state.character_shape[character] = 0x80 + character;
    state.character_status[character] = U3_COMBAT_STATUS_GOOD;
    state.character_armour[character] = 1;
    state.character_hp[character] = 50;
    state.character_experience[character] = 200;
    state.tile_array[(5 * 11) + 4] = state.character_shape[character];
}

static void test_dead_monster_has_no_action(void)
{
    u3_combat_monster_action_input input;
    u3_combat_monster_action_result result;

    reset_state();
    place_live_character(0);
    input = base_action();

    result = u3_combat_monster_action(&state, &input);

    ASSERT_EQ_INT(1, result.no_action);
    ASSERT_EQ_INT(0, result.attacked);
    ASSERT_EQ_INT(0, result.moved);
}

static void test_nonadjacent_monster_moves_to_supplied_position(void)
{
    u3_combat_monster_action_input input;
    u3_combat_monster_action_result result;

    reset_state();
    place_live_monster();
    place_live_character(0);
    state.tile_array[(5 * 11) + 5] = 0x06;
    input = base_action();
    input.target_distance = 4;

    result = u3_combat_monster_action(&state, &input);

    ASSERT_EQ_INT(1, result.moved);
    ASSERT_EQ_INT(4, result.moved_from_x);
    ASSERT_EQ_INT(4, result.moved_from_y);
    ASSERT_EQ_INT(5, result.moved_to_x);
    ASSERT_EQ_INT(5, result.moved_to_y);
    ASSERT_EQ_INT(5, state.monster_x[1]);
    ASSERT_EQ_INT(5, state.monster_y[1]);
    ASSERT_EQ_INT(0x06, state.monster_tile[1]);
    ASSERT_EQ_INT(0x02, state.tile_array[(4 * 11) + 4]);
    ASSERT_EQ_INT(state.monster_type, state.tile_array[(5 * 11) + 5]);
}

static void test_archer_shoots_supplied_character_without_attack_roll(void)
{
    u3_combat_monster_action_input input;
    u3_combat_monster_action_result result;

    reset_state();
    state.monster_type = 0x3A;
    place_live_monster();
    place_live_character(2);
    input = base_action();
    input.target_distance = 4;
    input.shoot_choice_roll = 127;
    input.projectile_hit_character = 2;
    input.armour_hit_roll = 99;

    result = u3_combat_monster_action(&state, &input);

    ASSERT_EQ_INT(1, result.shot);
    ASSERT_EQ_INT(1, result.hit);
    ASSERT_EQ_INT(0, result.attacked);
    ASSERT_EQ_INT(1, result.play_shoot_sound);
    ASSERT_EQ_INT(1, result.play_hit_sound);
    ASSERT_EQ_INT(0, result.message_id);
    ASSERT_EQ_INT(0, result.outcome_message_id);
    ASSERT_EQ_INT(5, result.damage_amount);
    ASSERT_EQ_INT(45, state.character_hp[2]);
}

static void test_archer_projectile_miss_does_not_damage(void)
{
    u3_combat_monster_action_input input;
    u3_combat_monster_action_result result;

    reset_state();
    state.monster_type = 0x3A;
    place_live_monster();
    place_live_character(0);
    input = base_action();
    input.target_distance = 4;
    input.shoot_choice_roll = 0;
    input.projectile_hit_character = U3_COMBAT_NO_SLOT;

    result = u3_combat_monster_action(&state, &input);

    ASSERT_EQ_INT(1, result.shot);
    ASSERT_EQ_INT(1, result.projectile_missed);
    ASSERT_EQ_INT(0, result.hit);
    ASSERT_EQ_INT(50, state.character_hp[0]);
}

static void test_magic_monster_casts_on_good_random_target_then_hits(void)
{
    u3_combat_monster_action_input input;
    u3_combat_monster_action_result result;

    reset_state();
    state.monster_type = 0x1C;
    place_live_monster();
    place_live_character(0);
    place_live_character(3);
    input = base_action();
    input.target_distance = 5;
    input.magic_choice_roll = 128;
    input.magic_target_character = 3;
    input.poison_roll = 1;

    result = u3_combat_monster_action(&state, &input);

    ASSERT_EQ_INT(1, result.cast_spell);
    ASSERT_EQ_INT(1, result.attacked);
    ASSERT_EQ_INT(1, result.hit);
    ASSERT_EQ_INT(3, result.target_character);
    ASSERT_EQ_INT(0x78, result.hit_tile);
    ASSERT_EQ_INT(45, state.character_hp[3]);
}

static void test_lord_british_moves_instead_of_casting(void)
{
    u3_combat_monster_action_input input;
    u3_combat_monster_action_result result;

    reset_state();
    state.monster_type = U3_COMBAT_LORD_BRITISH_TYPE;
    place_live_monster();
    place_live_character(0);
    state.tile_array[(5 * 11) + 5] = 0x06;
    input = base_action();
    input.target_distance = 5;
    input.magic_choice_roll = 128;
    input.magic_target_character = 0;

    result = u3_combat_monster_action(&state, &input);

    ASSERT_EQ_INT(1, result.moved);
    ASSERT_EQ_INT(0, result.cast_spell);
    ASSERT_EQ_INT(5, state.monster_x[1]);
    ASSERT_EQ_INT(5, state.monster_y[1]);
}

static void test_magic_monster_can_cast_without_figure_target(void)
{
    u3_combat_monster_action_input input;
    u3_combat_monster_action_result result;

    reset_state();
    state.monster_type = 0x1C;
    place_live_monster();
    place_live_character(2);
    input = base_action();
    input.target_character = U3_COMBAT_NO_SLOT;
    input.target_distance = 0xFF;
    input.magic_choice_roll = 128;
    input.magic_target_character = 2;
    input.poison_roll = 1;

    result = u3_combat_monster_action(&state, &input);

    ASSERT_EQ_INT(1, result.cast_spell);
    ASSERT_EQ_INT(1, result.attacked);
    ASSERT_EQ_INT(1, result.hit);
    ASSERT_EQ_INT(2, result.target_character);
    ASSERT_EQ_INT(45, state.character_hp[2]);
}

static void test_magic_monster_moves_when_random_target_is_not_good(void)
{
    u3_combat_monster_action_input input;
    u3_combat_monster_action_result result;

    reset_state();
    state.monster_type = 0x1C;
    place_live_monster();
    place_live_character(0);
    state.character_status[3] = U3_COMBAT_STATUS_DEAD;
    input = base_action();
    input.target_distance = 5;
    input.magic_choice_roll = 128;
    input.magic_target_character = 3;

    result = u3_combat_monster_action(&state, &input);

    ASSERT_EQ_INT(1, result.moved);
    ASSERT_EQ_INT(0, result.cast_spell);
    ASSERT_EQ_INT(0, result.hit);
}

static void test_poison_monster_poison_attempt_precedes_attack_miss(void)
{
    u3_combat_monster_action_input input;
    u3_combat_monster_action_result result;

    reset_state();
    state.monster_type = 0x38;
    place_live_monster();
    place_live_character(0);
    input = base_action();
    input.poison_roll = 0;
    input.armour_hit_roll = 8;

    result = u3_combat_monster_action(&state, &input);

    ASSERT_EQ_INT(1, result.poisoned);
    ASSERT_EQ_INT(U3_COMBAT_STATUS_POISONED, state.character_status[0]);
    ASSERT_EQ_INT(1, result.miss);
    ASSERT_EQ_INT(U3_COMBAT_MONSTER_MISS_MESSAGE, result.message_id);
    ASSERT_EQ_INT(U3_COMBAT_MONSTER_ATTACK_MESSAGE, result.attack_message_id);
    ASSERT_EQ_INT(U3_COMBAT_MONSTER_MISS_MESSAGE, result.outcome_message_id);
    ASSERT_EQ_INT(U3_COMBAT_POISON_MESSAGE, result.status_message_id);
}

static void test_pilfer_monster_removes_carried_weapon(void)
{
    u3_combat_monster_action_input input;
    u3_combat_monster_action_result result;

    reset_state();
    state.monster_type = 0x2E;
    place_live_monster();
    place_live_character(0);
    state.character_weapon[0] = 2;
    state.character_weapon_inventory[0][4] = 1;
    input = base_action();
    input.pilfer_branch_roll = 0;
    input.pilfer_item_roll = 4;
    input.armour_hit_roll = 8;

    result = u3_combat_monster_action(&state, &input);

    ASSERT_EQ_INT(1, result.pilfered);
    ASSERT_EQ_INT(1, result.pilfered_weapon);
    ASSERT_EQ_INT(4, result.pilfer_item);
    ASSERT_EQ_INT(0, state.character_weapon_inventory[0][4]);
    ASSERT_EQ_INT(U3_COMBAT_PILFER_MESSAGE, result.status_message_id);
}

static void test_pilfer_monster_does_not_remove_equipped_armour(void)
{
    u3_combat_monster_action_input input;
    u3_combat_monster_action_result result;

    reset_state();
    state.monster_type = 0x2E;
    place_live_monster();
    place_live_character(0);
    state.character_armour[0] = 3;
    state.character_armour_inventory[0][3] = 1;
    input = base_action();
    input.pilfer_branch_roll = 128;
    input.pilfer_item_roll = 3;
    input.armour_hit_roll = 8;

    result = u3_combat_monster_action(&state, &input);

    ASSERT_EQ_INT(0, result.pilfered);
    ASSERT_EQ_INT(1, state.character_armour_inventory[0][3]);
    ASSERT_EQ_INT(1, result.miss);
}

static void test_exodus_castle_auto_hits_non_exotic_armour_and_adds_damage(void)
{
    u3_combat_monster_action_input input;
    u3_combat_monster_action_result result;

    reset_state();
    place_live_monster();
    place_live_character(0);
    input = base_action();
    input.exodus_castle_active = 1;
    input.exodus_damage_flags = 3;
    input.armour_hit_roll = 99;

    result = u3_combat_monster_action(&state, &input);

    ASSERT_EQ_INT(1, result.hit);
    ASSERT_EQ_INT(53, result.damage_amount);
    ASSERT_EQ_INT(0, state.character_hp[0]);
    ASSERT_EQ_INT(U3_COMBAT_STATUS_DEAD, state.character_status[0]);
    ASSERT_EQ_INT(1, result.character_died);
}

static void test_poison_and_lethal_hit_preserve_separate_messages(void)
{
    u3_combat_monster_action_input input;
    u3_combat_monster_action_result result;

    reset_state();
    state.monster_type = 0x38;
    place_live_monster();
    place_live_character(0);
    state.character_hp[0] = 5;
    input = base_action();
    input.poison_roll = 0;
    input.damage_roll = 4;

    result = u3_combat_monster_action(&state, &input);

    ASSERT_EQ_INT(1, result.poisoned);
    ASSERT_EQ_INT(1, result.hit);
    ASSERT_EQ_INT(1, result.character_died);
    ASSERT_EQ_INT(U3_COMBAT_MONSTER_ATTACK_MESSAGE, result.attack_message_id);
    ASSERT_EQ_INT(U3_COMBAT_POISON_MESSAGE, result.status_message_id);
    ASSERT_EQ_INT(U3_COMBAT_MONSTER_HIT_MESSAGE, result.outcome_message_id);
    ASSERT_EQ_INT(U3_COMBAT_CHARACTER_DIED_MESSAGE, result.death_message_id);
}

static void test_exotic_armour_in_exodus_still_uses_armour_roll(void)
{
    u3_combat_monster_action_input input;
    u3_combat_monster_action_result result;

    reset_state();
    place_live_monster();
    place_live_character(0);
    state.character_armour[0] = 7;
    input = base_action();
    input.exodus_castle_active = 1;
    input.armour_hit_roll = 8;

    result = u3_combat_monster_action(&state, &input);

    ASSERT_EQ_INT(1, result.miss);
    ASSERT_EQ_INT(0, result.hit);
    ASSERT_EQ_INT(50, state.character_hp[0]);
}

static u3_combat_monster_turn_input base_turn(void)
{
    u3_combat_monster_turn_input input;

    memset(&input, 0, sizeof(input));
    input.party_size = 4;
    input.starting_monster = 0;
    input.projectile_hit_character = U3_COMBAT_NO_SLOT;
    input.magic_target_character = 0;
    input.poison_roll = 0xFF;
    input.armour_hit_roll = 0;
    input.damage_roll = 4;
    return input;
}

static void test_monster_turn_moves_first_live_monster_toward_nearest_character(void)
{
    u3_combat_monster_turn_input input;
    u3_combat_monster_turn_result result;

    reset_state();
    state.monster_x[3] = 2;
    state.monster_y[3] = 2;
    state.monster_tile[3] = 0x02;
    state.monster_hp[3] = 20;
    state.tile_array[(2 * 11) + 2] = (uint8_t)state.monster_type;
    place_live_character(0);
    input = base_turn();
    input.party_size = 1;

    result = u3_combat_monster_turn(&state, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(3, result.monster);
    ASSERT_EQ_INT(4, result.next_starting_monster);
    ASSERT_EQ_INT(0, result.target_character);
    ASSERT_EQ_INT(U3_COMBAT_MONSTER_TURN_STATUS_MOVED, result.status);
    ASSERT_EQ_INT(3, state.monster_x[3]);
    ASSERT_EQ_INT(3, state.monster_y[3]);
    ASSERT_EQ_INT(1, result.redraw);
}

static void test_monster_turn_uses_manhattan_target_after_adjacent_check(void)
{
    u3_combat_monster_turn_input input;
    u3_combat_monster_turn_result result;

    reset_state();
    state.monster_x[0] = 1;
    state.monster_y[0] = 1;
    state.monster_tile[0] = 0x02;
    state.monster_hp[0] = 20;
    state.tile_array[(1 * 11) + 1] = (uint8_t)state.monster_type;
    place_live_character(0);
    state.character_x[0] = 5;
    state.character_y[0] = 5;
    state.character_shape[0] = 0x80;
    state.tile_array[(5 * 11) + 5] = state.character_shape[0];
    place_live_character(1);
    state.character_x[1] = 7;
    state.character_y[1] = 1;
    state.character_shape[1] = 0x81;
    state.tile_array[(1 * 11) + 7] = state.character_shape[1];
    input = base_turn();
    input.party_size = 2;

    result = u3_combat_monster_turn(&state, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.target_character);
    ASSERT_EQ_INT(U3_COMBAT_MONSTER_TURN_STATUS_MOVED, result.status);
    ASSERT_EQ_INT(2, state.monster_x[0]);
    ASSERT_EQ_INT(1, state.monster_y[0]);
}

static void test_monster_turn_attacks_and_mutates_character_state(void)
{
    u3_combat_monster_turn_input input;
    u3_combat_monster_turn_result result;

    reset_state();
    state.monster_type = 0x38;
    place_live_monster();
    state.monster_hp[1] = 0x20;
    place_live_character(0);
    state.character_hp[0] = 5;
    input = base_turn();
    input.party_size = 1;
    input.starting_monster = 1;
    input.poison_roll = 0;

    result = u3_combat_monster_turn(&state, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.monster);
    ASSERT_EQ_INT(U3_COMBAT_MONSTER_TURN_STATUS_ATTACK_HIT, result.status);
    ASSERT_EQ_INT(1, result.action_result.poisoned);
    ASSERT_EQ_INT(1, result.action_result.character_died);
    ASSERT_EQ_INT(0, state.character_hp[0]);
    ASSERT_EQ_INT(U3_COMBAT_STATUS_DEAD, state.character_status[0]);
}

static void test_monster_turn_reports_no_action_when_blocked_from_nonadjacent_target(void)
{
    u3_combat_monster_turn_input input;
    u3_combat_monster_turn_result result;

    reset_state();
    state.monster_x[0] = 1;
    state.monster_y[0] = 1;
    state.monster_tile[0] = 0x02;
    state.monster_hp[0] = 20;
    state.tile_array[(1 * 11) + 1] = (uint8_t)state.monster_type;
    place_live_character(0);
    state.character_x[0] = 4;
    state.character_y[0] = 4;
    state.character_shape[0] = 0x80;
    state.tile_array[(4 * 11) + 4] = state.character_shape[0];
    state.tile_array[(2 * 11) + 2] = 2;
    state.tile_array[(1 * 11) + 2] = 2;
    state.tile_array[(2 * 11) + 1] = 2;
    input = base_turn();
    input.party_size = 1;

    result = u3_combat_monster_turn(&state, &input);

    ASSERT_EQ_INT(1, result.handled);
    ASSERT_EQ_INT(1, result.no_action);
    ASSERT_EQ_INT(U3_COMBAT_MONSTER_TURN_STATUS_NO_ACTION, result.status);
    ASSERT_EQ_INT(1, state.monster_x[0]);
    ASSERT_EQ_INT(1, state.monster_y[0]);
}

static void test_monster_turn_suppresses_action_without_live_targets(void)
{
    u3_combat_monster_turn_input input;
    u3_combat_monster_turn_result result;

    reset_state();
    place_live_monster();
    place_live_character(0);
    state.character_status[0] = U3_COMBAT_STATUS_DEAD;
    state.character_x[0] = 0xFF;
    state.character_y[0] = 0xFF;
    input = base_turn();
    input.party_size = 1;
    input.starting_monster = 1;

    result = u3_combat_monster_turn(&state, &input);

    ASSERT_EQ_INT(0, result.handled);
    ASSERT_EQ_INT(1, result.no_action);
    ASSERT_EQ_INT(U3_COMBAT_MONSTER_TURN_STATUS_NO_ACTION, result.status);
    ASSERT_EQ_INT(10, state.monster_hp[1]);
}

int main(void)
{
    test_dead_monster_has_no_action();
    test_nonadjacent_monster_moves_to_supplied_position();
    test_archer_shoots_supplied_character_without_attack_roll();
    test_archer_projectile_miss_does_not_damage();
    test_magic_monster_casts_on_good_random_target_then_hits();
    test_lord_british_moves_instead_of_casting();
    test_magic_monster_can_cast_without_figure_target();
    test_magic_monster_moves_when_random_target_is_not_good();
    test_poison_monster_poison_attempt_precedes_attack_miss();
    test_pilfer_monster_removes_carried_weapon();
    test_pilfer_monster_does_not_remove_equipped_armour();
    test_exodus_castle_auto_hits_non_exotic_armour_and_adds_damage();
    test_poison_and_lethal_hit_preserve_separate_messages();
    test_exotic_armour_in_exodus_still_uses_armour_roll();
    test_monster_turn_moves_first_live_monster_toward_nearest_character();
    test_monster_turn_uses_manhattan_target_after_adjacent_check();
    test_monster_turn_attacks_and_mutates_character_state();
    test_monster_turn_reports_no_action_when_blocked_from_nonadjacent_target();
    test_monster_turn_suppresses_action_without_live_targets();

    puts("combat monster action characterization passed");
    return 0;
}
