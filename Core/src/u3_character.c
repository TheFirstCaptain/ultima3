#include "u3_character.h"

#include <stddef.h>

static uint8_t u3_character_code_allowed(uint8_t value, const char *allowed)
{
    uint8_t index;

    for (index = 0; allowed[index] != 0; index++) {
        if (value == (uint8_t)allowed[index])
            return 1;
    }

    return 0;
}

static uint8_t u3_character_name_length(const char *name, uint8_t *invalid_byte)
{
    uint8_t index;

    if (invalid_byte != 0)
        *invalid_byte = 0;

    for (index = 0; index <= U3_CHARACTER_NAME_CAPACITY; index++) {
        uint8_t value = (uint8_t)name[index];

        if (value == 0)
            return index;
        if (value < 32 || value > 126) {
            if (invalid_byte != 0)
                *invalid_byte = 1;
            return index;
        }
    }

    return (uint8_t)(U3_CHARACTER_NAME_CAPACITY + 1);
}

static uint8_t u3_character_attribute_in_range(uint8_t value)
{
    return value >= U3_CHARACTER_ATTRIBUTE_MIN && value <= U3_CHARACTER_ATTRIBUTE_MAX;
}

static u3_character_validation u3_character_validation_make(uint8_t valid,
                                                            uint8_t reason,
                                                            uint8_t name_length,
                                                            uint8_t points_remaining,
                                                            uint8_t points_over)
{
    u3_character_validation validation;

    validation.valid = valid;
    validation.reason = reason;
    validation.name_length = name_length;
    validation.points_remaining = points_remaining;
    validation.points_over = points_over;
    return validation;
}

static u3_character_roster_write u3_character_roster_write_make(uint8_t written,
                                                                uint8_t reason,
                                                                uint8_t roster_id,
                                                                uint16_t record_offset,
                                                                u3_character_validation validation)
{
    u3_character_roster_write write;

    write.written = written;
    write.reason = reason;
    write.roster_id = roster_id;
    write.record_offset = record_offset;
    write.validation = validation;
    return write;
}

static void u3_character_write_u16(uint8_t *bytes, uint8_t offset, uint16_t value)
{
    bytes[offset] = (uint8_t)(value >> 8);
    bytes[offset + 1] = (uint8_t)(value & 0xFF);
}

u3_character_validation u3_character_validate_candidate(const u3_character_candidate *candidate)
{
    uint8_t name_length;
    uint8_t invalid_name_byte;
    uint16_t total;
    uint8_t points_remaining;
    uint8_t points_over;

    if (candidate == 0)
        return u3_character_validation_make(0, U3_CHARACTER_VALIDATION_MISSING_NAME, 0, U3_CHARACTER_ATTRIBUTE_TOTAL, 0);

    name_length = u3_character_name_length(candidate->name, &invalid_name_byte);
    total = (uint16_t)candidate->strength +
            (uint16_t)candidate->dexterity +
            (uint16_t)candidate->intelligence +
            (uint16_t)candidate->wisdom;
    points_remaining = total <= U3_CHARACTER_ATTRIBUTE_TOTAL ? (uint8_t)(U3_CHARACTER_ATTRIBUTE_TOTAL - total) : 0;
    points_over = total > U3_CHARACTER_ATTRIBUTE_TOTAL ? (uint8_t)(total - U3_CHARACTER_ATTRIBUTE_TOTAL) : 0;

    if (name_length == 0)
        return u3_character_validation_make(0, U3_CHARACTER_VALIDATION_MISSING_NAME, name_length, points_remaining, points_over);
    if (name_length > U3_CHARACTER_NAME_CAPACITY)
        return u3_character_validation_make(0, U3_CHARACTER_VALIDATION_NAME_TOO_LONG, name_length, points_remaining, points_over);
    if (invalid_name_byte)
        return u3_character_validation_make(0, U3_CHARACTER_VALIDATION_INVALID_NAME_BYTE, name_length, points_remaining, points_over);
    if (!u3_character_code_allowed(candidate->race, "HEDBF"))
        return u3_character_validation_make(0, U3_CHARACTER_VALIDATION_INVALID_RACE, name_length, points_remaining, points_over);
    if (!u3_character_code_allowed(candidate->character_class, "FCWTPBLIDAR"))
        return u3_character_validation_make(0, U3_CHARACTER_VALIDATION_INVALID_CLASS, name_length, points_remaining, points_over);
    if (!u3_character_code_allowed(candidate->sex, "FMO"))
        return u3_character_validation_make(0, U3_CHARACTER_VALIDATION_INVALID_SEX, name_length, points_remaining, points_over);
    if (!u3_character_attribute_in_range(candidate->strength) ||
        !u3_character_attribute_in_range(candidate->dexterity) ||
        !u3_character_attribute_in_range(candidate->intelligence) ||
        !u3_character_attribute_in_range(candidate->wisdom))
        return u3_character_validation_make(0, U3_CHARACTER_VALIDATION_ATTRIBUTE_RANGE, name_length, points_remaining, points_over);
    if (total != U3_CHARACTER_ATTRIBUTE_TOTAL)
        return u3_character_validation_make(0, U3_CHARACTER_VALIDATION_ATTRIBUTE_TOTAL, name_length, points_remaining, points_over);

    return u3_character_validation_make(1, U3_CHARACTER_VALIDATION_OK, name_length, points_remaining, points_over);
}

u3_character_roster_write u3_character_add_to_roster(const u3_character_candidate *candidate, uint8_t *roster, size_t roster_length)
{
    u3_character_validation validation = u3_character_validate_candidate(candidate);
    uint8_t slot;

    if (!validation.valid)
        return u3_character_roster_write_make(0, U3_CHARACTER_ROSTER_WRITE_INVALID_CANDIDATE, 0, 0, validation);
    if (roster == 0 || roster_length != (size_t)(U3_CHARACTER_ROSTER_SLOT_COUNT * U3_CHARACTER_ROSTER_RECORD_LENGTH))
        return u3_character_roster_write_make(0, U3_CHARACTER_ROSTER_WRITE_INVALID_ROSTER, 0, 0, validation);

    for (slot = 0; slot < U3_CHARACTER_ROSTER_SLOT_COUNT; slot++) {
        uint16_t offset = (uint16_t)(slot * U3_CHARACTER_ROSTER_RECORD_LENGTH);
        uint8_t *record = roster + offset;
        uint8_t index;

        if (record[0] > 22)
            continue;

        for (index = 0; index < U3_CHARACTER_ROSTER_RECORD_LENGTH; index++)
            record[index] = 0;
        for (index = 0; index < validation.name_length; index++)
            record[index] = (uint8_t)candidate->name[index];

        record[17] = (uint8_t)'G';
        record[18] = candidate->strength;
        record[19] = candidate->dexterity;
        record[20] = candidate->intelligence;
        record[21] = candidate->wisdom;
        record[22] = candidate->race;
        record[23] = candidate->character_class;
        record[24] = candidate->sex;
        u3_character_write_u16(record, 26, 100);
        u3_character_write_u16(record, 28, 100);
        record[32] = 1;
        record[33] = 50;
        u3_character_write_u16(record, 35, 150);
        record[40] = 1;
        record[41] = 1;
        record[48] = 1;
        record[49] = 1;
        return u3_character_roster_write_make(1, U3_CHARACTER_ROSTER_WRITE_OK, (uint8_t)(slot + 1), offset, validation);
    }

    return u3_character_roster_write_make(0, U3_CHARACTER_ROSTER_WRITE_FULL, 0, 0, validation);
}
