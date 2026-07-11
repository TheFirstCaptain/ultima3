#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_dungeon.h"

#define ASSERT_EQ_INT(expected, actual) assert_eq_int((expected), (actual), __FILE__, __LINE__)
#define ASSERT_TRUE(actual) assert_true((actual), __FILE__, __LINE__)
#define ASSERT_FALSE(actual) assert_false((actual), __FILE__, __LINE__)

static uint8_t dungeon[U3_DUNGEON_CELL_COUNT];

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

static uint16_t dungeon_offset(int level, int x, int y)
{
    x &= 0x0f;
    y &= 0x0f;
    return (uint16_t)((level * U3_DUNGEON_WIDTH * U3_DUNGEON_HEIGHT) +
                      (y * U3_DUNGEON_WIDTH) +
                      x);
}

static void reset_dungeon(void)
{
    memset(dungeon, 0, sizeof(dungeon));
}

static void put_cell(int level, int x, int y, uint8_t value)
{
    dungeon[dungeon_offset(level, x, y)] = value;
}

static int count_commands_with_value(const u3_render_frame *frame, uint16_t value)
{
    uint16_t index;
    int count = 0;

    for (index = 0; index < frame->command_count; index++) {
        if (frame->commands[index].value == value)
            count++;
    }
    return count;
}

static const u3_render_command *first_command_with_value(const u3_render_frame *frame, uint16_t value)
{
    uint16_t index;

    for (index = 0; index < frame->command_count; index++) {
        if (frame->commands[index].value == value)
            return &frame->commands[index];
    }
    return 0;
}

static void test_builds_corridor_with_side_walls_and_front_door(void)
{
    u3_render_frame frame;
    const u3_render_command *door;

    reset_dungeon();
    put_cell(0, 1, 0, U3_DUNGEON_TILE_WALL);
    put_cell(0, 2, 0, U3_DUNGEON_TILE_WALL);
    put_cell(0, 3, 1, 0xC0);

    frame = u3_dungeon_make_view_frame(dungeon, sizeof(dungeon), 0, 1, 1, 1);

    ASSERT_EQ_INT(U3_RENDER_LOGICAL_WIDTH, frame.logical_width);
    ASSERT_EQ_INT(U3_RENDER_LOGICAL_HEIGHT, frame.logical_height);
    ASSERT_FALSE(frame.overflowed);
    ASSERT_TRUE(frame.command_count > 2);
    ASSERT_EQ_INT(U3_RENDER_COMMAND_CLEAR, frame.commands[0].kind);
    ASSERT_TRUE(count_commands_with_value(&frame, U3_DUNGEON_RENDER_VALUE_WALL) >= 2);
    ASSERT_EQ_INT(1, count_commands_with_value(&frame, U3_DUNGEON_RENDER_VALUE_DOOR));
    door = first_command_with_value(&frame, U3_DUNGEON_RENDER_VALUE_DOOR);
    ASSERT_TRUE(door != 0);
    ASSERT_EQ_INT(12, door->x);
    ASSERT_EQ_INT(8, door->y);
    ASSERT_EQ_INT(16, door->width);
    ASSERT_EQ_INT(8, door->height);
}

static void test_heading_rotation_changes_sampled_cells(void)
{
    u3_render_frame east_frame;
    u3_render_frame south_frame;

    reset_dungeon();
    put_cell(0, 2, 1, U3_DUNGEON_TILE_WALL);
    put_cell(0, 1, 3, 0xC0);

    east_frame = u3_dungeon_make_view_frame(dungeon, sizeof(dungeon), 0, 1, 1, 1);
    south_frame = u3_dungeon_make_view_frame(dungeon, sizeof(dungeon), 0, 1, 1, 2);

    ASSERT_TRUE(count_commands_with_value(&east_frame, U3_DUNGEON_RENDER_VALUE_WALL) >= 1);
    ASSERT_EQ_INT(0, count_commands_with_value(&east_frame, U3_DUNGEON_RENDER_VALUE_DOOR));
    ASSERT_EQ_INT(1, count_commands_with_value(&south_frame, U3_DUNGEON_RENDER_VALUE_DOOR));
}

static void test_current_cell_features_are_distinguishable(void)
{
    u3_render_frame frame;

    reset_dungeon();
    put_cell(0, 1, 1, U3_DUNGEON_TILE_UP_LADDER | U3_DUNGEON_TILE_DOWN_LADDER | U3_DUNGEON_TILE_CHEST);

    frame = u3_dungeon_make_view_frame(dungeon, sizeof(dungeon), 0, 1, 1, 1);

    ASSERT_EQ_INT(1, count_commands_with_value(&frame, U3_DUNGEON_RENDER_VALUE_UP_LADDER));
    ASSERT_EQ_INT(1, count_commands_with_value(&frame, U3_DUNGEON_RENDER_VALUE_DOWN_LADDER));
    ASSERT_EQ_INT(1, count_commands_with_value(&frame, U3_DUNGEON_RENDER_VALUE_CHEST));
}

static void test_visible_non_current_features_are_distinguishable(void)
{
    u3_render_frame frame;

    reset_dungeon();
    put_cell(0, 3, 1, U3_DUNGEON_TILE_UP_LADDER);
    put_cell(0, 2, 1, U3_DUNGEON_TILE_DOWN_LADDER);
    put_cell(0, 3, 2, U3_DUNGEON_TILE_CHEST);

    frame = u3_dungeon_make_view_frame(dungeon, sizeof(dungeon), 0, 1, 1, 1);

    ASSERT_EQ_INT(1, count_commands_with_value(&frame, U3_DUNGEON_RENDER_VALUE_UP_LADDER));
    ASSERT_EQ_INT(1, count_commands_with_value(&frame, U3_DUNGEON_RENDER_VALUE_DOWN_LADDER));
    ASSERT_EQ_INT(1, count_commands_with_value(&frame, U3_DUNGEON_RENDER_VALUE_CHEST));
}

static void test_boundary_sampling_wraps_like_legacy_dungeon_access(void)
{
    u3_render_frame frame;
    const u3_render_command *wall;

    reset_dungeon();
    put_cell(0, 15, 0, U3_DUNGEON_TILE_WALL);

    frame = u3_dungeon_make_view_frame(dungeon, sizeof(dungeon), 0, 0, 0, 0);

    ASSERT_EQ_INT(1, count_commands_with_value(&frame, U3_DUNGEON_RENDER_VALUE_WALL));
    wall = first_command_with_value(&frame, U3_DUNGEON_RENDER_VALUE_WALL);
    ASSERT_TRUE(wall != 0);
    ASSERT_EQ_INT(0, wall->x);
    ASSERT_EQ_INT(2, wall->y);
}

static void test_rejects_invalid_input_with_safe_empty_view(void)
{
    u3_render_frame frame;

    reset_dungeon();

    frame = u3_dungeon_make_view_frame(dungeon, U3_DUNGEON_CELL_COUNT - 1, 0, 1, 1, 1);
    ASSERT_EQ_INT(2, frame.command_count);

    frame = u3_dungeon_make_view_frame(dungeon, sizeof(dungeon), 8, 1, 1, 1);
    ASSERT_EQ_INT(2, frame.command_count);

    frame = u3_dungeon_make_view_frame(dungeon, sizeof(dungeon), 0, -1, 1, 1);
    ASSERT_EQ_INT(2, frame.command_count);

    frame = u3_dungeon_make_view_frame(dungeon, sizeof(dungeon), 0, 1, 16, 1);
    ASSERT_EQ_INT(2, frame.command_count);

    frame = u3_dungeon_make_view_frame(dungeon, sizeof(dungeon), 0, 1, 1, 4);
    ASSERT_EQ_INT(2, frame.command_count);

    frame = u3_dungeon_make_view_frame(0, sizeof(dungeon), 0, 1, 1, 1);
    ASSERT_EQ_INT(2, frame.command_count);
}

static void test_rendering_does_not_mutate_dungeon_bytes(void)
{
    uint8_t before[U3_DUNGEON_CELL_COUNT];
    u3_render_frame frame;

    reset_dungeon();
    put_cell(0, 1, 1, U3_DUNGEON_TILE_UP_LADDER);
    put_cell(0, 2, 1, U3_DUNGEON_TILE_WALL);
    memcpy(before, dungeon, sizeof(before));

    frame = u3_dungeon_make_view_frame(dungeon, sizeof(dungeon), 0, 1, 1, 1);

    ASSERT_TRUE(frame.command_count > 2);
    ASSERT_EQ_INT(0, memcmp(before, dungeon, sizeof(before)));
}

static void test_light_limited_frame_suppresses_deeper_geometry(void)
{
    u3_render_frame dark_frame;
    u3_render_frame lit_frame;

    reset_dungeon();
    put_cell(0, 2, 1, U3_DUNGEON_TILE_WALL);

    dark_frame = u3_dungeon_make_lit_view_frame(dungeon, sizeof(dungeon), 0, 1, 1, 1, 0);
    lit_frame = u3_dungeon_make_lit_view_frame(dungeon, sizeof(dungeon), 0, 1, 1, 1, U3_DUNGEON_LIGHT_FULL);

    ASSERT_EQ_INT(2, dark_frame.command_count);
    ASSERT_TRUE(lit_frame.command_count > dark_frame.command_count);
}

static void test_dungeon_light_decay_saturates_at_zero(void)
{
    ASSERT_EQ_INT(254, u3_dungeon_decay_light(255));
    ASSERT_EQ_INT(1, u3_dungeon_decay_light(2));
    ASSERT_EQ_INT(0, u3_dungeon_decay_light(1));
    ASSERT_EQ_INT(0, u3_dungeon_decay_light(0));
}

int main(void)
{
    test_builds_corridor_with_side_walls_and_front_door();
    test_heading_rotation_changes_sampled_cells();
    test_current_cell_features_are_distinguishable();
    test_visible_non_current_features_are_distinguishable();
    test_boundary_sampling_wraps_like_legacy_dungeon_access();
    test_rejects_invalid_input_with_safe_empty_view();
    test_rendering_does_not_mutate_dungeon_bytes();
    test_light_limited_frame_suppresses_deeper_geometry();
    test_dungeon_light_decay_saturates_at_zero();

    printf("dungeon render characterization passed\n");
    return 0;
}
