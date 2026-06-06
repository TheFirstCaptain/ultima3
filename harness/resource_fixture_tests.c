#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_resource.h"

#define ASSERT_EQ_INT(expected, actual) assert_eq_int((expected), (actual), __FILE__, __LINE__)
#define ASSERT_TRUE(actual) assert_true((actual), __FILE__, __LINE__)
#define ASSERT_FALSE(actual) assert_false((actual), __FILE__, __LINE__)
#define ASSERT_BYTES_EQ(expected, actual, length) assert_bytes_eq((expected), (actual), (length), __FILE__, __LINE__)

#define U3_FIXTURE_MAP_BYTES_MAX 4096
#define U3_FIXTURE_DUNGEON_BYTES 2048
#define U3_FIXTURE_MONSTER_BYTES 256
#define U3_FIXTURE_TALK_BYTES 256
#define U3_FIXTURE_COMBAT_TILE_BYTES 121
#define U3_FIXTURE_COMBAT_MONSTERS 8
#define U3_FIXTURE_COMBAT_CHARACTERS 4

typedef struct map_load_fixture {
    uint8_t map_size;
    uint8_t map[U3_FIXTURE_MAP_BYTES_MAX];
    uint8_t monsters[U3_FIXTURE_MONSTER_BYTES];
    uint8_t talk[U3_FIXTURE_TALK_BYTES];
} map_load_fixture;

typedef struct dungeon_load_fixture {
    uint8_t dungeon[U3_FIXTURE_DUNGEON_BYTES];
    uint8_t talk[U3_FIXTURE_TALK_BYTES];
} dungeon_load_fixture;

typedef struct combat_screen_fixture {
    uint8_t tiles[U3_FIXTURE_COMBAT_TILE_BYTES];
    uint8_t monster_x[U3_FIXTURE_COMBAT_MONSTERS];
    uint8_t monster_y[U3_FIXTURE_COMBAT_MONSTERS];
    uint8_t monster_tile[U3_FIXTURE_COMBAT_MONSTERS];
    uint8_t monster_hp[U3_FIXTURE_COMBAT_MONSTERS];
    uint8_t char_x[U3_FIXTURE_COMBAT_CHARACTERS];
    uint8_t char_y[U3_FIXTURE_COMBAT_CHARACTERS];
    uint8_t char_tile[U3_FIXTURE_COMBAT_CHARACTERS];
    uint8_t char_shape[U3_FIXTURE_COMBAT_CHARACTERS];
} combat_screen_fixture;

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

static void assert_bytes_eq(const uint8_t *expected, const uint8_t *actual, size_t length, const char *file, int line)
{
    size_t i;

    for (i = 0; i < length; i++) {
        if (expected[i] != actual[i]) {
            fprintf(stderr, "%s:%d: byte %zu expected 0x%02x, got 0x%02x\n",
                    file, line, i, expected[i], actual[i]);
            exit(1);
        }
    }
}

static uint8_t *try_read_file(const char *path, size_t *length)
{
    FILE *file;
    long file_length;
    uint8_t *bytes;

    file = fopen(path, "rb");
    if (file == 0)
        return 0;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 0;
    }
    file_length = ftell(file);
    if (file_length < 0) {
        fclose(file);
        return 0;
    }
    rewind(file);

    bytes = (uint8_t *)malloc((size_t)file_length);
    if (bytes == 0) {
        fclose(file);
        return 0;
    }
    if (fread(bytes, 1, (size_t)file_length, file) != (size_t)file_length) {
        free(bytes);
        fclose(file);
        return 0;
    }
    fclose(file);

    *length = (size_t)file_length;
    return bytes;
}

static uint8_t *read_file(const char *path, size_t *length)
{
    uint8_t *bytes = try_read_file(path, length);

    if (bytes == 0) {
        perror(path);
        exit(1);
    }

    return bytes;
}

static int load_main_resources_from_root(const char *resource_root, uint8_t **bytes, size_t *length, u3_resource_file *resource_file)
{
    char path[4096];
    int path_length;

    if (resource_root == 0 || bytes == 0 || length == 0 || resource_file == 0)
        return 0;

    *bytes = 0;
    *length = 0;

    path_length = snprintf(path, sizeof(path), "%s/English.lproj/MainResources.rsrc", resource_root);
    if (path_length < 0 || (size_t)path_length >= sizeof(path))
        return 0;

    *bytes = try_read_file(path, length);
    if (*bytes == 0)
        return 0;

    if (!u3_resource_open(*bytes, *length, resource_file)) {
        free(*bytes);
        *bytes = 0;
        *length = 0;
        return 0;
    }

    return 1;
}

static void assert_named_resource(const u3_resource_file *resource_file,
                                  uint32_t type,
                                  int16_t id,
                                  uint32_t expected_length,
                                  const char *expected_name)
{
    u3_resource_record record;

    ASSERT_TRUE(u3_resource_find(resource_file, type, id, &record));
    ASSERT_EQ_INT((int)expected_length, (int)record.length);
    ASSERT_EQ_INT((int)strlen(expected_name), record.name_length);
    ASSERT_BYTES_EQ((const uint8_t *)expected_name, record.name, record.name_length);
}

static uint32_t resource_type_from_string(const char *type)
{
    return U3_RESOURCE_TYPE(type[0], type[1], type[2], type[3]);
}

static u3_resource_record require_resource(const u3_resource_file *resource_file, const char *type, int16_t id)
{
    u3_resource_record record;

    ASSERT_TRUE(u3_resource_find(resource_file, resource_type_from_string(type), id, &record));
    return record;
}

static uint32_t count_records_for_type(const u3_resource_file *resource_file, uint32_t expected_type)
{
    uint32_t type_index;

    for (type_index = 0; type_index < resource_file->type_count; type_index++) {
        uint32_t type;
        uint32_t record_count;

        ASSERT_TRUE(u3_resource_get_type(resource_file, type_index, &type, &record_count));
        if (type == expected_type)
            return record_count;
    }

    return 0;
}

static void test_enumerates_resource_inventory(void)
{
    size_t length;
    uint8_t *bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &length);
    u3_resource_file resource_file;
    uint32_t type_index;
    uint32_t total_records = 0;

    ASSERT_TRUE(u3_resource_open(bytes, length, &resource_file));
    ASSERT_EQ_INT(45, (int)resource_file.type_count);

    for (type_index = 0; type_index < resource_file.type_count; type_index++) {
        uint32_t type;
        uint32_t record_count;
        uint32_t record_index;

        ASSERT_TRUE(u3_resource_get_type(&resource_file, type_index, &type, &record_count));
        total_records += record_count;
        for (record_index = 0; record_index < record_count; record_index++) {
            u3_resource_record record;

            ASSERT_TRUE(u3_resource_get_record(&resource_file, type_index, record_index, &record));
            ASSERT_EQ_INT((int)type, (int)record.type);
        }
    }

    ASSERT_EQ_INT(303, (int)total_records);
    ASSERT_EQ_INT(21, (int)count_records_for_type(&resource_file, U3_RESOURCE_TYPE('M', 'A', 'P', 'S')));
    ASSERT_EQ_INT(14, (int)count_records_for_type(&resource_file, U3_RESOURCE_TYPE('M', 'O', 'N', 'S')));
    ASSERT_EQ_INT(20, (int)count_records_for_type(&resource_file, U3_RESOURCE_TYPE('T', 'L', 'K', 'S')));
    ASSERT_EQ_INT(13, (int)count_records_for_type(&resource_file, U3_RESOURCE_TYPE('C', 'O', 'N', 'S')));
    ASSERT_EQ_INT(6, (int)count_records_for_type(&resource_file, U3_RESOURCE_TYPE('M', 'I', 'S', 'C')));
    ASSERT_EQ_INT(6, (int)count_records_for_type(&resource_file, U3_RESOURCE_TYPE('S', 'T', 'R', '#')));
    ASSERT_EQ_INT(5, (int)count_records_for_type(&resource_file, U3_RESOURCE_TYPE('P', 'I', 'C', 'T')));

    free(bytes);
}

static void test_open_main_resources(void)
{
    size_t length;
    uint8_t *bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &length);
    u3_resource_file resource_file;

    ASSERT_TRUE(u3_resource_open(bytes, length, &resource_file));
    ASSERT_EQ_INT(256, (int)resource_file.data_offset);
    ASSERT_EQ_INT(180264, (int)resource_file.map_offset);
    ASSERT_EQ_INT(45, resource_file.type_count);

    free(bytes);
}

static void test_validates_main_resources_from_supplied_resource_root(void)
{
    size_t length;
    uint8_t *bytes;
    u3_resource_file resource_file;
    u3_resource_record record;

    ASSERT_TRUE(load_main_resources_from_root("../Resources", &bytes, &length, &resource_file));
    ASSERT_EQ_INT(45, resource_file.type_count);
    ASSERT_TRUE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('P', 'R', 'T', 'Y'), 500, &record));
    ASSERT_EQ_INT(64, (int)record.length);
    ASSERT_TRUE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('M', 'A', 'P', 'S'), 420, &record));
    ASSERT_EQ_INT(4101, (int)record.length);

    free(bytes);
}

static void test_missing_supplied_resource_root_fails_without_partial_state(void)
{
    size_t length = 123;
    uint8_t *bytes = (uint8_t *)1;
    u3_resource_file resource_file;

    ASSERT_FALSE(load_main_resources_from_root("../MissingResources", &bytes, &length, &resource_file));
    ASSERT_EQ_INT(0, (int)length);
    ASSERT_TRUE(bytes == 0);
}

static void test_extracts_default_save_templates(void)
{
    size_t length;
    uint8_t *bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &length);
    u3_resource_file resource_file;
    u3_resource_record record;

    ASSERT_TRUE(u3_resource_open(bytes, length, &resource_file));

    assert_named_resource(&resource_file, U3_RESOURCE_TYPE('P', 'R', 'T', 'Y'), 500, 64, "Party");
    assert_named_resource(&resource_file, U3_RESOURCE_TYPE('R', 'O', 'S', 'T'), 500, 1280, "Roster");
    assert_named_resource(&resource_file, U3_RESOURCE_TYPE('M', 'A', 'P', 'S'), 420, 4101, "Sosaria");

    ASSERT_TRUE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('M', 'O', 'N', 'S'), 420, &record));
    ASSERT_EQ_INT(256, (int)record.length);
    ASSERT_EQ_INT(0, record.name_length);

    assert_named_resource(&resource_file, U3_RESOURCE_TYPE('M', 'I', 'S', 'C'), 400, 16, "Moongate Locations");
    assert_named_resource(&resource_file, U3_RESOURCE_TYPE('M', 'I', 'S', 'C'), 401, 11, "Type Initial Table");
    assert_named_resource(&resource_file, U3_RESOURCE_TYPE('M', 'I', 'S', 'C'), 402, 11, "Weapon Use By Class");
    assert_named_resource(&resource_file, U3_RESOURCE_TYPE('M', 'I', 'S', 'C'), 403, 11, "Armour Use By Class");
    assert_named_resource(&resource_file, U3_RESOURCE_TYPE('M', 'I', 'S', 'C'), 404, 64, "Location Table");
    assert_named_resource(&resource_file, U3_RESOURCE_TYPE('M', 'I', 'S', 'C'), 405, 16, "Experience Table");

    free(bytes);
}

static void load_map_fixture(const u3_resource_file *resource_file, int16_t resid, map_load_fixture *fixture)
{
    u3_resource_record maps = require_resource(resource_file, "MAPS", resid);
    u3_resource_record mons = require_resource(resource_file, "MONS", resid);
    u3_resource_record tlks = require_resource(resource_file, "TLKS", resid);
    uint32_t map_length;

    ASSERT_EQ_INT(4097, (int)maps.length);
    ASSERT_EQ_INT(256, (int)mons.length);
    ASSERT_EQ_INT(256, (int)tlks.length);

    fixture->map_size = maps.data[0];
    ASSERT_EQ_INT(64, fixture->map_size);
    map_length = (uint32_t)fixture->map_size * fixture->map_size;
    ASSERT_EQ_INT(4096, (int)map_length);

    memcpy(fixture->map, maps.data + 1, map_length);
    memcpy(fixture->monsters, mons.data, U3_FIXTURE_MONSTER_BYTES);
    memcpy(fixture->talk, tlks.data, U3_FIXTURE_TALK_BYTES);
}

static void load_dungeon_fixture(const u3_resource_file *resource_file, int16_t resid, dungeon_load_fixture *fixture)
{
    u3_resource_record maps = require_resource(resource_file, "MAPS", resid);
    u3_resource_record tlks = require_resource(resource_file, "TLKS", resid);

    ASSERT_EQ_INT(2048, (int)maps.length);
    ASSERT_EQ_INT(256, (int)tlks.length);
    memcpy(fixture->dungeon, maps.data, U3_FIXTURE_DUNGEON_BYTES);
    memcpy(fixture->talk, tlks.data, U3_FIXTURE_TALK_BYTES);
}

static void load_combat_screen_fixture(const u3_resource_file *resource_file, int16_t resid, combat_screen_fixture *fixture)
{
    u3_resource_record cons = require_resource(resource_file, "CONS", resid);

    ASSERT_TRUE(cons.length >= 0xB0);
    memcpy(fixture->tiles, cons.data, U3_FIXTURE_COMBAT_TILE_BYTES);
    memcpy(fixture->monster_x, cons.data + 0x80, U3_FIXTURE_COMBAT_MONSTERS);
    memcpy(fixture->monster_y, cons.data + 0x88, U3_FIXTURE_COMBAT_MONSTERS);
    memcpy(fixture->monster_tile, cons.data + 0x90, U3_FIXTURE_COMBAT_MONSTERS);
    memcpy(fixture->monster_hp, cons.data + 0x98, U3_FIXTURE_COMBAT_MONSTERS);
    memcpy(fixture->char_x, cons.data + 0xA0, U3_FIXTURE_COMBAT_CHARACTERS);
    memcpy(fixture->char_y, cons.data + 0xA4, U3_FIXTURE_COMBAT_CHARACTERS);
    memcpy(fixture->char_tile, cons.data + 0xA8, U3_FIXTURE_COMBAT_CHARACTERS);
    memcpy(fixture->char_shape, cons.data + 0xAC, U3_FIXTURE_COMBAT_CHARACTERS);
}

static void test_characterizes_town_map_monster_and_talk_fixture(void)
{
    size_t length;
    uint8_t *bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &length);
    u3_resource_file resource_file;
    map_load_fixture fixture;

    ASSERT_TRUE(u3_resource_open(bytes, length, &resource_file));
    load_map_fixture(&resource_file, 400, &fixture);

    ASSERT_BYTES_EQ(require_resource(&resource_file, "MAPS", 400).data + 1, fixture.map, 4096);
    ASSERT_BYTES_EQ(require_resource(&resource_file, "MONS", 400).data, fixture.monsters, 256);
    ASSERT_BYTES_EQ(require_resource(&resource_file, "TLKS", 400).data, fixture.talk, 256);
    ASSERT_EQ_INT(0x48, fixture.monsters[0]);
    ASSERT_EQ_INT(0x48, fixture.monsters[1]);
    ASSERT_EQ_INT(0x00, fixture.talk[0]);
    ASSERT_EQ_INT(0x04, fixture.map[0]);

    free(bytes);
}

static void test_characterizes_ambrosia_map_monster_and_talk_fixture(void)
{
    size_t length;
    uint8_t *bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &length);
    u3_resource_file resource_file;
    map_load_fixture fixture;

    ASSERT_TRUE(u3_resource_open(bytes, length, &resource_file));
    load_map_fixture(&resource_file, 421, &fixture);

    ASSERT_BYTES_EQ(require_resource(&resource_file, "MAPS", 421).data + 1, fixture.map, 4096);
    ASSERT_BYTES_EQ(require_resource(&resource_file, "MONS", 421).data, fixture.monsters, 256);
    ASSERT_BYTES_EQ(require_resource(&resource_file, "TLKS", 421).data, fixture.talk, 256);
    ASSERT_EQ_INT(0x74, fixture.monsters[0]);
    ASSERT_EQ_INT(0x00, fixture.talk[0]);
    ASSERT_EQ_INT(0x10, fixture.map[0]);

    free(bytes);
}

static void test_characterizes_dungeon_map_fixture(void)
{
    size_t length;
    uint8_t *bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &length);
    u3_resource_file resource_file;
    u3_resource_record record;
    dungeon_load_fixture fixture;

    ASSERT_TRUE(u3_resource_open(bytes, length, &resource_file));
    load_dungeon_fixture(&resource_file, 412, &fixture);

    ASSERT_BYTES_EQ(require_resource(&resource_file, "MAPS", 412).data, fixture.dungeon, 2048);
    ASSERT_BYTES_EQ(require_resource(&resource_file, "TLKS", 412).data, fixture.talk, 256);
    ASSERT_FALSE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('M', 'O', 'N', 'S'), 412, &record));
    ASSERT_EQ_INT(0x80, fixture.dungeon[0]);
    ASSERT_EQ_INT(0x40, fixture.dungeon[2047]);
    ASSERT_EQ_INT(0x00, fixture.talk[0]);

    free(bytes);
}

static void test_characterizes_full_combat_screen_fixture(void)
{
    size_t length;
    uint8_t *bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &length);
    u3_resource_file resource_file;
    u3_resource_record cons;
    combat_screen_fixture fixture;

    ASSERT_TRUE(u3_resource_open(bytes, length, &resource_file));
    cons = require_resource(&resource_file, "CONS", 400);
    ASSERT_EQ_INT(200, (int)cons.length);

    load_combat_screen_fixture(&resource_file, 400, &fixture);

    ASSERT_BYTES_EQ(cons.data, fixture.tiles, 121);
    ASSERT_BYTES_EQ(cons.data + 0x80, fixture.monster_x, 8);
    ASSERT_BYTES_EQ(cons.data + 0x88, fixture.monster_y, 8);
    ASSERT_BYTES_EQ(cons.data + 0x90, fixture.monster_tile, 8);
    ASSERT_BYTES_EQ(cons.data + 0x98, fixture.monster_hp, 8);
    ASSERT_BYTES_EQ(cons.data + 0xA0, fixture.char_x, 4);
    ASSERT_BYTES_EQ(cons.data + 0xA4, fixture.char_y, 4);
    ASSERT_BYTES_EQ(cons.data + 0xA8, fixture.char_tile, 4);
    ASSERT_BYTES_EQ(cons.data + 0xAC, fixture.char_shape, 4);
    ASSERT_EQ_INT(0x00, fixture.tiles[0]);
    ASSERT_EQ_INT(0x03, fixture.monster_x[0]);
    ASSERT_EQ_INT(0x03, fixture.monster_y[0]);
    ASSERT_EQ_INT(0x46, fixture.monster_tile[0]);
    ASSERT_EQ_INT(0x00, fixture.monster_hp[0]);
    ASSERT_EQ_INT(0x03, fixture.char_x[0]);
    ASSERT_EQ_INT(0x07, fixture.char_y[0]);
    ASSERT_EQ_INT(0x02, fixture.char_tile[0]);
    ASSERT_EQ_INT(0x99, fixture.char_shape[0]);

    free(bytes);
}

static void test_special_combat_screen_records_are_tile_only_for_full_getscreen_layout(void)
{
    size_t length;
    uint8_t *bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &length);
    u3_resource_file resource_file;
    u3_resource_record cons;

    ASSERT_TRUE(u3_resource_open(bytes, length, &resource_file));
    cons = require_resource(&resource_file, "CONS", 410);
    ASSERT_EQ_INT(121, (int)cons.length);
    ASSERT_FALSE(cons.length >= 0xB0);
    ASSERT_EQ_INT(0x46, cons.data[0]);
    ASSERT_EQ_INT(0x46, cons.data[120]);

    free(bytes);
}

static void test_runtime_save_records_are_not_in_bundled_templates(void)
{
    size_t length;
    uint8_t *bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &length);
    u3_resource_file resource_file;
    u3_resource_record record;

    ASSERT_TRUE(u3_resource_open(bytes, length, &resource_file));

    ASSERT_FALSE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('P', 'R', 'T', 'Y'), 400, &record));
    ASSERT_FALSE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('R', 'O', 'S', 'T'), 400, &record));
    ASSERT_FALSE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('M', 'A', 'P', 'S'), 419, &record));
    ASSERT_FALSE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('M', 'O', 'N', 'S'), 419, &record));
    ASSERT_FALSE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('M', 'I', 'S', 'C'), 500, &record));
    ASSERT_FALSE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('M', 'I', 'S', 'C'), 501, &record));
    ASSERT_FALSE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('M', 'I', 'S', 'C'), 502, &record));
    ASSERT_FALSE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('M', 'I', 'S', 'C'), 503, &record));
    ASSERT_FALSE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('M', 'I', 'S', 'C'), 504, &record));
    ASSERT_FALSE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('M', 'I', 'S', 'C'), 505, &record));

    free(bytes);
}

static void test_rejects_truncated_resources(void)
{
    size_t length;
    uint8_t *bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &length);
    u3_resource_file resource_file;

    ASSERT_FALSE(u3_resource_open(bytes, 12, &resource_file));
    ASSERT_FALSE(u3_resource_open(bytes, 180264, &resource_file));
    ASSERT_TRUE(u3_resource_open(bytes, length, &resource_file));

    free(bytes);
}

static void write_u16(uint8_t *bytes, size_t offset, uint16_t value)
{
    bytes[offset] = (uint8_t)(value >> 8);
    bytes[offset + 1] = (uint8_t)(value & 0xFF);
}

static void test_rejects_map_offsets_outside_declared_map(void)
{
    size_t length;
    uint8_t *bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &length);
    u3_resource_file resource_file;

    write_u16(bytes, 180264 + 24, 5621);
    ASSERT_FALSE(u3_resource_open(bytes, length, &resource_file));

    free(bytes);
}

static void test_rejects_wrapped_type_count(void)
{
    size_t length;
    uint8_t *bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &length);
    u3_resource_file resource_file;

    write_u16(bytes, 180292, 0xFFFF);
    ASSERT_FALSE(u3_resource_open(bytes, length, &resource_file));

    free(bytes);
}

int main(void)
{
    test_open_main_resources();
    test_validates_main_resources_from_supplied_resource_root();
    test_missing_supplied_resource_root_fails_without_partial_state();
    test_enumerates_resource_inventory();
    test_extracts_default_save_templates();
    test_characterizes_town_map_monster_and_talk_fixture();
    test_characterizes_ambrosia_map_monster_and_talk_fixture();
    test_characterizes_dungeon_map_fixture();
    test_characterizes_full_combat_screen_fixture();
    test_special_combat_screen_records_are_tile_only_for_full_getscreen_layout();
    test_runtime_save_records_are_not_in_bundled_templates();
    test_rejects_truncated_resources();
    test_rejects_map_offsets_outside_declared_map();
    test_rejects_wrapped_type_count();

    return 0;
}
