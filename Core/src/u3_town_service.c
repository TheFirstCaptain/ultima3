#include "u3_town_service.h"

#include <string.h>

#include "u3_audio.h"
#include "u3_party.h"
#include "u3_save.h"

static uint16_t u3_town_service_read_u16(const uint8_t *bytes, uint32_t offset)
{
    return (uint16_t)(((uint16_t)bytes[offset] << 8) | bytes[offset + 1]);
}

static void u3_town_service_write_u16(uint8_t *bytes, uint32_t offset, uint16_t value)
{
    bytes[offset] = (uint8_t)(value >> 8);
    bytes[offset + 1] = (uint8_t)(value & 0xff);
}

static uint8_t u3_town_service_valid_party_roster(const uint8_t *party,
                                                  uint32_t party_length,
                                                  const uint8_t *roster,
                                                  uint32_t roster_length)
{
    return (uint8_t)(party != 0 &&
                     roster != 0 &&
                     party_length == U3_SAVE_PARTY_LENGTH &&
                     roster_length == U3_SAVE_ROSTER_LENGTH &&
                     party[1] >= 1 &&
                     party[1] <= U3_PARTY_ACTIVE_SLOT_COUNT);
}

static uint8_t *u3_town_service_roster_record(uint8_t *roster, uint8_t roster_id)
{
    if (roster == 0 || roster_id < 1 || roster_id > U3_PARTY_ROSTER_SLOT_COUNT)
        return 0;
    return roster + ((uint32_t)(roster_id - 1) * U3_PARTY_ROSTER_RECORD_LENGTH);
}

static uint8_t u3_town_service_roster_occupied(const uint8_t *record)
{
    return (uint8_t)(record != 0 && record[0] != 0);
}

static uint8_t u3_town_service_roster_living(const uint8_t *record)
{
    return (uint8_t)(record != 0 &&
                     (record[U3_PARTY_ROSTER_STATUS_OFFSET] == 'G' ||
                      record[U3_PARTY_ROSTER_STATUS_OFFSET] == 'P'));
}

static uint8_t u3_town_service_find_selected(uint8_t *party,
                                             uint8_t *roster,
                                             uint8_t selected_active_slot,
                                             uint8_t *roster_id,
                                             uint8_t **record)
{
    uint8_t candidate_id;
    uint8_t *candidate;

    if (selected_active_slot < 1 || selected_active_slot > party[1])
        return 0;

    candidate_id = party[6 + (selected_active_slot - 1)];
    candidate = u3_town_service_roster_record(roster, candidate_id);
    if (!u3_town_service_roster_occupied(candidate))
        return 0;

    *roster_id = candidate_id;
    *record = candidate;
    return 1;
}

static uint8_t u3_town_service_valid_session(const u3_location_session *session)
{
    return (uint8_t)(session != 0 &&
                     session->active != 0 &&
                     session->destination_kind == U3_LOCATION_KIND_TOWN &&
                     session->map_shape == U3_LOCATION_MAP_SHAPE_TWO_DIMENSIONAL &&
                     session->map_size == U3_LOCATION_TWO_DIMENSIONAL_MAP_SIZE);
}

u3_town_service_result u3_town_service_apply(
    const u3_location_session *session,
    u3_town_service_input input,
    uint8_t *party,
    uint32_t party_length,
    uint8_t *roster,
    uint32_t roster_length)
{
    u3_town_service_result result;
    uint8_t roster_id = 0;
    uint8_t *record = 0;

    memset(&result, 0, sizeof(result));
    if (input.command != U3_TOWN_SERVICE_COMMAND_HEAL)
        return result;

    result.handled = 1;
    result.kind = U3_TOWN_SERVICE_KIND_HEALER;
    result.cost = U3_TOWN_SERVICE_HEAL_COST;
    if (!u3_town_service_valid_session(session)) {
        result.status = U3_TOWN_SERVICE_STATUS_INVALID_INPUT;
        result.sound_id = U3_AUDIO_SOUND_BUMP;
        return result;
    }

    if (input.selected_active_slot == 0) {
        result.requires_selection = 1;
        result.status = U3_TOWN_SERVICE_STATUS_SELECTION_REQUIRED;
        return result;
    }

    if (input.selected_active_slot > U3_PARTY_ACTIVE_SLOT_COUNT) {
        result.status = U3_TOWN_SERVICE_STATUS_CANCELLED;
        result.sound_id = U3_AUDIO_SOUND_BUMP;
        return result;
    }

    if (!u3_town_service_valid_party_roster(party, party_length, roster, roster_length)) {
        result.status = U3_TOWN_SERVICE_STATUS_INVALID_INPUT;
        result.sound_id = U3_AUDIO_SOUND_BUMP;
        return result;
    }

    if (!u3_town_service_find_selected(party, roster, input.selected_active_slot, &roster_id, &record)) {
        result.status = U3_TOWN_SERVICE_STATUS_INVALID_CHARACTER;
        result.active_slot = input.selected_active_slot;
        result.sound_id = U3_AUDIO_SOUND_BUMP;
        return result;
    }

    result.active_slot = input.selected_active_slot;
    result.roster_id = roster_id;
    result.status_before = record[U3_PARTY_ROSTER_STATUS_OFFSET];
    result.status_after = result.status_before;
    result.hit_points_before = u3_town_service_read_u16(record, 26);
    result.hit_points_after = result.hit_points_before;
    result.max_hit_points = u3_town_service_read_u16(record, 28);
    result.gold_before = u3_town_service_read_u16(record, 35);
    result.gold_after = result.gold_before;

    if (!u3_town_service_roster_living(record)) {
        result.status = U3_TOWN_SERVICE_STATUS_INCAPACITATED;
        result.sound_id = U3_AUDIO_SOUND_BUMP;
        return result;
    }

    if (result.hit_points_before >= result.max_hit_points) {
        result.status = U3_TOWN_SERVICE_STATUS_ALREADY_FULL;
        result.sound_id = U3_AUDIO_SOUND_BUMP;
        return result;
    }

    if (result.gold_before < U3_TOWN_SERVICE_HEAL_COST) {
        result.status = U3_TOWN_SERVICE_STATUS_INSUFFICIENT_GOLD;
        result.sound_id = U3_AUDIO_SOUND_BUMP;
        return result;
    }

    u3_town_service_write_u16(record, 26, result.max_hit_points);
    u3_town_service_write_u16(record, 35, (uint16_t)(result.gold_before - U3_TOWN_SERVICE_HEAL_COST));
    result.hit_points_after = result.max_hit_points;
    result.gold_after = (uint16_t)(result.gold_before - U3_TOWN_SERVICE_HEAL_COST);
    result.status = U3_TOWN_SERVICE_STATUS_HEALED;
    result.sound_id = U3_AUDIO_SOUND_HEAL;
    return result;
}
