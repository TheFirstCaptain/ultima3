#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char CharX[4];
static unsigned char CharY[4];
static unsigned char MonsterX[8];
static unsigned char MonsterY[8];
static unsigned char MonsterHP[8];
static short gMonType;

static unsigned char CombatValidMove(short value)
{
    /* Legacy reference: Sources/UltimaSpellCombat.c CombatValidMove, $7E31. */
    if (gMonType < 0x16 || gMonType >= 0x20) {
        if (value == 2)
            return 0;
        if (value == 4)
            return 0;
        if (value == 6)
            return 0;
        if (value == 0x10)
            return 0;
        return 0xFF;
    } else {
        if (value == 0)
            return 0;
        return 0xFF;
    }
}

static unsigned char CombatMonsterHere(short x, short y)
{
    /* Legacy reference: Sources/UltimaSpellCombat.c CombatMonsterHere, $7D24. */
    short mon, result;
    result = 255;
    for (mon = 7; mon >= 0; mon--) {
        if (MonsterHP[mon] != 0) {
            if (MonsterX[mon] == x && MonsterY[mon] == y) {
                result = mon;
            }
        }
    }
    return result;
}

static unsigned char CombatCharacterHere(short x, short y)
{
    /* Legacy reference: Sources/UltimaSpellCombat.c CombatCharacterHere. */
    char i;
    for (i = 3; i >= 0; i--) {
        if (CharX[(int)i] == x && CharY[(int)i] == y)
            return i;
    }
    return 255;
}

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
    memset(CharX, 0, sizeof(CharX));
    memset(CharY, 0, sizeof(CharY));
    memset(MonsterX, 0, sizeof(MonsterX));
    memset(MonsterY, 0, sizeof(MonsterY));
    memset(MonsterHP, 0, sizeof(MonsterHP));
    gMonType = 0;
}

static void test_land_monster_valid_move_tiles(void)
{
    reset_state();
    gMonType = 0x10;

    ASSERT_EQ_INT(0, CombatValidMove(2));
    ASSERT_EQ_INT(0, CombatValidMove(4));
    ASSERT_EQ_INT(0, CombatValidMove(6));
    ASSERT_EQ_INT(0, CombatValidMove(0x10));
    ASSERT_EQ_INT(0xFF, CombatValidMove(0));
    ASSERT_EQ_INT(0xFF, CombatValidMove(3));
}

static void test_high_monster_type_uses_land_rules(void)
{
    reset_state();
    gMonType = 0x20;

    ASSERT_EQ_INT(0, CombatValidMove(2));
    ASSERT_EQ_INT(0xFF, CombatValidMove(0));
}

static void test_special_monster_type_range_only_accepts_zero_tile(void)
{
    short monType;

    for (monType = 0x16; monType < 0x20; monType++) {
        reset_state();
        gMonType = monType;

        ASSERT_EQ_INT(0, CombatValidMove(0));
        ASSERT_EQ_INT(0xFF, CombatValidMove(2));
        ASSERT_EQ_INT(0xFF, CombatValidMove(4));
        ASSERT_EQ_INT(0xFF, CombatValidMove(6));
        ASSERT_EQ_INT(0xFF, CombatValidMove(0x10));
    }
}

static void test_combat_monster_here_ignores_dead_slots(void)
{
    reset_state();
    MonsterX[4] = 5;
    MonsterY[4] = 6;
    MonsterHP[4] = 0;

    ASSERT_EQ_INT(255, CombatMonsterHere(5, 6));
}

static void test_combat_monster_here_returns_lowest_matching_live_slot(void)
{
    reset_state();
    MonsterX[2] = 7;
    MonsterY[2] = 8;
    MonsterHP[2] = 1;
    MonsterX[6] = 7;
    MonsterY[6] = 8;
    MonsterHP[6] = 99;

    ASSERT_EQ_INT(2, CombatMonsterHere(7, 8));
}

static void test_combat_monster_here_returns_255_when_missing(void)
{
    reset_state();
    MonsterX[1] = 1;
    MonsterY[1] = 2;
    MonsterHP[1] = 3;

    ASSERT_EQ_INT(255, CombatMonsterHere(2, 1));
}

static void test_combat_character_here_returns_highest_matching_slot(void)
{
    reset_state();
    CharX[0] = 4;
    CharY[0] = 9;
    CharX[3] = 4;
    CharY[3] = 9;

    ASSERT_EQ_INT(3, CombatCharacterHere(4, 9));
}

static void test_combat_character_here_does_not_check_health_or_activity(void)
{
    reset_state();
    CharX[2] = 0xFF;
    CharY[2] = 0xFF;

    ASSERT_EQ_INT(2, CombatCharacterHere(0xFF, 0xFF));
}

static void test_combat_character_here_returns_255_when_missing(void)
{
    reset_state();
    CharX[0] = 1;
    CharY[0] = 1;

    ASSERT_EQ_INT(255, CombatCharacterHere(1, 2));
}

int main(void)
{
    test_land_monster_valid_move_tiles();
    test_high_monster_type_uses_land_rules();
    test_special_monster_type_range_only_accepts_zero_tile();
    test_combat_monster_here_ignores_dead_slots();
    test_combat_monster_here_returns_lowest_matching_live_slot();
    test_combat_monster_here_returns_255_when_missing();
    test_combat_character_here_returns_highest_matching_slot();
    test_combat_character_here_does_not_check_health_or_activity();
    test_combat_character_here_returns_255_when_missing();

    puts("combat predicate characterization passed");
    return 0;
}
