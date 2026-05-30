#include "u3_map_math.h"

int8_t u3_map_math_get_heading(int16_t value)
{
    /* Legacy reference: Sources/UltimaMisc.c GetHeading, $7DFC. */
    if (value == 0)
        return 0;
    if (value < 0)
        return -1;
    if (value > 127)
        return -1;
    return 1;
}

int16_t u3_map_math_absolute(int16_t value)
{
    /* Legacy reference: Sources/UltimaMisc.c Absolute, $7E0D. */
    if (value > 127)
        value = (255 - value) + 1;
    if (value < 0)
        value = (int16_t)(-value);
    return value;
}

uint8_t u3_map_math_get_tile(const u3_map_math_state *state, int16_t x, int16_t y)
{
    /* Legacy reference: Sources/UltimaMisc.c GetXY, $7E18. */
    return state->tile_array[(y * 11) + x];
}

void u3_map_math_put_tile(u3_map_math_state *state, int16_t value, int16_t x, int16_t y)
{
    /* Legacy reference: Sources/UltimaMisc.c PutXY. */
    state->tile_array[(y * 11) + x] = (uint8_t)value;
}

int16_t u3_map_math_constrain(const u3_map_math_state *state, int16_t value)
{
    /* Legacy reference: Sources/UltimaMisc.c MapConstrain. */
    while (value < 0) {
        value = (int16_t)(value + state->cur_map_size);
    }
    while (value >= state->cur_map_size) {
        value = (int16_t)(value - state->cur_map_size);
    }
    return value;
}

uint8_t u3_map_math_get_map_value(u3_map_math_state *state, int x, int y)
{
    /* Legacy reference: Sources/UltimaMisc.c GetXYVal. */
    uint8_t value;
    state->map_offset = u3_map_math_constrain(state, (int16_t)y) * state->cur_map_size;
    state->map_offset += u3_map_math_constrain(state, (int16_t)x);
    value = state->map[state->map_offset];
    if (state->party[3] != 0) {
        if (x < 0 || x >= state->cur_map_size || y < 0 || y >= state->cur_map_size)
            value = 0x04;
    }
    return value;
}

void u3_map_math_put_map_value(u3_map_math_state *state, uint8_t value, uint8_t x, uint8_t y)
{
    /* Legacy reference: Sources/UltimaMisc.c PutXYVal. */
    state->map_offset = u3_map_math_constrain(state, y) * state->cur_map_size;
    state->map_offset += u3_map_math_constrain(state, x);
    state->map[state->map_offset] = value;
}
