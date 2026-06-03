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
}

static void test_damage_subtracts_hp_without_side_effect_events(void)
{
    u3_combat_damage_result result;

    reset_state();
    state.monster_type = 0x10;
    state.monster_hp[2] = 12;

    result = u3_combat_damage_monster(&state, experience, 2, 5, 3);

    ASSERT_EQ_INT(7, state.monster_hp[2]);
    ASSERT_EQ_INT(1, result.damaged);
    ASSERT_EQ_INT(0, result.defeated);
    ASSERT_EQ_INT(0, result.award_experience);
    ASSERT_EQ_INT(0, result.restore_tile);
    ASSERT_EQ_INT(3, result.character);
}

static void test_exact_damage_defeats_monster_and_restores_tile(void)
{
    u3_combat_damage_result result;

    reset_state();
    state.monster_type = 0x10;
    state.monster_x[4] = 6;
    state.monster_y[4] = 3;
    state.monster_tile[4] = 0x2A;
    state.monster_hp[4] = 8;
    state.tile_array[(3 * 11) + 6] = 0x10;
    experience[(0x10 / 2) & 0x0F] = 25;

    result = u3_combat_damage_monster(&state, experience, 4, 8, 1);

    ASSERT_EQ_INT(0, state.monster_hp[4]);
    ASSERT_EQ_INT(0x2A, state.tile_array[(3 * 11) + 6]);
    ASSERT_EQ_INT(1, result.defeated);
    ASSERT_EQ_INT(1, result.award_experience);
    ASSERT_EQ_INT(1, result.redraw_tiles);
    ASSERT_EQ_INT(1, result.restore_tile);
    ASSERT_EQ_INT(U3_COMBAT_MONSTER_DEFEATED_MESSAGE, result.message_id);
    ASSERT_EQ_INT(25, result.experience_awarded);
    ASSERT_EQ_INT(6, result.monster_x);
    ASSERT_EQ_INT(3, result.monster_y);
    ASSERT_EQ_INT(0x2A, result.restored_tile);
    ASSERT_EQ_INT(1, result.character);
}

static void test_overkill_defeats_monster(void)
{
    u3_combat_damage_result result;

    reset_state();
    state.monster_type = 0x1E;
    state.monster_x[0] = 10;
    state.monster_y[0] = 10;
    state.monster_tile[0] = 0x04;
    state.monster_hp[0] = 1;
    state.tile_array[(10 * 11) + 10] = 0x1E;
    experience[(0x1E / 2) & 0x0F] = 99;

    result = u3_combat_damage_monster(&state, experience, 0, 255, 4);

    ASSERT_EQ_INT(0, state.monster_hp[0]);
    ASSERT_EQ_INT(0x04, state.tile_array[(10 * 11) + 10]);
    ASSERT_EQ_INT(1, result.defeated);
    ASSERT_EQ_INT(99, result.experience_awarded);
}

static void test_defeat_with_out_of_bounds_coordinates_does_not_restore_tile(void)
{
    u3_combat_damage_result result;
    int tile;

    reset_state();
    memset(state.tile_array, 0xEE, sizeof(state.tile_array));
    state.monster_type = 0x12;
    state.monster_x[3] = 0xFF;
    state.monster_y[3] = 4;
    state.monster_tile[3] = 0x09;
    state.monster_hp[3] = 2;
    experience[(0x12 / 2) & 0x0F] = 30;

    result = u3_combat_damage_monster(&state, experience, 3, 2, 1);

    ASSERT_EQ_INT(0, state.monster_hp[3]);
    for (tile = 0; tile < U3_COMBAT_TILE_COUNT; tile++)
        ASSERT_EQ_INT(0xEE, state.tile_array[tile]);
    ASSERT_EQ_INT(1, result.defeated);
    ASSERT_EQ_INT(1, result.restore_tile);
    ASSERT_EQ_INT(0xFF, result.monster_x);
    ASSERT_EQ_INT(4, result.monster_y);
    ASSERT_EQ_INT(0x09, result.restored_tile);
}

static void test_lord_british_takes_no_damage_and_emits_no_events(void)
{
    u3_combat_damage_result result;

    reset_state();
    state.monster_type = U3_COMBAT_LORD_BRITISH_TYPE;
    state.monster_x[1] = 2;
    state.monster_y[1] = 5;
    state.monster_tile[1] = 0x07;
    state.monster_hp[1] = 4;
    state.tile_array[(5 * 11) + 2] = U3_COMBAT_LORD_BRITISH_TYPE;
    experience[(U3_COMBAT_LORD_BRITISH_TYPE / 2) & 0x0F] = 42;

    result = u3_combat_damage_monster(&state, experience, 1, 99, 2);

    ASSERT_EQ_INT(4, state.monster_hp[1]);
    ASSERT_EQ_INT(U3_COMBAT_LORD_BRITISH_TYPE, state.tile_array[(5 * 11) + 2]);
    ASSERT_EQ_INT(1, result.ignored_lord_british);
    ASSERT_EQ_INT(0, result.damaged);
    ASSERT_EQ_INT(0, result.defeated);
    ASSERT_EQ_INT(0, result.award_experience);
    ASSERT_EQ_INT(0, result.restore_tile);
}

int main(void)
{
    test_damage_subtracts_hp_without_side_effect_events();
    test_exact_damage_defeats_monster_and_restores_tile();
    test_overkill_defeats_monster();
    test_defeat_with_out_of_bounds_coordinates_does_not_restore_tile();
    test_lord_british_takes_no_damage_and_emits_no_events();

    puts("combat damage characterization passed");
    return 0;
}
