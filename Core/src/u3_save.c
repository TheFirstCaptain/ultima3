#include "u3_save.h"

#include <stdbool.h>
#include <string.h>

#define U3_SAVE_HEADER_LENGTH 16
#define U3_SAVE_RECORD_HEADER_LENGTH 16
#define U3_SAVE_MAGIC_0 'U'
#define U3_SAVE_MAGIC_1 '3'
#define U3_SAVE_MAGIC_2 'S'
#define U3_SAVE_MAGIC_3 'V'
#define U3_SAVE_LEGACY_IMPORT_MAX_RECORDS 64

typedef struct u3_save_record_source {
    uint32_t type;
    int16_t id;
    const uint8_t *data;
    uint32_t length;
    bool zero_fill;
} u3_save_record_source;

static const uint32_t u3_save_misc_lengths[U3_SAVE_MISC_TABLE_COUNT] = {
    U3_SAVE_MISC_MOONGATE_LENGTH,
    U3_SAVE_MISC_TYPE_INITIAL_LENGTH,
    U3_SAVE_MISC_WEAPON_USE_LENGTH,
    U3_SAVE_MISC_ARMOUR_USE_LENGTH,
    U3_SAVE_MISC_LOCATION_LENGTH,
    U3_SAVE_MISC_EXPERIENCE_LENGTH
};

static bool u3_save_range_valid(size_t length, uint32_t offset, uint32_t range_length)
{
    return offset <= length && range_length <= length - offset;
}

static uint16_t u3_save_read_u16(const uint8_t *bytes, uint32_t offset)
{
    return (uint16_t)(((uint16_t)bytes[offset] << 8) | bytes[offset + 1]);
}

static int16_t u3_save_read_i16(const uint8_t *bytes, uint32_t offset)
{
    return (int16_t)u3_save_read_u16(bytes, offset);
}

static uint32_t u3_save_read_u32(const uint8_t *bytes, uint32_t offset)
{
    return ((uint32_t)bytes[offset] << 24) | ((uint32_t)bytes[offset + 1] << 16) | ((uint32_t)bytes[offset + 2] << 8) | bytes[offset + 3];
}

static void u3_save_write_u16(uint8_t *bytes, uint32_t offset, uint16_t value)
{
    bytes[offset] = (uint8_t)(value >> 8);
    bytes[offset + 1] = (uint8_t)(value & 0xFF);
}

static void u3_save_write_i16(uint8_t *bytes, uint32_t offset, int16_t value)
{
    u3_save_write_u16(bytes, offset, (uint16_t)value);
}

static void u3_save_write_u32(uint8_t *bytes, uint32_t offset, uint32_t value)
{
    bytes[offset] = (uint8_t)(value >> 24);
    bytes[offset + 1] = (uint8_t)((value >> 16) & 0xFF);
    bytes[offset + 2] = (uint8_t)((value >> 8) & 0xFF);
    bytes[offset + 3] = (uint8_t)(value & 0xFF);
}

static bool u3_save_template_valid(const uint8_t *data, size_t length, size_t expected_length)
{
    return data != 0 && length == expected_length;
}

static bool u3_save_resource_record_is_required(uint32_t type, int16_t id)
{
    if (type == U3_SAVE_TYPE_PARTY && id == U3_SAVE_ID_PARTY)
        return true;
    if (type == U3_SAVE_TYPE_ROSTER && id == U3_SAVE_ID_ROSTER)
        return true;
    if (type == U3_SAVE_TYPE_MAP && id == U3_SAVE_ID_CURRENT_SOSARIA)
        return true;
    if (type == U3_SAVE_TYPE_CREATURES && id == U3_SAVE_ID_CURRENT_SOSARIA)
        return true;
    if (type == U3_SAVE_TYPE_MISC && id >= U3_SAVE_ID_MISC_BASE && id < U3_SAVE_ID_MISC_BASE + U3_SAVE_MISC_TABLE_COUNT)
        return true;

    return false;
}

static bool u3_save_templates_valid(const u3_save_templates *templates)
{
    size_t index;

    if (templates == 0)
        return false;
    if (!u3_save_template_valid(templates->party, templates->party_length, U3_SAVE_PARTY_LENGTH))
        return false;
    if (!u3_save_template_valid(templates->roster, templates->roster_length, U3_SAVE_ROSTER_LENGTH))
        return false;
    if (!u3_save_template_valid(templates->current_sosaria_map, templates->current_sosaria_map_length, U3_SAVE_CURRENT_SOSARIA_MAP_LENGTH))
        return false;

    for (index = 0; index < U3_SAVE_MISC_TABLE_COUNT; index++) {
        if (!u3_save_template_valid(templates->misc[index], templates->misc_length[index], u3_save_misc_lengths[index]))
            return false;
    }

    return true;
}

static void u3_save_make_metadata(uint8_t *metadata, uint16_t record_count)
{
    memset(metadata, 0, U3_SAVE_METADATA_LENGTH);
    metadata[0] = (uint8_t)'U';
    metadata[1] = (uint8_t)'3';
    metadata[2] = (uint8_t)'N';
    metadata[3] = (uint8_t)'S';
    u3_save_write_u16(metadata, 4, U3_SAVE_FORMAT_VERSION);
    u3_save_write_u16(metadata, 6, record_count);
}

size_t u3_save_new_game_fixture_size(void)
{
    return U3_SAVE_HEADER_LENGTH +
           (U3_SAVE_NEW_GAME_RECORD_COUNT * U3_SAVE_RECORD_HEADER_LENGTH) +
           U3_SAVE_METADATA_LENGTH +
           U3_SAVE_PARTY_LENGTH +
           U3_SAVE_ROSTER_LENGTH +
           U3_SAVE_CURRENT_SOSARIA_MAP_LENGTH +
           U3_SAVE_CURRENT_SOSARIA_CREATURE_LENGTH +
           U3_SAVE_MISC_MOONGATE_LENGTH +
           U3_SAVE_MISC_TYPE_INITIAL_LENGTH +
           U3_SAVE_MISC_WEAPON_USE_LENGTH +
           U3_SAVE_MISC_ARMOUR_USE_LENGTH +
           U3_SAVE_MISC_LOCATION_LENGTH +
           U3_SAVE_MISC_EXPERIENCE_LENGTH;
}

int u3_save_build_new_game_fixture(const u3_save_templates *templates, uint8_t *bytes, size_t capacity, size_t *written)
{
    uint8_t metadata[U3_SAVE_METADATA_LENGTH];
    u3_save_record_source records[U3_SAVE_NEW_GAME_RECORD_COUNT];
    uint32_t data_offset = U3_SAVE_HEADER_LENGTH + (U3_SAVE_NEW_GAME_RECORD_COUNT * U3_SAVE_RECORD_HEADER_LENGTH);
    uint32_t cursor = data_offset;
    uint16_t index;

    if (written != 0)
        *written = 0;
    if (bytes == 0 || capacity < u3_save_new_game_fixture_size())
        return 0;
    if (!u3_save_templates_valid(templates))
        return 0;

    u3_save_make_metadata(metadata, U3_SAVE_NEW_GAME_RECORD_COUNT);

    records[0] = (u3_save_record_source){U3_SAVE_TYPE_META, U3_SAVE_ID_METADATA, metadata, U3_SAVE_METADATA_LENGTH, false};
    records[1] = (u3_save_record_source){U3_SAVE_TYPE_PARTY, U3_SAVE_ID_PARTY, templates->party, U3_SAVE_PARTY_LENGTH, false};
    records[2] = (u3_save_record_source){U3_SAVE_TYPE_ROSTER, U3_SAVE_ID_ROSTER, templates->roster, U3_SAVE_ROSTER_LENGTH, false};
    records[3] = (u3_save_record_source){U3_SAVE_TYPE_MAP, U3_SAVE_ID_CURRENT_SOSARIA, templates->current_sosaria_map, U3_SAVE_CURRENT_SOSARIA_MAP_LENGTH, false};
    records[4] = (u3_save_record_source){U3_SAVE_TYPE_CREATURES, U3_SAVE_ID_CURRENT_SOSARIA, 0, U3_SAVE_CURRENT_SOSARIA_CREATURE_LENGTH, true};
    records[5] = (u3_save_record_source){U3_SAVE_TYPE_MISC, U3_SAVE_ID_MISC_BASE, templates->misc[0], u3_save_misc_lengths[0], false};
    records[6] = (u3_save_record_source){U3_SAVE_TYPE_MISC, U3_SAVE_ID_MISC_BASE + 1, templates->misc[1], u3_save_misc_lengths[1], false};
    records[7] = (u3_save_record_source){U3_SAVE_TYPE_MISC, U3_SAVE_ID_MISC_BASE + 2, templates->misc[2], u3_save_misc_lengths[2], false};
    records[8] = (u3_save_record_source){U3_SAVE_TYPE_MISC, U3_SAVE_ID_MISC_BASE + 3, templates->misc[3], u3_save_misc_lengths[3], false};
    records[9] = (u3_save_record_source){U3_SAVE_TYPE_MISC, U3_SAVE_ID_MISC_BASE + 4, templates->misc[4], u3_save_misc_lengths[4], false};
    records[10] = (u3_save_record_source){U3_SAVE_TYPE_MISC, U3_SAVE_ID_MISC_BASE + 5, templates->misc[5], u3_save_misc_lengths[5], false};

    memset(bytes, 0, u3_save_new_game_fixture_size());
    bytes[0] = (uint8_t)U3_SAVE_MAGIC_0;
    bytes[1] = (uint8_t)U3_SAVE_MAGIC_1;
    bytes[2] = (uint8_t)U3_SAVE_MAGIC_2;
    bytes[3] = (uint8_t)U3_SAVE_MAGIC_3;
    u3_save_write_u16(bytes, 4, U3_SAVE_FORMAT_VERSION);
    u3_save_write_u16(bytes, 6, U3_SAVE_NEW_GAME_RECORD_COUNT);
    u3_save_write_u32(bytes, 8, data_offset);

    for (index = 0; index < U3_SAVE_NEW_GAME_RECORD_COUNT; index++) {
        uint32_t record_offset = U3_SAVE_HEADER_LENGTH + ((uint32_t)index * U3_SAVE_RECORD_HEADER_LENGTH);

        u3_save_write_u32(bytes, record_offset, records[index].type);
        u3_save_write_i16(bytes, record_offset + 4, records[index].id);
        u3_save_write_u16(bytes, record_offset + 6, 0);
        u3_save_write_u32(bytes, record_offset + 8, cursor);
        u3_save_write_u32(bytes, record_offset + 12, records[index].length);

        if (records[index].zero_fill) {
            memset(bytes + cursor, 0, records[index].length);
        } else {
            memcpy(bytes + cursor, records[index].data, records[index].length);
        }
        cursor += records[index].length;
    }

    if (written != 0)
        *written = cursor;
    return 1;
}

static int u3_save_require_resource(const u3_resource_file *resource_file,
                                    uint32_t type,
                                    int16_t id,
                                    uint32_t expected_length,
                                    u3_resource_record *record)
{
    if (!u3_resource_find(resource_file, type, id, record))
        return 0;
    return record->length == expected_length;
}

static int u3_save_count_unknown_resources(const u3_resource_file *resource_file, uint16_t *unknown_count, size_t *unknown_length)
{
    uint32_t type_index;

    if (resource_file == 0 || unknown_count == 0 || unknown_length == 0)
        return 0;

    *unknown_count = 0;
    *unknown_length = 0;
    for (type_index = 0; type_index < resource_file->type_count; type_index++) {
        uint32_t type;
        uint32_t record_count;
        uint32_t record_index;

        if (!u3_resource_get_type(resource_file, type_index, &type, &record_count))
            return 0;
        for (record_index = 0; record_index < record_count; record_index++) {
            u3_resource_record record;

            if (!u3_resource_get_record(resource_file, type_index, record_index, &record))
                return 0;
            if (u3_save_resource_record_is_required(record.type, record.id))
                continue;
            if (*unknown_count == UINT16_MAX)
                return 0;
            if (record.length > SIZE_MAX - *unknown_length)
                return 0;
            (*unknown_count)++;
            *unknown_length += record.length;
        }
    }

    return 1;
}

static int u3_save_legacy_required_resources_valid(const u3_resource_file *resource_file)
{
    u3_resource_record record;
    size_t misc_index;

    if (!u3_save_require_resource(resource_file, U3_SAVE_TYPE_PARTY, U3_SAVE_ID_PARTY, U3_SAVE_PARTY_LENGTH, &record))
        return 0;
    if (!u3_save_require_resource(resource_file, U3_SAVE_TYPE_ROSTER, U3_SAVE_ID_ROSTER, U3_SAVE_ROSTER_LENGTH, &record))
        return 0;
    if (!u3_save_require_resource(resource_file, U3_SAVE_TYPE_MAP, U3_SAVE_ID_CURRENT_SOSARIA, U3_SAVE_CURRENT_SOSARIA_MAP_LENGTH, &record))
        return 0;
    if (!u3_save_require_resource(resource_file, U3_SAVE_TYPE_CREATURES, U3_SAVE_ID_CURRENT_SOSARIA, U3_SAVE_CURRENT_SOSARIA_CREATURE_LENGTH, &record))
        return 0;

    for (misc_index = 0; misc_index < U3_SAVE_MISC_TABLE_COUNT; misc_index++) {
        if (!u3_save_require_resource(resource_file, U3_SAVE_TYPE_MISC, (int16_t)(U3_SAVE_ID_MISC_BASE + misc_index), u3_save_misc_lengths[misc_index], &record))
            return 0;
    }

    return 1;
}

static int u3_save_add_record_source(u3_save_record_source *records,
                                     uint16_t record_capacity,
                                     uint16_t *record_count,
                                     uint32_t type,
                                     int16_t id,
                                     const uint8_t *data,
                                     uint32_t length)
{
    if (*record_count >= record_capacity)
        return 0;

    records[*record_count] = (u3_save_record_source){type, id, data, length, false};
    (*record_count)++;
    return 1;
}

size_t u3_save_legacy_roster_import_size(const u3_resource_file *resource_file)
{
    uint16_t unknown_count;
    size_t unknown_length;

    if (!u3_save_count_unknown_resources(resource_file, &unknown_count, &unknown_length))
        return 0;
    if (!u3_save_legacy_required_resources_valid(resource_file))
        return 0;
    if ((size_t)1 + U3_SAVE_LEGACY_REQUIRED_RECORD_COUNT + unknown_count > U3_SAVE_LEGACY_IMPORT_MAX_RECORDS)
        return 0;

    return U3_SAVE_HEADER_LENGTH +
           ((size_t)(1 + U3_SAVE_LEGACY_REQUIRED_RECORD_COUNT + unknown_count) * U3_SAVE_RECORD_HEADER_LENGTH) +
           U3_SAVE_METADATA_LENGTH +
           U3_SAVE_PARTY_LENGTH +
           U3_SAVE_ROSTER_LENGTH +
           U3_SAVE_CURRENT_SOSARIA_MAP_LENGTH +
           U3_SAVE_CURRENT_SOSARIA_CREATURE_LENGTH +
           U3_SAVE_MISC_MOONGATE_LENGTH +
           U3_SAVE_MISC_TYPE_INITIAL_LENGTH +
           U3_SAVE_MISC_WEAPON_USE_LENGTH +
           U3_SAVE_MISC_ARMOUR_USE_LENGTH +
           U3_SAVE_MISC_LOCATION_LENGTH +
           U3_SAVE_MISC_EXPERIENCE_LENGTH +
           unknown_length;
}

int u3_save_build_legacy_roster_import(const u3_resource_file *resource_file, uint8_t *bytes, size_t capacity, size_t *written)
{
    uint8_t metadata[U3_SAVE_METADATA_LENGTH];
    u3_resource_record resource_record;
    u3_save_record_source records[U3_SAVE_LEGACY_IMPORT_MAX_RECORDS];
    uint16_t record_count = 0;
    uint16_t unknown_count;
    size_t unknown_length;
    size_t required_size;
    uint32_t data_offset;
    uint32_t cursor;
    uint16_t index;
    uint32_t type_index;
    size_t misc_index;

    if (written != 0)
        *written = 0;
    if (resource_file == 0 || resource_file->bytes == 0 || bytes == 0)
        return 0;
    if (!u3_save_count_unknown_resources(resource_file, &unknown_count, &unknown_length))
        return 0;
    (void)unknown_length;
    if ((size_t)1 + U3_SAVE_LEGACY_REQUIRED_RECORD_COUNT + unknown_count > sizeof(records) / sizeof(records[0]))
        return 0;

    required_size = u3_save_legacy_roster_import_size(resource_file);
    if (required_size == 0 || capacity < required_size)
        return 0;

    u3_save_make_metadata(metadata, (uint16_t)(1 + U3_SAVE_LEGACY_REQUIRED_RECORD_COUNT + unknown_count));
    if (!u3_save_add_record_source(records, (uint16_t)(sizeof(records) / sizeof(records[0])), &record_count, U3_SAVE_TYPE_META, U3_SAVE_ID_METADATA, metadata, U3_SAVE_METADATA_LENGTH))
        return 0;

    if (!u3_save_require_resource(resource_file, U3_SAVE_TYPE_PARTY, U3_SAVE_ID_PARTY, U3_SAVE_PARTY_LENGTH, &resource_record))
        return 0;
    if (!u3_save_add_record_source(records, (uint16_t)(sizeof(records) / sizeof(records[0])), &record_count, U3_SAVE_TYPE_PARTY, U3_SAVE_ID_PARTY, resource_record.data, resource_record.length))
        return 0;

    if (!u3_save_require_resource(resource_file, U3_SAVE_TYPE_ROSTER, U3_SAVE_ID_ROSTER, U3_SAVE_ROSTER_LENGTH, &resource_record))
        return 0;
    if (!u3_save_add_record_source(records, (uint16_t)(sizeof(records) / sizeof(records[0])), &record_count, U3_SAVE_TYPE_ROSTER, U3_SAVE_ID_ROSTER, resource_record.data, resource_record.length))
        return 0;

    if (!u3_save_require_resource(resource_file, U3_SAVE_TYPE_MAP, U3_SAVE_ID_CURRENT_SOSARIA, U3_SAVE_CURRENT_SOSARIA_MAP_LENGTH, &resource_record))
        return 0;
    if (!u3_save_add_record_source(records, (uint16_t)(sizeof(records) / sizeof(records[0])), &record_count, U3_SAVE_TYPE_MAP, U3_SAVE_ID_CURRENT_SOSARIA, resource_record.data, resource_record.length))
        return 0;

    if (!u3_save_require_resource(resource_file, U3_SAVE_TYPE_CREATURES, U3_SAVE_ID_CURRENT_SOSARIA, U3_SAVE_CURRENT_SOSARIA_CREATURE_LENGTH, &resource_record))
        return 0;
    if (!u3_save_add_record_source(records, (uint16_t)(sizeof(records) / sizeof(records[0])), &record_count, U3_SAVE_TYPE_CREATURES, U3_SAVE_ID_CURRENT_SOSARIA, resource_record.data, resource_record.length))
        return 0;

    for (misc_index = 0; misc_index < U3_SAVE_MISC_TABLE_COUNT; misc_index++) {
        if (!u3_save_require_resource(resource_file, U3_SAVE_TYPE_MISC, (int16_t)(U3_SAVE_ID_MISC_BASE + misc_index), u3_save_misc_lengths[misc_index], &resource_record))
            return 0;
        if (!u3_save_add_record_source(records, (uint16_t)(sizeof(records) / sizeof(records[0])), &record_count, U3_SAVE_TYPE_MISC, (int16_t)(U3_SAVE_ID_MISC_BASE + misc_index), resource_record.data, resource_record.length))
            return 0;
    }

    for (type_index = 0; type_index < resource_file->type_count; type_index++) {
        uint32_t type;
        uint32_t type_record_count;
        uint32_t record_index;

        if (!u3_resource_get_type(resource_file, type_index, &type, &type_record_count))
            return 0;
        for (record_index = 0; record_index < type_record_count; record_index++) {
            if (!u3_resource_get_record(resource_file, type_index, record_index, &resource_record))
                return 0;
            if (u3_save_resource_record_is_required(resource_record.type, resource_record.id))
                continue;
            if (!u3_save_add_record_source(records, (uint16_t)(sizeof(records) / sizeof(records[0])), &record_count, resource_record.type, resource_record.id, resource_record.data, resource_record.length))
                return 0;
        }
    }

    data_offset = U3_SAVE_HEADER_LENGTH + ((uint32_t)record_count * U3_SAVE_RECORD_HEADER_LENGTH);
    cursor = data_offset;
    memset(bytes, 0, required_size);
    bytes[0] = (uint8_t)U3_SAVE_MAGIC_0;
    bytes[1] = (uint8_t)U3_SAVE_MAGIC_1;
    bytes[2] = (uint8_t)U3_SAVE_MAGIC_2;
    bytes[3] = (uint8_t)U3_SAVE_MAGIC_3;
    u3_save_write_u16(bytes, 4, U3_SAVE_FORMAT_VERSION);
    u3_save_write_u16(bytes, 6, record_count);
    u3_save_write_u32(bytes, 8, data_offset);

    for (index = 0; index < record_count; index++) {
        uint32_t record_offset = U3_SAVE_HEADER_LENGTH + ((uint32_t)index * U3_SAVE_RECORD_HEADER_LENGTH);

        u3_save_write_u32(bytes, record_offset, records[index].type);
        u3_save_write_i16(bytes, record_offset + 4, records[index].id);
        u3_save_write_u16(bytes, record_offset + 6, 0);
        u3_save_write_u32(bytes, record_offset + 8, cursor);
        u3_save_write_u32(bytes, record_offset + 12, records[index].length);
        memcpy(bytes + cursor, records[index].data, records[index].length);
        cursor += records[index].length;
    }

    if (written != 0)
        *written = cursor;
    return cursor == required_size;
}

int u3_save_open(const uint8_t *bytes, size_t length, u3_save_document *document)
{
    uint16_t record_count;
    uint32_t data_offset;
    uint32_t directory_length;
    uint16_t index;

    if (bytes == 0 || document == 0 || length < U3_SAVE_HEADER_LENGTH)
        return 0;
    if (bytes[0] != U3_SAVE_MAGIC_0 || bytes[1] != U3_SAVE_MAGIC_1 || bytes[2] != U3_SAVE_MAGIC_2 || bytes[3] != U3_SAVE_MAGIC_3)
        return 0;
    if (u3_save_read_u16(bytes, 4) != U3_SAVE_FORMAT_VERSION)
        return 0;

    record_count = u3_save_read_u16(bytes, 6);
    data_offset = u3_save_read_u32(bytes, 8);
    directory_length = (uint32_t)record_count * U3_SAVE_RECORD_HEADER_LENGTH;
    if (record_count == 0)
        return 0;
    if (!u3_save_range_valid(length, U3_SAVE_HEADER_LENGTH, directory_length))
        return 0;
    if (data_offset < U3_SAVE_HEADER_LENGTH + directory_length)
        return 0;
    if (!u3_save_range_valid(length, data_offset, 0))
        return 0;

    for (index = 0; index < record_count; index++) {
        uint32_t record_offset = U3_SAVE_HEADER_LENGTH + ((uint32_t)index * U3_SAVE_RECORD_HEADER_LENGTH);
        uint32_t payload_offset = u3_save_read_u32(bytes, record_offset + 8);
        uint32_t payload_length = u3_save_read_u32(bytes, record_offset + 12);
        uint32_t expected_payload_offset = index == 0 ? data_offset : 0;

        if (u3_save_read_u32(bytes, record_offset) == 0)
            return 0;
        if (!u3_save_range_valid(length, payload_offset, payload_length))
            return 0;
        if (payload_offset < data_offset)
            return 0;
        if (index > 0) {
            uint32_t previous_record_offset = U3_SAVE_HEADER_LENGTH + ((uint32_t)(index - 1) * U3_SAVE_RECORD_HEADER_LENGTH);
            uint32_t previous_payload_offset = u3_save_read_u32(bytes, previous_record_offset + 8);
            uint32_t previous_payload_length = u3_save_read_u32(bytes, previous_record_offset + 12);

            if (previous_payload_length > UINT32_MAX - previous_payload_offset)
                return 0;
            expected_payload_offset = previous_payload_offset + previous_payload_length;
        }
        if (payload_offset != expected_payload_offset)
            return 0;
    }
    if (record_count > 0) {
        uint32_t final_record_offset = U3_SAVE_HEADER_LENGTH + ((uint32_t)(record_count - 1) * U3_SAVE_RECORD_HEADER_LENGTH);
        uint32_t final_payload_offset = u3_save_read_u32(bytes, final_record_offset + 8);
        uint32_t final_payload_length = u3_save_read_u32(bytes, final_record_offset + 12);

        if (final_payload_length > UINT32_MAX - final_payload_offset)
            return 0;
        if (final_payload_offset + final_payload_length != length)
            return 0;
    }

    document->bytes = bytes;
    document->length = length;
    document->version = U3_SAVE_FORMAT_VERSION;
    document->record_count = record_count;
    document->data_offset = data_offset;
    return 1;
}

int u3_save_get_record(const u3_save_document *document, uint16_t record_index, u3_save_record *record)
{
    uint32_t record_offset;
    uint32_t payload_offset;
    uint32_t payload_length;

    if (document == 0 || document->bytes == 0 || record == 0)
        return 0;
    if (record_index >= document->record_count)
        return 0;

    record_offset = U3_SAVE_HEADER_LENGTH + ((uint32_t)record_index * U3_SAVE_RECORD_HEADER_LENGTH);
    if (!u3_save_range_valid(document->length, record_offset, U3_SAVE_RECORD_HEADER_LENGTH))
        return 0;

    payload_offset = u3_save_read_u32(document->bytes, record_offset + 8);
    payload_length = u3_save_read_u32(document->bytes, record_offset + 12);
    if (!u3_save_range_valid(document->length, payload_offset, payload_length))
        return 0;

    record->type = u3_save_read_u32(document->bytes, record_offset);
    record->id = u3_save_read_i16(document->bytes, record_offset + 4);
    record->data = document->bytes + payload_offset;
    record->length = payload_length;
    return 1;
}

static int u3_save_require_document_record(const u3_save_document *document,
                                           uint32_t type,
                                           int16_t id,
                                           uint32_t expected_length,
                                           u3_save_record *record)
{
    if (!u3_save_find_record(document, type, id, record))
        return 0;
    return record->length == expected_length;
}

int u3_save_find_record(const u3_save_document *document, uint32_t type, int16_t id, u3_save_record *record)
{
    uint16_t index;

    if (document == 0 || record == 0)
        return 0;

    for (index = 0; index < document->record_count; index++) {
        if (!u3_save_get_record(document, index, record))
            return 0;
        if (record->type == type && record->id == id)
            return 1;
    }

    return 0;
}

int u3_save_load_domain_state(const u3_save_document *document, u3_save_domain_state *state)
{
    u3_save_domain_state loaded_state;
    u3_save_record record;
    size_t misc_index;

    if (document == 0 || document->bytes == 0 || state == 0)
        return 0;

    memset(&loaded_state, 0, sizeof(loaded_state));

    if (!u3_save_require_document_record(document, U3_SAVE_TYPE_PARTY, U3_SAVE_ID_PARTY, U3_SAVE_PARTY_LENGTH, &record))
        return 0;
    loaded_state.party = record.data;
    loaded_state.party_length = record.length;

    if (!u3_save_require_document_record(document, U3_SAVE_TYPE_ROSTER, U3_SAVE_ID_ROSTER, U3_SAVE_ROSTER_LENGTH, &record))
        return 0;
    loaded_state.roster = record.data;
    loaded_state.roster_length = record.length;

    if (!u3_save_require_document_record(document, U3_SAVE_TYPE_MAP, U3_SAVE_ID_CURRENT_SOSARIA, U3_SAVE_CURRENT_SOSARIA_MAP_LENGTH, &record))
        return 0;
    loaded_state.current_sosaria_map = record.data;
    loaded_state.current_sosaria_map_length = record.length;

    if (!u3_save_require_document_record(document, U3_SAVE_TYPE_CREATURES, U3_SAVE_ID_CURRENT_SOSARIA, U3_SAVE_CURRENT_SOSARIA_CREATURE_LENGTH, &record))
        return 0;
    loaded_state.current_sosaria_creatures = record.data;
    loaded_state.current_sosaria_creatures_length = record.length;

    for (misc_index = 0; misc_index < U3_SAVE_MISC_TABLE_COUNT; misc_index++) {
        if (!u3_save_require_document_record(document, U3_SAVE_TYPE_MISC, (int16_t)(U3_SAVE_ID_MISC_BASE + misc_index), u3_save_misc_lengths[misc_index], &record))
            return 0;
        loaded_state.misc[misc_index] = record.data;
        loaded_state.misc_length[misc_index] = record.length;
    }

    *state = loaded_state;
    return 1;
}
