#ifndef U3_PARTY_H
#define U3_PARTY_H

#include <stdint.h>

#include "u3_save.h"

#define U3_PARTY_ACTIVE_SLOT_COUNT 4
#define U3_PARTY_ROSTER_SLOT_COUNT 20
#define U3_PARTY_ROSTER_RECORD_LENGTH 64
#define U3_PARTY_NAME_CAPACITY 13

#define U3_PARTY_FORM_OK 0
#define U3_PARTY_FORM_INVALID_ARGUMENT 1
#define U3_PARTY_FORM_INVALID_PARTY 2
#define U3_PARTY_FORM_INVALID_ROSTER 3
#define U3_PARTY_FORM_INVALID_SIZE 4
#define U3_PARTY_FORM_INVALID_ROSTER_ID 5
#define U3_PARTY_FORM_DUPLICATE_ROSTER_ID 6
#define U3_PARTY_FORM_UNOCCUPIED_ROSTER_ID 7

typedef struct u3_party_roster_entry {
    uint8_t roster_id;
    uint8_t occupied;
    char name[U3_PARTY_NAME_CAPACITY + 1];
    uint8_t status;
    uint8_t race;
    uint8_t character_class;
    uint8_t sex;
    uint16_t hit_points;
    uint16_t max_hit_points;
    uint8_t level;
    uint16_t food;
    uint16_t gold;
} u3_party_roster_entry;

typedef struct u3_party_summary {
    uint8_t party_size;
    uint8_t active_roster_ids[U3_PARTY_ACTIVE_SLOT_COUNT];
    uint8_t occupied_roster_count;
    u3_party_roster_entry roster[U3_PARTY_ROSTER_SLOT_COUNT];
} u3_party_summary;

typedef struct u3_party_form_result {
    uint8_t formed;
    uint8_t reason;
    uint8_t party_size;
    uint8_t active_roster_ids[U3_PARTY_ACTIVE_SLOT_COUNT];
} u3_party_form_result;

int u3_party_load_summary(const u3_save_domain_state *state, u3_party_summary *summary);
u3_party_form_result u3_party_form_from_roster(const u3_save_domain_state *state, const uint8_t *selected_roster_ids, uint8_t selected_count, uint8_t *party, uint32_t party_length);

#endif
