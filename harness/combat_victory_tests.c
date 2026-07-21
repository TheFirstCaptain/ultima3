#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_combat.h"

static u3_combat_state state;

#define ASSERT_EQ_INT(expected, actual) assert_eq_int((expected), (actual), __FILE__, __LINE__)

static void assert_eq_int(int expected, int actual, const char *file, int line)
{
    if (expected != actual) {
        fprintf(stderr, "%s:%d: expected %d, got %d\n", file, line, expected, actual);
        exit(1);
    }
}

static void reset_state(void)
{
    memset(&state, 0, sizeof(state));
}

static void test_victory_reports_only_when_all_monsters_are_defeated(void)
{
    u3_combat_victory_result result;

    reset_state();
    state.monster_hp[0] = 0;
    state.monster_hp[1] = 3;

    result = u3_combat_check_victory(&state);

    ASSERT_EQ_INT(1, result.checked);
    ASSERT_EQ_INT(0, result.victorious);
    ASSERT_EQ_INT(1, result.live_monsters);
    ASSERT_EQ_INT(7, result.defeated_monsters);

    state.monster_hp[1] = 0;

    result = u3_combat_check_victory(&state);

    ASSERT_EQ_INT(1, result.victorious);
    ASSERT_EQ_INT(0, result.live_monsters);
    ASSERT_EQ_INT(8, result.defeated_monsters);
}

static void test_experience_award_uses_damage_result_and_caps_legacy_value(void)
{
    u3_combat_damage_result damage = {0};
    u3_combat_experience_award_result result;

    reset_state();
    state.character_experience[2] = 9890;
    damage.award_experience = 1;
    damage.character = 3;
    damage.experience_awarded = 25;

    result = u3_combat_apply_experience_award(&state, &damage);

    ASSERT_EQ_INT(1, result.applied);
    ASSERT_EQ_INT(3, result.character);
    ASSERT_EQ_INT(25, result.amount);
    ASSERT_EQ_INT(9890, result.experience_before);
    ASSERT_EQ_INT(U3_COMBAT_EXPERIENCE_MAX, result.experience_after);
    ASSERT_EQ_INT(U3_COMBAT_EXPERIENCE_MAX, state.character_experience[2]);
}

static void test_experience_award_reports_level_increase(void)
{
    u3_combat_damage_result damage = {0};
    u3_combat_experience_award_result result;

    reset_state();
    state.character_experience[0] = 95;
    damage.award_experience = 1;
    damage.character = 1;
    damage.experience_awarded = 10;

    result = u3_combat_apply_experience_award(&state, &damage);

    ASSERT_EQ_INT(1, result.applied);
    ASSERT_EQ_INT(1, result.level_increased);
    ASSERT_EQ_INT(105, result.experience_after);
}

static void test_victory_reward_adds_gold_and_inventory(void)
{
    u3_combat_victory_result victory = {0};
    u3_combat_victory_reward_input input = {0};
    u3_combat_victory_reward_result result;

    reset_state();
    victory.victorious = 1;
    input.character = 0;
    input.gold = 75;
    input.weapon = 5;
    input.armour = 3;
    state.character_gold[0] = 150;

    result = u3_combat_apply_victory_reward(&state, &victory, &input);

    ASSERT_EQ_INT(U3_COMBAT_REWARD_STATUS_APPLIED, result.status);
    ASSERT_EQ_INT(1, result.applied);
    ASSERT_EQ_INT(1, result.character);
    ASSERT_EQ_INT(150, result.gold_before);
    ASSERT_EQ_INT(225, result.gold_after);
    ASSERT_EQ_INT(75, result.gold_awarded);
    ASSERT_EQ_INT(225, state.character_gold[0]);
    ASSERT_EQ_INT(5, result.weapon_reward);
    ASSERT_EQ_INT(1, result.weapon_after);
    ASSERT_EQ_INT(1, state.character_weapon_inventory[0][5]);
    ASSERT_EQ_INT(3, result.armour_reward);
    ASSERT_EQ_INT(1, result.armour_after);
    ASSERT_EQ_INT(1, state.character_armour_inventory[0][3]);
}

static void test_victory_reward_caps_gold_and_reports_full_inventory(void)
{
    u3_combat_victory_result victory = {0};
    u3_combat_victory_reward_input input = {0};
    u3_combat_victory_reward_result result;

    reset_state();
    victory.victorious = 1;
    input.character = 1;
    input.gold = 50;
    input.weapon = 2;
    input.armour = 4;
    state.character_gold[1] = 9980;
    state.character_weapon_inventory[1][2] = U3_COMBAT_REWARD_ITEM_MAX;
    state.character_armour_inventory[1][4] = U3_COMBAT_REWARD_ITEM_MAX;

    result = u3_combat_apply_victory_reward(&state, &victory, &input);

    ASSERT_EQ_INT(U3_COMBAT_REWARD_STATUS_APPLIED, result.status);
    ASSERT_EQ_INT(1, result.applied);
    ASSERT_EQ_INT(1, result.gold_capped);
    ASSERT_EQ_INT(19, result.gold_awarded);
    ASSERT_EQ_INT(U3_COMBAT_GOLD_MAX, state.character_gold[1]);
    ASSERT_EQ_INT(1, result.weapon_full);
    ASSERT_EQ_INT(U3_COMBAT_REWARD_ITEM_MAX, result.weapon_after);
    ASSERT_EQ_INT(1, result.armour_full);
    ASSERT_EQ_INT(U3_COMBAT_REWARD_ITEM_MAX, result.armour_after);
}

static void test_victory_reward_does_not_reduce_malformed_over_cap_gold(void)
{
    u3_combat_victory_result victory = {0};
    u3_combat_victory_reward_input input = {0};
    u3_combat_victory_reward_result result;

    reset_state();
    victory.victorious = 1;
    input.character = 0;
    input.gold = 50;
    state.character_gold[0] = 10000;

    result = u3_combat_apply_victory_reward(&state, &victory, &input);

    ASSERT_EQ_INT(U3_COMBAT_REWARD_STATUS_SKIPPED, result.status);
    ASSERT_EQ_INT(0, result.applied);
    ASSERT_EQ_INT(1, result.gold_capped);
    ASSERT_EQ_INT(10000, result.gold_before);
    ASSERT_EQ_INT(10000, result.gold_after);
    ASSERT_EQ_INT(0, result.gold_awarded);
    ASSERT_EQ_INT(10000, state.character_gold[0]);
}

static void test_victory_reward_reports_no_reward_without_mutation(void)
{
    u3_combat_victory_result victory = {0};
    u3_combat_victory_reward_input input = {0};
    u3_combat_victory_reward_result result;

    reset_state();
    victory.victorious = 1;
    input.character = 0;
    state.character_gold[0] = 150;

    result = u3_combat_apply_victory_reward(&state, &victory, &input);

    ASSERT_EQ_INT(U3_COMBAT_REWARD_STATUS_NO_REWARD, result.status);
    ASSERT_EQ_INT(0, result.applied);
    ASSERT_EQ_INT(150, state.character_gold[0]);
}

static void test_victory_reward_rejects_no_victory_and_invalid_character(void)
{
    u3_combat_victory_result victory = {0};
    u3_combat_victory_reward_input input = {0};
    u3_combat_victory_reward_result result;

    reset_state();
    input.character = 0;
    input.gold = 75;

    result = u3_combat_apply_victory_reward(&state, &victory, &input);

    ASSERT_EQ_INT(U3_COMBAT_REWARD_STATUS_NO_VICTORY, result.status);
    ASSERT_EQ_INT(0, result.applied);
    ASSERT_EQ_INT(0, state.character_gold[0]);

    victory.victorious = 1;
    input.character = U3_COMBAT_CHARACTER_COUNT;

    result = u3_combat_apply_victory_reward(&state, &victory, &input);

    ASSERT_EQ_INT(U3_COMBAT_REWARD_STATUS_INVALID_CHARACTER, result.status);
    ASSERT_EQ_INT(0, result.applied);
}

int main(void)
{
    test_victory_reports_only_when_all_monsters_are_defeated();
    test_experience_award_uses_damage_result_and_caps_legacy_value();
    test_experience_award_reports_level_increase();
    test_victory_reward_adds_gold_and_inventory();
    test_victory_reward_caps_gold_and_reports_full_inventory();
    test_victory_reward_does_not_reduce_malformed_over_cap_gold();
    test_victory_reward_reports_no_reward_without_mutation();
    test_victory_reward_rejects_no_victory_and_invalid_character();

    puts("combat victory characterization passed");
    return 0;
}
