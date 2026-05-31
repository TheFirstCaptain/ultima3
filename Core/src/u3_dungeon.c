#include "u3_dungeon.h"

static const int8_t u3_dungeon_head_x[4] = {0, 1, 0, -1};
static const int8_t u3_dungeon_head_y[4] = {-1, 0, 1, 0};

static int16_t u3_dungeon_wrap_coord(int16_t value)
{
    /* Legacy reference: Sources/UltimaSpellCombat.c GetXYDng. */
    if (value < 0)
        value += U3_DUNGEON_WIDTH;
    if (value > U3_DUNGEON_WIDTH - 1)
        value -= U3_DUNGEON_WIDTH;
    return value;
}

static uint16_t u3_dungeon_offset(const u3_dungeon_state *state, int16_t x, int16_t y)
{
    x = u3_dungeon_wrap_coord(x);
    y = u3_dungeon_wrap_coord(y);
    return (uint16_t)((state->level * U3_DUNGEON_WIDTH * U3_DUNGEON_HEIGHT) +
                      (y * U3_DUNGEON_WIDTH) +
                      x);
}

static uint16_t u3_dungeon_raw_offset(const u3_dungeon_state *state, int16_t x, int16_t y)
{
    return (uint16_t)((state->level * U3_DUNGEON_WIDTH * U3_DUNGEON_HEIGHT) +
                      (y * U3_DUNGEON_WIDTH) +
                      x);
}

static u3_dungeon_result u3_dungeon_make_result(uint16_t message_id)
{
    u3_dungeon_result result = {false, false, false, false, false, false, false, message_id, 0};
    return result;
}

static void u3_dungeon_mark_no_go(u3_dungeon_result *result)
{
    /* Legacy reference: Sources/UltimaMain.c NoGo. */
    result->blocked = true;
    result->effect_message_id = 116;
}

static void u3_dungeon_mark_invalid(u3_dungeon_result *result)
{
    /* Legacy reference: Sources/UltimaDngn.c InvalCmd. */
    result->invalid = true;
    result->effect_message_id = 171;
}

uint8_t u3_dungeon_get_cell(const u3_dungeon_state *state, int16_t x, int16_t y)
{
    return state->dungeon[u3_dungeon_offset(state, x, y)];
}

void u3_dungeon_put_cell(u3_dungeon_state *state, uint8_t value, int16_t x, int16_t y)
{
    /* Legacy reference: Sources/UltimaSpellCombat.c PutXYDng. */
    state->dungeon[u3_dungeon_raw_offset(state, x, y)] = value;
}

u3_dungeon_result u3_dungeon_forward(u3_dungeon_state *state)
{
    /* Legacy reference: Sources/UltimaDngn.c Forward, $8DF2. */
    u3_dungeon_result result = u3_dungeon_make_result(165);
    int16_t x = u3_dungeon_wrap_coord((int16_t)(state->x + u3_dungeon_head_x[state->heading]));
    int16_t y = u3_dungeon_wrap_coord((int16_t)(state->y + u3_dungeon_head_y[state->heading]));

    if (u3_dungeon_get_cell(state, x, y) == U3_DUNGEON_TILE_WALL) {
        u3_dungeon_mark_no_go(&result);
        return result;
    }
    state->x = x;
    state->y = y;
    result.moved = true;
    result.needs_redraw = true;
    return result;
}

u3_dungeon_result u3_dungeon_retreat(u3_dungeon_state *state)
{
    /* Legacy reference: Sources/UltimaDngn.c Retreat, $8E2C. */
    u3_dungeon_result result = u3_dungeon_make_result(166);
    int16_t heading = (int16_t)((state->heading + 2) & 3);
    int16_t x = u3_dungeon_wrap_coord((int16_t)(state->x + u3_dungeon_head_x[heading]));
    int16_t y = u3_dungeon_wrap_coord((int16_t)(state->y + u3_dungeon_head_y[heading]));

    if (u3_dungeon_get_cell(state, x, y) == U3_DUNGEON_TILE_WALL) {
        u3_dungeon_mark_no_go(&result);
        return result;
    }
    state->x = x;
    state->y = y;
    result.moved = true;
    result.needs_redraw = true;
    return result;
}

u3_dungeon_result u3_dungeon_turn_right(u3_dungeon_state *state)
{
    /* Legacy reference: Sources/UltimaDngn.c Right, $8E67. */
    u3_dungeon_result result = u3_dungeon_make_result(167);

    if (u3_dungeon_get_cell(state, state->x, state->y) >= U3_DUNGEON_TILE_TURN_BLOCK) {
        u3_dungeon_mark_no_go(&result);
        return result;
    }
    state->heading++;
    if (state->heading > 3)
        state->heading -= 4;
    result.turned = true;
    result.needs_redraw = true;
    return result;
}

u3_dungeon_result u3_dungeon_turn_left(u3_dungeon_state *state)
{
    /* Legacy reference: Sources/UltimaDngn.c Left, $8E93. */
    u3_dungeon_result result = u3_dungeon_make_result(168);

    if (u3_dungeon_get_cell(state, state->x, state->y) >= U3_DUNGEON_TILE_TURN_BLOCK) {
        u3_dungeon_mark_no_go(&result);
        return result;
    }
    state->heading--;
    if (state->heading < 0)
        state->heading += 4;
    result.turned = true;
    result.needs_redraw = true;
    return result;
}

u3_dungeon_result u3_dungeon_descend(u3_dungeon_state *state)
{
    /* Legacy reference: Sources/UltimaDngn.c dDescend, $8F0C. */
    u3_dungeon_result result = u3_dungeon_make_result(169);
    uint8_t cell = u3_dungeon_get_cell(state, state->x, state->y);

    if (cell > 127) {
        u3_dungeon_mark_invalid(&result);
        return result;
    }
    if ((cell & U3_DUNGEON_TILE_DOWN_LADDER) == 0) {
        u3_dungeon_mark_invalid(&result);
        return result;
    }
    if (state->level >= U3_DUNGEON_LEVEL_COUNT - 1) {
        u3_dungeon_mark_invalid(&result);
        return result;
    }
    state->level++;
    result.level_changed = true;
    result.needs_redraw = true;
    return result;
}

u3_dungeon_result u3_dungeon_climb(u3_dungeon_state *state)
{
    /* Legacy reference: Sources/UltimaDngn.c dKlimb, $8F37. */
    u3_dungeon_result result = u3_dungeon_make_result(170);
    uint8_t cell = u3_dungeon_get_cell(state, state->x, state->y);

    if ((cell & U3_DUNGEON_TILE_UP_LADDER) == 0) {
        u3_dungeon_mark_invalid(&result);
        return result;
    }
    state->level--;
    result.level_changed = true;
    if (state->level >= 0 && state->level < U3_DUNGEON_LEVEL_COUNT) {
        result.needs_redraw = true;
        return result;
    }
    state->level = 0;
    state->exit_dungeon = true;
    result.exited = true;
    return result;
}
