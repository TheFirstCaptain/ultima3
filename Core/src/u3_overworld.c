#include "u3_overworld.h"

#include <string.h>

static void u3_overworld_result_clear(u3_overworld_move_result *result)
{
    if (result != 0)
        memset(result, 0, sizeof(*result));
}

static int16_t u3_overworld_constrain(int16_t value, uint8_t size);

static uint8_t u3_overworld_state_init_mode(u3_overworld_state *state,
                                            uint8_t x,
                                            uint8_t y,
                                            uint8_t width,
                                            uint8_t height,
                                            uint8_t wraps)
{
    if (state == 0 || width == 0 || height == 0)
        return 0;

    state->width = width;
    state->height = height;
    state->wraps = wraps != 0;
    if (state->wraps) {
        state->x = (uint8_t)u3_overworld_constrain(x, width);
        state->y = (uint8_t)u3_overworld_constrain(y, height);
    } else {
        state->x = x < width ? x : (uint8_t)(width - 1);
        state->y = y < height ? y : (uint8_t)(height - 1);
    }
    return 1;
}

static int16_t u3_overworld_constrain(int16_t value, uint8_t size)
{
    while (value < 0) {
        value = (int16_t)(value + size);
    }
    while (value >= size) {
        value = (int16_t)(value - size);
    }
    return value;
}

static uint8_t u3_overworld_valid_map(const uint8_t *map_record,
                                      uint32_t map_record_length,
                                      uint8_t *map_size)
{
    if (map_record == 0 || map_record_length < 2 || map_size == 0)
        return 0;

    *map_size = map_record[0];
    if (*map_size == 0 || map_record_length < (uint32_t)(1 + (*map_size * *map_size)))
        return 0;

    return 1;
}

uint8_t u3_overworld_state_init(u3_overworld_state *state,
                                uint8_t x,
                                uint8_t y,
                                uint8_t width,
                                uint8_t height)
{
    return u3_overworld_state_init_mode(state, x, y, width, height, 0);
}

uint8_t u3_overworld_state_init_wrapped(u3_overworld_state *state,
                                        uint8_t x,
                                        uint8_t y,
                                        uint8_t width,
                                        uint8_t height)
{
    return u3_overworld_state_init_mode(state, x, y, width, height, 1);
}

uint8_t u3_overworld_state_init_from_party(u3_overworld_state *state,
                                           const uint8_t *party,
                                           uint32_t party_length,
                                           uint8_t map_size)
{
    if (party == 0 || party_length <= U3_OVERWORLD_PARTY_Y_OFFSET || map_size == 0)
        return 0;

    return u3_overworld_state_init_wrapped(state,
                                           party[U3_OVERWORLD_PARTY_X_OFFSET],
                                           party[U3_OVERWORLD_PARTY_Y_OFFSET],
                                           map_size,
                                           map_size);
}

uint8_t u3_overworld_write_party_position(uint8_t *party,
                                          uint32_t party_length,
                                          const u3_overworld_state *state)
{
    if (party == 0 || state == 0 || party_length <= U3_OVERWORLD_PARTY_Y_OFFSET)
        return 0;

    party[U3_OVERWORLD_PARTY_X_OFFSET] = state->x;
    party[U3_OVERWORLD_PARTY_Y_OFFSET] = state->y;
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
        if (state->wraps)
            state->y = (uint8_t)u3_overworld_constrain((int16_t)(state->y - 1), state->height);
        else if (state->y > 0)
            state->y--;
        break;
    case U3_OVERWORLD_COMMAND_SOUTH:
        if (state->wraps)
            state->y = (uint8_t)u3_overworld_constrain((int16_t)(state->y + 1), state->height);
        else if (state->y + 1 < state->height)
            state->y++;
        break;
    case U3_OVERWORLD_COMMAND_WEST:
        if (state->wraps)
            state->x = (uint8_t)u3_overworld_constrain((int16_t)(state->x - 1), state->width);
        else if (state->x > 0)
            state->x--;
        break;
    case U3_OVERWORLD_COMMAND_EAST:
        if (state->wraps)
            state->x = (uint8_t)u3_overworld_constrain((int16_t)(state->x + 1), state->width);
        else if (state->x + 1 < state->width)
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

    if (state == 0 || !u3_overworld_valid_map(map_record, map_record_length, &map_size))
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

u3_render_frame u3_overworld_make_view_frame(const uint8_t *map_record,
                                             uint32_t map_record_length,
                                             const u3_overworld_state *state)
{
    uint8_t tiles[U3_RENDER_TILE_COUNT];
    uint8_t map_size;
    uint16_t index;

    if (state == 0 || !u3_overworld_valid_map(map_record, map_record_length, &map_size))
        return u3_render_make_synthetic_tile_frame();

    memset(tiles, 0, sizeof(tiles));
    for (index = 0; index < U3_RENDER_TILE_COUNT; index++) {
        uint8_t screen_x = (uint8_t)(index % U3_RENDER_TILE_GRID_WIDTH);
        uint8_t screen_y = (uint8_t)(index / U3_RENDER_TILE_GRID_WIDTH);
        int16_t world_x = (int16_t)(state->x + screen_x - (U3_RENDER_TILE_GRID_WIDTH / 2));
        int16_t world_y = (int16_t)(state->y + screen_y - (U3_RENDER_TILE_GRID_HEIGHT / 2));

        world_x = u3_overworld_constrain(world_x, map_size);
        world_y = u3_overworld_constrain(world_y, map_size);
        tiles[index] = map_record[1 + (world_y * map_size) + world_x];
    }

    tiles[(U3_RENDER_TILE_GRID_HEIGHT / 2 * U3_RENDER_TILE_GRID_WIDTH) + (U3_RENDER_TILE_GRID_WIDTH / 2)] = U3_OVERWORLD_PARTY_TILE;
    return u3_render_make_tile_grid_frame(tiles, U3_RENDER_TILE_COUNT);
}
