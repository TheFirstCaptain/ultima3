#ifndef U3_LOCATION_H
#define U3_LOCATION_H

#include <stdint.h>

#include "u3_overworld.h"

#define U3_LOCATION_COMMAND_ENTER 'E'

#define U3_LOCATION_KIND_NONE 0
#define U3_LOCATION_KIND_TOWN 1
#define U3_LOCATION_KIND_CASTLE 2
#define U3_LOCATION_KIND_DUNGEON 3

#define U3_LOCATION_MAP_SHAPE_NONE 0
#define U3_LOCATION_MAP_SHAPE_TWO_DIMENSIONAL 1
#define U3_LOCATION_MAP_SHAPE_DUNGEON 2

#define U3_LOCATION_STATUS_NONE 0
#define U3_LOCATION_STATUS_NOT_ENTERABLE 1
#define U3_LOCATION_STATUS_TOWN_REQUESTED 2

#define U3_LOCATION_MOVE_STATUS_NONE 0
#define U3_LOCATION_MOVE_STATUS_MOVED 1
#define U3_LOCATION_MOVE_STATUS_BLOCKED 2
#define U3_LOCATION_MOVE_STATUS_EXIT_REQUESTED 3

#define U3_LOCATION_TABLE_COUNT 32
#define U3_LOCATION_TABLE_LENGTH 64
#define U3_LOCATION_TOWN_TILE_CLASS 0x0c
#define U3_LOCATION_RESOURCE_BASE 400
#define U3_LOCATION_RESOURCE_GAP_INDEX 18
#define U3_LOCATION_RESOURCE_GAP_SIZE 3
#define U3_LOCATION_TOWN_INITIAL_X 1
#define U3_LOCATION_TOWN_INITIAL_Y 32
#define U3_LOCATION_TOWN_INITIAL_HEADING 2
#define U3_LOCATION_PARTY_MODE_OFFSET 2
#define U3_LOCATION_PARTY_MODE_SOSARIA 0
#define U3_LOCATION_PARTY_MODE_TOWN 2
#define U3_LOCATION_TWO_DIMENSIONAL_MAP_SIZE 64
#define U3_LOCATION_TWO_DIMENSIONAL_MAP_LENGTH 4097
#define U3_LOCATION_DUNGEON_MAP_LENGTH 2048
#define U3_LOCATION_MONSTER_LENGTH 256
#define U3_LOCATION_TALK_LENGTH 256

typedef struct u3_location_transition_result {
    uint8_t handled;
    uint8_t requested;
    uint8_t destination_kind;
    uint8_t location_index;
    uint16_t resource_id;
    uint8_t return_x;
    uint8_t return_y;
    uint8_t initial_x;
    uint8_t initial_y;
    uint8_t initial_heading;
    uint8_t status;
    uint8_t turn_applied;
    uint8_t turn_delta;
    uint32_t move_counter_before;
    uint32_t move_counter_after;
} u3_location_transition_result;

typedef struct u3_location_session {
    uint8_t active;
    uint8_t destination_kind;
    uint8_t location_index;
    uint16_t resource_id;
    uint8_t map_shape;
    uint16_t map_size;
    uint32_t map_length;
    uint32_t monster_length;
    uint32_t talk_length;
    uint8_t x;
    uint8_t y;
    uint8_t heading;
    uint8_t return_x;
    uint8_t return_y;
} u3_location_session;

typedef struct u3_location_move_result {
    uint8_t handled;
    uint8_t moved;
    uint8_t blocked;
    uint8_t exit_requested;
    uint8_t x;
    uint8_t y;
    uint8_t target_x;
    uint8_t target_y;
    uint8_t target_tile;
    uint8_t redraw;
    uint8_t status;
    uint16_t sound_id;
    uint8_t turn_applied;
    uint8_t turn_delta;
    uint32_t move_counter_before;
    uint32_t move_counter_after;
} u3_location_move_result;

uint16_t u3_location_resource_id_for_index(uint8_t location_index);

uint8_t u3_location_handle_overworld_command(
    const u3_overworld_state *state,
    const uint8_t *map_record,
    uint32_t map_record_length,
    const uint8_t *location_table,
    uint32_t location_table_length,
    uint16_t command,
    u3_location_transition_result *result);

uint8_t u3_location_session_init(
    const u3_location_transition_result *request,
    const uint8_t *map_record,
    uint32_t map_record_length,
    const uint8_t *monster_record,
    uint32_t monster_record_length,
    const uint8_t *talk_record,
    uint32_t talk_record_length,
    u3_location_session *session);

uint8_t u3_location_enter_party(
    uint8_t *party,
    uint32_t party_length,
    u3_location_transition_result *request);
uint8_t u3_location_restore_party(
    uint8_t *party,
    uint32_t party_length,
    const u3_location_session *session);

uint8_t u3_location_tile_passable(uint8_t tile);
uint8_t u3_location_move(
    u3_location_session *session,
    const uint8_t *map_record,
    uint32_t map_record_length,
    uint16_t command,
    u3_location_move_result *result);
uint8_t u3_location_apply_party_turn(
    uint8_t *party,
    uint32_t party_length,
    u3_location_move_result *result);
u3_render_frame u3_location_make_view_frame(
    const u3_location_session *session,
    const uint8_t *map_record,
    uint32_t map_record_length);

#endif
