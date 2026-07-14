#include "u3_combat_render.h"

#include <stddef.h>
#include <string.h>

static uint8_t u3_combat_coordinate_valid(uint8_t value)
{
    return value < U3_RENDER_TILE_GRID_WIDTH;
}

u3_combat_screen_init_result u3_combat_state_init_from_screen(u3_combat_state *state,
                                                              const uint8_t *screen,
                                                              uint32_t screen_length,
                                                              uint8_t monster_type,
                                                              uint8_t monster_count,
                                                              uint8_t party_size)
{
    u3_combat_screen_init_result result;
    uint8_t index;
    uint8_t capped_monsters = monster_count;
    uint8_t capped_party = party_size;

    memset(&result, 0, sizeof(result));
    if (state == NULL || screen == NULL || screen_length < U3_COMBAT_SCREEN_TILE_BYTES) {
        result.status = U3_COMBAT_SCREEN_STATUS_INVALID;
        return result;
    }

    memset(state, 0, sizeof(*state));
    state->monster_type = monster_type;
    memcpy(state->tile_array, screen, U3_COMBAT_SCREEN_TILE_BYTES);
    result.copied_tiles = U3_COMBAT_SCREEN_TILE_BYTES;

    if (screen_length < U3_COMBAT_SCREEN_FULL_LENGTH) {
        result.status = U3_COMBAT_SCREEN_STATUS_TILE_ONLY;
        return result;
    }

    if (capped_monsters > U3_COMBAT_MONSTER_COUNT)
        capped_monsters = U3_COMBAT_MONSTER_COUNT;
    if (capped_party > U3_COMBAT_CHARACTER_COUNT)
        capped_party = U3_COMBAT_CHARACTER_COUNT;

    for (index = 0; index < U3_COMBAT_MONSTER_COUNT; index++) {
        state->monster_x[index] = screen[0x80 + index];
        state->monster_y[index] = screen[0x88 + index];
        state->monster_tile[index] = screen[0x90 + index];
        state->monster_hp[index] = 0;
    }

    for (index = 0; index < capped_monsters; index++) {
        state->monster_tile[index] = monster_type;
        state->monster_hp[index] = 1;
        result.initialized_monsters++;
    }

    for (index = 0; index < U3_COMBAT_CHARACTER_COUNT; index++) {
        state->character_x[index] = screen[0xA0 + index];
        state->character_y[index] = screen[0xA4 + index];
        state->character_tile[index] = screen[0xA8 + index];
        state->character_shape[index] = index < capped_party ? screen[0xAC + index] : 0;
    }
    result.initialized_characters = capped_party;
    result.status = U3_COMBAT_SCREEN_STATUS_OK;
    return result;
}

u3_render_frame u3_combat_make_frame(const u3_combat_state *state,
                                     u3_combat_render_result *result)
{
    u3_render_frame frame;
    u3_combat_render_result local_result;
    uint8_t index;

    memset(&local_result, 0, sizeof(local_result));
    if (result != NULL)
        *result = local_result;

    if (state == NULL)
        return u3_render_make_synthetic_tile_frame();

    frame = u3_render_make_tile_grid_frame(state->tile_array, U3_COMBAT_SCREEN_TILE_BYTES);
    local_result.rendered = 1;
    local_result.terrain_commands = U3_COMBAT_SCREEN_TILE_BYTES;

    for (index = 0; index < U3_COMBAT_MONSTER_COUNT; index++) {
        if (state->monster_hp[index] == 0)
            continue;
        if (!u3_combat_coordinate_valid(state->monster_x[index]) ||
            !u3_combat_coordinate_valid(state->monster_y[index])) {
            local_result.invalid_occupants++;
            continue;
        }
        if (u3_render_frame_add_tile(&frame,
                                     (uint8_t)(U3_COMBAT_RENDER_MONSTER_BASE + index),
                                     (int16_t)(1 + state->monster_x[index]),
                                     (int16_t)(1 + state->monster_y[index])))
            local_result.monster_commands++;
    }

    for (index = 0; index < U3_COMBAT_CHARACTER_COUNT; index++) {
        if (state->character_shape[index] == 0)
            continue;
        if (!u3_combat_coordinate_valid(state->character_x[index]) ||
            !u3_combat_coordinate_valid(state->character_y[index])) {
            local_result.invalid_occupants++;
            continue;
        }
        if (u3_render_frame_add_tile(&frame,
                                     (uint8_t)(U3_COMBAT_RENDER_PARTY_BASE + index),
                                     (int16_t)(1 + state->character_x[index]),
                                     (int16_t)(1 + state->character_y[index])))
            local_result.party_commands++;
    }

    if (result != NULL)
        *result = local_result;
    return frame;
}
