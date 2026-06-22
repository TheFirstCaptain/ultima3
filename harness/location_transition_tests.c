#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_location.h"
#include "u3_resource.h"

#define ASSERT_EQ_INT(expected, actual) assert_eq_int((expected), (actual), __FILE__, __LINE__)
#define ASSERT_TRUE(actual) assert_true((actual), __FILE__, __LINE__)
#define ASSERT_FALSE(actual) assert_false((actual), __FILE__, __LINE__)

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

static uint8_t *read_file(const char *path, size_t *length)
{
    FILE *file = fopen(path, "rb");
    uint8_t *bytes;
    long file_length;

    if (file == 0)
        return 0;
    if (fseek(file, 0, SEEK_END) != 0 || (file_length = ftell(file)) < 0) {
        fclose(file);
        return 0;
    }
    rewind(file);
    bytes = (uint8_t *)malloc((size_t)file_length);
    if (bytes == 0 || fread(bytes, 1, (size_t)file_length, file) != (size_t)file_length) {
        free(bytes);
        fclose(file);
        return 0;
    }
    fclose(file);
    *length = (size_t)file_length;
    return bytes;
}

static void load_fixture(uint8_t **resource_bytes,
                         u3_resource_record *map,
                         u3_resource_record *locations)
{
    size_t resource_length;
    u3_resource_file resource_file;

    *resource_bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &resource_length);
    ASSERT_TRUE(*resource_bytes != 0);
    ASSERT_TRUE(u3_resource_open(*resource_bytes, resource_length, &resource_file));
    ASSERT_TRUE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('M', 'A', 'P', 'S'), 420, map));
    ASSERT_TRUE(u3_resource_find(&resource_file, U3_RESOURCE_TYPE('M', 'I', 'S', 'C'), 404, locations));
}

static u3_resource_record require_resource(const u3_resource_file *resource_file,
                                           uint32_t type,
                                           int16_t id)
{
    u3_resource_record record;

    ASSERT_TRUE(u3_resource_find(resource_file, type, id, &record));
    return record;
}

static u3_location_transition_result make_request(uint8_t kind,
                                                  uint8_t index,
                                                  uint8_t initial_x,
                                                  uint8_t initial_y,
                                                  uint8_t heading)
{
    u3_location_transition_result request;

    memset(&request, 0, sizeof(request));
    request.handled = 1;
    request.requested = 1;
    request.destination_kind = kind;
    request.location_index = index;
    request.resource_id = u3_location_resource_id_for_index(index);
    request.return_x = 46;
    request.return_y = 19;
    request.initial_x = initial_x;
    request.initial_y = initial_y;
    request.initial_heading = heading;
    return request;
}

static void test_detects_lcb_towne_fixture(void)
{
    uint8_t *resource_bytes;
    u3_resource_record map;
    u3_resource_record locations;
    u3_overworld_state state;
    u3_location_transition_result result;

    load_fixture(&resource_bytes, &map, &locations);
    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 46, 19, 64, 64));

    ASSERT_TRUE(u3_location_handle_overworld_command(&state,
                                                     map.data,
                                                     map.length,
                                                     locations.data,
                                                     locations.length,
                                                     U3_LOCATION_COMMAND_ENTER,
                                                     &result));
    ASSERT_TRUE(result.handled);
    ASSERT_TRUE(result.requested);
    ASSERT_EQ_INT(U3_LOCATION_KIND_TOWN, result.destination_kind);
    ASSERT_EQ_INT(2, result.location_index);
    ASSERT_EQ_INT(402, result.resource_id);
    ASSERT_EQ_INT(46, result.return_x);
    ASSERT_EQ_INT(19, result.return_y);
    ASSERT_EQ_INT(1, result.initial_x);
    ASSERT_EQ_INT(32, result.initial_y);
    ASSERT_EQ_INT(2, result.initial_heading);
    ASSERT_EQ_INT(U3_LOCATION_STATUS_TOWN_REQUESTED, result.status);

    free(resource_bytes);
}

static void test_detects_only_dungeon_doom_fixture_for_first_slice(void)
{
    uint8_t *resource_bytes;
    u3_resource_record map;
    u3_resource_record locations;
    u3_overworld_state state;
    u3_location_transition_result result;

    load_fixture(&resource_bytes, &map, &locations);
    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 19, 57, 64, 64));
    ASSERT_TRUE(u3_location_handle_overworld_command(&state,
                                                     map.data,
                                                     map.length,
                                                     locations.data,
                                                     locations.length,
                                                     U3_LOCATION_COMMAND_ENTER,
                                                     &result));
    ASSERT_TRUE(result.handled);
    ASSERT_TRUE(result.requested);
    ASSERT_EQ_INT(U3_LOCATION_KIND_DUNGEON, result.destination_kind);
    ASSERT_EQ_INT(U3_LOCATION_DUNGEON_FIXTURE_INDEX, result.location_index);
    ASSERT_EQ_INT(U3_LOCATION_DUNGEON_FIXTURE_RESOURCE_ID, result.resource_id);
    ASSERT_EQ_INT(19, result.return_x);
    ASSERT_EQ_INT(57, result.return_y);
    ASSERT_EQ_INT(U3_LOCATION_DUNGEON_INITIAL_X, result.initial_x);
    ASSERT_EQ_INT(U3_LOCATION_DUNGEON_INITIAL_Y, result.initial_y);
    ASSERT_EQ_INT(U3_LOCATION_DUNGEON_INITIAL_HEADING, result.initial_heading);
    ASSERT_EQ_INT(U3_LOCATION_DUNGEON_INITIAL_LEVEL, result.initial_level);
    ASSERT_EQ_INT(U3_LOCATION_STATUS_DUNGEON_REQUESTED, result.status);

    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 49, 34, 64, 64));
    ASSERT_TRUE(u3_location_handle_overworld_command(&state,
                                                     map.data,
                                                     map.length,
                                                     locations.data,
                                                     locations.length,
                                                     U3_LOCATION_COMMAND_ENTER,
                                                     &result));
    ASSERT_TRUE(result.handled);
    ASSERT_FALSE(result.requested);
    ASSERT_EQ_INT(U3_LOCATION_STATUS_NOT_ENTERABLE, result.status);
    free(resource_bytes);
}

static void test_enter_rejects_unrecognized_coordinate_without_mutation(void)
{
    uint8_t map[65] = {8};
    uint8_t locations[U3_LOCATION_TABLE_LENGTH];
    u3_overworld_state state;
    u3_location_transition_result result;

    memset(locations, 0xff, sizeof(locations));
    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 3, 4, 8, 8));

    ASSERT_TRUE(u3_location_handle_overworld_command(&state,
                                                     map,
                                                     sizeof(map),
                                                     locations,
                                                     sizeof(locations),
                                                     U3_LOCATION_COMMAND_ENTER,
                                                     &result));
    ASSERT_TRUE(result.handled);
    ASSERT_FALSE(result.requested);
    ASSERT_EQ_INT(U3_LOCATION_KIND_NONE, result.destination_kind);
    ASSERT_EQ_INT(U3_LOCATION_STATUS_NOT_ENTERABLE, result.status);
    ASSERT_EQ_INT(3, result.return_x);
    ASSERT_EQ_INT(4, result.return_y);
    ASSERT_EQ_INT(3, state.x);
    ASSERT_EQ_INT(4, state.y);
}

static void test_matching_non_town_tile_is_not_enterable(void)
{
    uint8_t map[65] = {8};
    uint8_t locations[U3_LOCATION_TABLE_LENGTH];
    u3_overworld_state state;
    u3_location_transition_result result;

    memset(locations, 0xff, sizeof(locations));
    locations[5] = 3;
    locations[U3_LOCATION_TABLE_COUNT + 5] = 4;
    map[1 + (4 * 8) + 3] = 0x1c;
    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 3, 4, 8, 8));

    ASSERT_TRUE(u3_location_handle_overworld_command(&state,
                                                     map,
                                                     sizeof(map),
                                                     locations,
                                                     sizeof(locations),
                                                     U3_LOCATION_COMMAND_ENTER,
                                                     &result));
    ASSERT_TRUE(result.handled);
    ASSERT_FALSE(result.requested);
    ASSERT_EQ_INT(U3_LOCATION_STATUS_NOT_ENTERABLE, result.status);
}

static void test_non_enter_command_is_ignored(void)
{
    uint8_t map[65] = {8};
    uint8_t locations[U3_LOCATION_TABLE_LENGTH];
    u3_overworld_state state;
    u3_location_transition_result result;

    memset(locations, 0xff, sizeof(locations));
    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 3, 4, 8, 8));
    ASSERT_TRUE(u3_location_handle_overworld_command(&state,
                                                     map,
                                                     sizeof(map),
                                                     locations,
                                                     sizeof(locations),
                                                     U3_OVERWORLD_COMMAND_EAST,
                                                     &result));
    ASSERT_FALSE(result.handled);
    ASSERT_FALSE(result.requested);
    ASSERT_EQ_INT(U3_LOCATION_STATUS_NONE, result.status);
}

static void test_rejects_truncated_inputs(void)
{
    uint8_t map[65] = {8};
    uint8_t locations[U3_LOCATION_TABLE_LENGTH] = {0};
    u3_overworld_state state;
    u3_location_transition_result result;

    ASSERT_TRUE(u3_overworld_state_init_wrapped(&state, 3, 4, 8, 8));
    ASSERT_FALSE(u3_location_handle_overworld_command(&state,
                                                      map,
                                                      sizeof(map) - 1,
                                                      locations,
                                                      sizeof(locations),
                                                      U3_LOCATION_COMMAND_ENTER,
                                                      &result));
    ASSERT_FALSE(u3_location_handle_overworld_command(&state,
                                                      map,
                                                      sizeof(map),
                                                      locations,
                                                      sizeof(locations) - 1,
                                                      U3_LOCATION_COMMAND_ENTER,
                                                      &result));
}

static void test_initializes_town_castle_and_dungeon_sessions(void)
{
    uint8_t *resource_bytes;
    size_t resource_length;
    u3_resource_file resource_file;
    u3_resource_record map;
    u3_resource_record monsters;
    u3_resource_record talk;
    u3_location_transition_result request;
    u3_location_session session;

    resource_bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &resource_length);
    ASSERT_TRUE(resource_bytes != 0);
    ASSERT_TRUE(u3_resource_open(resource_bytes, resource_length, &resource_file));

    request = make_request(U3_LOCATION_KIND_TOWN, 2, 1, 32, 2);
    map = require_resource(&resource_file, U3_RESOURCE_TYPE('M', 'A', 'P', 'S'), 402);
    monsters = require_resource(&resource_file, U3_RESOURCE_TYPE('M', 'O', 'N', 'S'), 402);
    talk = require_resource(&resource_file, U3_RESOURCE_TYPE('T', 'L', 'K', 'S'), 402);
    ASSERT_TRUE(u3_location_session_init(&request,
                                         map.data, map.length,
                                         monsters.data, monsters.length,
                                         talk.data, talk.length,
                                         &session));
    ASSERT_TRUE(session.active);
    ASSERT_EQ_INT(U3_LOCATION_KIND_TOWN, session.destination_kind);
    ASSERT_EQ_INT(U3_LOCATION_MAP_SHAPE_TWO_DIMENSIONAL, session.map_shape);
    ASSERT_EQ_INT(64, session.map_size);
    ASSERT_EQ_INT(4097, (int)session.map_length);
    ASSERT_EQ_INT(256, (int)session.monster_length);
    ASSERT_EQ_INT(256, (int)session.talk_length);
    ASSERT_EQ_INT(1, session.x);
    ASSERT_EQ_INT(32, session.y);
    ASSERT_EQ_INT(2, session.heading);
    ASSERT_EQ_INT(46, session.return_x);
    ASSERT_EQ_INT(19, session.return_y);

    request = make_request(U3_LOCATION_KIND_CASTLE, 0, 32, 62, 2);
    map = require_resource(&resource_file, U3_RESOURCE_TYPE('M', 'A', 'P', 'S'), 400);
    monsters = require_resource(&resource_file, U3_RESOURCE_TYPE('M', 'O', 'N', 'S'), 400);
    talk = require_resource(&resource_file, U3_RESOURCE_TYPE('T', 'L', 'K', 'S'), 400);
    ASSERT_TRUE(u3_location_session_init(&request,
                                         map.data, map.length,
                                         monsters.data, monsters.length,
                                         talk.data, talk.length,
                                         &session));
    ASSERT_EQ_INT(U3_LOCATION_KIND_CASTLE, session.destination_kind);
    ASSERT_EQ_INT(400, session.resource_id);
    ASSERT_EQ_INT(U3_LOCATION_MAP_SHAPE_TWO_DIMENSIONAL, session.map_shape);

    request = make_request(U3_LOCATION_KIND_DUNGEON, 12, 1, 1, 1);
    map = require_resource(&resource_file, U3_RESOURCE_TYPE('M', 'A', 'P', 'S'), 412);
    talk = require_resource(&resource_file, U3_RESOURCE_TYPE('T', 'L', 'K', 'S'), 412);
    ASSERT_TRUE(u3_location_session_init(&request,
                                         map.data, map.length,
                                         0, 0,
                                         talk.data, talk.length,
                                         &session));
    ASSERT_EQ_INT(U3_LOCATION_KIND_DUNGEON, session.destination_kind);
    ASSERT_EQ_INT(U3_LOCATION_MAP_SHAPE_DUNGEON, session.map_shape);
    ASSERT_EQ_INT(16, session.map_size);
    ASSERT_EQ_INT(2048, (int)session.map_length);
    ASSERT_EQ_INT(0, (int)session.monster_length);
    ASSERT_EQ_INT(0, session.dungeon_level);

    free(resource_bytes);
}

static void test_session_validation_rejects_partial_or_mismatched_records(void)
{
    uint8_t map[U3_LOCATION_TWO_DIMENSIONAL_MAP_LENGTH] = {64};
    uint8_t monsters[U3_LOCATION_MONSTER_LENGTH] = {0};
    uint8_t talk[U3_LOCATION_TALK_LENGTH] = {0};
    u3_location_transition_result request = make_request(U3_LOCATION_KIND_TOWN, 2, 1, 32, 2);
    u3_location_session session;

    ASSERT_FALSE(u3_location_session_init(&request,
                                          map, sizeof(map),
                                          0, 0,
                                          talk, sizeof(talk),
                                          &session));
    ASSERT_FALSE(session.active);
    ASSERT_FALSE(u3_location_session_init(&request,
                                          map, sizeof(map),
                                          monsters, sizeof(monsters),
                                          talk, sizeof(talk) - 1,
                                          &session));
    request.resource_id++;
    ASSERT_FALSE(u3_location_session_init(&request,
                                          map, sizeof(map),
                                          monsters, sizeof(monsters),
                                          talk, sizeof(talk),
                                          &session));
    request = make_request(U3_LOCATION_KIND_TOWN, 2, 64, 32, 2);
    ASSERT_FALSE(u3_location_session_init(&request,
                                          map, sizeof(map),
                                          monsters, sizeof(monsters),
                                          talk, sizeof(talk),
                                          &session));
    request = make_request(U3_LOCATION_KIND_TOWN, 2, 1, 32, 4);
    ASSERT_FALSE(u3_location_session_init(&request,
                                          map, sizeof(map),
                                          monsters, sizeof(monsters),
                                          talk, sizeof(talk),
                                          &session));

    request = make_request(U3_LOCATION_KIND_DUNGEON, 12, 1, 1, 1);
    request.initial_level = 1;
    ASSERT_FALSE(u3_location_session_init(&request,
                                          map, U3_LOCATION_DUNGEON_MAP_LENGTH,
                                          0, 0,
                                          talk, sizeof(talk),
                                          &session));
}

static void test_resource_id_mapping_preserves_legacy_gap(void)
{
    ASSERT_EQ_INT(400, u3_location_resource_id_for_index(0));
    ASSERT_EQ_INT(418, u3_location_resource_id_for_index(18));
    ASSERT_EQ_INT(422, u3_location_resource_id_for_index(19));
    ASSERT_EQ_INT(434, u3_location_resource_id_for_index(31));
}

static void test_entry_and_return_mutate_only_party_transition_fields(void)
{
    uint8_t party[64] = {0};
    u3_location_transition_result request =
        make_request(U3_LOCATION_KIND_TOWN, 2, 1, 32, 2);
    u3_location_session session;

    party[U3_OVERWORLD_PARTY_SIZE_OFFSET] = 4;
    party[U3_OVERWORLD_PARTY_X_OFFSET] = 46;
    party[U3_OVERWORLD_PARTY_Y_OFFSET] = 19;
    ASSERT_TRUE(u3_location_enter_party(party, sizeof(party), &request));
    ASSERT_EQ_INT(U3_LOCATION_PARTY_MODE_TOWN,
                  party[U3_LOCATION_PARTY_MODE_OFFSET]);
    ASSERT_EQ_INT(46, party[U3_OVERWORLD_PARTY_X_OFFSET]);
    ASSERT_EQ_INT(19, party[U3_OVERWORLD_PARTY_Y_OFFSET]);
    ASSERT_TRUE(request.turn_applied);
    ASSERT_EQ_INT(4, request.turn_delta);
    ASSERT_EQ_INT(4, (int)request.move_counter_after);

    memset(&session, 0, sizeof(session));
    session.active = 1;
    session.destination_kind = U3_LOCATION_KIND_TOWN;
    session.location_index = 2;
    session.resource_id = 402;
    session.return_x = 46;
    session.return_y = 19;
    ASSERT_TRUE(u3_location_restore_party(party, sizeof(party), &session));
    ASSERT_EQ_INT(U3_LOCATION_PARTY_MODE_SOSARIA,
                  party[U3_LOCATION_PARTY_MODE_OFFSET]);
    ASSERT_EQ_INT(46, party[U3_OVERWORLD_PARTY_X_OFFSET]);
    ASSERT_EQ_INT(19, party[U3_OVERWORLD_PARTY_Y_OFFSET]);
    ASSERT_EQ_INT(4, (int)u3_overworld_read_party_move_counter(party, sizeof(party)));
}

static void test_invalid_entry_and_return_leave_party_unchanged(void)
{
    uint8_t party[64] = {0};
    uint8_t before[64];
    u3_location_transition_result request =
        make_request(U3_LOCATION_KIND_TOWN, 2, 1, 32, 2);
    u3_location_session session;

    party[U3_OVERWORLD_PARTY_SIZE_OFFSET] = 4;
    memcpy(before, party, sizeof(party));
    ASSERT_FALSE(u3_location_enter_party(party, sizeof(party), &request));
    ASSERT_TRUE(memcmp(before, party, sizeof(party)) == 0);

    memset(&session, 0, sizeof(session));
    session.active = 1;
    session.destination_kind = U3_LOCATION_KIND_TOWN;
    session.location_index = 2;
    session.resource_id = 402;
    party[U3_LOCATION_PARTY_MODE_OFFSET] = U3_LOCATION_PARTY_MODE_SOSARIA;
    memcpy(before, party, sizeof(party));
    ASSERT_FALSE(u3_location_restore_party(party, sizeof(party), &session));
    ASSERT_TRUE(memcmp(before, party, sizeof(party)) == 0);
}

static void test_dungeon_entry_sets_mode_and_turn_without_overwriting_return_position(void)
{
    uint8_t party[64] = {0};
    u3_location_transition_result request =
        make_request(U3_LOCATION_KIND_DUNGEON, 12, 1, 1, 1);

    party[U3_OVERWORLD_PARTY_SIZE_OFFSET] = 4;
    party[U3_OVERWORLD_PARTY_X_OFFSET] = 46;
    party[U3_OVERWORLD_PARTY_Y_OFFSET] = 19;
    ASSERT_TRUE(u3_location_enter_party(party, sizeof(party), &request));
    ASSERT_EQ_INT(U3_LOCATION_PARTY_MODE_DUNGEON,
                  party[U3_LOCATION_PARTY_MODE_OFFSET]);
    ASSERT_EQ_INT(46, party[U3_OVERWORLD_PARTY_X_OFFSET]);
    ASSERT_EQ_INT(19, party[U3_OVERWORLD_PARTY_Y_OFFSET]);
    ASSERT_TRUE(request.turn_applied);
    ASSERT_EQ_INT(4, (int)request.move_counter_after);
}

int main(void)
{
    test_detects_lcb_towne_fixture();
    test_detects_only_dungeon_doom_fixture_for_first_slice();
    test_enter_rejects_unrecognized_coordinate_without_mutation();
    test_matching_non_town_tile_is_not_enterable();
    test_non_enter_command_is_ignored();
    test_rejects_truncated_inputs();
    test_initializes_town_castle_and_dungeon_sessions();
    test_session_validation_rejects_partial_or_mismatched_records();
    test_resource_id_mapping_preserves_legacy_gap();
    test_entry_and_return_mutate_only_party_transition_fields();
    test_invalid_entry_and_return_leave_party_unchanged();
    test_dungeon_entry_sets_mode_and_turn_without_overwriting_return_position();

    printf("location transition tests passed\n");
    return 0;
}
