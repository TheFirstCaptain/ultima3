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
