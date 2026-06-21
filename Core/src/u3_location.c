#include "u3_location.h"

#include <string.h>

static uint8_t u3_location_valid_map(const u3_overworld_state *state,
                                     const uint8_t *map_record,
                                     uint32_t map_record_length,
                                     uint8_t *map_size)
{
    uint32_t required_length;

    if (state == 0 || map_record == 0 || map_record_length < 2 || map_size == 0)
        return 0;

    *map_size = map_record[0];
    if (*map_size == 0 || state->width != *map_size || state->height != *map_size ||
        state->x >= *map_size || state->y >= *map_size)
        return 0;

    required_length = 1u + ((uint32_t)*map_size * (uint32_t)*map_size);
    return (uint8_t)(map_record_length >= required_length);
}

static uint16_t u3_location_resource_id(uint8_t location_index)
{
    uint16_t resource_id = (uint16_t)(U3_LOCATION_RESOURCE_BASE + location_index);

    if (location_index > U3_LOCATION_RESOURCE_GAP_INDEX)
        resource_id = (uint16_t)(resource_id + U3_LOCATION_RESOURCE_GAP_SIZE);
    return resource_id;
}

uint8_t u3_location_handle_overworld_command(
    const u3_overworld_state *state,
    const uint8_t *map_record,
    uint32_t map_record_length,
    const uint8_t *location_table,
    uint32_t location_table_length,
    uint16_t command,
    u3_location_transition_result *result)
{
    uint8_t map_size;
    uint8_t index;
    uint8_t tile;

    if (result == 0)
        return 0;
    memset(result, 0, sizeof(*result));

    if (!u3_location_valid_map(state, map_record, map_record_length, &map_size) ||
        location_table == 0 || location_table_length < U3_LOCATION_TABLE_LENGTH)
        return 0;

    if (command != U3_LOCATION_COMMAND_ENTER)
        return 1;

    result->handled = 1;
    result->return_x = state->x;
    result->return_y = state->y;
    result->status = U3_LOCATION_STATUS_NOT_ENTERABLE;

    for (index = 0; index < U3_LOCATION_TABLE_COUNT; index++) {
        if (location_table[index] != state->x ||
            location_table[U3_LOCATION_TABLE_COUNT + index] != state->y)
            continue;

        tile = map_record[1u + ((uint32_t)state->y * map_size) + state->x];
        if ((uint8_t)(tile / 2u) != U3_LOCATION_TOWN_TILE_CLASS)
            return 1;

        result->requested = 1;
        result->destination_kind = U3_LOCATION_KIND_TOWN;
        result->location_index = index;
        result->resource_id = u3_location_resource_id(index);
        result->initial_x = U3_LOCATION_TOWN_INITIAL_X;
        result->initial_y = U3_LOCATION_TOWN_INITIAL_Y;
        result->initial_heading = U3_LOCATION_TOWN_INITIAL_HEADING;
        result->status = U3_LOCATION_STATUS_TOWN_REQUESTED;
        return 1;
    }

    return 1;
}
