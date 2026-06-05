#ifndef U3_RESOURCE_H
#define U3_RESOURCE_H

#include <stddef.h>
#include <stdint.h>

#define U3_RESOURCE_TYPE(a, b, c, d) \
    ((((uint32_t)(uint8_t)(a)) << 24) | (((uint32_t)(uint8_t)(b)) << 16) | (((uint32_t)(uint8_t)(c)) << 8) | ((uint32_t)(uint8_t)(d)))

typedef struct u3_resource_file {
    const uint8_t *bytes;
    size_t length;
    uint32_t data_offset;
    uint32_t data_length;
    uint32_t map_offset;
    uint32_t map_length;
    uint32_t type_list_offset;
    uint32_t name_list_offset;
    uint32_t type_count;
} u3_resource_file;

typedef struct u3_resource_record {
    uint32_t type;
    int16_t id;
    const uint8_t *data;
    uint32_t length;
    const uint8_t *name;
    uint8_t name_length;
    uint8_t attributes;
} u3_resource_record;

int u3_resource_open(const uint8_t *bytes, size_t length, u3_resource_file *resource_file);
int u3_resource_find(const u3_resource_file *resource_file, uint32_t type, int16_t id, u3_resource_record *record);

#endif
