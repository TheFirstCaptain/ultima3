#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_location_talk.h"
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

static u3_location_session make_session(uint8_t x, uint8_t y)
{
    u3_location_session session;

    memset(&session, 0, sizeof(session));
    session.active = 1;
    session.destination_kind = U3_LOCATION_KIND_TOWN;
    session.map_shape = U3_LOCATION_MAP_SHAPE_TWO_DIMENSIONAL;
    session.map_size = U3_LOCATION_TWO_DIMENSIONAL_MAP_SIZE;
    session.x = x;
    session.y = y;
    return session;
}

static void place_npc(uint8_t *monsters,
                      uint8_t slot,
                      uint8_t x,
                      uint8_t y,
                      uint8_t talk_index)
{
    monsters[slot] = 84;
    monsters[U3_LOCATION_TALK_MONSTER_X_OFFSET + slot] = x;
    monsters[U3_LOCATION_TALK_MONSTER_Y_OFFSET + slot] = y;
    monsters[U3_LOCATION_TALK_MONSTER_STATE_OFFSET + slot] = talk_index;
}

static void test_decodes_stable_lcb_towne_guard_line(void)
{
    size_t length;
    uint8_t *bytes = read_file("../Resources/English.lproj/MainResources.rsrc", &length);
    u3_resource_file file;
    u3_resource_record monsters;
    u3_resource_record talk;
    u3_location_session session = make_session(26, 32);
    u3_location_talk_result result;
    const char expected[] = "\nHO! HO!\n\n";

    ASSERT_TRUE(bytes != 0);
    ASSERT_TRUE(u3_resource_open(bytes, length, &file));
    ASSERT_TRUE(u3_resource_find(&file, U3_RESOURCE_TYPE('M', 'O', 'N', 'S'), 402, &monsters));
    ASSERT_TRUE(u3_resource_find(&file, U3_RESOURCE_TYPE('T', 'L', 'K', 'S'), 402, &talk));
    ASSERT_TRUE(u3_location_talk(&session, monsters.data, monsters.length,
                                 talk.data, talk.length,
                                 U3_OVERWORLD_COMMAND_EAST, &result));
    ASSERT_TRUE(result.handled);
    ASSERT_TRUE(result.found_npc);
    ASSERT_EQ_INT(31, result.npc_slot);
    ASSERT_EQ_INT(8, result.talk_index);
    ASSERT_EQ_INT(U3_LOCATION_TALK_STATUS_MESSAGE, result.status);
    ASSERT_EQ_INT((int)strlen(expected), result.message_length);
    ASSERT_TRUE(memcmp(expected, result.message, result.message_length) == 0);
    free(bytes);
}

static void test_uses_explicit_direction_and_reverse_slot_priority(void)
{
    uint8_t monsters[U3_LOCATION_MONSTER_LENGTH] = {0};
    uint8_t talk[U3_LOCATION_TALK_LENGTH] = {0};
    uint8_t monsters_before[U3_LOCATION_MONSTER_LENGTH];
    uint8_t talk_before[U3_LOCATION_TALK_LENGTH];
    u3_location_session session = make_session(3, 3);
    u3_location_talk_result result;

    place_npc(monsters, 4, 4, 3, 1);
    place_npc(monsters, 9, 4, 3, 1);
    talk[1] = 'H';
    talk[2] = 'I';
    memcpy(monsters_before, monsters, sizeof(monsters));
    memcpy(talk_before, talk, sizeof(talk));

    ASSERT_TRUE(u3_location_talk(&session, monsters, sizeof(monsters),
                                 talk, sizeof(talk),
                                 U3_OVERWORLD_COMMAND_EAST, &result));
    ASSERT_EQ_INT(9, result.npc_slot);
    ASSERT_EQ_INT(2, result.message_length);
    ASSERT_TRUE(memcmp("HI", result.message, 2) == 0);
    ASSERT_TRUE(memcmp(monsters_before, monsters, sizeof(monsters)) == 0);
    ASSERT_TRUE(memcmp(talk_before, talk, sizeof(talk)) == 0);
}

static void test_reports_no_npc_and_invalid_direction(void)
{
    uint8_t monsters[U3_LOCATION_MONSTER_LENGTH] = {0};
    uint8_t talk[U3_LOCATION_TALK_LENGTH] = {0};
    u3_location_session session = make_session(3, 3);
    u3_location_talk_result result;

    ASSERT_TRUE(u3_location_talk(&session, monsters, sizeof(monsters),
                                 talk, sizeof(talk),
                                 U3_OVERWORLD_COMMAND_NORTH, &result));
    ASSERT_EQ_INT(U3_LOCATION_TALK_STATUS_NO_NPC, result.status);
    ASSERT_FALSE(result.found_npc);

    ASSERT_TRUE(u3_location_talk(&session, monsters, sizeof(monsters),
                                 talk, sizeof(talk), 999, &result));
    ASSERT_EQ_INT(U3_LOCATION_TALK_STATUS_INVALID_INPUT, result.status);
}

static void test_rejects_invalid_index_unsupported_entry_and_truncation(void)
{
    uint8_t monsters[U3_LOCATION_MONSTER_LENGTH] = {0};
    uint8_t talk[U3_LOCATION_TALK_LENGTH];
    u3_location_session session = make_session(3, 3);
    u3_location_talk_result result;

    place_npc(monsters, 4, 4, 3, 2);
    memset(talk, 'A', sizeof(talk));
    talk[0] = 0;
    ASSERT_TRUE(u3_location_talk(&session, monsters, sizeof(monsters),
                                 talk, sizeof(talk),
                                 U3_OVERWORLD_COMMAND_EAST, &result));
    ASSERT_EQ_INT(U3_LOCATION_TALK_STATUS_INVALID_INDEX, result.status);
    ASSERT_EQ_INT(0, result.message_length);

    memset(talk, 0, sizeof(talk));
    monsters[U3_LOCATION_TALK_MONSTER_STATE_OFFSET + 4] = 1;
    talk[1] = 1;
    ASSERT_TRUE(u3_location_talk(&session, monsters, sizeof(monsters),
                                 talk, sizeof(talk),
                                 U3_OVERWORLD_COMMAND_EAST, &result));
    ASSERT_EQ_INT(U3_LOCATION_TALK_STATUS_UNSUPPORTED, result.status);
    ASSERT_EQ_INT(0, result.message_length);

    ASSERT_FALSE(u3_location_talk(&session, monsters, sizeof(monsters) - 1,
                                  talk, sizeof(talk),
                                  U3_OVERWORLD_COMMAND_EAST, &result));
    ASSERT_EQ_INT(U3_LOCATION_TALK_STATUS_INVALID_INPUT, result.status);
    ASSERT_FALSE(result.handled);
    ASSERT_FALSE(u3_location_talk(&session, monsters, sizeof(monsters),
                                  talk, sizeof(talk) - 1,
                                  U3_OVERWORLD_COMMAND_EAST, &result));
    ASSERT_EQ_INT(U3_LOCATION_TALK_STATUS_INVALID_INPUT, result.status);
}

static void test_rejects_malformed_or_non_town_sessions(void)
{
    uint8_t monsters[U3_LOCATION_MONSTER_LENGTH] = {0};
    uint8_t talk[U3_LOCATION_TALK_LENGTH] = {0};
    u3_location_session session = make_session(3, 3);
    u3_location_talk_result result;

    session.map_size = 300;
    session.x = 255;
    ASSERT_FALSE(u3_location_talk(&session, monsters, sizeof(monsters),
                                  talk, sizeof(talk),
                                  U3_OVERWORLD_COMMAND_EAST, &result));
    ASSERT_EQ_INT(U3_LOCATION_TALK_STATUS_INVALID_INPUT, result.status);

    session = make_session(64, 3);
    ASSERT_FALSE(u3_location_talk(&session, monsters, sizeof(monsters),
                                  talk, sizeof(talk),
                                  U3_OVERWORLD_COMMAND_WEST, &result));
    ASSERT_EQ_INT(U3_LOCATION_TALK_STATUS_INVALID_INPUT, result.status);

    session = make_session(3, 3);
    session.destination_kind = U3_LOCATION_KIND_CASTLE;
    ASSERT_FALSE(u3_location_talk(&session, monsters, sizeof(monsters),
                                  talk, sizeof(talk),
                                  U3_OVERWORLD_COMMAND_NORTH, &result));
    ASSERT_EQ_INT(U3_LOCATION_TALK_STATUS_INVALID_INPUT, result.status);
}

int main(void)
{
    test_decodes_stable_lcb_towne_guard_line();
    test_uses_explicit_direction_and_reverse_slot_priority();
    test_reports_no_npc_and_invalid_direction();
    test_rejects_invalid_index_unsupported_entry_and_truncation();
    test_rejects_malformed_or_non_town_sessions();

    printf("location talk tests passed\n");
    return 0;
}
