#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_character.h"

#define ASSERT_EQ_INT(expected, actual) assert_eq_int((expected), (actual), __FILE__, __LINE__)
#define ASSERT_TRUE(actual) assert_true((actual), __FILE__, __LINE__)
#define ASSERT_FALSE(actual) assert_false((actual), __FILE__, __LINE__)
#define ASSERT_BYTES_EQ(expected, actual, length) assert_bytes_eq((expected), (actual), (length), __FILE__, __LINE__)

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

static void assert_bytes_eq(const uint8_t *expected, const uint8_t *actual, size_t length, const char *file, int line)
{
    size_t index;

    for (index = 0; index < length; index++) {
        if (expected[index] != actual[index]) {
            fprintf(stderr, "%s:%d: byte %zu expected 0x%02x, got 0x%02x\n", file, line, index, expected[index], actual[index]);
            exit(1);
        }
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

static void test_writes_candidate_to_first_empty_roster_slot(void)
{
    u3_character_candidate candidate = valid_candidate();
    uint8_t roster[U3_CHARACTER_ROSTER_SLOT_COUNT * U3_CHARACTER_ROSTER_RECORD_LENGTH];
    u3_character_roster_write write;
    uint8_t *record;

    memset(roster, 0, sizeof(roster));
    memset(roster, 0xA5, U3_CHARACTER_ROSTER_RECORD_LENGTH);

    write = u3_character_add_to_roster(&candidate, roster, sizeof(roster));
    ASSERT_TRUE(write.written);
    ASSERT_EQ_INT(U3_CHARACTER_ROSTER_WRITE_OK, write.reason);
    ASSERT_EQ_INT(2, write.roster_id);
    ASSERT_EQ_INT(U3_CHARACTER_ROSTER_RECORD_LENGTH, write.record_offset);

    record = roster + U3_CHARACTER_ROSTER_RECORD_LENGTH;
    ASSERT_BYTES_EQ((const uint8_t *)"Iolo", record, 4);
    ASSERT_EQ_INT(0, record[4]);
    ASSERT_EQ_INT('G', record[17]);
    ASSERT_EQ_INT(15, record[18]);
    ASSERT_EQ_INT(15, record[19]);
    ASSERT_EQ_INT(10, record[20]);
    ASSERT_EQ_INT(10, record[21]);
    ASSERT_EQ_INT('H', record[22]);
    ASSERT_EQ_INT('F', record[23]);
    ASSERT_EQ_INT('M', record[24]);
    ASSERT_EQ_INT(0, record[26]);
    ASSERT_EQ_INT(100, record[27]);
    ASSERT_EQ_INT(0, record[28]);
    ASSERT_EQ_INT(100, record[29]);
    ASSERT_EQ_INT(1, record[32]);
    ASSERT_EQ_INT(50, record[33]);
    ASSERT_EQ_INT(0, record[35]);
    ASSERT_EQ_INT(150, record[36]);
    ASSERT_EQ_INT(1, record[40]);
    ASSERT_EQ_INT(1, record[41]);
    ASSERT_EQ_INT(1, record[48]);
    ASSERT_EQ_INT(1, record[49]);
}

static void test_treats_loader_empty_marker_as_available_roster_slot(void)
{
    u3_character_candidate candidate = valid_candidate();
    uint8_t roster[U3_CHARACTER_ROSTER_SLOT_COUNT * U3_CHARACTER_ROSTER_RECORD_LENGTH];
    u3_character_roster_write write;
    uint8_t *record;

    memset(roster, 'A', sizeof(roster));
    roster[0] = 22;

    write = u3_character_add_to_roster(&candidate, roster, sizeof(roster));
    ASSERT_TRUE(write.written);
    ASSERT_EQ_INT(U3_CHARACTER_ROSTER_WRITE_OK, write.reason);
    ASSERT_EQ_INT(1, write.roster_id);
    ASSERT_EQ_INT(0, write.record_offset);

    record = roster;
    ASSERT_BYTES_EQ((const uint8_t *)"Iolo", record, 4);
    ASSERT_EQ_INT(0, record[4]);
    ASSERT_EQ_INT('G', record[17]);
}

static void test_roster_write_rejects_invalid_candidate_and_full_roster(void)
{
    u3_character_candidate candidate = valid_candidate();
    uint8_t roster[U3_CHARACTER_ROSTER_SLOT_COUNT * U3_CHARACTER_ROSTER_RECORD_LENGTH];
    u3_character_roster_write write;

    memset(roster, 0, sizeof(roster));
    candidate.name[0] = 0;
    write = u3_character_add_to_roster(&candidate, roster, sizeof(roster));
    ASSERT_FALSE(write.written);
    ASSERT_EQ_INT(U3_CHARACTER_ROSTER_WRITE_INVALID_CANDIDATE, write.reason);
    ASSERT_EQ_INT(U3_CHARACTER_VALIDATION_MISSING_NAME, write.validation.reason);

    candidate = valid_candidate();
    memset(roster, 'A', sizeof(roster));
    write = u3_character_add_to_roster(&candidate, roster, sizeof(roster));
    ASSERT_FALSE(write.written);
    ASSERT_EQ_INT(U3_CHARACTER_ROSTER_WRITE_FULL, write.reason);

    write = u3_character_add_to_roster(&candidate, roster, sizeof(roster) - 1);
    ASSERT_FALSE(write.written);
    ASSERT_EQ_INT(U3_CHARACTER_ROSTER_WRITE_INVALID_ROSTER, write.reason);
}

int main(void)
{
    test_accepts_valid_candidate();
    test_rejects_missing_or_too_long_name();
    test_rejects_invalid_codes();
    test_rejects_attribute_range_and_total();
    test_writes_candidate_to_first_empty_roster_slot();
    test_treats_loader_empty_marker_as_available_roster_slot();
    test_roster_write_rejects_invalid_candidate_and_full_roster();

    printf("character creation candidate validation passed\n");
    return 0;
}
