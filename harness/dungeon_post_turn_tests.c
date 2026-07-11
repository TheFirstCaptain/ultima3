#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "u3_dungeon.h"

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

static u3_dungeon_post_turn_input base_input(void)
{
    u3_dungeon_post_turn_input input = {0, 0, 3, 4, 4, 0, 0};
    return input;
}

static void test_post_turn_decays_light_and_reports_no_encounter_below_threshold(void)
{
    u3_dungeon_post_turn_input input = base_input();
    u3_dungeon_post_turn_result result;

    input.encounter_roll = 127;
    input.monster_roll = 2;
    result = u3_dungeon_post_turn(input);

    ASSERT_TRUE(result.handled);
    ASSERT_TRUE(result.party_age_requested);
    ASSERT_TRUE(result.status_refresh_requested);
    ASSERT_EQ_INT(3, result.light_before);
    ASSERT_EQ_INT(2, result.light_after);
    ASSERT_TRUE(result.light_decremented);
    ASSERT_FALSE(result.encounter_requested);
    ASSERT_EQ_INT(0, result.monster_type);
}

static void test_post_turn_requests_floor_encounter_at_threshold(void)
{
    u3_dungeon_post_turn_input input = base_input();
    u3_dungeon_post_turn_result result;

    input.encounter_roll = 128;
    input.monster_roll = 2;
    result = u3_dungeon_post_turn(input);

    ASSERT_TRUE(result.encounter_requested);
    ASSERT_EQ_INT(0x1A, result.monster_table_value);
    ASSERT_EQ_INT(0x34, result.monster_type);
    ASSERT_EQ_INT(U3_DUNGEON_ENCOUNTER_MARKER_TILE, result.marker_tile);
    ASSERT_EQ_INT(U3_DUNGEON_COMBAT_SCREEN_RESOURCE_ID, result.combat_screen_resource_id);
}

static void test_post_turn_clamps_level_deep_monster_roll(void)
{
    u3_dungeon_post_turn_input input = base_input();
    u3_dungeon_post_turn_result result;

    input.level = 7;
    input.encounter_roll = 128;
    input.monster_roll = 9;
    result = u3_dungeon_post_turn(input);

    ASSERT_TRUE(result.encounter_requested);
    ASSERT_EQ_INT(0x1E, result.monster_table_value);
    ASSERT_EQ_INT(0x3C, result.monster_type);
}

static void test_post_turn_suppresses_non_floor_and_invalid_party_encounters(void)
{
    u3_dungeon_post_turn_input input = base_input();
    u3_dungeon_post_turn_result result;

    input.current_tile = U3_DUNGEON_TILE_UP_LADDER;
    input.encounter_roll = 128;
    result = u3_dungeon_post_turn(input);
    ASSERT_FALSE(result.encounter_requested);

    input = base_input();
    input.living_party_members = 0;
    input.encounter_roll = 128;
    result = u3_dungeon_post_turn(input);
    ASSERT_FALSE(result.encounter_requested);
}

static void test_post_turn_saturates_light_decay(void)
{
    u3_dungeon_post_turn_input input = base_input();
    u3_dungeon_post_turn_result result;

    input.light = 0;
    result = u3_dungeon_post_turn(input);

    ASSERT_EQ_INT(0, result.light_before);
    ASSERT_EQ_INT(0, result.light_after);
    ASSERT_FALSE(result.light_decremented);
}

int main(void)
{
    test_post_turn_decays_light_and_reports_no_encounter_below_threshold();
    test_post_turn_requests_floor_encounter_at_threshold();
    test_post_turn_clamps_level_deep_monster_roll();
    test_post_turn_suppresses_non_floor_and_invalid_party_encounters();
    test_post_turn_saturates_light_decay();

    puts("dungeon post-turn characterization passed");
    return 0;
}
