#ifndef U3_LOCATION_TALK_H
#define U3_LOCATION_TALK_H

#include <stdint.h>

#include "u3_location.h"

#define U3_LOCATION_TALK_COMMAND 84
#define U3_LOCATION_TALK_MONSTER_COUNT 32
#define U3_LOCATION_TALK_MONSTER_TYPE_OFFSET 0
#define U3_LOCATION_TALK_MONSTER_X_OFFSET 64
#define U3_LOCATION_TALK_MONSTER_Y_OFFSET 96
#define U3_LOCATION_TALK_MONSTER_STATE_OFFSET 128
#define U3_LOCATION_TALK_INDEX_MASK 0x0f
#define U3_LOCATION_TALK_MESSAGE_CAPACITY 256

#define U3_LOCATION_TALK_STATUS_NONE 0
#define U3_LOCATION_TALK_STATUS_MESSAGE 1
#define U3_LOCATION_TALK_STATUS_NO_NPC 2
#define U3_LOCATION_TALK_STATUS_INVALID_INDEX 3
#define U3_LOCATION_TALK_STATUS_UNSUPPORTED 4
#define U3_LOCATION_TALK_STATUS_INVALID_INPUT 5

typedef struct u3_location_talk_result {
    uint8_t handled;
    uint8_t found_npc;
    uint8_t npc_slot;
    uint8_t talk_index;
    uint8_t target_x;
    uint8_t target_y;
    uint8_t status;
    uint16_t message_length;
    uint8_t message[U3_LOCATION_TALK_MESSAGE_CAPACITY];
} u3_location_talk_result;

uint8_t u3_location_talk(
    const u3_location_session *session,
    const uint8_t *monster_record,
    uint32_t monster_record_length,
    const uint8_t *talk_record,
    uint32_t talk_record_length,
    uint16_t direction,
    u3_location_talk_result *result);
uint8_t u3_location_decode_talk_entry(
    const uint8_t *talk_record,
    uint32_t talk_record_length,
    uint8_t talk_index,
    u3_location_talk_result *result);

#endif
