#ifndef U3_SAVE_H
#define U3_SAVE_H

#include <stddef.h>
#include <stdint.h>

#include "u3_resource.h"

#define U3_SAVE_RECORD_TYPE(a, b, c, d) \
    ((((uint32_t)(uint8_t)(a)) << 24) | (((uint32_t)(uint8_t)(b)) << 16) | (((uint32_t)(uint8_t)(c)) << 8) | ((uint32_t)(uint8_t)(d)))

#define U3_SAVE_FORMAT_VERSION 1
#define U3_SAVE_NEW_GAME_RECORD_COUNT 11
#define U3_SAVE_LEGACY_REQUIRED_RECORD_COUNT 10
#define U3_SAVE_METADATA_LENGTH 16
#define U3_SAVE_PARTY_LENGTH 64
#define U3_SAVE_ROSTER_LENGTH 1280
#define U3_SAVE_CURRENT_SOSARIA_MAP_LENGTH 4101
#define U3_SAVE_CURRENT_SOSARIA_CREATURE_LENGTH 256
#define U3_SAVE_MISC_TABLE_COUNT 6

#define U3_SAVE_TYPE_META U3_SAVE_RECORD_TYPE('M', 'E', 'T', 'A')
#define U3_SAVE_TYPE_PARTY U3_SAVE_RECORD_TYPE('P', 'R', 'T', 'Y')
#define U3_SAVE_TYPE_ROSTER U3_SAVE_RECORD_TYPE('R', 'O', 'S', 'T')
#define U3_SAVE_TYPE_MAP U3_SAVE_RECORD_TYPE('M', 'A', 'P', 'S')
#define U3_SAVE_TYPE_CREATURES 0x4D4F4E53u
#define U3_SAVE_TYPE_MISC U3_SAVE_RECORD_TYPE('M', 'I', 'S', 'C')

#define U3_SAVE_ID_METADATA 1
#define U3_SAVE_ID_PARTY 400
#define U3_SAVE_ID_ROSTER 400
#define U3_SAVE_ID_CURRENT_SOSARIA 419
#define U3_SAVE_ID_MISC_BASE 500

typedef struct u3_save_templates {
    const uint8_t *party;
    size_t party_length;
    const uint8_t *roster;
    size_t roster_length;
    const uint8_t *current_sosaria_map;
    size_t current_sosaria_map_length;
    const uint8_t *misc[U3_SAVE_MISC_TABLE_COUNT];
    size_t misc_length[U3_SAVE_MISC_TABLE_COUNT];
} u3_save_templates;

typedef struct u3_save_document {
    const uint8_t *bytes;
    size_t length;
    uint16_t version;
    uint16_t record_count;
    uint32_t data_offset;
} u3_save_document;

typedef struct u3_save_record {
    uint32_t type;
    int16_t id;
    const uint8_t *data;
    uint32_t length;
} u3_save_record;

size_t u3_save_new_game_fixture_size(void);
int u3_save_build_new_game_fixture(const u3_save_templates *templates, uint8_t *bytes, size_t capacity, size_t *written);
size_t u3_save_legacy_roster_import_size(const u3_resource_file *resource_file);
int u3_save_build_legacy_roster_import(const u3_resource_file *resource_file, uint8_t *bytes, size_t capacity, size_t *written);
int u3_save_open(const uint8_t *bytes, size_t length, u3_save_document *document);
int u3_save_get_record(const u3_save_document *document, uint16_t record_index, u3_save_record *record);
int u3_save_find_record(const u3_save_document *document, uint32_t type, int16_t id, u3_save_record *record);

#endif
