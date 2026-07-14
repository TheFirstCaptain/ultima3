#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_combat_render.h"

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

static void fill_screen(uint8_t screen[200])
{
    int index;

    memset(screen, 0, 200);
    for (index = 0; index < U3_COMBAT_SCREEN_TILE_BYTES; index++)
        screen[index] = (uint8_t)(index + 1);
    for (index = 0; index < U3_COMBAT_MONSTER_COUNT; index++) {
        screen[0x80 + index] = (uint8_t)(index + 1);
        screen[0x88 + index] = (uint8_t)(index + 2);
        screen[0x90 + index] = (uint8_t)(0x40 + index);
        screen[0x98 + index] = 0;
    }
    for (index = 0; index < U3_COMBAT_CHARACTER_COUNT; index++) {
        screen[0xA0 + index] = (uint8_t)(3 + index);
        screen[0xA4 + index] = (uint8_t)(7 + index);
        screen[0xA8 + index] = (uint8_t)(0x20 + index);
        screen[0xAC + index] = (uint8_t)(0x99 + index);
    }
}

static void test_initializes_state_from_full_screen_and_renders_occupants(void)
{
    uint8_t screen[200];
    u3_combat_state state;
    u3_combat_screen_init_result init;
    u3_combat_render_result render;
    u3_render_frame frame;

    fill_screen(screen);

    init = u3_combat_state_init_from_screen(&state, screen, 200, 0x34, 2, 4);
    ASSERT_EQ_INT(U3_COMBAT_SCREEN_STATUS_OK, init.status);
    ASSERT_EQ_INT(U3_COMBAT_SCREEN_TILE_BYTES, init.copied_tiles);
    ASSERT_EQ_INT(2, init.initialized_monsters);
    ASSERT_EQ_INT(4, init.initialized_characters);
    ASSERT_EQ_INT(0x34, state.monster_type);
    ASSERT_EQ_INT(1, state.monster_x[0]);
    ASSERT_EQ_INT(2, state.monster_y[0]);
    ASSERT_EQ_INT(0x34, state.monster_tile[0]);
    ASSERT_EQ_INT(1, state.monster_hp[0]);
    ASSERT_EQ_INT(3, state.character_x[0]);
    ASSERT_EQ_INT(7, state.character_y[0]);
    ASSERT_EQ_INT(0x99, state.character_shape[0]);

    frame = u3_combat_make_frame(&state, &render);

    ASSERT_TRUE(render.rendered);
    ASSERT_EQ_INT(U3_COMBAT_SCREEN_TILE_BYTES, render.terrain_commands);
    ASSERT_EQ_INT(2, render.monster_commands);
    ASSERT_EQ_INT(4, render.party_commands);
    ASSERT_EQ_INT(0, render.invalid_occupants);
    ASSERT_EQ_INT(U3_RENDER_TILE_COUNT + 2 + 2 + 4, frame.command_count);
    ASSERT_EQ_INT(1, frame.commands[2].value);
    ASSERT_EQ_INT(U3_COMBAT_RENDER_MONSTER_BASE, frame.commands[123].value);
    ASSERT_EQ_INT(2, frame.commands[123].x);
    ASSERT_EQ_INT(3, frame.commands[123].y);
    ASSERT_EQ_INT(U3_COMBAT_RENDER_PARTY_BASE, frame.commands[125].value);
    ASSERT_EQ_INT(4, frame.commands[125].x);
    ASSERT_EQ_INT(8, frame.commands[125].y);
}

static void test_tile_only_screen_renders_terrain_without_occupants(void)
{
    uint8_t screen[U3_COMBAT_SCREEN_TILE_BYTES];
    u3_combat_state state;
    u3_combat_screen_init_result init;
    u3_combat_render_result render;
    u3_render_frame frame;
    int index;

    for (index = 0; index < U3_COMBAT_SCREEN_TILE_BYTES; index++)
        screen[index] = (uint8_t)(0x40 + index);

    init = u3_combat_state_init_from_screen(&state, screen, U3_COMBAT_SCREEN_TILE_BYTES, 0x34, 8, 4);
    ASSERT_EQ_INT(U3_COMBAT_SCREEN_STATUS_TILE_ONLY, init.status);
    ASSERT_EQ_INT(U3_COMBAT_SCREEN_TILE_BYTES, init.copied_tiles);
    ASSERT_EQ_INT(0, init.initialized_monsters);
    ASSERT_EQ_INT(0, init.initialized_characters);

    frame = u3_combat_make_frame(&state, &render);

    ASSERT_EQ_INT(U3_RENDER_TILE_COUNT + 2, frame.command_count);
    ASSERT_EQ_INT(0, render.monster_commands);
    ASSERT_EQ_INT(0, render.party_commands);
}

static void test_invalid_screen_reports_safe_result(void)
{
    uint8_t screen[16];
    u3_combat_state state;
    u3_combat_screen_init_result init;

    init = u3_combat_state_init_from_screen(&state, screen, 16, 0x34, 1, 1);

    ASSERT_EQ_INT(U3_COMBAT_SCREEN_STATUS_INVALID, init.status);
    ASSERT_EQ_INT(0, init.copied_tiles);
    ASSERT_EQ_INT(0, init.initialized_monsters);
    ASSERT_EQ_INT(0, init.initialized_characters);
}

static void test_invalid_occupant_coordinates_are_skipped(void)
{
    uint8_t screen[200];
    u3_combat_state state;
    u3_combat_screen_init_result init;
    u3_combat_render_result render;

    fill_screen(screen);
    screen[0x80] = 12;
    screen[0xA4] = 12;

    init = u3_combat_state_init_from_screen(&state, screen, 200, 0x34, 1, 1);
    ASSERT_EQ_INT(U3_COMBAT_SCREEN_STATUS_OK, init.status);

    (void)u3_combat_make_frame(&state, &render);

    ASSERT_EQ_INT(0, render.monster_commands);
    ASSERT_EQ_INT(0, render.party_commands);
    ASSERT_EQ_INT(2, render.invalid_occupants);
}

static void test_render_null_state_uses_safe_fallback(void)
{
    u3_combat_render_result render;
    u3_render_frame frame;

    render.rendered = 9;
    frame = u3_combat_make_frame(NULL, &render);

    ASSERT_FALSE(render.rendered);
    ASSERT_EQ_INT(U3_RENDER_TILE_COUNT + 2, frame.command_count);
}

int main(void)
{
    test_initializes_state_from_full_screen_and_renders_occupants();
    test_tile_only_screen_renders_terrain_without_occupants();
    test_invalid_screen_reports_safe_result();
    test_invalid_occupant_coordinates_are_skipped();
    test_render_null_state_uses_safe_fallback();

    printf("combat render characterization passed\n");
    return 0;
}
