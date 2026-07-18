#ifndef U3_TOWN_SERVICE_H
#define U3_TOWN_SERVICE_H

#include <stdint.h>

#include "u3_location.h"

#define U3_TOWN_SERVICE_COMMAND_HEAL 'H'

#define U3_TOWN_SERVICE_KIND_NONE 0
#define U3_TOWN_SERVICE_KIND_HEALER 1

#define U3_TOWN_SERVICE_STATUS_NONE 0
#define U3_TOWN_SERVICE_STATUS_SELECTION_REQUIRED 1
#define U3_TOWN_SERVICE_STATUS_CANCELLED 2
#define U3_TOWN_SERVICE_STATUS_INVALID_INPUT 3
#define U3_TOWN_SERVICE_STATUS_INVALID_CHARACTER 4
#define U3_TOWN_SERVICE_STATUS_INCAPACITATED 5
#define U3_TOWN_SERVICE_STATUS_INSUFFICIENT_GOLD 6
#define U3_TOWN_SERVICE_STATUS_ALREADY_FULL 7
#define U3_TOWN_SERVICE_STATUS_HEALED 8
#define U3_TOWN_SERVICE_STATUS_UNSUPPORTED 9

#define U3_TOWN_SERVICE_HEAL_COST 200

typedef struct u3_town_service_input {
    uint16_t command;
    uint8_t selected_active_slot;
} u3_town_service_input;

typedef struct u3_town_service_result {
    uint8_t handled;
    uint8_t requires_selection;
    uint8_t kind;
    uint8_t status;
    uint8_t active_slot;
    uint8_t roster_id;
    uint8_t status_before;
    uint8_t status_after;
    uint16_t hit_points_before;
    uint16_t hit_points_after;
    uint16_t max_hit_points;
    uint16_t gold_before;
    uint16_t gold_after;
    uint16_t cost;
    uint16_t sound_id;
} u3_town_service_result;

u3_town_service_result u3_town_service_apply(
    const u3_location_session *session,
    u3_town_service_input input,
    uint8_t *party,
    uint32_t party_length,
    uint8_t *roster,
    uint32_t roster_length);

#endif
