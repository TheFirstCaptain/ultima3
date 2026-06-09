#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_character.h"

#define ASSERT_EQ_INT(expected, actual) assert_eq_int((expected), (actual), __FILE__, __LINE__)
#define ASSERT_TRUE(actual) assert_true((actual), __FILE__, __LINE__)
#define ASSERT_FALSE(actual) assert_false((actual), __FILE__, __LINE__)

static void assert_eq_int(int expected, int actual, const char *file, int line)
{
    if (expected != actual) {
        fprintf(stderr, "%s:%d: expected %d, got %d\n", file, line, expected, actual);
        exit(1);
    }
}

static void assert_true(int actual, const char *file, int line)
{
    if (!actual) {
        fprintf(stderr, "%s:%d: expected true\n", file, line);
        exit(1);
    }
}

static void assert_false(int actual, const char *file, int line)
{
    if (actual) {
        fprintf(stderr, "%s:%d: expected false\n", file, line);
        exit(1);
    }
}

static u3_character_candidate valid_candidate(void)
{
    u3_character_candidate candidate;

    memset(&candidate, 0, sizeof(candidate));
    strcpy(candidate.name, "Iolo");
    candidate.race = 'H';
    candidate.character_class = 'F';
    candidate.sex = 'M';
    candidate.strength = 15;
    candidate.dexterity = 15;
    candidate.intelligence = 10;
    candidate.wisdom = 10;
    return candidate;
}

static void test_accepts_valid_candidate(void)
{
    u3_character_candidate candidate = valid_candidate();
    u3_character_validation validation = u3_character_validate_candidate(&candidate);

    ASSERT_TRUE(validation.valid);
    ASSERT_EQ_INT(U3_CHARACTER_VALIDATION_OK, validation.reason);
    ASSERT_EQ_INT(4, validation.name_length);
    ASSERT_EQ_INT(0, validation.points_remaining);
    ASSERT_EQ_INT(0, validation.points_over);

    candidate.character_class = 'B';
    validation = u3_character_validate_candidate(&candidate);
    ASSERT_TRUE(validation.valid);
}

static void test_rejects_missing_or_too_long_name(void)
{
    u3_character_candidate candidate = valid_candidate();
    u3_character_validation validation;

    candidate.name[0] = 0;
    validation = u3_character_validate_candidate(&candidate);
    ASSERT_FALSE(validation.valid);
    ASSERT_EQ_INT(U3_CHARACTER_VALIDATION_MISSING_NAME, validation.reason);

    memcpy(candidate.name, "ABCDEFGHIJKLM", sizeof(candidate.name));
    validation = u3_character_validate_candidate(&candidate);
    ASSERT_FALSE(validation.valid);
    ASSERT_EQ_INT(U3_CHARACTER_VALIDATION_NAME_TOO_LONG, validation.reason);
}

static void test_rejects_invalid_codes(void)
{
    u3_character_candidate candidate = valid_candidate();
    u3_character_validation validation;

    candidate.race = 'X';
    validation = u3_character_validate_candidate(&candidate);
    ASSERT_FALSE(validation.valid);
    ASSERT_EQ_INT(U3_CHARACTER_VALIDATION_INVALID_RACE, validation.reason);

    candidate = valid_candidate();
    candidate.character_class = 'X';
    validation = u3_character_validate_candidate(&candidate);
    ASSERT_FALSE(validation.valid);
    ASSERT_EQ_INT(U3_CHARACTER_VALIDATION_INVALID_CLASS, validation.reason);

    candidate = valid_candidate();
    candidate.sex = 'X';
    validation = u3_character_validate_candidate(&candidate);
    ASSERT_FALSE(validation.valid);
    ASSERT_EQ_INT(U3_CHARACTER_VALIDATION_INVALID_SEX, validation.reason);
}

static void test_rejects_attribute_range_and_total(void)
{
    u3_character_candidate candidate = valid_candidate();
    u3_character_validation validation;

    candidate.strength = 4;
    validation = u3_character_validate_candidate(&candidate);
    ASSERT_FALSE(validation.valid);
    ASSERT_EQ_INT(U3_CHARACTER_VALIDATION_ATTRIBUTE_RANGE, validation.reason);

    candidate = valid_candidate();
    candidate.wisdom = 9;
    validation = u3_character_validate_candidate(&candidate);
    ASSERT_FALSE(validation.valid);
    ASSERT_EQ_INT(U3_CHARACTER_VALIDATION_ATTRIBUTE_TOTAL, validation.reason);
    ASSERT_EQ_INT(1, validation.points_remaining);

    candidate = valid_candidate();
    candidate.wisdom = 11;
    validation = u3_character_validate_candidate(&candidate);
    ASSERT_FALSE(validation.valid);
    ASSERT_EQ_INT(U3_CHARACTER_VALIDATION_ATTRIBUTE_TOTAL, validation.reason);
    ASSERT_EQ_INT(0, validation.points_remaining);
    ASSERT_EQ_INT(1, validation.points_over);
}

int main(void)
{
    test_accepts_valid_candidate();
    test_rejects_missing_or_too_long_name();
    test_rejects_invalid_codes();
    test_rejects_attribute_range_and_total();

    printf("character creation candidate validation passed\n");
    return 0;
}
