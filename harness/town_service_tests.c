#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_audio.h"
#include "u3_party.h"
#include "u3_save.h"
#include "u3_town_service.h"

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

static void write_u16(uint8_t *bytes, uint32_t offset, uint16_t value)
{
    bytes[offset] = (uint8_t)(value >> 8);
    bytes[offset + 1] = (uint8_t)(value & 0xff);
}

static uint16_t read_u16(const uint8_t *bytes, uint32_t offset)
{
    return (uint16_t)(((uint16_t)bytes[offset] << 8) | bytes[offset + 1]);
}

static u3_location_session make_town_session(void)
{
    u3_location_session session;

    memset(&session, 0, sizeof(session));
    session.active = 1;
    session.destination_kind = U3_LOCATION_KIND_TOWN;
    session.location_index = 2;
    session.resource_id = 402;
    session.map_shape = U3_LOCATION_MAP_SHAPE_TWO_DIMENSIONAL;
    session.map_size = U3_LOCATION_TWO_DIMENSIONAL_MAP_SIZE;
    session.map_length = U3_LOCATION_TWO_DIMENSIONAL_MAP_LENGTH;
    session.monster_length = U3_LOCATION_MONSTER_LENGTH;
    session.talk_length = U3_LOCATION_TALK_LENGTH;
    session.x = 1;
    session.y = 32;
    session.heading = 2;
    return session;
}

static void make_party(uint8_t *party, uint8_t *roster)
{
    uint8_t index;

    memset(party, 0, U3_SAVE_PARTY_LENGTH);
    memset(roster, 0, U3_SAVE_ROSTER_LENGTH);
    party[1] = 4;
    for (index = 0; index < 4; index++) {
        uint8_t *record = roster + ((uint32_t)index * U3_PARTY_ROSTER_RECORD_LENGTH);

        party[6 + index] = (uint8_t)(index + 1);
        record[0] = 'A';
        record[U3_PARTY_ROSTER_STATUS_OFFSET] = 'G';
        write_u16(record, 26, 50);
        write_u16(record, 28, 100);
        write_u16(record, 35, 250);
    }
}

static void test_healer_prompts_before_selection(void)
{
    u3_location_session session = make_town_session();
    u3_town_service_input input;
    u3_town_service_result result;

    memset(&input, 0, sizeof(input));
    input.command = U3_TOWN_SERVICE_COMMAND_HEAL;

    result = u3_town_service_apply(&session, input, 0, 0, 0, 0);

    ASSERT_TRUE(result.handled);
    ASSERT_TRUE(result.requires_selection);
    ASSERT_EQ_INT(U3_TOWN_SERVICE_KIND_HEALER, result.kind);
    ASSERT_EQ_INT(U3_TOWN_SERVICE_STATUS_SELECTION_REQUIRED, result.status);
}

static void test_healer_restores_hp_and_charges_gold(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_location_session session = make_town_session();
    u3_town_service_input input;
    u3_town_service_result result;

    make_party(party, roster);
    memset(&input, 0, sizeof(input));
    input.command = U3_TOWN_SERVICE_COMMAND_HEAL;
    input.selected_active_slot = 1;

    result = u3_town_service_apply(&session, input, party, sizeof(party), roster, sizeof(roster));

    ASSERT_EQ_INT(U3_TOWN_SERVICE_STATUS_HEALED, result.status);
    ASSERT_FALSE(result.requires_selection);
    ASSERT_EQ_INT(1, result.roster_id);
    ASSERT_EQ_INT(50, result.hit_points_before);
    ASSERT_EQ_INT(100, result.hit_points_after);
    ASSERT_EQ_INT(250, result.gold_before);
    ASSERT_EQ_INT(50, result.gold_after);
    ASSERT_EQ_INT(U3_TOWN_SERVICE_HEAL_COST, result.cost);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_HEAL, result.sound_id);
    ASSERT_EQ_INT(100, read_u16(roster, 26));
    ASSERT_EQ_INT(50, read_u16(roster, 35));
}

static void test_healer_rejects_insufficient_gold_without_mutation(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_location_session session = make_town_session();
    u3_town_service_input input;
    u3_town_service_result result;

    make_party(party, roster);
    write_u16(roster, 35, 199);
    memset(&input, 0, sizeof(input));
    input.command = U3_TOWN_SERVICE_COMMAND_HEAL;
    input.selected_active_slot = 1;

    result = u3_town_service_apply(&session, input, party, sizeof(party), roster, sizeof(roster));

    ASSERT_EQ_INT(U3_TOWN_SERVICE_STATUS_INSUFFICIENT_GOLD, result.status);
    ASSERT_EQ_INT(50, result.hit_points_after);
    ASSERT_EQ_INT(199, result.gold_after);
    ASSERT_EQ_INT(50, read_u16(roster, 26));
    ASSERT_EQ_INT(199, read_u16(roster, 35));
    ASSERT_EQ_INT(U3_AUDIO_SOUND_BUMP, result.sound_id);
}

static void test_healer_rejects_cancel_full_incapacitated_and_invalid_inputs(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_location_session session = make_town_session();
    u3_town_service_input input;
    u3_town_service_result result;

    make_party(party, roster);
    memset(&input, 0, sizeof(input));
    input.command = U3_TOWN_SERVICE_COMMAND_HEAL;
    input.selected_active_slot = 5;
    result = u3_town_service_apply(&session, input, party, sizeof(party), roster, sizeof(roster));
    ASSERT_EQ_INT(U3_TOWN_SERVICE_STATUS_CANCELLED, result.status);

    input.selected_active_slot = 1;
    write_u16(roster, 26, 100);
    result = u3_town_service_apply(&session, input, party, sizeof(party), roster, sizeof(roster));
    ASSERT_EQ_INT(U3_TOWN_SERVICE_STATUS_ALREADY_FULL, result.status);
    ASSERT_EQ_INT(250, read_u16(roster, 35));

    write_u16(roster, 26, 50);
    roster[U3_PARTY_ROSTER_STATUS_OFFSET] = 'D';
    result = u3_town_service_apply(&session, input, party, sizeof(party), roster, sizeof(roster));
    ASSERT_EQ_INT(U3_TOWN_SERVICE_STATUS_INCAPACITATED, result.status);

    session.destination_kind = U3_LOCATION_KIND_CASTLE;
    result = u3_town_service_apply(&session, input, party, sizeof(party), roster, sizeof(roster));
    ASSERT_EQ_INT(U3_TOWN_SERVICE_STATUS_INVALID_INPUT, result.status);
}

static void test_unsupported_command_is_not_handled(void)
{
    u3_location_session session = make_town_session();
    u3_town_service_input input;
    u3_town_service_result result;

    memset(&input, 0, sizeof(input));
    input.command = 'X';
    result = u3_town_service_apply(&session, input, 0, 0, 0, 0);

    ASSERT_FALSE(result.handled);
    ASSERT_EQ_INT(U3_TOWN_SERVICE_STATUS_NONE, result.status);
}

int main(void)
{
    test_healer_prompts_before_selection();
    test_healer_restores_hp_and_charges_gold();
    test_healer_rejects_insufficient_gold_without_mutation();
    test_healer_rejects_cancel_full_incapacitated_and_invalid_inputs();
    test_unsupported_command_is_not_handled();

    puts("town service tests passed");
    return 0;
}
