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

uint16_t u3_location_resource_id_for_index(uint8_t location_index)
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
        result->resource_id = u3_location_resource_id_for_index(index);
        result->initial_x = U3_LOCATION_TOWN_INITIAL_X;
        result->initial_y = U3_LOCATION_TOWN_INITIAL_Y;
        result->initial_heading = U3_LOCATION_TOWN_INITIAL_HEADING;
        result->status = U3_LOCATION_STATUS_TOWN_REQUESTED;
        return 1;
    }

    return 1;
}

uint8_t u3_location_session_init(
    const u3_location_transition_result *request,
    const uint8_t *map_record,
    uint32_t map_record_length,
    const uint8_t *monster_record,
    uint32_t monster_record_length,
    const uint8_t *talk_record,
    uint32_t talk_record_length,
    u3_location_session *session)
{
    uint8_t map_shape;
    uint16_t map_size;

    if (session == 0)
        return 0;
    memset(session, 0, sizeof(*session));

    if (request == 0 || request->requested == 0 ||
        request->destination_kind == U3_LOCATION_KIND_NONE ||
        request->resource_id != u3_location_resource_id_for_index(request->location_index) ||
        map_record == 0 || talk_record == 0 ||
        talk_record_length != U3_LOCATION_TALK_LENGTH)
        return 0;

    switch (request->destination_kind) {
    case U3_LOCATION_KIND_TOWN:
    case U3_LOCATION_KIND_CASTLE:
        if (map_record_length != U3_LOCATION_TWO_DIMENSIONAL_MAP_LENGTH ||
            map_record[0] != U3_LOCATION_TWO_DIMENSIONAL_MAP_SIZE ||
            monster_record == 0 || monster_record_length != U3_LOCATION_MONSTER_LENGTH)
            return 0;
        map_shape = U3_LOCATION_MAP_SHAPE_TWO_DIMENSIONAL;
        map_size = U3_LOCATION_TWO_DIMENSIONAL_MAP_SIZE;
        break;
    case U3_LOCATION_KIND_DUNGEON:
        if (map_record_length != U3_LOCATION_DUNGEON_MAP_LENGTH ||
            monster_record != 0 || monster_record_length != 0)
            return 0;
        map_shape = U3_LOCATION_MAP_SHAPE_DUNGEON;
        map_size = 16;
        break;
    default:
        return 0;
    }

    if (request->initial_x >= map_size || request->initial_y >= map_size ||
        request->initial_heading > 3)
        return 0;

    session->active = 1;
    session->destination_kind = request->destination_kind;
    session->location_index = request->location_index;
    session->resource_id = request->resource_id;
    session->map_shape = map_shape;
    session->map_size = map_size;
    session->map_length = map_record_length;
    session->monster_length = monster_record_length;
    session->talk_length = talk_record_length;
    session->x = request->initial_x;
    session->y = request->initial_y;
    session->heading = request->initial_heading;
    session->return_x = request->return_x;
    session->return_y = request->return_y;
    return 1;
}

static uint8_t u3_location_valid_two_dimensional_session(
    const u3_location_session *session,
    const uint8_t *map_record,
    uint32_t map_record_length)
{
    uint32_t required_length;

    if (session == 0 || session->active == 0 ||
        session->map_shape != U3_LOCATION_MAP_SHAPE_TWO_DIMENSIONAL ||
        session->map_size == 0 || session->map_size > 255 ||
        session->x >= session->map_size || session->y >= session->map_size ||
        map_record == 0 || map_record_length != session->map_length ||
        map_record[0] != session->map_size)
        return 0;

    required_length = 1u + ((uint32_t)session->map_size * session->map_size);
    return (uint8_t)(map_record_length == required_length);
}

uint8_t u3_location_tile_passable(uint8_t tile)
{
    /* Legacy reference: Sources/UltimaMain.c ValidDir non-ship branch. */
    if (tile == 136 || tile == 248)
        tile = 4;
    if (tile == 128 || tile == 132)
        return 1;
    return (uint8_t)(tile < 48 && tile != 0 && tile != 16);
}

uint8_t u3_location_move(
    u3_location_session *session,
    const uint8_t *map_record,
    uint32_t map_record_length,
    uint16_t command,
    u3_location_move_result *result)
{
    int16_t target_x;
    int16_t target_y;
    uint8_t target_tile;

    if (result == 0)
        return 0;
    memset(result, 0, sizeof(*result));
    if (!u3_location_valid_two_dimensional_session(session, map_record, map_record_length))
        return 0;

    target_x = session->x;
    target_y = session->y;
    switch (command) {
    case U3_OVERWORLD_COMMAND_NORTH:
        target_y--;
        break;
    case U3_OVERWORLD_COMMAND_SOUTH:
        target_y++;
        break;
    case U3_OVERWORLD_COMMAND_WEST:
        target_x--;
        break;
    case U3_OVERWORLD_COMMAND_EAST:
        target_x++;
        break;
    default:
        result->x = session->x;
        result->y = session->y;
        return 1;
    }

    result->handled = 1;
    result->x = session->x;
    result->y = session->y;
    if (target_x < 0 || target_y < 0 ||
        target_x >= session->map_size || target_y >= session->map_size) {
        result->blocked = 1;
        result->target_x = session->x;
        result->target_y = session->y;
        result->status = U3_LOCATION_MOVE_STATUS_BLOCKED;
        result->sound_id = U3_AUDIO_SOUND_BUMP;
        return 1;
    }

    result->target_x = (uint8_t)target_x;
    result->target_y = (uint8_t)target_y;
    target_tile = map_record[1u + ((uint32_t)target_y * session->map_size) + target_x];
    result->target_tile = target_tile;
    if (!u3_location_tile_passable(target_tile)) {
        result->blocked = 1;
        result->status = U3_LOCATION_MOVE_STATUS_BLOCKED;
        result->sound_id = U3_AUDIO_SOUND_BUMP;
        return 1;
    }

    session->x = (uint8_t)target_x;
    session->y = (uint8_t)target_y;
    result->moved = 1;
    result->x = session->x;
    result->y = session->y;
    result->redraw = 1;
    result->sound_id = U3_AUDIO_SOUND_STEP;
    if (session->x == 0 || session->y == 0) {
        result->exit_requested = 1;
        result->status = U3_LOCATION_MOVE_STATUS_EXIT_REQUESTED;
    } else {
        result->status = U3_LOCATION_MOVE_STATUS_MOVED;
    }
    return 1;
}

uint8_t u3_location_apply_party_turn(
    uint8_t *party,
    uint32_t party_length,
    u3_location_move_result *result)
{
    u3_overworld_move_result turn_result;

    if (result == 0 || result->handled == 0)
        return 0;
    memset(&turn_result, 0, sizeof(turn_result));
    if (!u3_overworld_increment_party_move_counter(party, party_length, &turn_result))
        return 0;

    result->turn_applied = turn_result.turn_applied;
    result->turn_delta = turn_result.turn_delta;
    result->move_counter_before = turn_result.move_counter_before;
    result->move_counter_after = turn_result.move_counter_after;
    return 1;
}

u3_render_frame u3_location_make_view_frame(
    const u3_location_session *session,
    const uint8_t *map_record,
    uint32_t map_record_length)
{
    uint8_t tiles[U3_RENDER_TILE_COUNT];
    uint16_t index;

    if (!u3_location_valid_two_dimensional_session(session, map_record, map_record_length))
        return u3_render_make_synthetic_tile_frame();

    for (index = 0; index < U3_RENDER_TILE_COUNT; index++) {
        uint8_t screen_x = (uint8_t)(index % U3_RENDER_TILE_GRID_WIDTH);
        uint8_t screen_y = (uint8_t)(index / U3_RENDER_TILE_GRID_WIDTH);
        int16_t map_x = (int16_t)(session->x + screen_x - (U3_RENDER_TILE_GRID_WIDTH / 2));
        int16_t map_y = (int16_t)(session->y + screen_y - (U3_RENDER_TILE_GRID_HEIGHT / 2));

        if (map_x < 0 || map_y < 0 || map_x >= session->map_size || map_y >= session->map_size)
            tiles[index] = 4;
        else
            tiles[index] = map_record[1u + ((uint32_t)map_y * session->map_size) + map_x];
    }

    tiles[(U3_RENDER_TILE_GRID_HEIGHT / 2 * U3_RENDER_TILE_GRID_WIDTH) +
          (U3_RENDER_TILE_GRID_WIDTH / 2)] = U3_OVERWORLD_PARTY_TILE;
    return u3_render_make_tile_grid_frame(tiles, U3_RENDER_TILE_COUNT);
}
