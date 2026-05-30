#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_autocombat.h"

static u3_autocombat_state state;

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
    int character;

    memset(&state, 0, sizeof(state));
    memset(state.tile_array, 2, sizeof(state.tile_array));
    for (character = 0; character < U3_COMBAT_CHARACTER_COUNT; character++) {
        state.character_alive[character] = 1;
        state.character_hp[character] = 100;
    }
    state.allow_diagonal = true;
}

static void block_tile(int x, int y)
{
    state.tile_array[(y * 11) + x] = 0;
}

static void place_character(int character, int x, int y)
{
    state.combat.character_x[character] = (uint8_t)x;
    state.combat.character_y[character] = (uint8_t)y;
}

static void place_monster(int monster, int x, int y, int hp)
{
    state.combat.monster_x[monster] = (uint8_t)x;
    state.combat.monster_y[monster] = (uint8_t)y;
    state.combat.monster_hp[monster] = (uint8_t)hp;
    state.future_monster_x[monster] = (uint8_t)x;
    state.future_monster_y[monster] = (uint8_t)y;
}

static void test_threat_value_counts_live_monsters(void)
{
    reset_state();
    state.combat.monster_type = 0x10;
    state.experience[(0x10 / 2) & 0x0F] = 12;
    place_monster(0, 1, 1, 1);
    place_monster(3, 2, 2, 9);
    place_monster(7, 3, 3, 0);

    ASSERT_EQ_INT(24, u3_autocombat_threat_value(&state));
}

static void test_threat_value_doubles_poisonous_types(void)
{
    reset_state();
    state.combat.monster_type = 0x1C;
    state.experience[(0x1C / 2) & 0x0F] = 7;
    place_monster(0, 1, 1, 1);
    place_monster(1, 2, 2, 1);

    ASSERT_EQ_INT(28, u3_autocombat_threat_value(&state));
}

static void test_monster_can_attack_checks_adjacent_monsters(void)
{
    reset_state();
    place_monster(0, 4, 4, 1);
    place_monster(1, 8, 8, 1);

    ASSERT_EQ_INT(1, u3_autocombat_monster_can_attack(&state, 5, 5));
    ASSERT_EQ_INT(0, u3_autocombat_monster_can_attack(&state, 6, 6));
}

static void test_monster_can_attack_allows_dragon_diagonal_shots(void)
{
    reset_state();
    state.combat.monster_type = 0x3A;
    place_monster(0, 8, 8, 1);
    place_monster(1, 2, 7, 0);

    ASSERT_EQ_INT(1, u3_autocombat_monster_can_attack(&state, 5, 5));
    ASSERT_EQ_INT(0, u3_autocombat_monster_can_attack(&state, 3, 5));
}

static void test_monster_can_attack_ignores_dragon_line_for_other_types(void)
{
    reset_state();
    state.combat.monster_type = 0x38;
    place_monster(0, 8, 8, 1);

    ASSERT_EQ_INT(0, u3_autocombat_monster_can_attack(&state, 5, 5));
}

static void test_nearly_dead_checks_specific_character_hp_only(void)
{
    reset_state();
    state.character_hp[1] = 49;
    state.character_alive[2] = 0;

    ASSERT_EQ_INT(1, u3_autocombat_nearly_dead(&state, 2));
    ASSERT_EQ_INT(0, u3_autocombat_nearly_dead(&state, 3));
}

static void test_nearly_dead_anybody_checks_dead_or_low_hp_party_members(void)
{
    reset_state();
    ASSERT_EQ_INT(0, u3_autocombat_nearly_dead(&state, 0));

    state.character_hp[3] = 49;
    ASSERT_EQ_INT(1, u3_autocombat_nearly_dead(&state, 0));

    reset_state();
    state.character_alive[0] = 0;
    ASSERT_EQ_INT(1, u3_autocombat_nearly_dead(&state, 0));
}

static void test_monster_nearby_cardinal_priority(void)
{
    reset_state();
    place_character(0, 5, 5);
    place_monster(0, 5, 4, 1);
    place_monster(1, 4, 5, 1);
    place_monster(2, 6, 5, 1);
    place_monster(3, 5, 6, 1);

    ASSERT_EQ_INT('8', u3_autocombat_monster_nearby(&state, 0));
}

static void test_monster_nearby_diagonal_policy_and_priority(void)
{
    reset_state();
    place_character(0, 5, 5);
    place_monster(0, 4, 4, 1);
    place_monster(1, 4, 6, 1);

    state.allow_diagonal = false;
    ASSERT_EQ_INT(0, u3_autocombat_monster_nearby(&state, 0));

    state.allow_diagonal = true;
    ASSERT_EQ_INT('7', u3_autocombat_monster_nearby(&state, 0));
}

static void test_monster_lined_up_returns_shooting_direction_for_non_character(void)
{
    reset_state();
    place_monster(0, 9, 5, 1);

    ASSERT_EQ_INT('6', u3_autocombat_monster_lined_up(&state, 5, 5, 5));
}

static void test_monster_lined_up_respects_diagonal_policy(void)
{
    reset_state();
    place_monster(0, 8, 8, 1);

    state.allow_diagonal = false;
    ASSERT_EQ_INT(0, u3_autocombat_monster_lined_up(&state, 5, 5, 5));

    state.allow_diagonal = true;
    ASSERT_EQ_INT('3', u3_autocombat_monster_lined_up(&state, 5, 5, 5));
}

static void test_monster_lined_up_moves_character_toward_lined_target(void)
{
    reset_state();
    place_character(0, 5, 5);
    place_monster(0, 8, 5, 1);

    ASSERT_EQ_INT('6', u3_autocombat_monster_lined_up(&state, 0, 5, 5));
}

static void test_auto_move_char_direct_move(void)
{
    reset_state();
    place_character(0, 5, 5);

    ASSERT_EQ_INT('6', u3_autocombat_auto_move_char(&state, 0, 1, 0));
}

static void test_auto_move_char_disallows_diagonal_by_preferring_vertical(void)
{
    reset_state();
    place_character(0, 5, 5);
    state.allow_diagonal = false;

    ASSERT_EQ_INT('2', u3_autocombat_auto_move_char(&state, 0, 1, 1));
}

static void test_auto_move_char_uses_diagonal_fallback_for_blocked_horizontal(void)
{
    reset_state();
    place_character(0, 5, 5);
    block_tile(6, 5);

    ASSERT_EQ_INT('9', u3_autocombat_auto_move_char(&state, 0, 1, 0));
}

static void test_auto_move_char_passes_when_all_fallbacks_blocked(void)
{
    reset_state();
    place_character(0, 5, 5);
    block_tile(5, 4);
    block_tile(6, 4);
    block_tile(4, 4);
    block_tile(6, 5);
    block_tile(4, 5);

    ASSERT_EQ_INT(' ', u3_autocombat_auto_move_char(&state, 0, 0, -1));
}

static void test_setup_now_copies_current_monster_positions(void)
{
    reset_state();
    place_monster(0, 2, 3, 1);
    place_monster(7, 9, 8, 1);
    state.future_monster_x[0] = 99;
    state.future_monster_y[7] = 99;

    u3_autocombat_setup_now(&state);

    ASSERT_EQ_INT(2, state.future_monster_x[0]);
    ASSERT_EQ_INT(3, state.future_monster_y[0]);
    ASSERT_EQ_INT(9, state.future_monster_x[7]);
    ASSERT_EQ_INT(8, state.future_monster_y[7]);
}

static void test_future_monster_here_checks_live_monsters_and_alive_characters(void)
{
    reset_state();
    place_monster(0, 2, 3, 1);
    place_monster(1, 4, 5, 0);
    place_character(0, 6, 7);
    place_character(1, 8, 9);
    state.character_alive[1] = 0;

    ASSERT_EQ_INT(1, u3_autocombat_future_monster_here(&state, 2, 3));
    ASSERT_EQ_INT(0, u3_autocombat_future_monster_here(&state, 4, 5));
    ASSERT_EQ_INT(1, u3_autocombat_future_monster_here(&state, 6, 7));
    ASSERT_EQ_INT(0, u3_autocombat_future_monster_here(&state, 8, 9));
}

static void test_setup_future_moves_monster_toward_nearest_character(void)
{
    reset_state();
    place_character(0, 5, 5);
    place_character(1, 10, 10);
    place_character(2, 10, 10);
    place_character(3, 10, 10);
    place_monster(0, 2, 5, 1);

    u3_autocombat_setup_future(&state);

    ASSERT_EQ_INT(3, state.future_monster_x[0]);
    ASSERT_EQ_INT(5, state.future_monster_y[0]);
}

static void test_setup_future_avoids_character_collision_with_vertical_fallback(void)
{
    reset_state();
    place_character(0, 5, 5);
    place_character(1, 10, 10);
    place_character(2, 10, 10);
    place_character(3, 10, 10);
    place_monster(0, 4, 4, 1);

    u3_autocombat_setup_future(&state);

    ASSERT_EQ_INT(4, state.future_monster_x[0]);
    ASSERT_EQ_INT(5, state.future_monster_y[0]);
}

static void test_setup_future_avoids_monster_collision_with_horizontal_fallback(void)
{
    reset_state();
    place_character(0, 5, 5);
    place_character(1, 10, 10);
    place_character(2, 10, 10);
    place_character(3, 10, 10);
    place_monster(0, 4, 4, 1);
    place_monster(1, 4, 5, 1);

    u3_autocombat_setup_future(&state);

    ASSERT_EQ_INT(5, state.future_monster_x[0]);
    ASSERT_EQ_INT(4, state.future_monster_y[0]);
}

static void test_dir_to_nearest_monster_uses_future_positions(void)
{
    reset_state();
    place_character(0, 5, 5);
    place_monster(0, 5, 2, 1);
    state.future_monster_y[0] = 3;

    ASSERT_EQ_INT('8', u3_autocombat_dir_to_nearest_monster(&state, 0));
}

static void test_line_up_to_monster_left_and_right_half_priority(void)
{
    reset_state();
    place_character(0, 4, 5);
    place_monster(0, 8, 5, 1);
    ASSERT_EQ_INT('6', u3_autocombat_line_up_to_monster(&state, 0));

    reset_state();
    place_character(0, 7, 5);
    place_monster(0, 3, 5, 1);
    ASSERT_EQ_INT('4', u3_autocombat_line_up_to_monster(&state, 0));
}

static void test_line_up_to_monster_uses_nearest_fallback_when_diagonals_disabled(void)
{
    reset_state();
    place_character(0, 4, 4);
    place_monster(0, 7, 7, 1);
    state.allow_diagonal = false;

    ASSERT_EQ_INT('2', u3_autocombat_line_up_to_monster(&state, 0));
}

int main(void)
{
    test_threat_value_counts_live_monsters();
    test_threat_value_doubles_poisonous_types();
    test_monster_can_attack_checks_adjacent_monsters();
    test_monster_can_attack_allows_dragon_diagonal_shots();
    test_monster_can_attack_ignores_dragon_line_for_other_types();
    test_nearly_dead_checks_specific_character_hp_only();
    test_nearly_dead_anybody_checks_dead_or_low_hp_party_members();
    test_monster_nearby_cardinal_priority();
    test_monster_nearby_diagonal_policy_and_priority();
    test_monster_lined_up_returns_shooting_direction_for_non_character();
    test_monster_lined_up_respects_diagonal_policy();
    test_monster_lined_up_moves_character_toward_lined_target();
    test_auto_move_char_direct_move();
    test_auto_move_char_disallows_diagonal_by_preferring_vertical();
    test_auto_move_char_uses_diagonal_fallback_for_blocked_horizontal();
    test_auto_move_char_passes_when_all_fallbacks_blocked();
    test_setup_now_copies_current_monster_positions();
    test_future_monster_here_checks_live_monsters_and_alive_characters();
    test_setup_future_moves_monster_toward_nearest_character();
    test_setup_future_avoids_character_collision_with_vertical_fallback();
    test_setup_future_avoids_monster_collision_with_horizontal_fallback();
    test_dir_to_nearest_monster_uses_future_positions();
    test_line_up_to_monster_left_and_right_half_priority();
    test_line_up_to_monster_uses_nearest_fallback_when_diagonals_disabled();

    puts("autocombat targeting characterization passed");
    return 0;
}
