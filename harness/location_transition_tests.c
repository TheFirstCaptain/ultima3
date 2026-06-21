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

static uint8_t *read_file(const char *path, size_t *length)
{
    FILE *file = fopen(path, "rb");
    uint8_t *bytes;
    long file_length;

    if (file == 0)
        return 0;
    if (fseek(file, 0, SEEK_END) != 0 || (file_length = ftell(file)) < 0) {
        fclose(file);
        return 0;
    }
    rewind(file);
    bytes = (uint8_t *)malloc((size_t)file_length);
    if (bytes == 0 || fread(bytes, 1, (size_t)file_length, file) != (size_t)file_length) {
        free(bytes);
        fclose(file);
        return 0;
    }
    fclose(file);
    *length = (size_t)file_length;
    return bytes;
}

static void load_fixture(uint8_t **resource_bytes,
                         u3_resource_record *map,
                         u3_resource_record *locations)
{
    size_t resource_length;
    u3_resource_file resource_file;

    *resource_bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &resource_length);
    ASSERT_TRUE(*resource_bytes != 0);
    ASSERT_TRUE(u3_resource_open(*resource_bytes, resource_length, &resource_file));
    ASSERT_TRUE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('M', 'A', 'P', 'S'), 420, map));
    ASSERT_TRUE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('M', 'I', 'S', 'C'), 404, locations));
}

static void test_detects_lcb_towne_fixture(void)
{
    uint8_t *resource_bytes;
    u3_resource_record map;
    u3_resource_record locations;
    u3_overworld_state state;
    u3_location_transition_result result;

    load_fixture(&resource_bytes, &map, &locations);
    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 46, 19, 64, 64));

    ASSERT_TRUE(u3_location_handle_overworld_command(&state,
                                                     map.data,
                                                     map.length,
                                                     locations.data,
                                                     locations.length,
                                                     U3_LOCATION_COMMAND_ENTER,
                                                     &result));
    ASSERT_TRUE(result.handled);
    ASSERT_TRUE(result.requested);
    ASSERT_EQ_INT(U3_LOCATION_KIND_TOWN, result.destination_kind);
    ASSERT_EQ_INT(2, result.location_index);
    ASSERT_EQ_INT(402, result.resource_id);
    ASSERT_EQ_INT(46, result.return_x);
    ASSERT_EQ_INT(19, result.return_y);
    ASSERT_EQ_INT(1, result.initial_x);
    ASSERT_EQ_INT(32, result.initial_y);
    ASSERT_EQ_INT(2, result.initial_heading);
    ASSERT_EQ_INT(U3_LOCATION_STATUS_TOWN_REQUESTED, result.status);

    free(resource_bytes);
}

static void test_enter_rejects_unrecognized_coordinate_without_mutation(void)
{
    uint8_t map[65] = {8};
    uint8_t locations[U3_LOCATION_TABLE_LENGTH];
    u3_overworld_state state;
    u3_location_transition_result result;

    memset(locations, 0xff, sizeof(locations));
    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 3, 4, 8, 8));

    ASSERT_TRUE(u3_location_handle_overworld_command(&state,
                                                     map,
                                                     sizeof(map),
                                                     locations,
                                                     sizeof(locations),
                                                     U3_LOCATION_COMMAND_ENTER,
                                                     &result));
    ASSERT_TRUE(result.handled);
    ASSERT_FALSE(result.requested);
    ASSERT_EQ_INT(U3_LOCATION_KIND_NONE, result.destination_kind);
    ASSERT_EQ_INT(U3_LOCATION_STATUS_NOT_ENTERABLE, result.status);
    ASSERT_EQ_INT(3, result.return_x);
    ASSERT_EQ_INT(4, result.return_y);
    ASSERT_EQ_INT(3, state.x);
    ASSERT_EQ_INT(4, state.y);
}

static void test_matching_non_town_tile_is_not_enterable(void)
{
    uint8_t map[65] = {8};
    uint8_t locations[U3_LOCATION_TABLE_LENGTH];
    u3_overworld_state state;
    u3_location_transition_result result;

    memset(locations, 0xff, sizeof(locations));
    locations[5] = 3;
    locations[U3_LOCATION_TABLE_COUNT + 5] = 4;
    map[1 + (4 * 8) + 3] = 0x1c;
    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 3, 4, 8, 8));

    ASSERT_TRUE(u3_location_handle_overworld_command(&state,
                                                     map,
                                                     sizeof(map),
                                                     locations,
                                                     sizeof(locations),
                                                     U3_LOCATION_COMMAND_ENTER,
                                                     &result));
    ASSERT_TRUE(result.handled);
    ASSERT_FALSE(result.requested);
    ASSERT_EQ_INT(U3_LOCATION_STATUS_NOT_ENTERABLE, result.status);
}

static void test_non_enter_command_is_ignored(void)
{
    uint8_t map[65] = {8};
    uint8_t locations[U3_LOCATION_TABLE_LENGTH];
    u3_overworld_state state;
    u3_location_transition_result result;

    memset(locations, 0xff, sizeof(locations));
    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 3, 4, 8, 8));
    ASSERT_TRUE(u3_location_handle_overworld_command(&state,
                                                     map,
                                                     sizeof(map),
                                                     locations,
                                                     sizeof(locations),
                                                     U3_OVERWORLD_COMMAND_EAST,
                                                     &result));
    ASSERT_FALSE(result.handled);
    ASSERT_FALSE(result.requested);
    ASSERT_EQ_INT(U3_LOCATION_STATUS_NONE, result.status);
}

static void test_rejects_truncated_inputs(void)
{
    uint8_t map[65] = {8};
    uint8_t locations[U3_LOCATION_TABLE_LENGTH] = {0};
    u3_overworld_state state;
    u3_location_transition_result result;

    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 3, 4, 8, 8));
    ASSERT_FALSE(u3_location_handle_overworld_command(&state,
                                                      map,
                                                      sizeof(map) - 1,
                                                      locations,
                                                      sizeof(locations),
                                                      U3_LOCATION_COMMAND_ENTER,
                                                      &result));
    ASSERT_FALSE(u3_location_handle_overworld_command(&state,
                                                      map,
                                                      sizeof(map),
                                                      locations,
                                                      sizeof(locations) - 1,
                                                      U3_LOCATION_COMMAND_ENTER,
                                                      &result));
}

int main(void)
{
    test_detects_lcb_towne_fixture();
    test_enter_rejects_unrecognized_coordinate_without_mutation();
    test_matching_non_town_tile_is_not_enterable();
    test_non_enter_command_is_ignored();
    test_rejects_truncated_inputs();

    printf("location transition tests passed\n");
    return 0;
}
