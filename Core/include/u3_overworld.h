#ifndef U3_OVERWORLD_H
#define U3_OVERWORLD_H

#include <stdint.h>

#include "u3_audio.h"
#include "u3_render.h"

#define U3_OVERWORLD_COMMAND_NONE 0
#define U3_OVERWORLD_COMMAND_NORTH 1
#define U3_OVERWORLD_COMMAND_SOUTH 2
#define U3_OVERWORLD_COMMAND_WEST 3
#define U3_OVERWORLD_COMMAND_EAST 4

#define U3_OVERWORLD_SMOKE_VIEW_WIDTH U3_RENDER_TILE_GRID_WIDTH
#define U3_OVERWORLD_SMOKE_VIEW_HEIGHT U3_RENDER_TILE_GRID_HEIGHT
#define U3_OVERWORLD_PARTY_TILE 0xff
#define U3_OVERWORLD_PARTY_X_OFFSET 3
#define U3_OVERWORLD_PARTY_Y_OFFSET 4
#define U3_OVERWORLD_PARTY_SIZE_OFFSET 1
#define U3_OVERWORLD_PARTY_MOVE_ONES_OFFSET 10
#define U3_OVERWORLD_PARTY_MOVE_HUNDREDS_OFFSET 11
#define U3_OVERWORLD_PARTY_MOVE_TEN_THOUSANDS_OFFSET 12
#define U3_OVERWORLD_PARTY_MOVE_MILLIONS_OFFSET 13
#define U3_OVERWORLD_PARTY_MOVE_COUNTER_MAX 99999999u

#define U3_OVERWORLD_FEEDBACK_NONE 0
#define U3_OVERWORLD_FEEDBACK_MOVED 1
#define U3_OVERWORLD_FEEDBACK_BLOCKED 2

#define U3_OVERWORLD_STATUS_NONE 0
#define U3_OVERWORLD_STATUS_MOVED 1
#define U3_OVERWORLD_STATUS_BLOCKED 2

typedef struct u3_overworld_state {
    uint8_t x;
    uint8_t y;
    uint8_t width;
    uint8_t height;
    uint8_t wraps;
} u3_overworld_state;

typedef struct u3_overworld_move_result {
    uint8_t handled;
    uint8_t moved;
    uint8_t blocked;
    uint8_t x;
    uint8_t y;
    uint8_t target_x;
    uint8_t target_y;
    uint8_t target_tile;
    uint8_t redraw;
    uint8_t feedback;
    uint8_t status;
    uint16_t sound_id;
    uint8_t turn_applied;
    uint8_t turn_delta;
    uint32_t move_counter_before;
    uint32_t move_counter_after;
} u3_overworld_move_result;

uint8_t u3_overworld_state_init(u3_overworld_state *state,
                                uint8_t x,
                                uint8_t y,
                                uint8_t width,
                                uint8_t height);
uint8_t u3_overworld_state_init_wrapped(u3_overworld_state *state,
                                        uint8_t x,
                                        uint8_t y,
                                        uint8_t width,
                                        uint8_t height);
uint8_t u3_overworld_state_init_from_party(u3_overworld_state *state,
                                           const uint8_t *party,
                                           uint32_t party_length,
                                           uint8_t map_size);
uint8_t u3_overworld_write_party_position(uint8_t *party,
                                          uint32_t party_length,
                                          const u3_overworld_state *state);
uint32_t u3_overworld_read_party_move_counter(const uint8_t *party,
                                              uint32_t party_length);
uint8_t u3_overworld_increment_party_move_counter(uint8_t *party,
                                                  uint32_t party_length,
                                                  u3_overworld_move_result *result);
uint8_t u3_overworld_move(u3_overworld_state *state,
                          uint16_t command,
                          u3_overworld_move_result *result);
uint8_t u3_overworld_move_on_map(u3_overworld_state *state,
                                 const uint8_t *map_record,
                                 uint32_t map_record_length,
                                 uint16_t command,
                                 u3_overworld_move_result *result);
uint8_t u3_overworld_tile_passable_on_foot(uint8_t tile);
u3_render_frame u3_overworld_make_smoke_frame(const uint8_t *map_record,
                                              uint32_t map_record_length,
                                              const u3_overworld_state *state);
u3_render_frame u3_overworld_make_view_frame(const uint8_t *map_record,
                                             uint32_t map_record_length,
                                             const u3_overworld_state *state);

#endif
