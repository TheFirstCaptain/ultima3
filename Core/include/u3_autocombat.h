#ifndef U3_AUTOCOMBAT_H
#define U3_AUTOCOMBAT_H

#include <stdbool.h>
#include <stdint.h>

#include "u3_combat.h"

#define U3_AUTOCOMBAT_TILE_COUNT 121

typedef struct u3_autocombat_state {
    u3_combat_state combat;
    uint8_t future_monster_x[U3_COMBAT_MONSTER_COUNT];
    uint8_t future_monster_y[U3_COMBAT_MONSTER_COUNT];
    uint8_t experience[17];
    uint8_t tile_array[U3_AUTOCOMBAT_TILE_COUNT];
    uint8_t character_alive[U3_COMBAT_CHARACTER_COUNT];
    uint16_t character_hp[U3_COMBAT_CHARACTER_COUNT];
    bool allow_diagonal;
} u3_autocombat_state;

bool u3_autocombat_monster_can_attack(const u3_autocombat_state *state, int16_t x, int16_t y);
bool u3_autocombat_nearly_dead(const u3_autocombat_state *state, int16_t who);
void u3_autocombat_setup_now(u3_autocombat_state *state);
void u3_autocombat_setup_future(u3_autocombat_state *state);
bool u3_autocombat_future_monster_here(const u3_autocombat_state *state, int16_t x, int16_t y);
char u3_autocombat_dir_to_nearest_monster(const u3_autocombat_state *state, int16_t character);
char u3_autocombat_line_up_to_monster(const u3_autocombat_state *state, int16_t character);
int16_t u3_autocombat_threat_value(const u3_autocombat_state *state);
char u3_autocombat_monster_nearby(const u3_autocombat_state *state, int16_t character);
char u3_autocombat_monster_lined_up(const u3_autocombat_state *state, int16_t character, int16_t x, int16_t y);
char u3_autocombat_auto_move_char(const u3_autocombat_state *state, int16_t character, int16_t delta_x, int16_t delta_y);

#endif
