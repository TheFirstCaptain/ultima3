#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_map_math.h"

static unsigned char TileArray[128];
static unsigned char Party[64];
static unsigned char mapStorage[256 * 256];
static u3_map_math_state state;

#define ASSERT_EQ_INT(expected, actual) assert_eq_int((expected), (actual), __FILE__, __LINE__)

static void assert_eq_int(int expected, int actual, const char *file, int line)
{
    if (expected != actual) {
        fprintf(stderr, "%s:%d: expected %d, got %d\n", file, line, expected, actual);
        exit(1);
    }
}

static void reset_state(short mapSize)
{
    memset(TileArray, 0, sizeof(TileArray));
    memset(Party, 0, sizeof(Party));
    memset(mapStorage, 0, sizeof(mapStorage));
    state.tile_array = TileArray;
    state.party = Party;
    state.map = mapStorage;
    state.map_offset = 0;
    state.cur_map_size = mapSize;
}

static void fill_map(short mapSize)
{
    int x;
    int y;

    for (y = 0; y < mapSize; y++) {
        for (x = 0; x < mapSize; x++) {
            mapStorage[(y * mapSize) + x] = (unsigned char)((y * 16) + x);
        }
    }
}

static void test_get_heading(void)
{
    ASSERT_EQ_INT(0, u3_map_math_get_heading(0));
    ASSERT_EQ_INT(1, u3_map_math_get_heading(1));
    ASSERT_EQ_INT(1, u3_map_math_get_heading(127));
    ASSERT_EQ_INT(-1, u3_map_math_get_heading(128));
    ASSERT_EQ_INT(-1, u3_map_math_get_heading(255));
    ASSERT_EQ_INT(-1, u3_map_math_get_heading(-1));
}

static void test_absolute(void)
{
    ASSERT_EQ_INT(0, u3_map_math_absolute(0));
    ASSERT_EQ_INT(1, u3_map_math_absolute(1));
    ASSERT_EQ_INT(127, u3_map_math_absolute(127));
    ASSERT_EQ_INT(128, u3_map_math_absolute(128));
    ASSERT_EQ_INT(1, u3_map_math_absolute(255));
    ASSERT_EQ_INT(3, u3_map_math_absolute(-3));
    ASSERT_EQ_INT(44, u3_map_math_absolute(300));
}

static void test_tile_array_accessors(void)
{
    reset_state(64);

    u3_map_math_put_tile(&state, 0x7a, 3, 4);
    ASSERT_EQ_INT(0x7a, u3_map_math_get_tile(&state, 3, 4));
    ASSERT_EQ_INT(0x7a, TileArray[(4 * 11) + 3]);

    u3_map_math_put_tile(&state, 0x123, 10, 10);
    ASSERT_EQ_INT(0x23, u3_map_math_get_tile(&state, 10, 10));
}

static void test_map_constrain(void)
{
    reset_state(64);

    ASSERT_EQ_INT(0, u3_map_math_constrain(&state, 0));
    ASSERT_EQ_INT(63, u3_map_math_constrain(&state, -1));
    ASSERT_EQ_INT(0, u3_map_math_constrain(&state, 64));
    ASSERT_EQ_INT(2, u3_map_math_constrain(&state, 130));
    ASSERT_EQ_INT(62, u3_map_math_constrain(&state, -130));
}

static void test_get_xy_val_surface_wraps_coordinates(void)
{
    reset_state(4);
    fill_map(4);
    Party[3] = 0;

    ASSERT_EQ_INT(0x03, u3_map_math_get_map_value(&state, -1, 0));
    ASSERT_EQ_INT(3, state.map_offset);
    ASSERT_EQ_INT(0x30, u3_map_math_get_map_value(&state, 0, -1));
    ASSERT_EQ_INT(12, state.map_offset);
    ASSERT_EQ_INT(0x00, u3_map_math_get_map_value(&state, 4, 4));
    ASSERT_EQ_INT(0, state.map_offset);
}

static void test_get_xy_val_non_surface_masks_out_of_bounds(void)
{
    reset_state(4);
    fill_map(4);
    Party[3] = 1;

    ASSERT_EQ_INT(0x04, u3_map_math_get_map_value(&state, -1, 0));
    ASSERT_EQ_INT(3, state.map_offset);
    ASSERT_EQ_INT(0x04, u3_map_math_get_map_value(&state, 4, 0));
    ASSERT_EQ_INT(0, state.map_offset);
    ASSERT_EQ_INT(0x11, u3_map_math_get_map_value(&state, 1, 1));
}

static void test_put_xy_val_wraps_unsigned_coordinates(void)
{
    reset_state(4);

    u3_map_math_put_map_value(&state, 0xab, 255, 0);
    ASSERT_EQ_INT(0xab, mapStorage[3]);
    ASSERT_EQ_INT(3, state.map_offset);

    u3_map_math_put_map_value(&state, 0xcd, 4, 4);
    ASSERT_EQ_INT(0xcd, mapStorage[0]);
    ASSERT_EQ_INT(0, state.map_offset);
}

int main(void)
{
    test_get_heading();
    test_absolute();
    test_tile_array_accessors();
    test_map_constrain();
    test_get_xy_val_surface_wraps_coordinates();
    test_get_xy_val_non_surface_masks_out_of_bounds();
    test_put_xy_val_wraps_unsigned_coordinates();

    puts("map/math accessor characterization passed");
    return 0;
}
