#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "u3_overworld.h"

#define ASSERT_EQ_INT(expected, actual) assert_eq_int((expected), (actual), __FILE__, __LINE__)
#define ASSERT_TRUE(actual) assert_true((actual), __FILE__, __LINE__)
#define ASSERT_FALSE(actual) assert_false((actual), __FILE__, __LINE__)

static void assert_eq_int(int expected, int actual, const char *file, int line)
{
    if (expected != actual) {
        fprintf(stderr, "%s:%d: expected %d, got %d\n", file, line, expected, actual);
        exit(1);
    }
}

static void assert_true(int actual, const char *file, int line)
{
    if (!actual) {
        fprintf(stderr, "%s:%d: expected true\n", file, line);
        exit(1);
    }
}

static void assert_false(int actual, const char *file, int line)
{
    if (actual) {
        fprintf(stderr, "%s:%d: expected false\n", file, line);
        exit(1);
    }
}

static void test_initializes_bounded_position(void)
{
    u3_overworld_state state;

    ASSERT_TRUE(u3_overworld_state_init(&state, 5, 6, 11, 11));
    ASSERT_EQ_INT(5, state.x);
    ASSERT_EQ_INT(6, state.y);
    ASSERT_EQ_INT(11, state.width);
    ASSERT_EQ_INT(11, state.height);

    ASSERT_TRUE(u3_overworld_state_init(&state, 20, 21, 11, 11));
    ASSERT_EQ_INT(10, state.x);
    ASSERT_EQ_INT(10, state.y);
}

static void test_moves_cardinally_with_bounds(void)
{
    u3_overworld_state state;
    u3_overworld_move_result result;

    ASSERT_TRUE(u3_overworld_state_init(&state, 5, 5, 11, 11));

    ASSERT_TRUE(u3_overworld_move(&state, U3_OVERWORLD_COMMAND_NORTH, &result));
    ASSERT_TRUE(result.handled);
    ASSERT_TRUE(result.moved);
    ASSERT_EQ_INT(5, state.x);
    ASSERT_EQ_INT(4, state.y);

    ASSERT_TRUE(u3_overworld_move(&state, U3_OVERWORLD_COMMAND_EAST, &result));
    ASSERT_TRUE(result.moved);
    ASSERT_EQ_INT(6, state.x);
    ASSERT_EQ_INT(4, state.y);

    ASSERT_TRUE(u3_overworld_move(&state, U3_OVERWORLD_COMMAND_SOUTH, &result));
    ASSERT_TRUE(result.moved);
    ASSERT_EQ_INT(6, state.x);
    ASSERT_EQ_INT(5, state.y);

    ASSERT_TRUE(u3_overworld_move(&state, U3_OVERWORLD_COMMAND_WEST, &result));
    ASSERT_TRUE(result.moved);
    ASSERT_EQ_INT(5, state.x);
    ASSERT_EQ_INT(5, state.y);
}

static void test_initializes_wrapped_position_from_party_record(void)
{
    uint8_t party[64] = {0};
    u3_overworld_state state;

    party[U3_OVERWORLD_PARTY_X_OFFSET] = 62;
    party[U3_OVERWORLD_PARTY_Y_OFFSET] = 63;

    ASSERT_TRUE(u3_overworld_state_init_from_party(&state, party, sizeof(party), 64));
    ASSERT_EQ_INT(62, state.x);
    ASSERT_EQ_INT(63, state.y);
    ASSERT_EQ_INT(64, state.width);
    ASSERT_EQ_INT(64, state.height);
    ASSERT_TRUE(state.wraps);

    party[U3_OVERWORLD_PARTY_X_OFFSET] = 64;
    party[U3_OVERWORLD_PARTY_Y_OFFSET] = 65;
    ASSERT_TRUE(u3_overworld_state_init_from_party(&state, party, sizeof(party), 64));
    ASSERT_EQ_INT(0, state.x);
    ASSERT_EQ_INT(1, state.y);
}

static void test_wrapped_movement_updates_party_record(void)
{
    uint8_t party[64] = {0};
    u3_overworld_state state;
    u3_overworld_move_result result;

    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 0, 0, 64, 64));
    ASSERT_TRUE(u3_overworld_move(&state, U3_OVERWORLD_COMMAND_WEST, &result));
    ASSERT_TRUE(result.moved);
    ASSERT_EQ_INT(63, result.x);
    ASSERT_EQ_INT(0, result.y);

    ASSERT_TRUE(u3_overworld_move(&state, U3_OVERWORLD_COMMAND_NORTH, &result));
    ASSERT_TRUE(result.moved);
    ASSERT_EQ_INT(63, result.x);
    ASSERT_EQ_INT(63, result.y);

    ASSERT_TRUE(u3_overworld_write_party_position(party, sizeof(party), &state));
    ASSERT_EQ_INT(63, party[U3_OVERWORLD_PARTY_X_OFFSET]);
    ASSERT_EQ_INT(63, party[U3_OVERWORLD_PARTY_Y_OFFSET]);
}

static void test_reports_blocked_edges_and_unknown_commands(void)
{
    u3_overworld_state state;
    u3_overworld_move_result result;

    ASSERT_TRUE(u3_overworld_state_init(&state, 0, 0, 11, 11));

    ASSERT_TRUE(u3_overworld_move(&state, U3_OVERWORLD_COMMAND_NORTH, &result));
    ASSERT_TRUE(result.handled);
    ASSERT_FALSE(result.moved);
    ASSERT_TRUE(result.blocked);
    ASSERT_EQ_INT(0, result.x);
    ASSERT_EQ_INT(0, result.y);

    ASSERT_TRUE(u3_overworld_move(&state, U3_OVERWORLD_COMMAND_WEST, &result));
    ASSERT_TRUE(result.handled);
    ASSERT_FALSE(result.moved);
    ASSERT_EQ_INT(0, state.x);
    ASSERT_EQ_INT(0, state.y);

    ASSERT_TRUE(u3_overworld_move(&state, 999, &result));
    ASSERT_FALSE(result.handled);
    ASSERT_FALSE(result.moved);
}

static void test_builds_centered_view_frame_from_party_position(void)
{
    uint8_t map_record[1 + (64 * 64)];
    u3_overworld_state state;
    u3_render_frame frame;
    uint16_t index;
    uint16_t party_command_index;

    map_record[0] = 64;
    for (index = 0; index < 64 * 64; index++)
        map_record[1 + index] = (uint8_t)(index & 0xff);

    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 10, 12, 64, 64));
    frame = u3_overworld_make_view_frame(map_record, sizeof(map_record), &state);

    ASSERT_EQ_INT(U3_RENDER_TILE_COUNT + 2, frame.command_count);
    ASSERT_EQ_INT(map_record[1 + (7 * 64) + 5], frame.commands[2].value);
    party_command_index = (uint16_t)(2 + (5 * U3_RENDER_TILE_GRID_WIDTH) + 5);
    ASSERT_EQ_INT(U3_OVERWORLD_PARTY_TILE, frame.commands[party_command_index].value);
    ASSERT_EQ_INT(6, frame.commands[party_command_index].x);
    ASSERT_EQ_INT(6, frame.commands[party_command_index].y);
}

static void test_passability_matches_first_non_ship_terrain_slice(void)
{
    ASSERT_FALSE(u3_overworld_tile_passable_on_foot(0));
    ASSERT_FALSE(u3_overworld_tile_passable_on_foot(16));
    ASSERT_FALSE(u3_overworld_tile_passable_on_foot(48));
    ASSERT_TRUE(u3_overworld_tile_passable_on_foot(4));
    ASSERT_TRUE(u3_overworld_tile_passable_on_foot(44));
    ASSERT_TRUE(u3_overworld_tile_passable_on_foot(128));
    ASSERT_TRUE(u3_overworld_tile_passable_on_foot(132));
    ASSERT_TRUE(u3_overworld_tile_passable_on_foot(136));
    ASSERT_TRUE(u3_overworld_tile_passable_on_foot(248));
}

static void test_map_aware_movement_blocks_impassable_target_tile(void)
{
    uint8_t map_record[1 + (64 * 64)] = {0};
    u3_overworld_state state;
    u3_overworld_move_result result;

    map_record[0] = 64;
    map_record[1 + (20 * 64) + 42] = 4;
    map_record[1 + (20 * 64) + 43] = 0;

    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 42, 20, 64, 64));
    ASSERT_TRUE(u3_overworld_move_on_map(&state, map_record, sizeof(map_record), U3_OVERWORLD_COMMAND_EAST, &result));
    ASSERT_TRUE(result.handled);
    ASSERT_FALSE(result.moved);
    ASSERT_TRUE(result.blocked);
    ASSERT_EQ_INT(42, result.x);
    ASSERT_EQ_INT(20, result.y);
    ASSERT_EQ_INT(43, result.target_x);
    ASSERT_EQ_INT(20, result.target_y);
    ASSERT_EQ_INT(0, result.target_tile);
    ASSERT_EQ_INT(42, state.x);
    ASSERT_EQ_INT(20, state.y);
}

static void test_map_aware_movement_moves_onto_passable_target_tile(void)
{
    uint8_t map_record[1 + (64 * 64)] = {0};
    u3_overworld_state state;
    u3_overworld_move_result result;

    map_record[0] = 64;
    map_record[1 + (20 * 64) + 42] = 4;
    map_record[1 + (20 * 64) + 43] = 4;

    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 42, 20, 64, 64));
    ASSERT_TRUE(u3_overworld_move_on_map(&state, map_record, sizeof(map_record), U3_OVERWORLD_COMMAND_EAST, &result));
    ASSERT_TRUE(result.handled);
    ASSERT_TRUE(result.moved);
    ASSERT_FALSE(result.blocked);
    ASSERT_EQ_INT(43, result.x);
    ASSERT_EQ_INT(20, result.y);
    ASSERT_EQ_INT(43, result.target_x);
    ASSERT_EQ_INT(20, result.target_y);
    ASSERT_EQ_INT(4, result.target_tile);
    ASSERT_EQ_INT(43, state.x);
    ASSERT_EQ_INT(20, state.y);
}

static void test_map_aware_movement_wraps_before_passability_check(void)
{
    uint8_t map_record[1 + (64 * 64)] = {0};
    u3_overworld_state state;
    u3_overworld_move_result result;

    map_record[0] = 64;
    map_record[1 + (20 * 64) + 63] = 4;
    map_record[1 + (20 * 64) + 0] = 16;

    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 63, 20, 64, 64));
    ASSERT_TRUE(u3_overworld_move_on_map(&state, map_record, sizeof(map_record), U3_OVERWORLD_COMMAND_EAST, &result));
    ASSERT_TRUE(result.handled);
    ASSERT_FALSE(result.moved);
    ASSERT_TRUE(result.blocked);
    ASSERT_EQ_INT(0, result.target_x);
    ASSERT_EQ_INT(20, result.target_y);
    ASSERT_EQ_INT(16, result.target_tile);
    ASSERT_EQ_INT(63, state.x);
    ASSERT_EQ_INT(20, state.y);
}

static void test_map_aware_movement_rejects_state_map_size_mismatch(void)
{
    uint8_t map_record[1 + (64 * 64)] = {0};
    u3_overworld_state state;
    u3_overworld_move_result result;

    map_record[0] = 64;
    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 42, 20, 65, 65));

    ASSERT_FALSE(u3_overworld_move_on_map(&state, map_record, sizeof(map_record), U3_OVERWORLD_COMMAND_EAST, &result));
}

static void test_map_aware_bounded_edge_blocks_without_moving(void)
{
    uint8_t map_record[1 + (11 * 11)] = {0};
    u3_overworld_state state;
    u3_overworld_move_result result;

    map_record[0] = 11;
    map_record[1] = 4;

    ASSERT_TRUE(u3_overworld_state_init(&state, 0, 0, 11, 11));
    ASSERT_TRUE(u3_overworld_move_on_map(&state, map_record, sizeof(map_record), U3_OVERWORLD_COMMAND_NORTH, &result));
    ASSERT_TRUE(result.handled);
    ASSERT_FALSE(result.moved);
    ASSERT_TRUE(result.blocked);
    ASSERT_EQ_INT(0, result.x);
    ASSERT_EQ_INT(0, result.y);
    ASSERT_EQ_INT(0, result.target_x);
    ASSERT_EQ_INT(0, result.target_y);
    ASSERT_EQ_INT(4, result.target_tile);
}

static void test_centered_view_frame_wraps_at_map_edges(void)
{
    uint8_t map_record[1 + (64 * 64)];
    u3_overworld_state state;
    u3_render_frame frame;
    uint16_t index;

    map_record[0] = 64;
    for (index = 0; index < 64 * 64; index++)
        map_record[1 + index] = (uint8_t)(index & 0xff);

    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 0, 0, 64, 64));
    frame = u3_overworld_make_view_frame(map_record, sizeof(map_record), &state);

    ASSERT_EQ_INT(map_record[1 + (59 * 64) + 59], frame.commands[2].value);
}

static void test_builds_smoke_frame_from_known_map_and_party_position(void)
{
    uint8_t map_record[1 + (11 * 11)];
    u3_overworld_state state;
    u3_render_frame frame;
    uint16_t index;
    uint16_t party_command_index;

    map_record[0] = 11;
    for (index = 0; index < 11 * 11; index++)
        map_record[1 + index] = (uint8_t)(0x20 + index);

    ASSERT_TRUE(u3_overworld_state_init(&state, 3, 4, 11, 11));
    frame = u3_overworld_make_smoke_frame(map_record, sizeof(map_record), &state);

    ASSERT_EQ_INT(U3_RENDER_TILE_COUNT + 2, frame.command_count);
    ASSERT_EQ_INT(0x20, frame.commands[2].value);
    party_command_index = (uint16_t)(2 + (4 * U3_RENDER_TILE_GRID_WIDTH) + 3);
    ASSERT_EQ_INT(U3_OVERWORLD_PARTY_TILE, frame.commands[party_command_index].value);
    ASSERT_EQ_INT(4, frame.commands[party_command_index].x);
    ASSERT_EQ_INT(5, frame.commands[party_command_index].y);
}

static void test_invalid_map_falls_back_to_synthetic_frame(void)
{
    uint8_t map_record[2] = {11, 0};
    u3_overworld_state state;
    u3_render_frame frame;

    ASSERT_TRUE(u3_overworld_state_init(&state, 3, 4, 11, 11));
    frame = u3_overworld_make_smoke_frame(map_record, sizeof(map_record), &state);

    ASSERT_EQ_INT(U3_RENDER_TILE_COUNT + 2, frame.command_count);
    ASSERT_EQ_INT(1, frame.commands[2].x);
    ASSERT_EQ_INT(1, frame.commands[2].y);
}

int main(void)
{
    test_initializes_bounded_position();
    test_moves_cardinally_with_bounds();
    test_initializes_wrapped_position_from_party_record();
    test_wrapped_movement_updates_party_record();
    test_reports_blocked_edges_and_unknown_commands();
    test_passability_matches_first_non_ship_terrain_slice();
    test_map_aware_movement_blocks_impassable_target_tile();
    test_map_aware_movement_moves_onto_passable_target_tile();
    test_map_aware_movement_wraps_before_passability_check();
    test_map_aware_movement_rejects_state_map_size_mismatch();
    test_map_aware_bounded_edge_blocks_without_moving();
    test_builds_centered_view_frame_from_party_position();
    test_centered_view_frame_wraps_at_map_edges();
    test_builds_smoke_frame_from_known_map_and_party_position();
    test_invalid_map_falls_back_to_synthetic_frame();

    printf("overworld movement smoke passed\n");
    return 0;
}
