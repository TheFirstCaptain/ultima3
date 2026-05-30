#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char TileArray[128];
static unsigned char Party[64];
static long gMapOffset;
static short gCurMapSize;
static unsigned char mapStorage[256 * 256];
static unsigned char *mapBytes = mapStorage;
static unsigned char **Map = &mapBytes;

static char GetHeading(short value)
{
    /* Legacy reference: Sources/UltimaMisc.c GetHeading, $7DFC. */
    if (value == 0)
        return 0;
    if (value < 0)
        return -1;
    if (value > 127)
        return -1;
    return 1;
}

static short Absolute(short value)
{
    /* Legacy reference: Sources/UltimaMisc.c Absolute, $7E0D. */
    if (value > 127)
        value = (255 - value) + 1;
    if (value < 0)
        value = (-value);
    return value;
}

static short GetXY(short x, short y)
{
    /* Legacy reference: Sources/UltimaMisc.c GetXY, $7E18. */
    return TileArray[y * 11 + x];
}

static void PutXY(short a, short x, short y)
{
    /* Legacy reference: Sources/UltimaMisc.c PutXY. */
    TileArray[y * 11 + x] = a;
}

static short MapConstrain(short value)
{
    /* Legacy reference: Sources/UltimaMisc.c MapConstrain. */
    while (value < 0) {
        value += gCurMapSize;
    }
    while (value >= gCurMapSize) {
        value -= gCurMapSize;
    }
    return value;
}

static unsigned char GetXYVal(int x, int y)
{
    /* Legacy reference: Sources/UltimaMisc.c GetXYVal. */
    unsigned char value;
    gMapOffset = (MapConstrain(y) * gCurMapSize);
    gMapOffset += MapConstrain(x);
    value = (*Map)[gMapOffset];
    if (Party[3] != 0) {
        if (x < 0 || x >= gCurMapSize || y < 0 || y >= gCurMapSize)
            value = 0x04;
    }
    return value;
}

static void PutXYVal(unsigned char value, unsigned char x, unsigned char y)
{
    /* Legacy reference: Sources/UltimaMisc.c PutXYVal. */
    gMapOffset = (MapConstrain(y) * gCurMapSize);
    gMapOffset += MapConstrain(x);
    (*Map)[gMapOffset] = value;
}

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
    gMapOffset = 0;
    gCurMapSize = mapSize;
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
    ASSERT_EQ_INT(0, GetHeading(0));
    ASSERT_EQ_INT(1, GetHeading(1));
    ASSERT_EQ_INT(1, GetHeading(127));
    ASSERT_EQ_INT(-1, GetHeading(128));
    ASSERT_EQ_INT(-1, GetHeading(255));
    ASSERT_EQ_INT(-1, GetHeading(-1));
}

static void test_absolute(void)
{
    ASSERT_EQ_INT(0, Absolute(0));
    ASSERT_EQ_INT(1, Absolute(1));
    ASSERT_EQ_INT(127, Absolute(127));
    ASSERT_EQ_INT(128, Absolute(128));
    ASSERT_EQ_INT(1, Absolute(255));
    ASSERT_EQ_INT(3, Absolute(-3));
    ASSERT_EQ_INT(44, Absolute(300));
}

static void test_tile_array_accessors(void)
{
    reset_state(64);

    PutXY(0x7a, 3, 4);
    ASSERT_EQ_INT(0x7a, GetXY(3, 4));
    ASSERT_EQ_INT(0x7a, TileArray[(4 * 11) + 3]);

    PutXY(0x123, 10, 10);
    ASSERT_EQ_INT(0x23, GetXY(10, 10));
}

static void test_map_constrain(void)
{
    reset_state(64);

    ASSERT_EQ_INT(0, MapConstrain(0));
    ASSERT_EQ_INT(63, MapConstrain(-1));
    ASSERT_EQ_INT(0, MapConstrain(64));
    ASSERT_EQ_INT(2, MapConstrain(130));
    ASSERT_EQ_INT(62, MapConstrain(-130));
}

static void test_get_xy_val_surface_wraps_coordinates(void)
{
    reset_state(4);
    fill_map(4);
    Party[3] = 0;

    ASSERT_EQ_INT(0x03, GetXYVal(-1, 0));
    ASSERT_EQ_INT(3, gMapOffset);
    ASSERT_EQ_INT(0x30, GetXYVal(0, -1));
    ASSERT_EQ_INT(12, gMapOffset);
    ASSERT_EQ_INT(0x00, GetXYVal(4, 4));
    ASSERT_EQ_INT(0, gMapOffset);
}

static void test_get_xy_val_non_surface_masks_out_of_bounds(void)
{
    reset_state(4);
    fill_map(4);
    Party[3] = 1;

    ASSERT_EQ_INT(0x04, GetXYVal(-1, 0));
    ASSERT_EQ_INT(3, gMapOffset);
    ASSERT_EQ_INT(0x04, GetXYVal(4, 0));
    ASSERT_EQ_INT(0, gMapOffset);
    ASSERT_EQ_INT(0x11, GetXYVal(1, 1));
}

static void test_put_xy_val_wraps_unsigned_coordinates(void)
{
    reset_state(4);

    PutXYVal(0xab, 255, 0);
    ASSERT_EQ_INT(0xab, mapStorage[3]);
    ASSERT_EQ_INT(3, gMapOffset);

    PutXYVal(0xcd, 4, 4);
    ASSERT_EQ_INT(0xcd, mapStorage[0]);
    ASSERT_EQ_INT(0, gMapOffset);
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
