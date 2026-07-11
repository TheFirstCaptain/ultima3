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

static void mark_roster_occupied(uint8_t *roster, uint8_t roster_id, const char *name)
{
    uint8_t *record = roster + ((roster_id - 1) * U3_PARTY_ROSTER_RECORD_LENGTH);
    size_t index;

    memset(record, 0, U3_PARTY_ROSTER_RECORD_LENGTH);
    for (index = 0; name[index] != 0 && index < U3_PARTY_NAME_CAPACITY; index++)
        record[index] = (uint8_t)name[index];
    record[17] = 'G';
}

static u3_save_domain_state synthetic_party_state(uint8_t *party, uint8_t *roster)
{
    u3_save_domain_state state;

    memset(&state, 0, sizeof(state));
    state.party = party;
    state.party_length = U3_SAVE_PARTY_LENGTH;
    state.roster = roster;
    state.roster_length = U3_SAVE_ROSTER_LENGTH;
    return state;
}

static void test_forms_party_from_occupied_roster_slots(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t updated_party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    uint8_t selected[3] = {3, 1, 2};
    u3_save_domain_state state;
    u3_party_form_result result;

    memset(party, 0, sizeof(party));
    memset(updated_party, 0xCC, sizeof(updated_party));
    memset(roster, 0, sizeof(roster));
    party[1] = 0;
    mark_roster_occupied(roster, 1, "A");
    mark_roster_occupied(roster, 2, "B");
    mark_roster_occupied(roster, 3, "C");
    state = synthetic_party_state(party, roster);

    result = u3_party_form_from_roster(&state, selected, 3, updated_party, sizeof(updated_party));

    ASSERT_TRUE(result.formed);
    ASSERT_EQ_INT(U3_PARTY_FORM_OK, result.reason);
    ASSERT_EQ_INT(3, result.party_size);
    ASSERT_EQ_INT(3, result.active_roster_ids[0]);
    ASSERT_EQ_INT(1, result.active_roster_ids[1]);
    ASSERT_EQ_INT(2, result.active_roster_ids[2]);
    ASSERT_EQ_INT(0x7E, updated_party[0]);
    ASSERT_EQ_INT(3, updated_party[1]);
    ASSERT_EQ_INT(0, updated_party[2]);
    ASSERT_EQ_INT(42, updated_party[3]);
    ASSERT_EQ_INT(20, updated_party[4]);
    ASSERT_EQ_INT(255, updated_party[5]);
    ASSERT_EQ_INT(3, updated_party[6]);
    ASSERT_EQ_INT(1, updated_party[7]);
    ASSERT_EQ_INT(2, updated_party[8]);
    ASSERT_EQ_INT(0, updated_party[9]);
}

static void test_form_rejects_invalid_selection_counts(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t updated_party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    uint8_t selected[5] = {1, 2, 3, 4, 5};
    u3_save_domain_state state;
    u3_party_form_result result;

    memset(party, 0, sizeof(party));
    memset(updated_party, 0xCC, sizeof(updated_party));
    memset(roster, 0, sizeof(roster));
    mark_roster_occupied(roster, 1, "A");
    state = synthetic_party_state(party, roster);

    result = u3_party_form_from_roster(&state, selected, 0, updated_party, sizeof(updated_party));
    ASSERT_FALSE(result.formed);
    ASSERT_EQ_INT(U3_PARTY_FORM_INVALID_SIZE, result.reason);
    ASSERT_EQ_INT(0xCC, updated_party[0]);

    result = u3_party_form_from_roster(&state, selected, 5, updated_party, sizeof(updated_party));
    ASSERT_FALSE(result.formed);
    ASSERT_EQ_INT(U3_PARTY_FORM_INVALID_SIZE, result.reason);
    ASSERT_EQ_INT(0xCC, updated_party[0]);
}

static void test_form_rejects_duplicate_invalid_and_unoccupied_roster_ids(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t updated_party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    uint8_t duplicate[2] = {1, 1};
    uint8_t invalid[1] = {21};
    uint8_t unoccupied[1] = {2};
    u3_save_domain_state state;
    u3_party_form_result result;

    memset(party, 0, sizeof(party));
    memset(updated_party, 0xCC, sizeof(updated_party));
    memset(roster, 0, sizeof(roster));
    mark_roster_occupied(roster, 1, "A");
    state = synthetic_party_state(party, roster);

    result = u3_party_form_from_roster(&state, duplicate, 2, updated_party, sizeof(updated_party));
    ASSERT_FALSE(result.formed);
    ASSERT_EQ_INT(U3_PARTY_FORM_DUPLICATE_ROSTER_ID, result.reason);

    result = u3_party_form_from_roster(&state, invalid, 1, updated_party, sizeof(updated_party));
    ASSERT_FALSE(result.formed);
    ASSERT_EQ_INT(U3_PARTY_FORM_INVALID_ROSTER_ID, result.reason);

    result = u3_party_form_from_roster(&state, unoccupied, 1, updated_party, sizeof(updated_party));
    ASSERT_FALSE(result.formed);
    ASSERT_EQ_INT(U3_PARTY_FORM_UNOCCUPIED_ROSTER_ID, result.reason);
    ASSERT_EQ_INT(0xCC, updated_party[0]);
}

static void test_ignite_auto_selects_first_living_active_torch_holder(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_party_ignite_result result;

    memset(party, 0, sizeof(party));
    memset(roster, 0, sizeof(roster));
    party[1] = 3;
    party[6] = 1;
    party[7] = 2;
    party[8] = 3;
    mark_roster_occupied(roster, 1, "A");
    mark_roster_occupied(roster, 2, "B");
    mark_roster_occupied(roster, 3, "C");
    roster[U3_PARTY_ROSTER_TORCH_OFFSET] = 0;
    roster[U3_PARTY_ROSTER_RECORD_LENGTH + U3_PARTY_ROSTER_TORCH_OFFSET] = 2;
    roster[(2 * U3_PARTY_ROSTER_RECORD_LENGTH) + U3_PARTY_ROSTER_TORCH_OFFSET] = 4;

    result = u3_party_ignite_torch(
        party,
        sizeof(party),
        roster,
        sizeof(roster),
        U3_PARTY_IGNITE_AUTO_SLOT);

    ASSERT_TRUE(result.ignited);
    ASSERT_EQ_INT(U3_PARTY_IGNITE_OK, result.reason);
    ASSERT_EQ_INT(2, result.active_slot);
    ASSERT_EQ_INT(2, result.roster_id);
    ASSERT_EQ_INT(2, result.torch_count_before);
    ASSERT_EQ_INT(1, result.torch_count_after);
    ASSERT_EQ_INT(U3_PARTY_TORCH_LIGHT_VALUE, result.light);
    ASSERT_EQ_INT(0, roster[U3_PARTY_ROSTER_TORCH_OFFSET]);
    ASSERT_EQ_INT(1, roster[U3_PARTY_ROSTER_RECORD_LENGTH + U3_PARTY_ROSTER_TORCH_OFFSET]);
    ASSERT_EQ_INT(4, roster[(2 * U3_PARTY_ROSTER_RECORD_LENGTH) + U3_PARTY_ROSTER_TORCH_OFFSET]);
}

static void test_ignite_accepts_poisoned_active_torch_holder(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_party_ignite_result result;

    memset(party, 0, sizeof(party));
    memset(roster, 0, sizeof(roster));
    party[1] = 1;
    party[6] = 1;
    mark_roster_occupied(roster, 1, "A");
    roster[U3_PARTY_ROSTER_STATUS_OFFSET] = 'P';
    roster[U3_PARTY_ROSTER_TORCH_OFFSET] = 2;

    result = u3_party_ignite_torch(
        party,
        sizeof(party),
        roster,
        sizeof(roster),
        U3_PARTY_IGNITE_AUTO_SLOT);

    ASSERT_TRUE(result.ignited);
    ASSERT_EQ_INT(U3_PARTY_IGNITE_OK, result.reason);
    ASSERT_EQ_INT(1, result.active_slot);
    ASSERT_EQ_INT(1, result.roster_id);
    ASSERT_EQ_INT(2, result.torch_count_before);
    ASSERT_EQ_INT(1, result.torch_count_after);
    ASSERT_EQ_INT(U3_PARTY_TORCH_LIGHT_VALUE, result.light);
    ASSERT_EQ_INT(1, roster[U3_PARTY_ROSTER_TORCH_OFFSET]);
}

static void test_ignite_reports_no_torch_and_invalid_character_without_mutation(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_party_ignite_result result;

    memset(party, 0, sizeof(party));
    memset(roster, 0, sizeof(roster));
    party[1] = 1;
    party[6] = 1;
    mark_roster_occupied(roster, 1, "A");
    roster[U3_PARTY_ROSTER_TORCH_OFFSET] = 0;

    result = u3_party_ignite_torch(
        party,
        sizeof(party),
        roster,
        sizeof(roster),
        U3_PARTY_IGNITE_AUTO_SLOT);

    ASSERT_FALSE(result.ignited);
    ASSERT_EQ_INT(U3_PARTY_IGNITE_NO_TORCH, result.reason);
    ASSERT_EQ_INT(1, result.active_slot);
    ASSERT_EQ_INT(1, result.roster_id);
    ASSERT_EQ_INT(0, result.torch_count_before);
    ASSERT_EQ_INT(0, roster[U3_PARTY_ROSTER_TORCH_OFFSET]);

    roster[U3_PARTY_ROSTER_STATUS_OFFSET] = 'D';
    result = u3_party_ignite_torch(
        party,
        sizeof(party),
        roster,
        sizeof(roster),
        1);

    ASSERT_FALSE(result.ignited);
    ASSERT_EQ_INT(U3_PARTY_IGNITE_INVALID_CHARACTER, result.reason);
    ASSERT_EQ_INT(0, roster[U3_PARTY_ROSTER_TORCH_OFFSET]);
}

int main(void)
{
    test_loads_default_party_roster_summary();
    test_loads_empty_roster_shape();
    test_rejects_invalid_party_shape();
    test_forms_party_from_occupied_roster_slots();
    test_form_rejects_invalid_selection_counts();
    test_form_rejects_duplicate_invalid_and_unoccupied_roster_ids();
    test_ignite_auto_selects_first_living_active_torch_holder();
    test_ignite_accepts_poisoned_active_torch_holder();
    test_ignite_reports_no_torch_and_invalid_character_without_mutation();

    printf("party roster characterization passed\n");
    return 0;
}
