#include "u3_dungeon.h"

#include <stddef.h>
#include <string.h>

static const int8_t u3_dungeon_head_x[4] = {0, 1, 0, -1};
static const int8_t u3_dungeon_head_y[4] = {-1, 0, 1, 0};
static const int8_t u3_dungeon_left_x[4] = {-1, 0, 1, 0};
static const int8_t u3_dungeon_left_y[4] = {0, -1, 0, 1};

typedef struct u3_dungeon_view_rect {
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
} u3_dungeon_view_rect;

static const u3_dungeon_view_rect u3_dungeon_front_rects[U3_DUNGEON_VIEW_DEPTH + 1] = {
    {4, 4, 32, 16},
    {8, 6, 24, 12},
    {12, 8, 16, 8},
    {15, 9, 10, 6},
    {18, 10, 4, 4}
};

static const u3_dungeon_view_rect u3_dungeon_left_rects[U3_DUNGEON_VIEW_DEPTH + 1] = {
    {0, 2, 6, 20},
    {5, 4, 5, 16},
    {10, 6, 4, 12},
    {14, 8, 3, 8},
    {17, 9, 2, 6}
};

static const u3_dungeon_view_rect u3_dungeon_right_rects[U3_DUNGEON_VIEW_DEPTH + 1] = {
    {34, 2, 6, 20},
    {30, 4, 5, 16},
    {26, 6, 4, 12},
    {23, 8, 3, 8},
    {21, 9, 2, 6}
};

static u3_render_color u3_dungeon_color(uint8_t red, uint8_t green, uint8_t blue)
{
    u3_render_color color;

    color.red = red;
    color.green = green;
    color.blue = blue;
    color.alpha = 255;
    return color;
}

static u3_render_frame u3_dungeon_make_background_frame(void)
{
    u3_render_frame frame;

    u3_render_frame_init(&frame);
    u3_render_frame_add_clear(&frame, u3_dungeon_color(4, 5, 7));
    u3_render_frame_add_rect(&frame,
                             0,
                             0,
                             U3_RENDER_LOGICAL_WIDTH,
                             U3_RENDER_LOGICAL_HEIGHT,
                             u3_dungeon_color(0, 0, 0),
                             u3_dungeon_color(56, 56, 64));
    return frame;
}

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

static uint32_t u3_dungeon_const_offset(int16_t level, int16_t x, int16_t y)
{
    x = u3_dungeon_wrap_coord(x);
    y = u3_dungeon_wrap_coord(y);
    return (uint32_t)(((uint32_t)level * U3_DUNGEON_WIDTH * U3_DUNGEON_HEIGHT) +
                      ((uint32_t)y * U3_DUNGEON_WIDTH) +
                      (uint32_t)x);
}

static uint8_t u3_dungeon_valid_view_input(const uint8_t *dungeon,
                                           uint32_t dungeon_length,
                                           int16_t level,
                                           int16_t x,
                                           int16_t y,
                                           int16_t heading)
{
    return (uint8_t)(dungeon != NULL &&
                     dungeon_length >= U3_DUNGEON_CELL_COUNT &&
                     level >= 0 && level < U3_DUNGEON_LEVEL_COUNT &&
                     x >= 0 && x < U3_DUNGEON_WIDTH &&
                     y >= 0 && y < U3_DUNGEON_HEIGHT &&
                     heading >= 0 && heading < 4);
}

static uint8_t u3_dungeon_const_cell(const uint8_t *dungeon,
                                     int16_t level,
                                     int16_t x,
                                     int16_t y)
{
    return dungeon[u3_dungeon_const_offset(level, x, y)];
}

static uint8_t u3_dungeon_tile_is_wall(uint8_t tile)
{
    return (uint8_t)(tile >= U3_DUNGEON_TILE_WALL);
}

static uint8_t u3_dungeon_tile_is_door(uint8_t tile)
{
    return (uint8_t)(tile >= 0xC0);
}

static void u3_dungeon_add_view_rect(u3_render_frame *frame,
                                     const u3_dungeon_view_rect *rect,
                                     uint16_t value,
                                     u3_render_color fill,
                                     u3_render_color stroke)
{
    u3_render_command command;

    if (frame == NULL || rect == NULL)
        return;

    memset(&command, 0, sizeof(command));
    command.kind = U3_RENDER_COMMAND_RECT;
    command.x = rect->x;
    command.y = rect->y;
    command.width = rect->width;
    command.height = rect->height;
    command.value = value;
    command.fill = fill;
    command.stroke = stroke;
    u3_render_frame_add_command(frame, &command);
}

static void u3_dungeon_add_depth_wall(u3_render_frame *frame,
                                      const u3_dungeon_view_rect *rect,
                                      uint8_t tile,
                                      uint8_t depth)
{
    uint8_t shade = (uint8_t)(92 - (depth * 13));
    uint16_t value = u3_dungeon_tile_is_door(tile) ?
        U3_DUNGEON_RENDER_VALUE_DOOR :
        U3_DUNGEON_RENDER_VALUE_WALL;
    u3_render_color fill = u3_dungeon_tile_is_door(tile) ?
        u3_dungeon_color((uint8_t)(80 - (depth * 8)), (uint8_t)(48 - (depth * 4)), 24) :
        u3_dungeon_color(shade, shade, shade);

    u3_dungeon_add_view_rect(frame,
                             rect,
                             value,
                             fill,
                             u3_dungeon_color(8, 8, 10));
}

static void u3_dungeon_add_current_feature(u3_render_frame *frame,
                                           uint8_t tile,
                                           uint8_t feature,
                                           uint16_t value,
                                           const u3_dungeon_view_rect *feature_rect)
{
    if ((tile & feature) == 0 || feature_rect == NULL)
        return;

    u3_dungeon_add_view_rect(frame,
                             feature_rect,
                             value,
                             value == U3_DUNGEON_RENDER_VALUE_CHEST ?
                                u3_dungeon_color(144, 96, 32) :
                                u3_dungeon_color(176, 176, 144),
                             u3_dungeon_color(12, 12, 12));
}

static u3_dungeon_view_rect u3_dungeon_feature_rect(uint8_t depth,
                                                    int8_t side,
                                                    uint16_t value)
{
    u3_dungeon_view_rect rect;
    int16_t width = (int16_t)(6 - depth);
    int16_t height = value == U3_DUNGEON_RENDER_VALUE_CHEST ?
        (int16_t)(4 - (depth / 2)) :
        (int16_t)(3 - (depth / 3));

    if (width < 2)
        width = 2;
    if (height < 2)
        height = 2;

    rect.x = (int16_t)(20 - (width / 2) + (side * (9 - (depth * 2))));
    rect.y = value == U3_DUNGEON_RENDER_VALUE_CHEST ?
        (int16_t)(15 - depth) :
        (int16_t)(18 - depth);
    rect.width = width;
    rect.height = height;
    return rect;
}

static void u3_dungeon_add_visible_features(u3_render_frame *frame,
                                            uint8_t tile,
                                            uint8_t depth,
                                            int8_t side)
{
    u3_dungeon_view_rect rect;

    if (u3_dungeon_tile_is_wall(tile))
        return;

    rect = u3_dungeon_feature_rect(depth, side, U3_DUNGEON_RENDER_VALUE_UP_LADDER);
    u3_dungeon_add_current_feature(frame,
                                   tile,
                                   U3_DUNGEON_TILE_UP_LADDER,
                                   U3_DUNGEON_RENDER_VALUE_UP_LADDER,
                                   &rect);
    rect = u3_dungeon_feature_rect(depth, side, U3_DUNGEON_RENDER_VALUE_DOWN_LADDER);
    u3_dungeon_add_current_feature(frame,
                                   tile,
                                   U3_DUNGEON_TILE_DOWN_LADDER,
                                   U3_DUNGEON_RENDER_VALUE_DOWN_LADDER,
                                   &rect);
    rect = u3_dungeon_feature_rect(depth, side, U3_DUNGEON_RENDER_VALUE_CHEST);
    u3_dungeon_add_current_feature(frame,
                                   tile,
                                   U3_DUNGEON_TILE_CHEST,
                                   U3_DUNGEON_RENDER_VALUE_CHEST,
                                   &rect);
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

u3_dungeon_result u3_dungeon_pass(u3_dungeon_state *state)
{
    (void)state;
    /* Legacy reference: Sources/UltimaDngn.c dPass, $8DE6. */
    return u3_dungeon_make_result(23);
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

uint8_t u3_dungeon_decay_light(uint8_t light)
{
    if (light > 0)
        light--;
    return light;
}

u3_dungeon_post_turn_result u3_dungeon_post_turn(u3_dungeon_post_turn_input input)
{
    u3_dungeon_post_turn_result result;
    uint8_t monster_value;

    memset(&result, 0, sizeof(result));
    result.handled = 1;
    result.party_age_requested = 1;
    result.status_refresh_requested = 1;
    result.light_before = input.light;
    result.light_after = u3_dungeon_decay_light(input.light);
    result.light_decremented = (uint8_t)(result.light_after != result.light_before);
    result.current_tile = input.current_tile;

    if (input.level >= U3_DUNGEON_LEVEL_COUNT)
        return result;
    if (input.party_size == 0 || input.living_party_members == 0)
        return result;
    if (input.current_tile != 0)
        return result;

    /* Legacy reference: Sources/UltimaDngn.c dungeonmech, $8FC2. */
    if (input.encounter_roll < 128)
        return result;

    monster_value = (uint8_t)input.monster_roll;
    if (monster_value > 6)
        monster_value = 6;
    monster_value = (uint8_t)(monster_value + 0x18);

    result.encounter_requested = 1;
    result.monster_table_value = monster_value;
    result.monster_type = (uint8_t)(monster_value * 2);
    result.marker_tile = U3_DUNGEON_ENCOUNTER_MARKER_TILE;
    result.combat_screen_resource_id = U3_DUNGEON_COMBAT_SCREEN_RESOURCE_ID;
    return result;
}

static u3_render_frame u3_dungeon_make_view_frame_with_depth(const uint8_t *dungeon,
                                                             uint32_t dungeon_length,
                                                             int16_t level,
                                                             int16_t x,
                                                             int16_t y,
                                                             int16_t heading,
                                                             uint8_t max_depth)
{
    u3_render_frame frame;
    uint8_t depth;

    frame = u3_dungeon_make_background_frame();

    if (!u3_dungeon_valid_view_input(dungeon, dungeon_length, level, x, y, heading))
        return frame;

    for (depth = 0; depth <= max_depth; depth++) {
        int16_t center_x = (int16_t)(x + (u3_dungeon_head_x[heading] * depth));
        int16_t center_y = (int16_t)(y + (u3_dungeon_head_y[heading] * depth));
        uint8_t left_tile = u3_dungeon_const_cell(dungeon,
                                                  level,
                                                  (int16_t)(center_x + u3_dungeon_left_x[heading]),
                                                  (int16_t)(center_y + u3_dungeon_left_y[heading]));
        uint8_t right_tile = u3_dungeon_const_cell(dungeon,
                                                   level,
                                                   (int16_t)(center_x - u3_dungeon_left_x[heading]),
                                                   (int16_t)(center_y - u3_dungeon_left_y[heading]));
        uint8_t front_tile = u3_dungeon_const_cell(dungeon, level, center_x, center_y);

        if (u3_dungeon_tile_is_wall(left_tile))
            u3_dungeon_add_depth_wall(&frame, &u3_dungeon_left_rects[depth], left_tile, depth);
        else
            u3_dungeon_add_visible_features(&frame, left_tile, depth, -1);
        if (u3_dungeon_tile_is_wall(right_tile))
            u3_dungeon_add_depth_wall(&frame, &u3_dungeon_right_rects[depth], right_tile, depth);
        else
            u3_dungeon_add_visible_features(&frame, right_tile, depth, 1);

        if (depth > 0 && u3_dungeon_tile_is_wall(front_tile)) {
            u3_dungeon_add_depth_wall(&frame, &u3_dungeon_front_rects[depth], front_tile, depth);
            break;
        }
        u3_dungeon_add_visible_features(&frame, front_tile, depth, 0);
    }

    return frame;
}

u3_render_frame u3_dungeon_make_view_frame(const uint8_t *dungeon,
                                           uint32_t dungeon_length,
                                           int16_t level,
                                           int16_t x,
                                           int16_t y,
                                           int16_t heading)
{
    return u3_dungeon_make_view_frame_with_depth(
        dungeon,
        dungeon_length,
        level,
        x,
        y,
        heading,
        U3_DUNGEON_VIEW_DEPTH);
}

u3_render_frame u3_dungeon_make_lit_view_frame(const uint8_t *dungeon,
                                               uint32_t dungeon_length,
                                               int16_t level,
                                               int16_t x,
                                               int16_t y,
                                               int16_t heading,
                                               uint8_t light)
{
    if (light == 0)
        return u3_dungeon_make_background_frame();
    if (light < 3)
        return u3_dungeon_make_view_frame_with_depth(dungeon, dungeon_length, level, x, y, heading, 0);
    return u3_dungeon_make_view_frame_with_depth(
        dungeon,
        dungeon_length,
        level,
        x,
        y,
        heading,
        U3_DUNGEON_VIEW_DEPTH);
}
