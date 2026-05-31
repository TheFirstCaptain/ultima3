#ifndef U3_DUNGEON_H
#define U3_DUNGEON_H

#include <stdbool.h>
#include <stdint.h>

#define U3_DUNGEON_WIDTH 16
#define U3_DUNGEON_HEIGHT 16
#define U3_DUNGEON_LEVEL_COUNT 8
#define U3_DUNGEON_CELL_COUNT (U3_DUNGEON_WIDTH * U3_DUNGEON_HEIGHT * U3_DUNGEON_LEVEL_COUNT)

#define U3_DUNGEON_TILE_WALL 0x80
#define U3_DUNGEON_TILE_TURN_BLOCK 0xA0
#define U3_DUNGEON_TILE_UP_LADDER 0x10
#define U3_DUNGEON_TILE_DOWN_LADDER 0x20

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

uint8_t u3_dungeon_get_cell(const u3_dungeon_state *state, int16_t x, int16_t y);
void u3_dungeon_put_cell(u3_dungeon_state *state, uint8_t value, int16_t x, int16_t y);

u3_dungeon_result u3_dungeon_forward(u3_dungeon_state *state);
u3_dungeon_result u3_dungeon_retreat(u3_dungeon_state *state);
u3_dungeon_result u3_dungeon_turn_right(u3_dungeon_state *state);
u3_dungeon_result u3_dungeon_turn_left(u3_dungeon_state *state);
u3_dungeon_result u3_dungeon_descend(u3_dungeon_state *state);
u3_dungeon_result u3_dungeon_climb(u3_dungeon_state *state);

#endif
