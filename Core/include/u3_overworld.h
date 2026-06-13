#ifndef U3_OVERWORLD_H
#define U3_OVERWORLD_H

#include <stdint.h>

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
    uint8_t x;
    uint8_t y;
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
uint8_t u3_overworld_move(u3_overworld_state *state,
                          uint16_t command,
                          u3_overworld_move_result *result);
u3_render_frame u3_overworld_make_smoke_frame(const uint8_t *map_record,
                                              uint32_t map_record_length,
                                              const u3_overworld_state *state);
u3_render_frame u3_overworld_make_view_frame(const uint8_t *map_record,
                                             uint32_t map_record_length,
                                             const u3_overworld_state *state);

#endif
