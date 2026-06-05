#include "u3_resource.h"

#include <stdbool.h>

static bool u3_resource_range_valid(size_t length, uint32_t offset, uint32_t range_length)
{
    return offset <= length && range_length <= length - offset;
}

static bool u3_resource_subrange_valid(uint32_t parent_offset, uint32_t parent_length, uint32_t offset, uint32_t range_length)
{
    uint32_t relative_offset;

    if (offset < parent_offset)
        return false;

    relative_offset = offset - parent_offset;
    return relative_offset <= parent_length && range_length <= parent_length - relative_offset;
}

static uint16_t u3_resource_read_u16(const uint8_t *bytes, uint32_t offset)
{
    return (uint16_t)(((uint16_t)bytes[offset] << 8) | bytes[offset + 1]);
}

static int16_t u3_resource_read_i16(const uint8_t *bytes, uint32_t offset)
{
    return (int16_t)u3_resource_read_u16(bytes, offset);
}

static uint32_t u3_resource_read_u24(const uint8_t *bytes, uint32_t offset)
{
    return ((uint32_t)bytes[offset] << 16) | ((uint32_t)bytes[offset + 1] << 8) | bytes[offset + 2];
}

static uint32_t u3_resource_read_u32(const uint8_t *bytes, uint32_t offset)
{
    return ((uint32_t)bytes[offset] << 24) | ((uint32_t)bytes[offset + 1] << 16) | ((uint32_t)bytes[offset + 2] << 8) | bytes[offset + 3];
}

static int u3_resource_read_record(const u3_resource_file *resource_file,
                                   uint32_t ref_offset,
                                   uint32_t type,
                                   u3_resource_record *record)
{
    const uint8_t *bytes = resource_file->bytes;
    int16_t name_offset;
    uint32_t data_relative_offset;
    uint32_t data_length_offset;
    uint32_t record_data_length;
    const uint8_t *name = 0;
    uint8_t name_length = 0;

    name_offset = u3_resource_read_i16(bytes, ref_offset + 2);
    data_relative_offset = u3_resource_read_u24(bytes, ref_offset + 5);
    if (!u3_resource_subrange_valid(0, resource_file->data_length, data_relative_offset, 4))
        return 0;
    data_length_offset = resource_file->data_offset + data_relative_offset;

    if (!u3_resource_range_valid(resource_file->length, data_length_offset, 4))
        return 0;
    record_data_length = u3_resource_read_u32(bytes, data_length_offset);
    if (!u3_resource_range_valid(resource_file->length, data_length_offset + 4, record_data_length))
        return 0;
    if (!u3_resource_subrange_valid(0, resource_file->data_length, data_relative_offset + 4, record_data_length))
        return 0;

    if (name_offset >= 0) {
        uint32_t absolute_name_offset = resource_file->name_list_offset + (uint16_t)name_offset;

        if (!u3_resource_range_valid(resource_file->length, absolute_name_offset, 1))
            return 0;
        if (!u3_resource_subrange_valid(resource_file->map_offset, resource_file->map_length, absolute_name_offset, 1))
            return 0;
        name_length = bytes[absolute_name_offset];
        if (!u3_resource_range_valid(resource_file->length, absolute_name_offset + 1, name_length))
            return 0;
        if (!u3_resource_subrange_valid(resource_file->map_offset, resource_file->map_length, absolute_name_offset + 1, name_length))
            return 0;
        name = bytes + absolute_name_offset + 1;
    }

    record->type = type;
    record->id = u3_resource_read_i16(bytes, ref_offset);
    record->data = bytes + data_length_offset + 4;
    record->length = record_data_length;
    record->name = name;
    record->name_length = name_length;
    record->attributes = bytes[ref_offset + 4];
    return 1;
}

int u3_resource_open(const uint8_t *bytes, size_t length, u3_resource_file *resource_file)
{
    uint32_t data_offset;
    uint32_t data_length;
    uint32_t map_offset;
    uint32_t map_length;
    uint32_t type_list_offset;
    uint32_t name_list_offset;
    uint32_t type_count;

    if (bytes == 0 || resource_file == 0 || length < 30)
        return 0;

    data_offset = u3_resource_read_u32(bytes, 0);
    map_offset = u3_resource_read_u32(bytes, 4);
    data_length = u3_resource_read_u32(bytes, 8);
    map_length = u3_resource_read_u32(bytes, 12);

    if (!u3_resource_range_valid(length, data_offset, data_length))
        return 0;
    if (!u3_resource_range_valid(length, map_offset, map_length))
        return 0;
    if (map_length < 30)
        return 0;

    type_list_offset = map_offset + u3_resource_read_u16(bytes, map_offset + 24);
    name_list_offset = map_offset + u3_resource_read_u16(bytes, map_offset + 26);

    if (!u3_resource_range_valid(length, type_list_offset, 2))
        return 0;
    if (!u3_resource_subrange_valid(map_offset, map_length, type_list_offset, 2))
        return 0;
    if (!u3_resource_range_valid(length, name_list_offset, 0))
        return 0;
    if (!u3_resource_subrange_valid(map_offset, map_length, name_list_offset, 0))
        return 0;

    type_count = (uint32_t)u3_resource_read_u16(bytes, type_list_offset) + 1;
    if (!u3_resource_range_valid(length, type_list_offset + 2, (uint32_t)type_count * 8))
        return 0;
    if (!u3_resource_subrange_valid(map_offset, map_length, type_list_offset + 2, type_count * 8))
        return 0;

    resource_file->bytes = bytes;
    resource_file->length = length;
    resource_file->data_offset = data_offset;
    resource_file->data_length = data_length;
    resource_file->map_offset = map_offset;
    resource_file->map_length = map_length;
    resource_file->type_list_offset = type_list_offset;
    resource_file->name_list_offset = name_list_offset;
    resource_file->type_count = type_count;
    return 1;
}

int u3_resource_get_type(const u3_resource_file *resource_file, uint32_t type_index, uint32_t *type, uint32_t *record_count)
{
    const uint8_t *bytes;
    uint32_t type_offset;

    if (resource_file == 0 || resource_file->bytes == 0 || type == 0 || record_count == 0)
        return 0;
    if (type_index >= resource_file->type_count)
        return 0;

    bytes = resource_file->bytes;
    type_offset = resource_file->type_list_offset + 2 + (type_index * 8);
    *type = u3_resource_read_u32(bytes, type_offset);
    *record_count = (uint32_t)u3_resource_read_u16(bytes, type_offset + 4) + 1;
    return 1;
}

int u3_resource_get_record(const u3_resource_file *resource_file, uint32_t type_index, uint32_t record_index, u3_resource_record *record)
{
    const uint8_t *bytes;
    uint32_t type_offset;
    uint32_t type;
    uint32_t record_count;
    uint32_t ref_list_offset;
    uint32_t ref_offset;

    if (resource_file == 0 || resource_file->bytes == 0 || record == 0)
        return 0;
    if (type_index >= resource_file->type_count)
        return 0;

    bytes = resource_file->bytes;
    type_offset = resource_file->type_list_offset + 2 + (type_index * 8);
    type = u3_resource_read_u32(bytes, type_offset);
    record_count = (uint32_t)u3_resource_read_u16(bytes, type_offset + 4) + 1;
    if (record_index >= record_count)
        return 0;

    ref_list_offset = resource_file->type_list_offset + u3_resource_read_u16(bytes, type_offset + 6);
    if (!u3_resource_range_valid(resource_file->length, ref_list_offset, record_count * 12))
        return 0;
    if (!u3_resource_subrange_valid(resource_file->map_offset, resource_file->map_length, ref_list_offset, record_count * 12))
        return 0;

    ref_offset = ref_list_offset + (record_index * 12);
    return u3_resource_read_record(resource_file, ref_offset, type, record);
}

int u3_resource_find(const u3_resource_file *resource_file, uint32_t type, int16_t id, u3_resource_record *record)
{
    uint32_t type_index;

    if (resource_file == 0 || record == 0 || resource_file->bytes == 0)
        return 0;

    for (type_index = 0; type_index < resource_file->type_count; type_index++) {
        uint32_t entry_type;
        uint32_t record_count;
        uint32_t record_index;

        if (!u3_resource_get_type(resource_file, type_index, &entry_type, &record_count))
            return 0;
        if (entry_type != type)
            continue;

        for (record_index = 0; record_index < record_count; record_index++) {
            if (!u3_resource_get_record(resource_file, type_index, record_index, record))
                return 0;
            if (record->id != id)
                continue;
            return 1;
        }
    }

    return 0;
}
