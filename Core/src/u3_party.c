#include "u3_party.h"

#include <string.h>

static uint16_t u3_party_read_u16(const uint8_t *bytes, uint32_t offset)
{
    return (uint16_t)(((uint16_t)bytes[offset] << 8) | bytes[offset + 1]);
}

static void u3_party_decode_name(const uint8_t *record, char *name)
{
    uint32_t index;

    for (index = 0; index < U3_PARTY_NAME_CAPACITY; index++) {
        if (record[index] == 0)
            break;
        name[index] = (char)(record[index] & 0x7F);
    }
    name[index] = 0;
}

int u3_party_load_summary(const u3_save_domain_state *state, u3_party_summary *summary)
{
    u3_party_summary loaded;
    uint32_t index;

    if (state == 0 || summary == 0)
        return 0;
    if (state->party == 0 || state->party_length != U3_SAVE_PARTY_LENGTH)
        return 0;
    if (state->roster == 0 || state->roster_length != U3_SAVE_ROSTER_LENGTH)
        return 0;

    memset(&loaded, 0, sizeof(loaded));
    loaded.party_size = state->party[1];
    if (loaded.party_size > U3_PARTY_ACTIVE_SLOT_COUNT)
        return 0;

    for (index = 0; index < U3_PARTY_ACTIVE_SLOT_COUNT; index++) {
        uint8_t roster_id = state->party[6 + index];

        if (roster_id > U3_PARTY_ROSTER_SLOT_COUNT)
            return 0;
        if (index < loaded.party_size && roster_id == 0)
            return 0;
        loaded.active_roster_ids[index] = roster_id;
    }

    for (index = 0; index < U3_PARTY_ROSTER_SLOT_COUNT; index++) {
        const uint8_t *record = state->roster + (index * U3_PARTY_ROSTER_RECORD_LENGTH);
        u3_party_roster_entry *entry = &loaded.roster[index];

        entry->roster_id = (uint8_t)(index + 1);
        entry->occupied = record[0] > 22 ? 1 : 0;
        if (!entry->occupied)
            continue;

        u3_party_decode_name(record, entry->name);
        entry->status = record[17];
        entry->race = record[22];
        entry->character_class = record[23];
        entry->sex = record[24];
        entry->hit_points = u3_party_read_u16(record, 26);
        entry->max_hit_points = u3_party_read_u16(record, 28);
        entry->level = (uint8_t)(record[30] + 1);
        entry->food = (uint16_t)((record[32] * 100) + record[33]);
        entry->gold = u3_party_read_u16(record, 35);
        loaded.occupied_roster_count++;
    }

    for (index = 0; index < loaded.party_size; index++) {
        uint8_t roster_id = loaded.active_roster_ids[index];

        if (!loaded.roster[roster_id - 1].occupied)
            return 0;
    }

    *summary = loaded;
    return 1;
}

static u3_party_form_result u3_party_make_form_result(uint8_t formed, uint8_t reason)
{
    u3_party_form_result result;

    memset(&result, 0, sizeof(result));
    result.formed = formed;
    result.reason = reason;
    return result;
}

u3_party_form_result u3_party_form_from_roster(const u3_save_domain_state *state, const uint8_t *selected_roster_ids, uint8_t selected_count, uint8_t *party, uint32_t party_length)
{
    u3_party_summary summary;
    u3_party_form_result result;
    uint8_t seen[U3_PARTY_ROSTER_SLOT_COUNT + 1];
    uint8_t candidate[U3_SAVE_PARTY_LENGTH];
    uint32_t index;

    if (state == 0 || selected_roster_ids == 0 || party == 0)
        return u3_party_make_form_result(0, U3_PARTY_FORM_INVALID_ARGUMENT);
    if (party_length != U3_SAVE_PARTY_LENGTH)
        return u3_party_make_form_result(0, U3_PARTY_FORM_INVALID_PARTY);
    if (selected_count == 0 || selected_count > U3_PARTY_ACTIVE_SLOT_COUNT)
        return u3_party_make_form_result(0, U3_PARTY_FORM_INVALID_SIZE);
    if (!u3_party_load_summary(state, &summary))
        return u3_party_make_form_result(0, U3_PARTY_FORM_INVALID_ROSTER);

    memset(seen, 0, sizeof(seen));
    for (index = 0; index < selected_count; index++) {
        uint8_t roster_id = selected_roster_ids[index];

        if (roster_id == 0 || roster_id > U3_PARTY_ROSTER_SLOT_COUNT)
            return u3_party_make_form_result(0, U3_PARTY_FORM_INVALID_ROSTER_ID);
        if (seen[roster_id])
            return u3_party_make_form_result(0, U3_PARTY_FORM_DUPLICATE_ROSTER_ID);
        if (!summary.roster[roster_id - 1].occupied)
            return u3_party_make_form_result(0, U3_PARTY_FORM_UNOCCUPIED_ROSTER_ID);
        seen[roster_id] = 1;
    }

    memset(candidate, 0, sizeof(candidate));
    candidate[0] = 0x7E;
    candidate[1] = selected_count;
    candidate[2] = 0;
    candidate[3] = 42;
    candidate[4] = 20;
    candidate[5] = 255;
    for (index = 0; index < selected_count; index++)
        candidate[6 + index] = selected_roster_ids[index];

    memcpy(party, candidate, sizeof(candidate));

    result = u3_party_make_form_result(1, U3_PARTY_FORM_OK);
    result.party_size = selected_count;
    for (index = 0; index < selected_count; index++)
        result.active_roster_ids[index] = selected_roster_ids[index];
    return result;
}
