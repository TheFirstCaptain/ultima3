#ifndef U3_MAP_MATH_H
#define U3_MAP_MATH_H

#include <stdint.h>

typedef struct u3_map_math_state {
    uint8_t *tile_array;
    uint8_t *party;
    uint8_t *map;
    int32_t map_offset;
    int16_t cur_map_size;
} u3_map_math_state;

int8_t u3_map_math_get_heading(int16_t value);
int16_t u3_map_math_absolute(int16_t value);

uint8_t u3_map_math_get_tile(const u3_map_math_state *state, int16_t x, int16_t y);
void u3_map_math_put_tile(u3_map_math_state *state, int16_t value, int16_t x, int16_t y);

int16_t u3_map_math_constrain(const u3_map_math_state *state, int16_t value);
uint8_t u3_map_math_get_map_value(u3_map_math_state *state, int x, int y);
void u3_map_math_put_map_value(u3_map_math_state *state, uint8_t value, uint8_t x, uint8_t y);

#endif
