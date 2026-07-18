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

int main(void)
{
    test_victory_reports_only_when_all_monsters_are_defeated();
    test_experience_award_uses_damage_result_and_caps_legacy_value();
    test_experience_award_reports_level_increase();

    puts("combat victory characterization passed");
    return 0;
}
