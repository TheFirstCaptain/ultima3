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
}

static void test_land_monster_valid_move_tiles(void)
{
    reset_state();
    state.monster_type = 0x10;

    ASSERT_EQ_INT(0, u3_combat_valid_move(&state, 2));
    ASSERT_EQ_INT(0, u3_combat_valid_move(&state, 4));
    ASSERT_EQ_INT(0, u3_combat_valid_move(&state, 6));
    ASSERT_EQ_INT(0, u3_combat_valid_move(&state, 0x10));
    ASSERT_EQ_INT(0xFF, u3_combat_valid_move(&state, 0));
    ASSERT_EQ_INT(0xFF, u3_combat_valid_move(&state, 3));
}

static void test_high_monster_type_uses_land_rules(void)
{
    reset_state();
    state.monster_type = 0x20;

    ASSERT_EQ_INT(0, u3_combat_valid_move(&state, 2));
    ASSERT_EQ_INT(0xFF, u3_combat_valid_move(&state, 0));
}

static void test_special_monster_type_range_only_accepts_zero_tile(void)
{
    short monType;

    for (monType = 0x16; monType < 0x20; monType++) {
        reset_state();
        state.monster_type = monType;

        ASSERT_EQ_INT(0, u3_combat_valid_move(&state, 0));
        ASSERT_EQ_INT(0xFF, u3_combat_valid_move(&state, 2));
        ASSERT_EQ_INT(0xFF, u3_combat_valid_move(&state, 4));
        ASSERT_EQ_INT(0xFF, u3_combat_valid_move(&state, 6));
        ASSERT_EQ_INT(0xFF, u3_combat_valid_move(&state, 0x10));
    }
}

static void test_combat_monster_here_ignores_dead_slots(void)
{
    reset_state();
    state.monster_x[4] = 5;
    state.monster_y[4] = 6;
    state.monster_hp[4] = 0;

    ASSERT_EQ_INT(255, u3_combat_monster_here(&state, 5, 6));
}

static void test_combat_monster_here_returns_lowest_matching_live_slot(void)
{
    reset_state();
    state.monster_x[2] = 7;
    state.monster_y[2] = 8;
    state.monster_hp[2] = 1;
    state.monster_x[6] = 7;
    state.monster_y[6] = 8;
    state.monster_hp[6] = 99;

    ASSERT_EQ_INT(2, u3_combat_monster_here(&state, 7, 8));
}

static void test_combat_monster_here_returns_255_when_missing(void)
{
    reset_state();
    state.monster_x[1] = 1;
    state.monster_y[1] = 2;
    state.monster_hp[1] = 3;

    ASSERT_EQ_INT(255, u3_combat_monster_here(&state, 2, 1));
}

static void test_combat_character_here_returns_highest_matching_slot(void)
{
    reset_state();
    state.character_x[0] = 4;
    state.character_y[0] = 9;
    state.character_x[3] = 4;
    state.character_y[3] = 9;

    ASSERT_EQ_INT(3, u3_combat_character_here(&state, 4, 9));
}

static void test_combat_character_here_does_not_check_health_or_activity(void)
{
    reset_state();
    state.character_x[2] = 0xFF;
    state.character_y[2] = 0xFF;

    ASSERT_EQ_INT(2, u3_combat_character_here(&state, 0xFF, 0xFF));
}

static void test_combat_character_here_returns_255_when_missing(void)
{
    reset_state();
    state.character_x[0] = 1;
    state.character_y[0] = 1;

    ASSERT_EQ_INT(255, u3_combat_character_here(&state, 1, 2));
}

int main(void)
{
    test_land_monster_valid_move_tiles();
    test_high_monster_type_uses_land_rules();
    test_special_monster_type_range_only_accepts_zero_tile();
    test_combat_monster_here_ignores_dead_slots();
    test_combat_monster_here_returns_lowest_matching_live_slot();
    test_combat_monster_here_returns_255_when_missing();
    test_combat_character_here_returns_highest_matching_slot();
    test_combat_character_here_does_not_check_health_or_activity();
    test_combat_character_here_returns_255_when_missing();

    puts("combat predicate characterization passed");
    return 0;
}
