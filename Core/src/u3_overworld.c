#include "u3_overworld.h"

#include <string.h>

static void u3_overworld_result_clear(u3_overworld_move_result *result)
{
    if (result != 0)
        memset(result, 0, sizeof(*result));
}

uint8_t u3_overworld_state_init(u3_overworld_state *state,
                                uint8_t x,
                                uint8_t y,
                                uint8_t width,
                                uint8_t height)
{
    if (state == 0 || width == 0 || height == 0)
        return 0;

    state->width = width;
    state->height = height;
    state->x = x < width ? x : (uint8_t)(width - 1);
    state->y = y < height ? y : (uint8_t)(height - 1);
    return 1;
}

uint8_t u3_overworld_move(u3_overworld_state *state,
                          uint16_t command,
                          u3_overworld_move_result *result)
{
    uint8_t original_x;
    uint8_t original_y;

    u3_overworld_result_clear(result);
    if (state == 0 || result == 0 || state->width == 0 || state->height == 0)
        return 0;

    original_x = state->x;
    original_y = state->y;
    result->handled = 1;

    switch (command) {
    case U3_OVERWORLD_COMMAND_NORTH:
        if (state->y > 0)
            state->y--;
        break;
    case U3_OVERWORLD_COMMAND_SOUTH:
        if (state->y + 1 < state->height)
            state->y++;
        break;
    case U3_OVERWORLD_COMMAND_WEST:
        if (state->x > 0)
            state->x--;
        break;
    case U3_OVERWORLD_COMMAND_EAST:
        if (state->x + 1 < state->width)
            state->x++;
        break;
    default:
        result->handled = 0;
        break;
    }

    result->moved = (uint8_t)(state->x != original_x || state->y != original_y);
    result->x = state->x;
    result->y = state->y;
    return 1;
}

u3_render_frame u3_overworld_make_smoke_frame(const uint8_t *map_record,
                                              uint32_t map_record_length,
                                              const u3_overworld_state *state)
{
    uint8_t tiles[U3_RENDER_TILE_COUNT];
    uint8_t map_size;
    uint8_t view_width;
    uint8_t view_height;
    uint16_t index;
    u3_render_frame frame;

    if (map_record == 0 || map_record_length < 2 || state == 0)
        return u3_render_make_synthetic_tile_frame();

    map_size = map_record[0];
    if (map_size == 0 || map_record_length < (uint32_t)(1 + (map_size * map_size)))
        return u3_render_make_synthetic_tile_frame();

    view_width = map_size < U3_OVERWORLD_SMOKE_VIEW_WIDTH ? map_size : U3_OVERWORLD_SMOKE_VIEW_WIDTH;
    view_height = map_size < U3_OVERWORLD_SMOKE_VIEW_HEIGHT ? map_size : U3_OVERWORLD_SMOKE_VIEW_HEIGHT;

    memset(tiles, 0, sizeof(tiles));
    for (index = 0; index < U3_RENDER_TILE_COUNT; index++) {
        uint8_t x = (uint8_t)(index % U3_RENDER_TILE_GRID_WIDTH);
        uint8_t y = (uint8_t)(index / U3_RENDER_TILE_GRID_WIDTH);

        if (x < view_width && y < view_height)
            tiles[index] = map_record[1 + (y * map_size) + x];
    }

    if (state->x < U3_OVERWORLD_SMOKE_VIEW_WIDTH && state->y < U3_OVERWORLD_SMOKE_VIEW_HEIGHT)
        tiles[(state->y * U3_RENDER_TILE_GRID_WIDTH) + state->x] = U3_OVERWORLD_PARTY_TILE;

    frame = u3_render_make_tile_grid_frame(tiles, U3_RENDER_TILE_COUNT);
    return frame;
}
