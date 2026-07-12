#include "u3_location_talk.h"

#include <string.h>

static uint8_t u3_location_talk_target(
    const u3_location_session *session,
    uint16_t direction,
    uint8_t *target_x,
    uint8_t *target_y)
{
    int16_t x;
    int16_t y;

    if (session == 0 || target_x == 0 || target_y == 0)
        return 0;

    x = session->x;
    y = session->y;
    switch (direction) {
    case U3_OVERWORLD_COMMAND_NORTH:
        y--;
        break;
    case U3_OVERWORLD_COMMAND_SOUTH:
        y++;
        break;
    case U3_OVERWORLD_COMMAND_WEST:
        x--;
        break;
    case U3_OVERWORLD_COMMAND_EAST:
        x++;
        break;
    default:
        return 0;
    }

    if (x < 0 || y < 0 || x >= session->map_size || y >= session->map_size)
        return 0;
    *target_x = (uint8_t)x;
    *target_y = (uint8_t)y;
    return 1;
}

static uint8_t u3_location_decode_talk_entry_unchecked(
    const uint8_t *talk_record,
    uint32_t talk_record_length,
    uint8_t talk_index,
    u3_location_talk_result *result)
{
    uint32_t offset = 0;
    uint8_t entry = 0;

    result->handled = 1;
    while (entry < talk_index) {
        while (offset < talk_record_length && talk_record[offset] != 0)
            offset++;
        if (offset >= talk_record_length) {
            result->status = U3_LOCATION_TALK_STATUS_INVALID_INDEX;
            return 1;
        }
        offset++;
        entry++;
    }

    if (offset >= talk_record_length || talk_record[offset] == 0) {
        result->status = U3_LOCATION_TALK_STATUS_INVALID_INDEX;
        return 1;
    }

    while (offset < talk_record_length && talk_record[offset] != 0) {
        uint8_t byte = talk_record[offset++];
        uint8_t decoded;

        if (byte == 0xff)
            decoded = '\n';
        else
            decoded = (uint8_t)(byte & 0x7f);
        if (decoded != '\n' && (decoded < 0x20 || decoded > 0x7e)) {
            result->message_length = 0;
            result->status = U3_LOCATION_TALK_STATUS_UNSUPPORTED;
            return 1;
        }
        if (result->message_length >= U3_LOCATION_TALK_MESSAGE_CAPACITY) {
            result->message_length = 0;
            result->status = U3_LOCATION_TALK_STATUS_UNSUPPORTED;
            return 1;
        }
        result->message[result->message_length++] = decoded;
    }

    if (offset >= talk_record_length) {
        result->message_length = 0;
        result->status = U3_LOCATION_TALK_STATUS_INVALID_INDEX;
        return 1;
    }

    result->status = U3_LOCATION_TALK_STATUS_MESSAGE;
    return 1;
}

uint8_t u3_location_decode_talk_entry(
    const uint8_t *talk_record,
    uint32_t talk_record_length,
    uint8_t talk_index,
    u3_location_talk_result *result)
{
    if (result == 0)
        return 0;
    memset(result, 0, sizeof(*result));
    if (talk_record == 0 || talk_record_length != U3_LOCATION_TALK_LENGTH) {
        result->status = U3_LOCATION_TALK_STATUS_INVALID_INPUT;
        return 0;
    }
    return u3_location_decode_talk_entry_unchecked(talk_record, talk_record_length, talk_index, result);
}

uint8_t u3_location_talk(
    const u3_location_session *session,
    const uint8_t *monster_record,
    uint32_t monster_record_length,
    const uint8_t *talk_record,
    uint32_t talk_record_length,
    uint16_t direction,
    u3_location_talk_result *result)
{
    int16_t slot;

    if (result == 0)
        return 0;
    memset(result, 0, sizeof(*result));
    if (session == 0 || session->active == 0 ||
        session->destination_kind != U3_LOCATION_KIND_TOWN ||
        session->map_shape != U3_LOCATION_MAP_SHAPE_TWO_DIMENSIONAL ||
        session->map_size != U3_LOCATION_TWO_DIMENSIONAL_MAP_SIZE ||
        session->x >= session->map_size || session->y >= session->map_size ||
        monster_record == 0 || monster_record_length != U3_LOCATION_MONSTER_LENGTH ||
        talk_record == 0 || talk_record_length != U3_LOCATION_TALK_LENGTH) {
        result->status = U3_LOCATION_TALK_STATUS_INVALID_INPUT;
        return 0;
    }

    result->handled = 1;
    if (!u3_location_talk_target(session, direction, &result->target_x, &result->target_y)) {
        result->status = U3_LOCATION_TALK_STATUS_INVALID_INPUT;
        return 1;
    }

    for (slot = U3_LOCATION_TALK_MONSTER_COUNT - 1; slot >= 0; slot--) {
        if (monster_record[U3_LOCATION_TALK_MONSTER_TYPE_OFFSET + slot] == 0 ||
            monster_record[U3_LOCATION_TALK_MONSTER_X_OFFSET + slot] != result->target_x ||
            monster_record[U3_LOCATION_TALK_MONSTER_Y_OFFSET + slot] != result->target_y)
            continue;

        result->found_npc = 1;
        result->npc_slot = (uint8_t)slot;
        result->talk_index = (uint8_t)(
            monster_record[U3_LOCATION_TALK_MONSTER_STATE_OFFSET + slot] &
            U3_LOCATION_TALK_INDEX_MASK);
        if (result->talk_index == 0) {
            result->status = U3_LOCATION_TALK_STATUS_UNSUPPORTED;
            return 1;
        }
        return u3_location_decode_talk_entry_unchecked(
            talk_record,
            talk_record_length,
            result->talk_index,
            result);
    }

    result->status = U3_LOCATION_TALK_STATUS_NO_NPC;
    return 1;
}
