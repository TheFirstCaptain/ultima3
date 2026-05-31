#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_dungeon.h"

static uint8_t dungeon[U3_DUNGEON_CELL_COUNT + U3_DUNGEON_WIDTH + 1];
static u3_dungeon_state state;

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
    memset(dungeon, 0, sizeof(dungeon));
    state.dungeon = dungeon;
    state.x = 5;
    state.y = 5;
    state.heading = 0;
    state.level = 0;
    state.exit_dungeon = false;
}

static void set_cell(int level, int x, int y, int value)
{
    dungeon[(level * 256) + (y * 16) + x] = (uint8_t)value;
}

static void assert_clean_effects(u3_dungeon_result result, int message_id)
{
    ASSERT_EQ_INT(message_id, result.message_id);
    ASSERT_EQ_INT(0, result.blocked);
    ASSERT_EQ_INT(0, result.invalid);
    ASSERT_EQ_INT(0, result.exited);
    ASSERT_EQ_INT(0, result.effect_message_id);
}

static void test_get_cell_wraps_one_tile_edges_like_legacy(void)
{
    reset_state();
    set_cell(0, 15, 5, 0xAA);
    set_cell(0, 0, 15, 0xBB);
    set_cell(0, 0, 0, 0xCC);

    ASSERT_EQ_INT(0xAA, u3_dungeon_get_cell(&state, -1, 5));
    ASSERT_EQ_INT(0xBB, u3_dungeon_get_cell(&state, 0, -1));
    ASSERT_EQ_INT(0xCC, u3_dungeon_get_cell(&state, 16, 16));
}

static void test_put_cell_writes_current_level_without_wrapping_coordinates(void)
{
    reset_state();
    state.level = 2;

    u3_dungeon_put_cell(&state, 0x7C, 16, 0);

    ASSERT_EQ_INT(0x7C, dungeon[(2 * 256) + 16]);
    ASSERT_EQ_INT(0, dungeon[2 * 256]);
}

static void test_forward_moves_by_heading_and_wraps_north(void)
{
    u3_dungeon_result result;

    reset_state();
    state.x = 3;
    state.y = 0;
    state.heading = 0;

    result = u3_dungeon_forward(&state);

    assert_clean_effects(result, 165);
    ASSERT_EQ_INT(1, result.moved);
    ASSERT_EQ_INT(1, result.needs_redraw);
    ASSERT_EQ_INT(3, state.x);
    ASSERT_EQ_INT(15, state.y);
}

static void test_forward_blocks_exact_wall_tile(void)
{
    u3_dungeon_result result;

    reset_state();
    state.heading = 1;
    set_cell(0, 6, 5, U3_DUNGEON_TILE_WALL);

    result = u3_dungeon_forward(&state);

    ASSERT_EQ_INT(165, result.message_id);
    ASSERT_EQ_INT(1, result.blocked);
    ASSERT_EQ_INT(116, result.effect_message_id);
    ASSERT_EQ_INT(0, result.moved);
    ASSERT_EQ_INT(0, result.needs_redraw);
    ASSERT_EQ_INT(5, state.x);
    ASSERT_EQ_INT(5, state.y);
}

static void test_forward_allows_non_exact_high_tile(void)
{
    u3_dungeon_result result;

    reset_state();
    state.heading = 1;
    set_cell(0, 6, 5, 0x81);

    result = u3_dungeon_forward(&state);

    ASSERT_EQ_INT(0, result.blocked);
    ASSERT_EQ_INT(1, result.moved);
    ASSERT_EQ_INT(6, state.x);
    ASSERT_EQ_INT(5, state.y);
}

static void test_retreat_moves_opposite_heading_and_blocks_wall(void)
{
    u3_dungeon_result result;

    reset_state();
    state.heading = 0;

    result = u3_dungeon_retreat(&state);

    assert_clean_effects(result, 166);
    ASSERT_EQ_INT(1, result.moved);
    ASSERT_EQ_INT(5, state.x);
    ASSERT_EQ_INT(6, state.y);

    reset_state();
    state.heading = 0;
    set_cell(0, 5, 6, U3_DUNGEON_TILE_WALL);

    result = u3_dungeon_retreat(&state);

    ASSERT_EQ_INT(1, result.blocked);
    ASSERT_EQ_INT(116, result.effect_message_id);
    ASSERT_EQ_INT(5, state.x);
    ASSERT_EQ_INT(5, state.y);
}

static void test_turn_right_and_left_wrap_heading(void)
{
    u3_dungeon_result result;

    reset_state();
    state.heading = 3;

    result = u3_dungeon_turn_right(&state);

    assert_clean_effects(result, 167);
    ASSERT_EQ_INT(1, result.turned);
    ASSERT_EQ_INT(1, result.needs_redraw);
    ASSERT_EQ_INT(0, state.heading);

    result = u3_dungeon_turn_left(&state);

    assert_clean_effects(result, 168);
    ASSERT_EQ_INT(1, result.turned);
    ASSERT_EQ_INT(3, state.heading);
}

static void test_turn_blocks_on_current_high_tile(void)
{
    u3_dungeon_result result;

    reset_state();
    set_cell(0, 5, 5, U3_DUNGEON_TILE_TURN_BLOCK);

    result = u3_dungeon_turn_right(&state);

    ASSERT_EQ_INT(167, result.message_id);
    ASSERT_EQ_INT(1, result.blocked);
    ASSERT_EQ_INT(116, result.effect_message_id);
    ASSERT_EQ_INT(0, result.turned);
    ASSERT_EQ_INT(0, result.needs_redraw);
    ASSERT_EQ_INT(0, state.heading);

    result = u3_dungeon_turn_left(&state);

    ASSERT_EQ_INT(168, result.message_id);
    ASSERT_EQ_INT(1, result.blocked);
    ASSERT_EQ_INT(0, state.heading);
}

static void test_descend_requires_non_wall_down_ladder(void)
{
    u3_dungeon_result result;

    reset_state();
    set_cell(0, 5, 5, U3_DUNGEON_TILE_DOWN_LADDER);

    result = u3_dungeon_descend(&state);

    assert_clean_effects(result, 169);
    ASSERT_EQ_INT(1, result.level_changed);
    ASSERT_EQ_INT(1, result.needs_redraw);
    ASSERT_EQ_INT(1, state.level);
}

static void test_descend_rejects_missing_ladder_and_wall_tiles(void)
{
    u3_dungeon_result result;

    reset_state();

    result = u3_dungeon_descend(&state);

    ASSERT_EQ_INT(169, result.message_id);
    ASSERT_EQ_INT(1, result.invalid);
    ASSERT_EQ_INT(171, result.effect_message_id);
    ASSERT_EQ_INT(0, state.level);

    set_cell(0, 5, 5, 0xA0);
    result = u3_dungeon_descend(&state);

    ASSERT_EQ_INT(1, result.invalid);
    ASSERT_EQ_INT(0, state.level);
}

static void test_descend_rejects_bottom_level_ladder_to_preserve_core_bounds(void)
{
    u3_dungeon_result result;

    reset_state();
    state.level = U3_DUNGEON_LEVEL_COUNT - 1;
    set_cell(U3_DUNGEON_LEVEL_COUNT - 1, 5, 5, U3_DUNGEON_TILE_DOWN_LADDER);

    result = u3_dungeon_descend(&state);

    ASSERT_EQ_INT(169, result.message_id);
    ASSERT_EQ_INT(1, result.invalid);
    ASSERT_EQ_INT(171, result.effect_message_id);
    ASSERT_EQ_INT(U3_DUNGEON_LEVEL_COUNT - 1, state.level);
    ASSERT_EQ_INT(0, result.needs_redraw);
}

static void test_climb_requires_up_ladder_and_redraws_within_dungeon(void)
{
    u3_dungeon_result result;

    reset_state();
    state.level = 3;
    set_cell(3, 5, 5, U3_DUNGEON_TILE_UP_LADDER);

    result = u3_dungeon_climb(&state);

    assert_clean_effects(result, 170);
    ASSERT_EQ_INT(1, result.level_changed);
    ASSERT_EQ_INT(1, result.needs_redraw);
    ASSERT_EQ_INT(2, state.level);
    ASSERT_EQ_INT(0, state.exit_dungeon);
}

static void test_climb_from_level_zero_exits_and_resets_level(void)
{
    u3_dungeon_result result;

    reset_state();
    state.level = 0;
    set_cell(0, 5, 5, U3_DUNGEON_TILE_UP_LADDER);

    result = u3_dungeon_climb(&state);

    ASSERT_EQ_INT(170, result.message_id);
    ASSERT_EQ_INT(1, result.level_changed);
    ASSERT_EQ_INT(1, result.exited);
    ASSERT_EQ_INT(0, result.needs_redraw);
    ASSERT_EQ_INT(0, state.level);
    ASSERT_EQ_INT(1, state.exit_dungeon);
}

static void test_climb_rejects_missing_ladder(void)
{
    u3_dungeon_result result;

    reset_state();
    state.level = 2;

    result = u3_dungeon_climb(&state);

    ASSERT_EQ_INT(170, result.message_id);
    ASSERT_EQ_INT(1, result.invalid);
    ASSERT_EQ_INT(171, result.effect_message_id);
    ASSERT_EQ_INT(2, state.level);
}

int main(void)
{
    test_get_cell_wraps_one_tile_edges_like_legacy();
    test_put_cell_writes_current_level_without_wrapping_coordinates();
    test_forward_moves_by_heading_and_wraps_north();
    test_forward_blocks_exact_wall_tile();
    test_forward_allows_non_exact_high_tile();
    test_retreat_moves_opposite_heading_and_blocks_wall();
    test_turn_right_and_left_wrap_heading();
    test_turn_blocks_on_current_high_tile();
    test_descend_requires_non_wall_down_ladder();
    test_descend_rejects_missing_ladder_and_wall_tiles();
    test_descend_rejects_bottom_level_ladder_to_preserve_core_bounds();
    test_climb_requires_up_ladder_and_redraws_within_dungeon();
    test_climb_from_level_zero_exits_and_resets_level();
    test_climb_rejects_missing_ladder();

    puts("dungeon navigation characterization passed");
    return 0;
}
