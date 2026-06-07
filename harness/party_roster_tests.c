#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_party.h"
#include "u3_resource.h"
#include "u3_save.h"

#define ASSERT_EQ_INT(expected, actual) assert_eq_int((expected), (actual), __FILE__, __LINE__)
#define ASSERT_TRUE(actual) assert_true((actual), __FILE__, __LINE__)
#define ASSERT_FALSE(actual) assert_false((actual), __FILE__, __LINE__)
#define ASSERT_STRING_EQ(expected, actual) assert_string_eq((expected), (actual), __FILE__, __LINE__)

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

static void assert_string_eq(const char *expected, const char *actual, const char *file, int line)
{
    if (strcmp(expected, actual) != 0) {
        fprintf(stderr, "%s:%d: expected \"%s\", got \"%s\"\n", file, line, expected, actual);
        exit(1);
    }
}

static uint8_t *read_file(const char *path, size_t *length)
{
    FILE *file = fopen(path, "rb");
    long file_length;
    uint8_t *bytes;

    if (file == 0) {
        perror(path);
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    file_length = ftell(file);
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

static u3_resource_record require_resource(const u3_resource_file *resource_file, uint32_t type, int16_t id)
{
    u3_resource_record record;

    ASSERT_TRUE(u3_resource_find(resource_file, type, id, &record));
    return record;
}

static void load_templates(u3_save_templates *templates, uint8_t **resource_bytes)
{
    size_t length;
    u3_resource_file resource_file;
    u3_resource_record record;
    size_t index;

    memset(templates, 0, sizeof(*templates));
    *resource_bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &length);
    ASSERT_TRUE(u3_resource_open(*resource_bytes, length, &resource_file));

    record = require_resource(&resource_file, U3_RESOURCE_TYPE('P', 'R', 'T', 'Y'), 500);
    templates->party = record.data;
    templates->party_length = record.length;
    record = require_resource(&resource_file, U3_RESOURCE_TYPE('R', 'O', 'S', 'T'), 500);
    templates->roster = record.data;
    templates->roster_length = record.length;
    record = require_resource(&resource_file, U3_RESOURCE_TYPE('M', 'A', 'P', 'S'), 420);
    templates->current_sosaria_map = record.data;
    templates->current_sosaria_map_length = record.length;
    for (index = 0; index < U3_SAVE_MISC_TABLE_COUNT; index++) {
        record = require_resource(&resource_file, U3_RESOURCE_TYPE('M', 'I', 'S', 'C'), (int16_t)(400 + index));
        templates->misc[index] = record.data;
        templates->misc_length[index] = record.length;
    }
}

static uint8_t *build_fixture(const u3_save_templates *templates, size_t *fixture_length)
{
    size_t capacity = u3_save_new_game_fixture_size();
    uint8_t *fixture = (uint8_t *)malloc(capacity);

    if (fixture == 0) {
        fprintf(stderr, "failed to allocate %zu bytes\n", capacity);
        exit(1);
    }
    ASSERT_TRUE(u3_save_build_new_game_fixture(templates, fixture, capacity, fixture_length));
    ASSERT_EQ_INT((int)capacity, (int)*fixture_length);
    return fixture;
}

static u3_save_domain_state load_domain_state(const uint8_t *fixture, size_t fixture_length)
{
    u3_save_document document;
    u3_save_domain_state state;

    ASSERT_TRUE(u3_save_open(fixture, fixture_length, &document));
    ASSERT_TRUE(u3_save_load_domain_state(&document, &state));
    return state;
}

static void test_loads_default_party_roster_summary(void)
{
    u3_save_templates templates;
    uint8_t *resource_bytes;
    uint8_t *fixture;
    size_t fixture_length;
    u3_save_domain_state state;
    u3_party_summary summary;

    load_templates(&templates, &resource_bytes);
    fixture = build_fixture(&templates, &fixture_length);
    state = load_domain_state(fixture, fixture_length);

    ASSERT_TRUE(u3_party_load_summary(&state, &summary));
    ASSERT_EQ_INT(4, summary.party_size);
    ASSERT_EQ_INT(1, summary.active_roster_ids[0]);
    ASSERT_EQ_INT(2, summary.active_roster_ids[1]);
    ASSERT_EQ_INT(3, summary.active_roster_ids[2]);
    ASSERT_EQ_INT(4, summary.active_roster_ids[3]);
    ASSERT_EQ_INT(4, summary.occupied_roster_count);

    ASSERT_TRUE(summary.roster[0].occupied);
    ASSERT_STRING_EQ("Tatiana", summary.roster[0].name);
    ASSERT_EQ_INT('G', summary.roster[0].status);
    ASSERT_EQ_INT('E', summary.roster[0].race);
    ASSERT_EQ_INT('T', summary.roster[0].character_class);
    ASSERT_EQ_INT('F', summary.roster[0].sex);
    ASSERT_EQ_INT(100, summary.roster[0].hit_points);
    ASSERT_EQ_INT(100, summary.roster[0].max_hit_points);
    ASSERT_EQ_INT(1, summary.roster[0].level);
    ASSERT_EQ_INT(150, summary.roster[0].food);
    ASSERT_EQ_INT(150, summary.roster[0].gold);

    free(fixture);
    free(resource_bytes);
}

static void test_loads_empty_roster_shape(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_save_domain_state state;
    u3_party_summary summary;

    memset(party, 0, sizeof(party));
    memset(roster, 0, sizeof(roster));
    party[1] = 0;
    state.party = party;
    state.party_length = sizeof(party);
    state.roster = roster;
    state.roster_length = sizeof(roster);

    ASSERT_TRUE(u3_party_load_summary(&state, &summary));
    ASSERT_EQ_INT(0, summary.party_size);
    ASSERT_EQ_INT(0, summary.occupied_roster_count);
    ASSERT_FALSE(summary.roster[0].occupied);
}

static void test_rejects_invalid_party_shape(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_save_domain_state state;
    u3_party_summary summary;

    memset(party, 0, sizeof(party));
    memset(roster, 0, sizeof(roster));
    party[1] = 5;
    state.party = party;
    state.party_length = sizeof(party);
    state.roster = roster;
    state.roster_length = sizeof(roster);
    ASSERT_FALSE(u3_party_load_summary(&state, &summary));

    party[1] = 1;
    party[6] = 21;
    ASSERT_FALSE(u3_party_load_summary(&state, &summary));

    party[6] = 0;
    ASSERT_FALSE(u3_party_load_summary(&state, &summary));

    party[6] = 5;
    ASSERT_FALSE(u3_party_load_summary(&state, &summary));
}

int main(void)
{
    test_loads_default_party_roster_summary();
    test_loads_empty_roster_shape();
    test_rejects_invalid_party_shape();

    printf("party roster characterization passed\n");
    return 0;
}
