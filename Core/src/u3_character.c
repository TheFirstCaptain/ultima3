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
