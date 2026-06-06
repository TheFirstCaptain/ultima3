#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "u3_resource.h"
#include "u3_save.h"

#define ASSERT_EQ_INT(expected, actual) assert_eq_int((expected), (actual), __FILE__, __LINE__)
#define ASSERT_TRUE(actual) assert_true((actual), __FILE__, __LINE__)
#define ASSERT_FALSE(actual) assert_false((actual), __FILE__, __LINE__)
#define ASSERT_BYTES_EQ(expected, actual, length) assert_bytes_eq((expected), (actual), (length), __FILE__, __LINE__)

typedef struct save_fixture_context {
    uint8_t *resource_bytes;
    u3_resource_file resource_file;
    u3_save_templates templates;
} save_fixture_context;

typedef struct legacy_resource_source {
    uint32_t type;
    int16_t id;
    const uint8_t *data;
    uint32_t length;
} legacy_resource_source;

typedef struct legacy_type_group {
    uint32_t type;
    uint16_t first_record;
    uint16_t record_count;
} legacy_type_group;

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
    size_t index;

    for (index = 0; index < length; index++) {
        if (expected[index] != actual[index]) {
            fprintf(stderr, "%s:%d: byte %zu expected 0x%02x, got 0x%02x\n",
                    file, line, index, expected[index], actual[index]);
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

static int write_file(const char *path, const uint8_t *bytes, size_t length)
{
    FILE *file = fopen(path, "wb");

    if (file == 0)
        return 0;
    if (fwrite(bytes, 1, length, file) != length) {
        fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

static int atomic_write_smoke_document(const char *path, const uint8_t *bytes, size_t length)
{
    char temporary_path[4096];
    uint8_t *staged_bytes;
    size_t staged_length;
    u3_save_document document;
    int path_length;

    if (path == 0 || bytes == 0)
        return 0;
    if (!u3_save_open(bytes, length, &document))
        return 0;

    path_length = snprintf(temporary_path, sizeof(temporary_path), "%s.tmp", path);
    if (path_length < 0 || (size_t)path_length >= sizeof(temporary_path))
        return 0;

    unlink(temporary_path);
    if (!write_file(temporary_path, bytes, length)) {
        unlink(temporary_path);
        return 0;
    }

    staged_bytes = read_file(temporary_path, &staged_length);
    if (!u3_save_open(staged_bytes, staged_length, &document)) {
        free(staged_bytes);
        unlink(temporary_path);
        return 0;
    }
    free(staged_bytes);

    if (rename(temporary_path, path) != 0) {
        unlink(temporary_path);
        return 0;
    }

    return 1;
}

static u3_resource_record require_resource(const u3_resource_file *resource_file, uint32_t type, int16_t id)
{
    u3_resource_record record;

    ASSERT_TRUE(u3_resource_find(resource_file, type, id, &record));
    return record;
}

static void load_save_fixture_context(save_fixture_context *context)
{
    size_t length;
    u3_resource_record record;
    size_t index;

    memset(context, 0, sizeof(*context));
    context->resource_bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &length);
    ASSERT_TRUE(u3_resource_open(context->resource_bytes, length, &context->resource_file));

    record = require_resource(&context->resource_file, U3_RESOURCE_TYPE('P', 'R', 'T', 'Y'), 500);
    context->templates.party = record.data;
    context->templates.party_length = record.length;

    record = require_resource(&context->resource_file, U3_RESOURCE_TYPE('R', 'O', 'S', 'T'), 500);
    context->templates.roster = record.data;
    context->templates.roster_length = record.length;

    record = require_resource(&context->resource_file, U3_RESOURCE_TYPE('M', 'A', 'P', 'S'), 420);
    context->templates.current_sosaria_map = record.data;
    context->templates.current_sosaria_map_length = record.length;

    for (index = 0; index < U3_SAVE_MISC_TABLE_COUNT; index++) {
        record = require_resource(&context->resource_file, U3_RESOURCE_TYPE('M', 'I', 'S', 'C'), (int16_t)(400 + index));
        context->templates.misc[index] = record.data;
        context->templates.misc_length[index] = record.length;
    }
}

static void unload_save_fixture_context(save_fixture_context *context)
{
    free(context->resource_bytes);
}

static void write_u16(uint8_t *bytes, size_t offset, uint16_t value)
{
    bytes[offset] = (uint8_t)(value >> 8);
    bytes[offset + 1] = (uint8_t)(value & 0xFF);
}

static void write_i16(uint8_t *bytes, size_t offset, int16_t value)
{
    write_u16(bytes, offset, (uint16_t)value);
}

static void write_u24(uint8_t *bytes, size_t offset, uint32_t value)
{
    bytes[offset] = (uint8_t)((value >> 16) & 0xFF);
    bytes[offset + 1] = (uint8_t)((value >> 8) & 0xFF);
    bytes[offset + 2] = (uint8_t)(value & 0xFF);
}

static void write_u32(uint8_t *bytes, size_t offset, uint32_t value)
{
    bytes[offset] = (uint8_t)(value >> 24);
    bytes[offset + 1] = (uint8_t)((value >> 16) & 0xFF);
    bytes[offset + 2] = (uint8_t)((value >> 8) & 0xFF);
    bytes[offset + 3] = (uint8_t)(value & 0xFF);
}

static void append_type_group(legacy_type_group *groups, size_t *group_count, uint32_t type, uint16_t first_record, uint16_t record_count)
{
    groups[*group_count].type = type;
    groups[*group_count].first_record = first_record;
    groups[*group_count].record_count = record_count;
    (*group_count)++;
}

static uint8_t *build_legacy_roster_resource_file(const legacy_resource_source *records, uint16_t record_count, size_t *length)
{
    legacy_type_group groups[16];
    size_t group_count = 0;
    uint32_t data_offset = 256;
    uint32_t data_length = 0;
    uint32_t map_offset;
    uint32_t map_length;
    uint32_t type_list_relative_offset = 28;
    uint32_t type_list_length;
    uint32_t ref_list_relative_offset;
    uint32_t name_list_relative_offset;
    size_t total_length;
    uint8_t *bytes;
    uint16_t record_index;
    size_t group_index;
    uint32_t cursor;

    for (record_index = 0; record_index < record_count; record_index++)
        data_length += 4 + records[record_index].length;

    record_index = 0;
    while (record_index < record_count) {
        uint16_t first_record = record_index;
        uint32_t type = records[record_index].type;

        while (record_index < record_count && records[record_index].type == type)
            record_index++;
        append_type_group(groups, &group_count, type, first_record, (uint16_t)(record_index - first_record));
    }

    map_offset = data_offset + data_length;
    type_list_length = 2 + ((uint32_t)group_count * 8);
    ref_list_relative_offset = type_list_relative_offset + type_list_length;
    name_list_relative_offset = ref_list_relative_offset + ((uint32_t)record_count * 12);
    map_length = name_list_relative_offset;
    total_length = map_offset + map_length;
    bytes = (uint8_t *)calloc(total_length, 1);
    if (bytes == 0) {
        fprintf(stderr, "failed to allocate %zu bytes\n", total_length);
        exit(1);
    }

    write_u32(bytes, 0, data_offset);
    write_u32(bytes, 4, map_offset);
    write_u32(bytes, 8, data_length);
    write_u32(bytes, 12, map_length);

    cursor = data_offset;
    for (record_index = 0; record_index < record_count; record_index++) {
        write_u32(bytes, cursor, records[record_index].length);
        memcpy(bytes + cursor + 4, records[record_index].data, records[record_index].length);
        cursor += 4 + records[record_index].length;
    }

    write_u16(bytes, map_offset + 24, (uint16_t)type_list_relative_offset);
    write_u16(bytes, map_offset + 26, (uint16_t)name_list_relative_offset);
    write_u16(bytes, map_offset + type_list_relative_offset, (uint16_t)(group_count - 1));

    for (group_index = 0; group_index < group_count; group_index++) {
        uint32_t type_entry_offset = map_offset + type_list_relative_offset + 2 + ((uint32_t)group_index * 8);
        uint32_t ref_offset = (ref_list_relative_offset - type_list_relative_offset) + ((uint32_t)groups[group_index].first_record * 12);

        write_u32(bytes, type_entry_offset, groups[group_index].type);
        write_u16(bytes, type_entry_offset + 4, (uint16_t)(groups[group_index].record_count - 1));
        write_u16(bytes, type_entry_offset + 6, (uint16_t)ref_offset);
    }

    cursor = 0;
    for (record_index = 0; record_index < record_count; record_index++) {
        uint32_t ref_offset = map_offset + ref_list_relative_offset + ((uint32_t)record_index * 12);

        write_i16(bytes, ref_offset, records[record_index].id);
        write_i16(bytes, ref_offset + 2, -1);
        bytes[ref_offset + 4] = 0;
        write_u24(bytes, ref_offset + 5, cursor);
        cursor += 4 + records[record_index].length;
    }

    *length = total_length;
    return bytes;
}

static uint8_t *build_new_game_fixture(const u3_save_templates *templates, size_t *written)
{
    size_t capacity = u3_save_new_game_fixture_size();
    uint8_t *bytes = (uint8_t *)malloc(capacity);

    if (bytes == 0) {
        fprintf(stderr, "failed to allocate %zu bytes\n", capacity);
        exit(1);
    }

    ASSERT_TRUE(u3_save_build_new_game_fixture(templates, bytes, capacity, written));
    ASSERT_EQ_INT((int)capacity, (int)*written);
    return bytes;
}

static u3_save_record require_save_record(const u3_save_document *document, uint32_t type, int16_t id)
{
    u3_save_record record;

    ASSERT_TRUE(u3_save_find_record(document, type, id, &record));
    return record;
}

static void assert_zero_filled(const uint8_t *bytes, size_t length)
{
    size_t index;

    for (index = 0; index < length; index++)
        ASSERT_EQ_INT(0, bytes[index]);
}

static void test_builds_deterministic_new_game_fixture(void)
{
    save_fixture_context context;
    size_t first_length;
    size_t second_length;
    uint8_t *first;
    uint8_t *second;

    load_save_fixture_context(&context);
    first = build_new_game_fixture(&context.templates, &first_length);
    second = build_new_game_fixture(&context.templates, &second_length);

    ASSERT_EQ_INT((int)first_length, (int)second_length);
    ASSERT_BYTES_EQ(first, second, first_length);

    free(first);
    free(second);
    unload_save_fixture_context(&context);
}

static void test_new_game_fixture_contains_expected_records(void)
{
    static const size_t misc_lengths[U3_SAVE_MISC_TABLE_COUNT] = {16, 11, 11, 11, 64, 16};
    save_fixture_context context;
    size_t fixture_length;
    uint8_t *fixture;
    u3_save_document document;
    u3_save_record record;
    size_t index;

    load_save_fixture_context(&context);
    fixture = build_new_game_fixture(&context.templates, &fixture_length);

    ASSERT_TRUE(u3_save_open(fixture, fixture_length, &document));
    ASSERT_EQ_INT(U3_SAVE_FORMAT_VERSION, document.version);
    ASSERT_EQ_INT(U3_SAVE_NEW_GAME_RECORD_COUNT, document.record_count);

    record = require_save_record(&document, U3_SAVE_TYPE_META, U3_SAVE_ID_METADATA);
    ASSERT_EQ_INT(U3_SAVE_METADATA_LENGTH, (int)record.length);
    ASSERT_EQ_INT('U', record.data[0]);
    ASSERT_EQ_INT('3', record.data[1]);
    ASSERT_EQ_INT('N', record.data[2]);
    ASSERT_EQ_INT('S', record.data[3]);

    record = require_save_record(&document, U3_SAVE_TYPE_PARTY, U3_SAVE_ID_PARTY);
    ASSERT_EQ_INT(U3_SAVE_PARTY_LENGTH, (int)record.length);
    ASSERT_BYTES_EQ(context.templates.party, record.data, U3_SAVE_PARTY_LENGTH);

    record = require_save_record(&document, U3_SAVE_TYPE_ROSTER, U3_SAVE_ID_ROSTER);
    ASSERT_EQ_INT(U3_SAVE_ROSTER_LENGTH, (int)record.length);
    ASSERT_BYTES_EQ(context.templates.roster, record.data, U3_SAVE_ROSTER_LENGTH);

    record = require_save_record(&document, U3_SAVE_TYPE_MAP, U3_SAVE_ID_CURRENT_SOSARIA);
    ASSERT_EQ_INT(U3_SAVE_CURRENT_SOSARIA_MAP_LENGTH, (int)record.length);
    ASSERT_BYTES_EQ(context.templates.current_sosaria_map, record.data, U3_SAVE_CURRENT_SOSARIA_MAP_LENGTH);

    record = require_save_record(&document, U3_SAVE_TYPE_CREATURES, U3_SAVE_ID_CURRENT_SOSARIA);
    ASSERT_EQ_INT(U3_SAVE_CURRENT_SOSARIA_CREATURE_LENGTH, (int)record.length);
    assert_zero_filled(record.data, record.length);

    for (index = 0; index < U3_SAVE_MISC_TABLE_COUNT; index++) {
        record = require_save_record(&document, U3_SAVE_TYPE_MISC, (int16_t)(U3_SAVE_ID_MISC_BASE + index));
        ASSERT_EQ_INT((int)misc_lengths[index], (int)record.length);
        ASSERT_BYTES_EQ(context.templates.misc[index], record.data, misc_lengths[index]);
    }

    free(fixture);
    unload_save_fixture_context(&context);
}

static void test_rejects_invalid_template_lengths(void)
{
    save_fixture_context context;
    size_t written = 99;
    size_t capacity = u3_save_new_game_fixture_size();
    uint8_t *fixture = (uint8_t *)malloc(capacity);

    if (fixture == 0) {
        fprintf(stderr, "failed to allocate %zu bytes\n", capacity);
        exit(1);
    }

    load_save_fixture_context(&context);
    context.templates.party_length = U3_SAVE_PARTY_LENGTH - 1;
    ASSERT_FALSE(u3_save_build_new_game_fixture(&context.templates, fixture, capacity, &written));
    ASSERT_EQ_INT(0, (int)written);

    free(fixture);
    unload_save_fixture_context(&context);
}

static void test_rejects_short_output_buffer(void)
{
    save_fixture_context context;
    size_t written = 99;
    size_t capacity = u3_save_new_game_fixture_size();
    uint8_t *fixture = (uint8_t *)malloc(capacity - 1);

    if (fixture == 0) {
        fprintf(stderr, "failed to allocate %zu bytes\n", capacity - 1);
        exit(1);
    }

    load_save_fixture_context(&context);
    ASSERT_FALSE(u3_save_build_new_game_fixture(&context.templates, fixture, capacity - 1, &written));
    ASSERT_EQ_INT(0, (int)written);

    free(fixture);
    unload_save_fixture_context(&context);
}

static void test_rejects_malformed_save_documents(void)
{
    save_fixture_context context;
    size_t fixture_length;
    uint8_t *fixture;
    u3_save_document document;

    load_save_fixture_context(&context);
    fixture = build_new_game_fixture(&context.templates, &fixture_length);

    ASSERT_FALSE(u3_save_open(fixture, fixture_length - 1, &document));

    fixture[0] = 'X';
    ASSERT_FALSE(u3_save_open(fixture, fixture_length, &document));
    fixture[0] = 'U';

    write_u16(fixture, 4, U3_SAVE_FORMAT_VERSION + 1);
    ASSERT_FALSE(u3_save_open(fixture, fixture_length, &document));
    write_u16(fixture, 4, U3_SAVE_FORMAT_VERSION);

    write_u32(fixture, 16 + 8, (uint32_t)fixture_length);
    write_u32(fixture, 16 + 12, 1);
    ASSERT_FALSE(u3_save_open(fixture, fixture_length, &document));

    free(fixture);
    unload_save_fixture_context(&context);
}

static void test_rejects_noncanonical_save_payload_layouts(void)
{
    save_fixture_context context;
    size_t fixture_length;
    uint8_t *fixture;
    uint8_t *fixture_with_trailing_byte;
    u3_save_document document;
    uint32_t data_offset = 16 + (U3_SAVE_NEW_GAME_RECORD_COUNT * 16);

    load_save_fixture_context(&context);
    fixture = build_new_game_fixture(&context.templates, &fixture_length);

    write_u32(fixture, 32 + 8, data_offset);
    ASSERT_FALSE(u3_save_open(fixture, fixture_length, &document));

    free(fixture);
    fixture = build_new_game_fixture(&context.templates, &fixture_length);
    write_u32(fixture, 32 + 8, data_offset + U3_SAVE_METADATA_LENGTH + 1);
    ASSERT_FALSE(u3_save_open(fixture, fixture_length, &document));

    free(fixture);
    fixture = build_new_game_fixture(&context.templates, &fixture_length);
    fixture_with_trailing_byte = (uint8_t *)malloc(fixture_length + 1);
    if (fixture_with_trailing_byte == 0) {
        fprintf(stderr, "failed to allocate %zu bytes\n", fixture_length + 1);
        exit(1);
    }
    memcpy(fixture_with_trailing_byte, fixture, fixture_length);
    fixture_with_trailing_byte[fixture_length] = 0;
    ASSERT_FALSE(u3_save_open(fixture_with_trailing_byte, fixture_length + 1, &document));

    free(fixture);
    free(fixture_with_trailing_byte);
    unload_save_fixture_context(&context);
}

static void test_get_record_rejects_forged_document_directory_bounds(void)
{
    save_fixture_context context;
    size_t fixture_length;
    uint8_t *fixture;
    u3_save_document forged_document;
    u3_save_record record;

    load_save_fixture_context(&context);
    fixture = build_new_game_fixture(&context.templates, &fixture_length);

    forged_document.bytes = fixture;
    forged_document.length = 20;
    forged_document.version = U3_SAVE_FORMAT_VERSION;
    forged_document.record_count = 2;
    forged_document.data_offset = 16 + (U3_SAVE_NEW_GAME_RECORD_COUNT * 16);

    ASSERT_FALSE(u3_save_get_record(&forged_document, 1, &record));

    free(fixture);
    unload_save_fixture_context(&context);
}

static void test_atomic_write_round_trips_valid_smoke_document(void)
{
    save_fixture_context context;
    size_t fixture_length;
    size_t readback_length;
    uint8_t *fixture;
    uint8_t *readback;
    u3_save_document document;
    char path[256];

    snprintf(path, sizeof(path), "/tmp/ultima3_save_fixture_roundtrip_%ld.u3save", (long)getpid());
    unlink(path);

    load_save_fixture_context(&context);
    fixture = build_new_game_fixture(&context.templates, &fixture_length);

    ASSERT_TRUE(atomic_write_smoke_document(path, fixture, fixture_length));
    readback = read_file(path, &readback_length);
    ASSERT_EQ_INT((int)fixture_length, (int)readback_length);
    ASSERT_BYTES_EQ(fixture, readback, fixture_length);
    ASSERT_TRUE(u3_save_open(readback, readback_length, &document));

    free(fixture);
    free(readback);
    unlink(path);
    unload_save_fixture_context(&context);
}

static void test_atomic_write_rejects_invalid_replacement_and_retains_previous_document(void)
{
    save_fixture_context context;
    size_t fixture_length;
    size_t readback_length;
    uint8_t *fixture;
    uint8_t *invalid_fixture;
    uint8_t *readback;
    char path[256];

    snprintf(path, sizeof(path), "/tmp/ultima3_save_fixture_retention_%ld.u3save", (long)getpid());
    unlink(path);

    load_save_fixture_context(&context);
    fixture = build_new_game_fixture(&context.templates, &fixture_length);
    invalid_fixture = (uint8_t *)malloc(fixture_length);
    if (invalid_fixture == 0) {
        fprintf(stderr, "failed to allocate %zu bytes\n", fixture_length);
        exit(1);
    }
    memcpy(invalid_fixture, fixture, fixture_length);
    invalid_fixture[0] = 'X';

    ASSERT_TRUE(atomic_write_smoke_document(path, fixture, fixture_length));
    ASSERT_FALSE(atomic_write_smoke_document(path, invalid_fixture, fixture_length));

    readback = read_file(path, &readback_length);
    ASSERT_EQ_INT((int)fixture_length, (int)readback_length);
    ASSERT_BYTES_EQ(fixture, readback, fixture_length);

    free(fixture);
    free(invalid_fixture);
    free(readback);
    unlink(path);
    unload_save_fixture_context(&context);
}

static uint8_t *build_synthesized_legacy_roster(const save_fixture_context *context,
                                                int include_party,
                                                uint32_t party_length,
                                                int malformed_misc_index,
                                                size_t *length,
                                                uint8_t *mutable_creatures,
                                                uint8_t *compatibility_pref)
{
    legacy_resource_source records[16];
    uint16_t record_count = 0;
    size_t index;

    if (include_party) {
        records[record_count++] = (legacy_resource_source){U3_SAVE_TYPE_PARTY, U3_SAVE_ID_PARTY, context->templates.party, party_length};
    }
    records[record_count++] = (legacy_resource_source){U3_SAVE_TYPE_ROSTER, U3_SAVE_ID_ROSTER, context->templates.roster, U3_SAVE_ROSTER_LENGTH};
    records[record_count++] = (legacy_resource_source){U3_SAVE_TYPE_MAP, U3_SAVE_ID_CURRENT_SOSARIA, context->templates.current_sosaria_map, U3_SAVE_CURRENT_SOSARIA_MAP_LENGTH};
    records[record_count++] = (legacy_resource_source){U3_SAVE_TYPE_CREATURES, U3_SAVE_ID_CURRENT_SOSARIA, mutable_creatures, U3_SAVE_CURRENT_SOSARIA_CREATURE_LENGTH};
    for (index = 0; index < U3_SAVE_MISC_TABLE_COUNT; index++) {
        records[record_count++] = (legacy_resource_source){
            U3_SAVE_TYPE_MISC,
            (int16_t)(U3_SAVE_ID_MISC_BASE + index),
            context->templates.misc[index],
            (uint32_t)(malformed_misc_index == (int)index ? context->templates.misc_length[index] - 1 : context->templates.misc_length[index])
        };
    }
    records[record_count++] = (legacy_resource_source){U3_SAVE_RECORD_TYPE('P', 'R', 'E', 'F'), 400, compatibility_pref, 32};

    return build_legacy_roster_resource_file(records, record_count, length);
}

static uint8_t *import_legacy_roster_resource(const uint8_t *legacy_bytes, size_t legacy_length, size_t *native_length)
{
    u3_resource_file legacy_resource_file;
    size_t capacity;
    uint8_t *native_bytes;

    ASSERT_TRUE(u3_resource_open(legacy_bytes, legacy_length, &legacy_resource_file));
    capacity = u3_save_legacy_roster_import_size(&legacy_resource_file);
    ASSERT_TRUE(capacity > 0);
    native_bytes = (uint8_t *)malloc(capacity);
    if (native_bytes == 0) {
        fprintf(stderr, "failed to allocate %zu bytes\n", capacity);
        exit(1);
    }
    ASSERT_TRUE(u3_save_build_legacy_roster_import(&legacy_resource_file, native_bytes, capacity, native_length));
    ASSERT_EQ_INT((int)capacity, (int)*native_length);
    return native_bytes;
}

static void test_imports_synthesized_legacy_roster_fixture(void)
{
    save_fixture_context context;
    uint8_t mutable_creatures[U3_SAVE_CURRENT_SOSARIA_CREATURE_LENGTH];
    uint8_t compatibility_pref[32];
    uint8_t *legacy_bytes;
    uint8_t *native_bytes;
    size_t legacy_length;
    size_t native_length;
    size_t index;
    u3_save_document document;
    u3_save_record record;

    load_save_fixture_context(&context);
    for (index = 0; index < sizeof(mutable_creatures); index++)
        mutable_creatures[index] = (uint8_t)(index ^ 0xA5);
    for (index = 0; index < sizeof(compatibility_pref); index++)
        compatibility_pref[index] = (uint8_t)(index + 1);

    legacy_bytes = build_synthesized_legacy_roster(&context, 1, U3_SAVE_PARTY_LENGTH, -1, &legacy_length, mutable_creatures, compatibility_pref);
    native_bytes = import_legacy_roster_resource(legacy_bytes, legacy_length, &native_length);

    ASSERT_TRUE(u3_save_open(native_bytes, native_length, &document));
    ASSERT_EQ_INT(U3_SAVE_LEGACY_REQUIRED_RECORD_COUNT + 2, document.record_count);

    record = require_save_record(&document, U3_SAVE_TYPE_PARTY, U3_SAVE_ID_PARTY);
    ASSERT_BYTES_EQ(context.templates.party, record.data, U3_SAVE_PARTY_LENGTH);
    record = require_save_record(&document, U3_SAVE_TYPE_ROSTER, U3_SAVE_ID_ROSTER);
    ASSERT_BYTES_EQ(context.templates.roster, record.data, U3_SAVE_ROSTER_LENGTH);
    record = require_save_record(&document, U3_SAVE_TYPE_MAP, U3_SAVE_ID_CURRENT_SOSARIA);
    ASSERT_BYTES_EQ(context.templates.current_sosaria_map, record.data, U3_SAVE_CURRENT_SOSARIA_MAP_LENGTH);
    record = require_save_record(&document, U3_SAVE_TYPE_CREATURES, U3_SAVE_ID_CURRENT_SOSARIA);
    ASSERT_BYTES_EQ(mutable_creatures, record.data, sizeof(mutable_creatures));
    for (index = 0; index < U3_SAVE_MISC_TABLE_COUNT; index++) {
        record = require_save_record(&document, U3_SAVE_TYPE_MISC, (int16_t)(U3_SAVE_ID_MISC_BASE + index));
        ASSERT_BYTES_EQ(context.templates.misc[index], record.data, context.templates.misc_length[index]);
    }
    record = require_save_record(&document, U3_SAVE_RECORD_TYPE('P', 'R', 'E', 'F'), 400);
    ASSERT_BYTES_EQ(compatibility_pref, record.data, sizeof(compatibility_pref));

    free(legacy_bytes);
    free(native_bytes);
    unload_save_fixture_context(&context);
}

static void test_legacy_import_rejects_missing_required_record(void)
{
    save_fixture_context context;
    uint8_t mutable_creatures[U3_SAVE_CURRENT_SOSARIA_CREATURE_LENGTH];
    uint8_t compatibility_pref[32];
    uint8_t *legacy_bytes;
    size_t legacy_length;
    size_t written = 99;
    u3_resource_file legacy_resource_file;
    uint8_t output[64];

    memset(mutable_creatures, 0x5A, sizeof(mutable_creatures));
    memset(compatibility_pref, 0x01, sizeof(compatibility_pref));
    load_save_fixture_context(&context);
    legacy_bytes = build_synthesized_legacy_roster(&context, 0, U3_SAVE_PARTY_LENGTH, -1, &legacy_length, mutable_creatures, compatibility_pref);

    ASSERT_TRUE(u3_resource_open(legacy_bytes, legacy_length, &legacy_resource_file));
    ASSERT_EQ_INT(0, (int)u3_save_legacy_roster_import_size(&legacy_resource_file));
    ASSERT_FALSE(u3_save_build_legacy_roster_import(&legacy_resource_file, output, sizeof(output), &written));
    ASSERT_EQ_INT(0, (int)written);

    free(legacy_bytes);
    unload_save_fixture_context(&context);
}

static void test_legacy_import_rejects_malformed_required_record(void)
{
    save_fixture_context context;
    uint8_t mutable_creatures[U3_SAVE_CURRENT_SOSARIA_CREATURE_LENGTH];
    uint8_t compatibility_pref[32];
    uint8_t *legacy_bytes;
    size_t legacy_length;
    size_t written = 99;
    u3_resource_file legacy_resource_file;
    uint8_t output[64];

    memset(mutable_creatures, 0x6B, sizeof(mutable_creatures));
    memset(compatibility_pref, 0x02, sizeof(compatibility_pref));
    load_save_fixture_context(&context);
    legacy_bytes = build_synthesized_legacy_roster(&context, 1, U3_SAVE_PARTY_LENGTH - 1, -1, &legacy_length, mutable_creatures, compatibility_pref);

    ASSERT_TRUE(u3_resource_open(legacy_bytes, legacy_length, &legacy_resource_file));
    ASSERT_EQ_INT(0, (int)u3_save_legacy_roster_import_size(&legacy_resource_file));
    ASSERT_FALSE(u3_save_build_legacy_roster_import(&legacy_resource_file, output, sizeof(output), &written));
    ASSERT_EQ_INT(0, (int)written);

    free(legacy_bytes);
    unload_save_fixture_context(&context);
}

static void test_legacy_import_rejects_malformed_misc_record(void)
{
    save_fixture_context context;
    uint8_t mutable_creatures[U3_SAVE_CURRENT_SOSARIA_CREATURE_LENGTH];
    uint8_t compatibility_pref[32];
    uint8_t *legacy_bytes;
    size_t legacy_length;
    size_t written = 99;
    u3_resource_file legacy_resource_file;
    uint8_t output[64];

    memset(mutable_creatures, 0x7C, sizeof(mutable_creatures));
    memset(compatibility_pref, 0x03, sizeof(compatibility_pref));
    load_save_fixture_context(&context);
    legacy_bytes = build_synthesized_legacy_roster(&context, 1, U3_SAVE_PARTY_LENGTH, 2, &legacy_length, mutable_creatures, compatibility_pref);

    ASSERT_TRUE(u3_resource_open(legacy_bytes, legacy_length, &legacy_resource_file));
    ASSERT_EQ_INT(0, (int)u3_save_legacy_roster_import_size(&legacy_resource_file));
    ASSERT_FALSE(u3_save_build_legacy_roster_import(&legacy_resource_file, output, sizeof(output), &written));
    ASSERT_EQ_INT(0, (int)written);

    free(legacy_bytes);
    unload_save_fixture_context(&context);
}

int main(void)
{
    test_builds_deterministic_new_game_fixture();
    test_new_game_fixture_contains_expected_records();
    test_rejects_invalid_template_lengths();
    test_rejects_short_output_buffer();
    test_rejects_malformed_save_documents();
    test_rejects_noncanonical_save_payload_layouts();
    test_get_record_rejects_forged_document_directory_bounds();
    test_atomic_write_round_trips_valid_smoke_document();
    test_atomic_write_rejects_invalid_replacement_and_retains_previous_document();
    test_imports_synthesized_legacy_roster_fixture();
    test_legacy_import_rejects_missing_required_record();
    test_legacy_import_rejects_malformed_required_record();
    test_legacy_import_rejects_malformed_misc_record();

    return 0;
}
