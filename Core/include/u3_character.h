#ifndef U3_CHARACTER_H
#define U3_CHARACTER_H

#include <stdint.h>

#define U3_CHARACTER_NAME_CAPACITY 12
#define U3_CHARACTER_ATTRIBUTE_MIN 5
#define U3_CHARACTER_ATTRIBUTE_MAX 25
#define U3_CHARACTER_ATTRIBUTE_TOTAL 50

#define U3_CHARACTER_VALIDATION_OK 0
#define U3_CHARACTER_VALIDATION_MISSING_NAME 1
#define U3_CHARACTER_VALIDATION_NAME_TOO_LONG 2
#define U3_CHARACTER_VALIDATION_INVALID_NAME_BYTE 3
#define U3_CHARACTER_VALIDATION_INVALID_RACE 4
#define U3_CHARACTER_VALIDATION_INVALID_CLASS 5
#define U3_CHARACTER_VALIDATION_INVALID_SEX 6
#define U3_CHARACTER_VALIDATION_ATTRIBUTE_RANGE 7
#define U3_CHARACTER_VALIDATION_ATTRIBUTE_TOTAL 8

typedef struct u3_character_candidate {
    char name[U3_CHARACTER_NAME_CAPACITY + 1];
    uint8_t race;
    uint8_t character_class;
    uint8_t sex;
    uint8_t strength;
    uint8_t dexterity;
    uint8_t intelligence;
    uint8_t wisdom;
} u3_character_candidate;

typedef struct u3_character_validation {
    uint8_t valid;
    uint8_t reason;
    uint8_t name_length;
    uint8_t points_remaining;
    uint8_t points_over;
} u3_character_validation;

u3_character_validation u3_character_validate_candidate(const u3_character_candidate *candidate);

#endif
