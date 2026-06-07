#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "u3_render.h"

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

static void test_initializes_frame_with_logical_dimensions(void)
{
    u3_render_frame frame;

    u3_render_frame_init(&frame);

    ASSERT_EQ_INT(U3_RENDER_LOGICAL_WIDTH, frame.logical_width);
    ASSERT_EQ_INT(U3_RENDER_LOGICAL_HEIGHT, frame.logical_height);
    ASSERT_EQ_INT(0, frame.command_count);
    ASSERT_FALSE(frame.overflowed);
}

static void test_adds_bounded_tile_grid_commands(void)
{
    u3_render_frame frame;
    uint8_t tiles[U3_RENDER_TILE_COUNT];
    uint16_t index;

    for (index = 0; index < U3_RENDER_TILE_COUNT; index++)
        tiles[index] = (uint8_t)index;

    u3_render_frame_init(&frame);
    ASSERT_TRUE(u3_render_frame_add_tile_grid(&frame, tiles, U3_RENDER_TILE_COUNT, 3, 4));

    ASSERT_EQ_INT(U3_RENDER_TILE_COUNT, frame.command_count);
    ASSERT_FALSE(frame.overflowed);
    ASSERT_EQ_INT(U3_RENDER_COMMAND_TILE, frame.commands[0].kind);
    ASSERT_EQ_INT(0, frame.commands[0].value);
    ASSERT_EQ_INT(3, frame.commands[0].x);
    ASSERT_EQ_INT(4, frame.commands[0].y);
    ASSERT_EQ_INT(13, frame.commands[10].x);
    ASSERT_EQ_INT(4, frame.commands[10].y);
    ASSERT_EQ_INT(3, frame.commands[11].x);
    ASSERT_EQ_INT(5, frame.commands[11].y);
    ASSERT_EQ_INT(13, frame.commands[120].x);
    ASSERT_EQ_INT(14, frame.commands[120].y);
}

static void test_reports_truncated_tile_input(void)
{
    u3_render_frame frame;
    uint8_t tiles[U3_RENDER_TILE_COUNT + 1];

    u3_render_frame_init(&frame);
    ASSERT_FALSE(u3_render_frame_add_tile_grid(&frame, tiles, U3_RENDER_TILE_COUNT + 1, 0, 0));

    ASSERT_EQ_INT(U3_RENDER_TILE_COUNT, frame.command_count);
    ASSERT_FALSE(frame.overflowed);
}

static void test_reports_command_capacity_overflow(void)
{
    u3_render_frame frame;
    u3_render_command command;
    int index;

    u3_render_frame_init(&frame);
    command.kind = U3_RENDER_COMMAND_RECT;
    command.x = 0;
    command.y = 0;
    command.width = 1;
    command.height = 1;
    command.value = 0;
    command.fill.red = 0;
    command.fill.green = 0;
    command.fill.blue = 0;
    command.fill.alpha = 255;
    command.stroke = command.fill;

    for (index = 0; index < U3_RENDER_MAX_COMMANDS; index++)
        ASSERT_TRUE(u3_render_frame_add_command(&frame, &command));

    ASSERT_FALSE(u3_render_frame_add_command(&frame, &command));
    ASSERT_TRUE(frame.overflowed);
    ASSERT_EQ_INT(U3_RENDER_MAX_COMMANDS, frame.command_count);
}

static void test_makes_synthetic_tile_frame(void)
{
    u3_render_frame frame = u3_render_make_synthetic_tile_frame();

    ASSERT_EQ_INT(U3_RENDER_LOGICAL_WIDTH, frame.logical_width);
    ASSERT_EQ_INT(U3_RENDER_LOGICAL_HEIGHT, frame.logical_height);
    ASSERT_EQ_INT(U3_RENDER_TILE_COUNT + 2, frame.command_count);
    ASSERT_FALSE(frame.overflowed);
    ASSERT_EQ_INT(U3_RENDER_COMMAND_CLEAR, frame.commands[0].kind);
    ASSERT_EQ_INT(U3_RENDER_COMMAND_RECT, frame.commands[1].kind);
    ASSERT_EQ_INT(U3_RENDER_COMMAND_TILE, frame.commands[2].kind);
    ASSERT_EQ_INT(1, frame.commands[2].x);
    ASSERT_EQ_INT(1, frame.commands[2].y);
}

static void test_makes_tile_grid_frame_from_supplied_tiles(void)
{
    uint8_t tiles[U3_RENDER_TILE_COUNT];
    u3_render_frame frame;
    uint16_t index;

    for (index = 0; index < U3_RENDER_TILE_COUNT; index++)
        tiles[index] = (uint8_t)(0x40 + index);

    frame = u3_render_make_tile_grid_frame(tiles, U3_RENDER_TILE_COUNT);

    ASSERT_EQ_INT(U3_RENDER_TILE_COUNT + 2, frame.command_count);
    ASSERT_FALSE(frame.overflowed);
    ASSERT_EQ_INT(U3_RENDER_COMMAND_TILE, frame.commands[2].kind);
    ASSERT_EQ_INT(0x40, frame.commands[2].value);
    ASSERT_EQ_INT(1, frame.commands[2].x);
    ASSERT_EQ_INT(1, frame.commands[2].y);
    ASSERT_EQ_INT(0x40 + 120, frame.commands[122].value);
    ASSERT_EQ_INT(11, frame.commands[122].y);
}

int main(void)
{
    test_initializes_frame_with_logical_dimensions();
    test_adds_bounded_tile_grid_commands();
    test_reports_truncated_tile_input();
    test_reports_command_capacity_overflow();
    test_makes_synthetic_tile_frame();
    test_makes_tile_grid_frame_from_supplied_tiles();

    printf("renderer contract characterization passed\n");
    return 0;
}
