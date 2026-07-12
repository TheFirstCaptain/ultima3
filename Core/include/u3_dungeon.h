#ifndef U3_DUNGEON_H
#define U3_DUNGEON_H

#include <stdbool.h>
#include <stdint.h>

#include "u3_render.h"
#include "u3_save.h"

#define U3_DUNGEON_WIDTH 16
#define U3_DUNGEON_HEIGHT 16
#define U3_DUNGEON_LEVEL_COUNT 8
#define U3_DUNGEON_CELL_COUNT (U3_DUNGEON_WIDTH * U3_DUNGEON_HEIGHT * U3_DUNGEON_LEVEL_COUNT)

#define U3_DUNGEON_TILE_WALL 0x80
#define U3_DUNGEON_TILE_TURN_BLOCK 0xA0
#define U3_DUNGEON_TILE_UP_LADDER 0x10
#define U3_DUNGEON_TILE_DOWN_LADDER 0x20
#define U3_DUNGEON_TILE_CHEST 0x40
#define U3_DUNGEON_ENCOUNTER_MARKER_TILE 0x40

#define U3_DUNGEON_VIEW_DEPTH 4
#define U3_DUNGEON_LIGHT_FULL 255
#define U3_DUNGEON_COMBAT_SCREEN_RESOURCE_ID 402

#define U3_DUNGEON_TILE_WIND 3
#define U3_DUNGEON_TILE_TRAP 4
#define U3_DUNGEON_TILE_GREMLINS 6
#define U3_DUNGEON_TILE_WRITING 8

#define U3_DUNGEON_SPECIAL_STATUS_NONE 0
#define U3_DUNGEON_SPECIAL_STATUS_WIND 1
#define U3_DUNGEON_SPECIAL_STATUS_TRAP_DISARMED 2
#define U3_DUNGEON_SPECIAL_STATUS_TRAP_DAMAGE 3
#define U3_DUNGEON_SPECIAL_STATUS_GREMLINS 4
#define U3_DUNGEON_SPECIAL_STATUS_WRITING 5
#define U3_DUNGEON_SPECIAL_STATUS_UNSUPPORTED 6
#define U3_DUNGEON_SPECIAL_STATUS_INVALID_INPUT 7
#define U3_DUNGEON_SPECIAL_STATUS_NO_ELIGIBLE_CHARACTER 8

#define U3_DUNGEON_RENDER_VALUE_WALL 1
#define U3_DUNGEON_RENDER_VALUE_DOOR 2
#define U3_DUNGEON_RENDER_VALUE_UP_LADDER 3
#define U3_DUNGEON_RENDER_VALUE_DOWN_LADDER 4
#define U3_DUNGEON_RENDER_VALUE_CHEST 5

typedef struct u3_dungeon_state {
    uint8_t *dungeon;
    int16_t x;
    int16_t y;
    int16_t heading;
    int16_t level;
    bool exit_dungeon;
} u3_dungeon_state;

typedef struct u3_dungeon_result {
    bool moved;
    bool turned;
    bool blocked;
    bool invalid;
    bool level_changed;
    bool exited;
    bool needs_redraw;
    uint16_t message_id;
    uint16_t effect_message_id;
} u3_dungeon_result;

typedef struct u3_dungeon_post_turn_input {
    uint8_t level;
    uint8_t current_tile;
    uint8_t light;
    uint8_t party_size;
    uint8_t living_party_members;
    uint16_t encounter_roll;
    uint16_t monster_roll;
} u3_dungeon_post_turn_input;

typedef struct u3_dungeon_post_turn_result {
    uint8_t handled;
    uint8_t party_age_requested;
    uint8_t status_refresh_requested;
    uint8_t light_before;
    uint8_t light_after;
    uint8_t light_decremented;
    uint8_t current_tile;
    uint8_t encounter_requested;
    uint8_t monster_table_value;
    uint8_t monster_type;
    uint8_t marker_tile;
    uint16_t combat_screen_resource_id;
} u3_dungeon_post_turn_result;

typedef struct u3_dungeon_special_effect_input {
    uint8_t current_tile;
    uint8_t level;
    uint8_t light;
    uint16_t disarm_roll;
    uint16_t gremlin_roll;
    uint16_t trap_damage_roll;
} u3_dungeon_special_effect_input;

typedef struct u3_dungeon_special_effect_result {
    uint8_t handled;
    uint8_t status;
    uint8_t current_tile;
    uint8_t clear_current_tile;
    uint8_t light_before;
    uint8_t light_after;
    uint8_t light_changed;
    uint8_t active_slot;
    uint8_t roster_id;
    uint16_t food_before;
    uint16_t food_after;
    uint16_t damage_per_living_member;
    uint8_t damaged_living_members;
    uint8_t killed_members;
    uint16_t message_id;
    uint16_t sound_id;
} u3_dungeon_special_effect_result;

uint8_t u3_dungeon_get_cell(const u3_dungeon_state *state, int16_t x, int16_t y);
void u3_dungeon_put_cell(u3_dungeon_state *state, uint8_t value, int16_t x, int16_t y);

u3_dungeon_result u3_dungeon_forward(u3_dungeon_state *state);
u3_dungeon_result u3_dungeon_retreat(u3_dungeon_state *state);
u3_dungeon_result u3_dungeon_turn_right(u3_dungeon_state *state);
u3_dungeon_result u3_dungeon_turn_left(u3_dungeon_state *state);
u3_dungeon_result u3_dungeon_pass(u3_dungeon_state *state);
u3_dungeon_result u3_dungeon_descend(u3_dungeon_state *state);
u3_dungeon_result u3_dungeon_climb(u3_dungeon_state *state);
uint8_t u3_dungeon_decay_light(uint8_t light);
u3_dungeon_post_turn_result u3_dungeon_post_turn(u3_dungeon_post_turn_input input);
u3_dungeon_special_effect_result u3_dungeon_apply_special_effect(
    u3_dungeon_special_effect_input input,
    uint8_t *party,
    uint32_t party_length,
    uint8_t *roster,
    uint32_t roster_length);
u3_render_frame u3_dungeon_make_view_frame(const uint8_t *dungeon,
                                           uint32_t dungeon_length,
                                           int16_t level,
                                           int16_t x,
                                           int16_t y,
                                           int16_t heading);
u3_render_frame u3_dungeon_make_lit_view_frame(const uint8_t *dungeon,
                                               uint32_t dungeon_length,
                                               int16_t level,
                                               int16_t x,
                                               int16_t y,
                                               int16_t heading,
                                               uint8_t light);

#endif
