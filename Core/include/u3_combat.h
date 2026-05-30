#ifndef U3_COMBAT_H
#define U3_COMBAT_H

#include <stdint.h>

#define U3_COMBAT_CHARACTER_COUNT 4
#define U3_COMBAT_MONSTER_COUNT 8
#define U3_COMBAT_NO_SLOT 255

typedef struct u3_combat_state {
    int16_t monster_type;
    uint8_t character_x[U3_COMBAT_CHARACTER_COUNT];
    uint8_t character_y[U3_COMBAT_CHARACTER_COUNT];
    uint8_t monster_x[U3_COMBAT_MONSTER_COUNT];
    uint8_t monster_y[U3_COMBAT_MONSTER_COUNT];
    uint8_t monster_hp[U3_COMBAT_MONSTER_COUNT];
} u3_combat_state;

uint8_t u3_combat_valid_move(const u3_combat_state *state, int16_t tile);
uint8_t u3_combat_monster_here(const u3_combat_state *state, int16_t x, int16_t y);
uint8_t u3_combat_character_here(const u3_combat_state *state, int16_t x, int16_t y);

#endif
