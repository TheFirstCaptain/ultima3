#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_resource.h"

#define ASSERT_EQ_INT(expected, actual) assert_eq_int((expected), (actual), __FILE__, __LINE__)
#define ASSERT_TRUE(actual) assert_true((actual), __FILE__, __LINE__)
#define ASSERT_FALSE(actual) assert_false((actual), __FILE__, __LINE__)
#define ASSERT_BYTES_EQ(expected, actual, length) assert_bytes_eq((expected), (actual), (length), __FILE__, __LINE__)

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

static uint8_t *read_file(const char *path, size_t *length)
{
    FILE *file;
    long file_length;
    uint8_t *bytes;

    file = fopen(path, "rb");
    if (file == 0) {
        perror(path);
        exit(1);
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        perror("fseek");
        exit(1);
    }
    file_length = ftell(file);
    if (file_length < 0) {
        perror("ftell");
        exit(1);
    }
    rewind(file);

    bytes = (uint8_t *)malloc((size_t)file_length);
    if (bytes == 0) {
        fprintf(stderr, "failed to allocate %ld bytes\n", file_length);
        exit(1);
    }
    if (fread(bytes, 1, (size_t)file_length, file) != (size_t)file_length) {
        perror("fread");
        exit(1);
    }
    fclose(file);

    *length = (size_t)file_length;
    return bytes;
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
    test_enumerates_resource_inventory();
    test_extracts_default_save_templates();
    test_runtime_save_records_are_not_in_bundled_templates();
    test_rejects_truncated_resources();
    test_rejects_map_offsets_outside_declared_map();
    test_rejects_wrapped_type_count();

    return 0;
}
