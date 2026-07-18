#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_audio.h"
#include "u3_dungeon.h"
#include "u3_party.h"

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

static void write_u16(uint8_t *bytes, uint32_t offset, uint16_t value)
{
    bytes[offset] = (uint8_t)(value >> 8);
    bytes[offset + 1] = (uint8_t)(value & 0xff);
}

static uint16_t read_u16(const uint8_t *bytes, uint32_t offset)
{
    return (uint16_t)(((uint16_t)bytes[offset] << 8) | bytes[offset + 1]);
}

static u3_dungeon_special_effect_input base_input(uint8_t tile)
{
    u3_dungeon_special_effect_input input;

    memset(&input, 0, sizeof(input));
    input.current_tile = tile;
    input.level = 0;
    input.light = 9;
    return input;
}

static void make_party(uint8_t *party, uint8_t *roster)
{
    uint8_t index;

    memset(party, 0, U3_SAVE_PARTY_LENGTH);
    memset(roster, 0, U3_SAVE_ROSTER_LENGTH);
    party[1] = 4;
    for (index = 0; index < 4; index++) {
        uint8_t *record = roster + (index * U3_PARTY_ROSTER_RECORD_LENGTH);

        party[6 + index] = (uint8_t)(index + 1);
        record[0] = 'A';
        record[U3_PARTY_ROSTER_STATUS_OFFSET] = 'G';
        record[19] = 10;
        record[23] = index == 0 ? 'F' : 'T';
        write_u16(record, 26, 100);
        write_u16(record, 28, 100);
        record[32] = 1;
        record[33] = 50;
    }
}

static void test_wind_extinguishes_light_without_party(void)
{
    u3_dungeon_special_effect_input input = base_input(U3_DUNGEON_TILE_WIND);
    u3_dungeon_special_effect_result result;

    result = u3_dungeon_apply_special_effect(input, 0, 0, 0, 0);

    ASSERT_TRUE(result.handled);
    ASSERT_EQ_INT(U3_DUNGEON_SPECIAL_STATUS_WIND, result.status);
    ASSERT_EQ_INT(9, result.light_before);
    ASSERT_EQ_INT(0, result.light_after);
    ASSERT_TRUE(result.light_changed);
    ASSERT_FALSE(result.clear_current_tile);
    ASSERT_EQ_INT(158, result.message_id);
}

static void test_trap_disarm_uses_first_living_active_member_and_clears_tile(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_dungeon_special_effect_input input = base_input(U3_DUNGEON_TILE_TRAP);
    u3_dungeon_special_effect_result result;

    make_party(party, roster);
    input.disarm_roll = 200;
    input.trap_damage_roll = 255;
    result = u3_dungeon_apply_special_effect(input, party, sizeof(party), roster, sizeof(roster));

    ASSERT_TRUE(result.handled);
    ASSERT_EQ_INT(U3_DUNGEON_SPECIAL_STATUS_TRAP_DISARMED, result.status);
    ASSERT_TRUE(result.clear_current_tile);
    ASSERT_EQ_INT(1, result.active_slot);
    ASSERT_EQ_INT(1, result.roster_id);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_STEP, result.sound_id);
    ASSERT_EQ_INT(100, read_u16(roster, 26));
}

static void test_trap_failure_damages_all_living_members(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_dungeon_special_effect_input input = base_input(U3_DUNGEON_TILE_TRAP);
    u3_dungeon_special_effect_result result;

    make_party(party, roster);
    roster[U3_PARTY_ROSTER_RECORD_LENGTH + U3_PARTY_ROSTER_STATUS_OFFSET] = 'D';
    input.level = 2;
    input.disarm_roll = 0;
    input.trap_damage_roll = 0x77;
    result = u3_dungeon_apply_special_effect(input, party, sizeof(party), roster, sizeof(roster));

    ASSERT_EQ_INT(U3_DUNGEON_SPECIAL_STATUS_TRAP_DAMAGE, result.status);
    ASSERT_TRUE(result.clear_current_tile);
    ASSERT_EQ_INT(143, result.damage_per_living_member);
    ASSERT_EQ_INT(3, result.damaged_living_members);
    ASSERT_EQ_INT(3, result.killed_members);
    ASSERT_EQ_INT('D', roster[U3_PARTY_ROSTER_STATUS_OFFSET]);
    ASSERT_EQ_INT('D', roster[(2 * U3_PARTY_ROSTER_RECORD_LENGTH) + U3_PARTY_ROSTER_STATUS_OFFSET]);
    ASSERT_EQ_INT('D', roster[(3 * U3_PARTY_ROSTER_RECORD_LENGTH) + U3_PARTY_ROSTER_STATUS_OFFSET]);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_HIT, result.sound_id);
}

static void test_gremlins_choose_living_member_and_decrement_food_without_underflow(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_dungeon_special_effect_input input = base_input(U3_DUNGEON_TILE_GREMLINS);
    u3_dungeon_special_effect_result result;

    make_party(party, roster);
    roster[U3_PARTY_ROSTER_STATUS_OFFSET] = 'D';
    roster[U3_PARTY_ROSTER_RECORD_LENGTH + 32] = 0;
    roster[U3_PARTY_ROSTER_RECORD_LENGTH + 33] = 50;
    input.gremlin_roll = 0;
    result = u3_dungeon_apply_special_effect(input, party, sizeof(party), roster, sizeof(roster));

    ASSERT_EQ_INT(U3_DUNGEON_SPECIAL_STATUS_GREMLINS, result.status);
    ASSERT_TRUE(result.clear_current_tile);
    ASSERT_EQ_INT(2, result.active_slot);
    ASSERT_EQ_INT(2, result.roster_id);
    ASSERT_EQ_INT(50, result.food_before);
    ASSERT_EQ_INT(50, result.food_after);
    ASSERT_EQ_INT(0, roster[U3_PARTY_ROSTER_RECORD_LENGTH + 32]);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_OUCH, result.sound_id);

    input.gremlin_roll = 2;
    result = u3_dungeon_apply_special_effect(input, party, sizeof(party), roster, sizeof(roster));
    ASSERT_EQ_INT(4, result.active_slot);
    ASSERT_EQ_INT(150, result.food_before);
    ASSERT_EQ_INT(50, result.food_after);
}

static void test_writing_and_unsupported_tiles_report_without_mutation_requirements(void)
{
    u3_dungeon_special_effect_result result;

    result = u3_dungeon_apply_special_effect(base_input(U3_DUNGEON_TILE_WRITING), 0, 0, 0, 0);
    ASSERT_TRUE(result.handled);
    ASSERT_EQ_INT(U3_DUNGEON_SPECIAL_STATUS_WRITING, result.status);
    ASSERT_EQ_INT(164, result.message_id);

    result = u3_dungeon_apply_special_effect(base_input(2), 0, 0, 0, 0);
    ASSERT_TRUE(result.handled);
    ASSERT_EQ_INT(U3_DUNGEON_SPECIAL_STATUS_UNSUPPORTED, result.status);
}

static void test_invalid_party_still_reports_tile_handling_for_clearing_hazards(void)
{
    u3_dungeon_special_effect_result result;

    result = u3_dungeon_apply_special_effect(base_input(U3_DUNGEON_TILE_TRAP), 0, 0, 0, 0);

    ASSERT_TRUE(result.handled);
    ASSERT_EQ_INT(U3_DUNGEON_SPECIAL_STATUS_INVALID_INPUT, result.status);
    ASSERT_TRUE(result.clear_current_tile);
}

static void test_fountain_variants_mutate_selected_living_character(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_dungeon_interaction_input input;
    u3_dungeon_interaction_result result;

    make_party(party, roster);
    memset(&input, 0, sizeof(input));
    input.current_tile = U3_DUNGEON_TILE_FOUNTAIN;
    input.selected_active_slot = 1;

    result = u3_dungeon_apply_interaction(input, party, sizeof(party), roster, sizeof(roster));
    ASSERT_EQ_INT(U3_DUNGEON_INTERACTION_STATUS_FOUNTAIN_POISON, result.status);
    ASSERT_EQ_INT('P', roster[U3_PARTY_ROSTER_STATUS_OFFSET]);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_HIT, result.sound_id);

    input.x = 1;
    write_u16(roster, 26, 25);
    result = u3_dungeon_apply_interaction(input, party, sizeof(party), roster, sizeof(roster));
    ASSERT_EQ_INT(U3_DUNGEON_INTERACTION_STATUS_FOUNTAIN_HEAL, result.status);
    ASSERT_EQ_INT(100, read_u16(roster, 26));

    input.x = 2;
    result = u3_dungeon_apply_interaction(input, party, sizeof(party), roster, sizeof(roster));
    ASSERT_EQ_INT(U3_DUNGEON_INTERACTION_STATUS_FOUNTAIN_DAMAGE, result.status);
    ASSERT_EQ_INT(75, read_u16(roster, 26));

    input.x = 3;
    result = u3_dungeon_apply_interaction(input, party, sizeof(party), roster, sizeof(roster));
    ASSERT_EQ_INT(U3_DUNGEON_INTERACTION_STATUS_FOUNTAIN_CURE, result.status);
    ASSERT_EQ_INT('G', roster[U3_PARTY_ROSTER_STATUS_OFFSET]);
}

static void test_mark_sets_position_bit_and_charges_hp(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_dungeon_interaction_input input;
    u3_dungeon_interaction_result result;

    make_party(party, roster);
    memset(&input, 0, sizeof(input));
    input.current_tile = U3_DUNGEON_TILE_MARK;
    input.x = 2;
    input.selected_active_slot = 2;

    result = u3_dungeon_apply_interaction(input, party, sizeof(party), roster, sizeof(roster));

    ASSERT_EQ_INT(U3_DUNGEON_INTERACTION_STATUS_MARK, result.status);
    ASSERT_EQ_INT(2, result.roster_id);
    ASSERT_EQ_INT(64, roster[U3_PARTY_ROSTER_RECORD_LENGTH + 14]);
    ASSERT_EQ_INT(50, read_u16(roster + U3_PARTY_ROSTER_RECORD_LENGTH, 26));
}

static void test_chest_opens_current_tile_and_adds_bounded_gold(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_dungeon_interaction_input input;
    u3_dungeon_interaction_result result;

    make_party(party, roster);
    memset(&input, 0, sizeof(input));
    input.current_tile = U3_DUNGEON_TILE_CHEST;
    input.command = U3_DUNGEON_COMMAND_GET_CHEST;
    input.selected_active_slot = 1;
    input.chest_trap_roll = 0;
    input.chest_gold_roll = 20;

    result = u3_dungeon_apply_interaction(input, party, sizeof(party), roster, sizeof(roster));

    ASSERT_EQ_INT(U3_DUNGEON_INTERACTION_STATUS_CHEST_OPENED, result.status);
    ASSERT_TRUE(result.clear_current_tile);
    ASSERT_EQ_INT(50, result.gold_added);
    ASSERT_EQ_INT(50, result.gold_after);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_CREAK, result.sound_id);
}

static void test_chest_trap_damages_selected_character_and_still_adds_gold(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_dungeon_interaction_input input;
    u3_dungeon_interaction_result result;

    make_party(party, roster);
    memset(&input, 0, sizeof(input));
    input.current_tile = U3_DUNGEON_TILE_CHEST;
    input.command = U3_DUNGEON_COMMAND_GET_CHEST;
    input.selected_active_slot = 1;
    input.chest_trap_roll = 200;
    input.chest_gold_roll = 20;

    result = u3_dungeon_apply_interaction(input, party, sizeof(party), roster, sizeof(roster));

    ASSERT_EQ_INT(U3_DUNGEON_INTERACTION_STATUS_CHEST_OPENED, result.status);
    ASSERT_TRUE(result.clear_current_tile);
    ASSERT_EQ_INT(U3_DUNGEON_CHEST_TRAP_DAMAGE, result.chest_trap_kind);
    ASSERT_EQ_INT(0, result.chest_trap_disarmed);
    ASSERT_EQ_INT(0x08, result.chest_trap_damage);
    ASSERT_EQ_INT(92, result.hit_points_after);
    ASSERT_EQ_INT(50, result.gold_added);
    ASSERT_EQ_INT(50, result.gold_after);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_HIT, result.sound_id);
}

static void test_chest_trap_can_be_disarmed_before_gold(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_dungeon_interaction_input input;
    u3_dungeon_interaction_result result;

    make_party(party, roster);
    roster[19] = 250;
    memset(&input, 0, sizeof(input));
    input.current_tile = U3_DUNGEON_TILE_CHEST;
    input.command = U3_DUNGEON_COMMAND_GET_CHEST;
    input.selected_active_slot = 1;
    input.chest_trap_roll = 200;
    input.chest_gold_roll = 20;

    result = u3_dungeon_apply_interaction(input, party, sizeof(party), roster, sizeof(roster));

    ASSERT_EQ_INT(U3_DUNGEON_INTERACTION_STATUS_CHEST_OPENED, result.status);
    ASSERT_TRUE(result.clear_current_tile);
    ASSERT_EQ_INT(U3_DUNGEON_CHEST_TRAP_DAMAGE, result.chest_trap_kind);
    ASSERT_EQ_INT(1, result.chest_trap_disarmed);
    ASSERT_EQ_INT(100, result.hit_points_after);
    ASSERT_EQ_INT(50, result.gold_added);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_OUCH, result.sound_id);
}

static void test_chest_poison_trap_sets_selected_character_status(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_dungeon_interaction_input input;
    u3_dungeon_interaction_result result;

    make_party(party, roster);
    memset(&input, 0, sizeof(input));
    input.current_tile = U3_DUNGEON_TILE_CHEST;
    input.command = U3_DUNGEON_COMMAND_GET_CHEST;
    input.selected_active_slot = 1;
    input.chest_trap_roll = 201;

    result = u3_dungeon_apply_interaction(input, party, sizeof(party), roster, sizeof(roster));

    ASSERT_EQ_INT(U3_DUNGEON_CHEST_TRAP_POISON, result.chest_trap_kind);
    ASSERT_EQ_INT('P', result.status_after);
    ASSERT_EQ_INT('P', roster[U3_PARTY_ROSTER_STATUS_OFFSET]);
}

static void test_chest_adds_first_weapon_or_armour_reward_from_high_gold_roll_byte(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_dungeon_interaction_input input;
    u3_dungeon_interaction_result result;

    make_party(party, roster);
    memset(&input, 0, sizeof(input));
    input.current_tile = U3_DUNGEON_TILE_CHEST;
    input.command = U3_DUNGEON_COMMAND_GET_CHEST;
    input.selected_active_slot = 1;
    input.chest_gold_roll = (uint16_t)((5 << 8) | 20);

    result = u3_dungeon_apply_interaction(input, party, sizeof(party), roster, sizeof(roster));

    ASSERT_EQ_INT(5, result.weapon_reward);
    ASSERT_EQ_INT(0, result.weapon_before);
    ASSERT_EQ_INT(1, result.weapon_after);
    ASSERT_EQ_INT(1, roster[48 + 5]);
    ASSERT_EQ_INT(0, result.armour_reward);

    input.chest_gold_roll = (uint16_t)((131 << 8) | 20);
    result = u3_dungeon_apply_interaction(input, party, sizeof(party), roster, sizeof(roster));

    ASSERT_EQ_INT(3, result.armour_reward);
    ASSERT_EQ_INT(0, result.armour_before);
    ASSERT_EQ_INT(1, result.armour_after);
    ASSERT_EQ_INT(1, roster[40 + 3]);
    ASSERT_EQ_INT(0, result.weapon_reward);
}

static void test_prompt_cancel_invalid_and_time_lord_paths(void)
{
    uint8_t party[U3_SAVE_PARTY_LENGTH];
    uint8_t roster[U3_SAVE_ROSTER_LENGTH];
    u3_dungeon_interaction_input input;
    u3_dungeon_interaction_result result;

    make_party(party, roster);
    memset(&input, 0, sizeof(input));
    input.current_tile = U3_DUNGEON_TILE_TIME_LORD;
    result = u3_dungeon_apply_interaction(input, 0, 0, 0, 0);
    ASSERT_EQ_INT(U3_DUNGEON_INTERACTION_STATUS_TIME_LORD, result.status);
    ASSERT_EQ_INT(151, result.message_id);

    input.current_tile = U3_DUNGEON_TILE_FOUNTAIN;
    result = u3_dungeon_apply_interaction(input, party, sizeof(party), roster, sizeof(roster));
    ASSERT_TRUE(result.requires_selection);
    ASSERT_EQ_INT(U3_DUNGEON_INTERACTION_STATUS_SELECTION_REQUIRED, result.status);

    input.selected_active_slot = 5;
    result = u3_dungeon_apply_interaction(input, party, sizeof(party), roster, sizeof(roster));
    ASSERT_EQ_INT(U3_DUNGEON_INTERACTION_STATUS_CANCELLED, result.status);

    input.selected_active_slot = 1;
    roster[U3_PARTY_ROSTER_STATUS_OFFSET] = 'D';
    result = u3_dungeon_apply_interaction(input, party, sizeof(party), roster, sizeof(roster));
    ASSERT_EQ_INT(U3_DUNGEON_INTERACTION_STATUS_INCAPACITATED, result.status);
}

int main(void)
{
    test_wind_extinguishes_light_without_party();
    test_trap_disarm_uses_first_living_active_member_and_clears_tile();
    test_trap_failure_damages_all_living_members();
    test_gremlins_choose_living_member_and_decrement_food_without_underflow();
    test_writing_and_unsupported_tiles_report_without_mutation_requirements();
    test_invalid_party_still_reports_tile_handling_for_clearing_hazards();
    test_fountain_variants_mutate_selected_living_character();
    test_mark_sets_position_bit_and_charges_hp();
    test_chest_opens_current_tile_and_adds_bounded_gold();
    test_chest_trap_damages_selected_character_and_still_adds_gold();
    test_chest_trap_can_be_disarmed_before_gold();
    test_chest_poison_trap_sets_selected_character_status();
    test_chest_adds_first_weapon_or_armour_reward_from_high_gold_roll_byte();
    test_prompt_cancel_invalid_and_time_lord_paths();

    puts("dungeon special tile tests passed");
    return 0;
}
