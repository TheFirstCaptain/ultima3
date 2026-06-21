#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_location.h"
#include "u3_resource.h"

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

static u3_location_session make_session(uint8_t x, uint8_t y, uint16_t map_size)
{
    u3_location_session session;

    memset(&session, 0, sizeof(session));
    session.active = 1;
    session.destination_kind = U3_LOCATION_KIND_TOWN;
    session.map_shape = U3_LOCATION_MAP_SHAPE_TWO_DIMENSIONAL;
    session.map_size = map_size;
    session.map_length = 1u + ((uint32_t)map_size * map_size);
    session.x = x;
    session.y = y;
    return session;
}

static void fill_map(uint8_t *map, uint8_t map_size, uint8_t tile)
{
    map[0] = map_size;
    memset(map + 1, tile, (size_t)map_size * map_size);
}

static void test_moves_cardinally_without_wrapping(void)
{
    uint8_t map[65];
    u3_location_session session = make_session(3, 3, 8);
    u3_location_move_result result;

    fill_map(map, 8, 4);
    ASSERT_TRUE(u3_location_move(&session, map, sizeof(map), U3_OVERWORLD_COMMAND_NORTH, &result));
    ASSERT_TRUE(result.moved);
    ASSERT_EQ_INT(3, session.x);
    ASSERT_EQ_INT(2, session.y);
    ASSERT_EQ_INT(U3_LOCATION_MOVE_STATUS_MOVED, result.status);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_STEP, result.sound_id);

    ASSERT_TRUE(u3_location_move(&session, map, sizeof(map), U3_OVERWORLD_COMMAND_EAST, &result));
    ASSERT_EQ_INT(4, session.x);
    ASSERT_EQ_INT(2, session.y);
    ASSERT_TRUE(u3_location_move(&session, map, sizeof(map), U3_OVERWORLD_COMMAND_SOUTH, &result));
    ASSERT_EQ_INT(4, session.x);
    ASSERT_EQ_INT(3, session.y);
    ASSERT_TRUE(u3_location_move(&session, map, sizeof(map), U3_OVERWORLD_COMMAND_WEST, &result));
    ASSERT_EQ_INT(3, session.x);
    ASSERT_EQ_INT(3, session.y);

    session.x = 7;
    ASSERT_TRUE(u3_location_move(&session, map, sizeof(map), U3_OVERWORLD_COMMAND_EAST, &result));
    ASSERT_TRUE(result.blocked);
    ASSERT_FALSE(result.moved);
    ASSERT_EQ_INT(7, session.x);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_BUMP, result.sound_id);
}

static void test_blocks_impassable_tiles_and_ignores_unknown_commands(void)
{
    uint8_t map[65];
    u3_location_session session = make_session(3, 3, 8);
    u3_location_move_result result;

    fill_map(map, 8, 4);
    map[1 + (3 * 8) + 4] = 48;
    ASSERT_TRUE(u3_location_move(&session, map, sizeof(map), U3_OVERWORLD_COMMAND_EAST, &result));
    ASSERT_TRUE(result.handled);
    ASSERT_TRUE(result.blocked);
    ASSERT_EQ_INT(48, result.target_tile);
    ASSERT_EQ_INT(3, session.x);
    ASSERT_EQ_INT(U3_LOCATION_MOVE_STATUS_BLOCKED, result.status);

    ASSERT_TRUE(u3_location_move(&session, map, sizeof(map), 999, &result));
    ASSERT_FALSE(result.handled);
    ASSERT_FALSE(result.moved);
}

static void test_requests_exit_on_legacy_zero_coordinate_boundary(void)
{
    uint8_t map[65];
    u3_location_session session = make_session(1, 3, 8);
    u3_location_move_result result;

    fill_map(map, 8, 4);
    ASSERT_TRUE(u3_location_move(&session, map, sizeof(map), U3_OVERWORLD_COMMAND_WEST, &result));
    ASSERT_TRUE(result.moved);
    ASSERT_TRUE(result.exit_requested);
    ASSERT_EQ_INT(0, session.x);
    ASSERT_EQ_INT(3, session.y);
    ASSERT_EQ_INT(U3_LOCATION_MOVE_STATUS_EXIT_REQUESTED, result.status);
}

static void test_passability_matches_legacy_location_rule(void)
{
    ASSERT_FALSE(u3_location_tile_passable(0));
    ASSERT_FALSE(u3_location_tile_passable(16));
    ASSERT_FALSE(u3_location_tile_passable(48));
    ASSERT_TRUE(u3_location_tile_passable(4));
    ASSERT_TRUE(u3_location_tile_passable(32));
    ASSERT_TRUE(u3_location_tile_passable(128));
    ASSERT_TRUE(u3_location_tile_passable(132));
    ASSERT_TRUE(u3_location_tile_passable(136));
    ASSERT_TRUE(u3_location_tile_passable(248));
}

static void test_applies_shared_turn_without_writing_sosaria_position(void)
{
    uint8_t party[64] = {0};
    u3_location_move_result result;

    memset(&result, 0, sizeof(result));
    party[U3_OVERWORLD_PARTY_SIZE_OFFSET] = 4;
    party[U3_OVERWORLD_PARTY_X_OFFSET] = 46;
    party[U3_OVERWORLD_PARTY_Y_OFFSET] = 19;
    result.handled = 1;
    ASSERT_TRUE(u3_location_apply_party_turn(party, sizeof(party), &result));
    ASSERT_TRUE(result.turn_applied);
    ASSERT_EQ_INT(4, result.turn_delta);
    ASSERT_EQ_INT(4, (int)result.move_counter_after);
    ASSERT_EQ_INT(46, party[U3_OVERWORLD_PARTY_X_OFFSET]);
    ASSERT_EQ_INT(19, party[U3_OVERWORLD_PARTY_Y_OFFSET]);
}

static void test_builds_non_wrapping_centered_frame(void)
{
    uint8_t map[65];
    u3_location_session session = make_session(1, 3, 8);
    u3_render_frame frame;
    uint16_t party_index = (uint16_t)(2 +
        (U3_RENDER_TILE_GRID_HEIGHT / 2 * U3_RENDER_TILE_GRID_WIDTH) +
        (U3_RENDER_TILE_GRID_WIDTH / 2));

    fill_map(map, 8, 32);
    map[1 + (3 * 8)] = 12;
    frame = u3_location_make_view_frame(&session, map, sizeof(map));

    ASSERT_EQ_INT(U3_RENDER_TILE_COUNT + 2, frame.command_count);
    ASSERT_EQ_INT(4, frame.commands[2].value);
    ASSERT_EQ_INT(12, frame.commands[2 + (5 * U3_RENDER_TILE_GRID_WIDTH) + 4].value);
    ASSERT_EQ_INT(U3_OVERWORLD_PARTY_TILE, frame.commands[party_index].value);
}

int main(void)
{
    test_moves_cardinally_without_wrapping();
    test_blocks_impassable_tiles_and_ignores_unknown_commands();
    test_requests_exit_on_legacy_zero_coordinate_boundary();
    test_passability_matches_legacy_location_rule();
    test_applies_shared_turn_without_writing_sosaria_position();
    test_builds_non_wrapping_centered_frame();

    printf("location navigation tests passed\n");
    return 0;
}
