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
    test_extracts_default_save_templates();
    test_runtime_save_records_are_not_in_bundled_templates();
    test_rejects_truncated_resources();
    test_rejects_map_offsets_outside_declared_map();
    test_rejects_wrapped_type_count();

    return 0;
}
