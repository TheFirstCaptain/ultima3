#include "u3_dungeon.h"

#include <stddef.h>
#include <string.h>

#include "u3_audio.h"
#include "u3_party.h"

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

static uint16_t u3_dungeon_read_u16(const uint8_t *bytes, uint32_t offset)
{
    return (uint16_t)(((uint16_t)bytes[offset] << 8) | bytes[offset + 1]);
}

static void u3_dungeon_write_u16(uint8_t *bytes, uint32_t offset, uint16_t value)
{
    bytes[offset] = (uint8_t)(value >> 8);
    bytes[offset + 1] = (uint8_t)(value & 0xff);
}

static uint8_t u3_dungeon_roster_slot_occupied(const uint8_t *record)
{
    return record[0] > 22 ? 1 : 0;
}

static uint8_t u3_dungeon_roster_slot_living(const uint8_t *record)
{
    uint8_t status = record[U3_PARTY_ROSTER_STATUS_OFFSET];

    return (uint8_t)(status == 'G' || status == 'P');
}

static uint8_t *u3_dungeon_roster_record(uint8_t *roster, uint8_t roster_id)
{
    if (roster_id == 0 || roster_id > U3_PARTY_ROSTER_SLOT_COUNT)
        return 0;
    return roster + ((roster_id - 1) * U3_PARTY_ROSTER_RECORD_LENGTH);
}

static uint8_t u3_dungeon_valid_party_roster(uint8_t *party,
                                             uint32_t party_length,
                                             uint8_t *roster,
                                             uint32_t roster_length)
{
    return (uint8_t)(party != 0 &&
                     roster != 0 &&
                     party_length == U3_SAVE_PARTY_LENGTH &&
                     roster_length == U3_SAVE_ROSTER_LENGTH &&
                     party[1] > 0 &&
                     party[1] <= U3_PARTY_ACTIVE_SLOT_COUNT);
}

static uint8_t u3_dungeon_find_first_living(uint8_t *party,
                                            uint8_t *roster,
                                            uint8_t *active_slot,
                                            uint8_t *roster_id,
                                            uint8_t **record)
{
    uint8_t index;

    for (index = 0; index < party[1]; index++) {
        uint8_t candidate_id = party[6 + index];
        uint8_t *candidate = u3_dungeon_roster_record(roster, candidate_id);

        if (candidate == 0 ||
            !u3_dungeon_roster_slot_occupied(candidate) ||
            !u3_dungeon_roster_slot_living(candidate))
            continue;

        *active_slot = (uint8_t)(index + 1);
        *roster_id = candidate_id;
        *record = candidate;
        return 1;
    }
    return 0;
}

static uint8_t u3_dungeon_find_rolled_living(uint8_t *party,
                                             uint8_t *roster,
                                             uint16_t roll,
                                             uint8_t *active_slot,
                                             uint8_t *roster_id,
                                             uint8_t **record)
{
    uint8_t living_slots[U3_PARTY_ACTIVE_SLOT_COUNT];
    uint8_t living_count = 0;
    uint8_t index;
    uint8_t selected;

    for (index = 0; index < party[1]; index++) {
        uint8_t candidate_id = party[6 + index];
        uint8_t *candidate = u3_dungeon_roster_record(roster, candidate_id);

        if (candidate == 0 ||
            !u3_dungeon_roster_slot_occupied(candidate) ||
            !u3_dungeon_roster_slot_living(candidate))
            continue;
        living_slots[living_count++] = index;
    }

    if (living_count == 0)
        return 0;

    selected = living_slots[roll % living_count];
    *active_slot = (uint8_t)(selected + 1);
    *roster_id = party[6 + selected];
    *record = u3_dungeon_roster_record(roster, *roster_id);
    return (uint8_t)(*record != 0);
}

static uint8_t u3_dungeon_find_selected_character(uint8_t *party,
                                                  uint8_t *roster,
                                                  uint8_t selected_active_slot,
                                                  uint8_t *roster_id,
                                                  uint8_t **record)
{
    uint8_t candidate_id;
    uint8_t *candidate;

    if (selected_active_slot < 1 || selected_active_slot > party[1])
        return 0;

    candidate_id = party[6 + (selected_active_slot - 1)];
    candidate = u3_dungeon_roster_record(roster, candidate_id);
    if (candidate == 0 || !u3_dungeon_roster_slot_occupied(candidate))
        return 0;

    *roster_id = candidate_id;
    *record = candidate;
    return 1;
}

static uint16_t u3_dungeon_food_value(const uint8_t *record)
{
    return (uint16_t)((record[32] * 100) + record[33]);
}

static uint16_t u3_dungeon_gold_value(const uint8_t *record)
{
    return u3_dungeon_read_u16(record, 35);
}

static uint8_t u3_dungeon_add_gold(uint8_t *record,
                                   uint16_t amount,
                                   u3_dungeon_interaction_result *result)
{
    uint16_t gold = u3_dungeon_gold_value(record);
    uint32_t total = (uint32_t)gold + amount;

    result->gold_before = gold;
    result->gold_added = amount;
    if (total > 9999)
        total = 9999;
    u3_dungeon_write_u16(record, 35, (uint16_t)total);
    result->gold_after = (uint16_t)total;
    return (uint8_t)(total != gold);
}

static uint8_t u3_dungeon_add_item(uint8_t *record, uint8_t offset)
{
    uint16_t total = (uint16_t)(record[offset] + 1);

    if (total > 99)
        total = 99;
    record[offset] = (uint8_t)total;
    return (uint8_t)total;
}

static uint8_t u3_dungeon_disarm_factor(const uint8_t *record)
{
    uint16_t factor = record[19];
    uint8_t class_type = record[23];

    if (class_type == 'T')
        factor += 0x80;
    if (class_type == 'B' || class_type == 'I' || class_type == 'R' || class_type == 'A')
        factor += 0x40;
    if (factor > 255)
        factor = 255;
    return (uint8_t)factor;
}

static uint8_t u3_dungeon_subtract_hp(uint8_t *record, uint16_t damage)
{
    uint16_t hp = u3_dungeon_read_u16(record, 26);

    if (hp <= damage) {
        record[26] = 0;
        record[27] = 0;
        record[U3_PARTY_ROSTER_STATUS_OFFSET] = 'D';
        return (uint8_t)(hp > 0);
    }
    hp = (uint16_t)(hp - damage);
    u3_dungeon_write_u16(record, 26, hp);
    return 0;
}

static void u3_dungeon_apply_trap_damage(uint8_t *party,
                                         uint8_t *roster,
                                         uint16_t damage,
                                         u3_dungeon_special_effect_result *result)
{
    uint8_t index;

    for (index = 0; index < party[1]; index++) {
        uint8_t roster_id = party[6 + index];
        uint8_t *record = u3_dungeon_roster_record(roster, roster_id);

        if (record == 0 ||
            !u3_dungeon_roster_slot_occupied(record) ||
            !u3_dungeon_roster_slot_living(record))
            continue;
        result->damaged_living_members++;
        if (u3_dungeon_subtract_hp(record, damage))
            result->killed_members++;
    }
}

u3_dungeon_special_effect_result u3_dungeon_apply_special_effect(
    u3_dungeon_special_effect_input input,
    uint8_t *party,
    uint32_t party_length,
    uint8_t *roster,
    uint32_t roster_length)
{
    u3_dungeon_special_effect_result result;
    uint8_t active_slot = 0;
    uint8_t roster_id = 0;
    uint8_t *record = 0;

    memset(&result, 0, sizeof(result));
    result.current_tile = input.current_tile;
    result.light_before = input.light;
    result.light_after = input.light;

    switch (input.current_tile) {
    case 0:
        return result;
    case U3_DUNGEON_TILE_WIND:
        result.handled = 1;
        result.status = U3_DUNGEON_SPECIAL_STATUS_WIND;
        result.light_after = 0;
        result.light_changed = (uint8_t)(result.light_after != result.light_before);
        result.message_id = 158;
        return result;
    case U3_DUNGEON_TILE_TRAP:
        result.handled = 1;
        result.status = U3_DUNGEON_SPECIAL_STATUS_INVALID_INPUT;
        result.clear_current_tile = 1;
        result.message_id = 159;
        result.sound_id = U3_AUDIO_SOUND_STEP;
        if (!u3_dungeon_valid_party_roster(party, party_length, roster, roster_length))
            return result;
        if (!u3_dungeon_find_first_living(party, roster, &active_slot, &roster_id, &record)) {
            result.status = U3_DUNGEON_SPECIAL_STATUS_NO_ELIGIBLE_CHARACTER;
            return result;
        }
        result.active_slot = active_slot;
        result.roster_id = roster_id;
        if ((uint8_t)input.disarm_roll > u3_dungeon_disarm_factor(record)) {
            result.status = U3_DUNGEON_SPECIAL_STATUS_TRAP_DISARMED;
            return result;
        }
        result.status = U3_DUNGEON_SPECIAL_STATUS_TRAP_DAMAGE;
        result.message_id = 160;
        result.sound_id = U3_AUDIO_SOUND_HIT;
        result.damage_per_living_member = (uint16_t)((input.trap_damage_roll & 0x77) +
                                                     ((input.level + 1) * 8));
        u3_dungeon_apply_trap_damage(party, roster, result.damage_per_living_member, &result);
        return result;
    case U3_DUNGEON_TILE_GREMLINS:
        result.handled = 1;
        result.status = U3_DUNGEON_SPECIAL_STATUS_INVALID_INPUT;
        result.clear_current_tile = 1;
        result.message_id = 163;
        result.sound_id = U3_AUDIO_SOUND_OUCH;
        if (!u3_dungeon_valid_party_roster(party, party_length, roster, roster_length))
            return result;
        if (!u3_dungeon_find_rolled_living(party, roster, input.gremlin_roll, &active_slot, &roster_id, &record)) {
            result.status = U3_DUNGEON_SPECIAL_STATUS_NO_ELIGIBLE_CHARACTER;
            return result;
        }
        result.status = U3_DUNGEON_SPECIAL_STATUS_GREMLINS;
        result.active_slot = active_slot;
        result.roster_id = roster_id;
        result.food_before = u3_dungeon_food_value(record);
        if (record[32] > 0)
            record[32]--;
        else
            record[32] = 0;
        result.food_after = u3_dungeon_food_value(record);
        return result;
    case U3_DUNGEON_TILE_WRITING:
        result.handled = 1;
        result.status = U3_DUNGEON_SPECIAL_STATUS_WRITING;
        result.message_id = 164;
        return result;
    case 1:
    case 2:
    case 5:
    case 7:
        result.handled = 1;
        result.status = U3_DUNGEON_SPECIAL_STATUS_UNSUPPORTED;
        return result;
    default:
        return result;
    }
}

static u3_dungeon_interaction_result u3_dungeon_make_interaction_result(
    u3_dungeon_interaction_input input)
{
    u3_dungeon_interaction_result result;

    memset(&result, 0, sizeof(result));
    result.current_tile = input.current_tile;
    result.fountain_variant = (uint8_t)(input.x & 0x03);

    if (input.current_tile == U3_DUNGEON_TILE_TIME_LORD) {
        result.handled = 1;
        result.kind = U3_DUNGEON_INTERACTION_KIND_TIME_LORD;
        result.status = U3_DUNGEON_INTERACTION_STATUS_TIME_LORD;
        result.message_id = 151;
        return result;
    }

    if (input.current_tile == U3_DUNGEON_TILE_INTERACTIVE_MESSAGE) {
        result.handled = 1;
        result.kind = U3_DUNGEON_INTERACTION_KIND_TIME_LORD;
        result.status = U3_DUNGEON_INTERACTION_STATUS_TIME_LORD;
        result.message_id = 151;
        return result;
    }

    if (input.current_tile == U3_DUNGEON_TILE_FOUNTAIN) {
        result.handled = 1;
        result.requires_selection = 1;
        result.kind = U3_DUNGEON_INTERACTION_KIND_FOUNTAIN;
        result.status = U3_DUNGEON_INTERACTION_STATUS_SELECTION_REQUIRED;
        result.message_id = 152;
        return result;
    }

    if (input.current_tile == U3_DUNGEON_TILE_MARK) {
        result.handled = 1;
        result.requires_selection = 1;
        result.kind = U3_DUNGEON_INTERACTION_KIND_MARK;
        result.status = U3_DUNGEON_INTERACTION_STATUS_SELECTION_REQUIRED;
        result.message_id = 161;
        return result;
    }

    if (input.command == U3_DUNGEON_COMMAND_GET_CHEST) {
        result.handled = 1;
        result.kind = U3_DUNGEON_INTERACTION_KIND_CHEST;
        if ((input.current_tile & U3_DUNGEON_TILE_CHEST) == 0) {
            result.status = U3_DUNGEON_INTERACTION_STATUS_INVALID_INPUT;
            return result;
        }
        result.requires_selection = 1;
        result.status = U3_DUNGEON_INTERACTION_STATUS_SELECTION_REQUIRED;
        result.message_id = 40;
        return result;
    }

    return result;
}

u3_dungeon_interaction_result u3_dungeon_apply_interaction(
    u3_dungeon_interaction_input input,
    uint8_t *party,
    uint32_t party_length,
    uint8_t *roster,
    uint32_t roster_length)
{
    u3_dungeon_interaction_result result;
    uint8_t roster_id = 0;
    uint8_t *record = 0;

    result = u3_dungeon_make_interaction_result(input);
    if (!result.handled)
        return result;
    if (!result.requires_selection)
        return result;

    if (input.selected_active_slot == 0)
        return result;
    if (input.selected_active_slot > U3_PARTY_ACTIVE_SLOT_COUNT) {
        result.status = U3_DUNGEON_INTERACTION_STATUS_CANCELLED;
        result.requires_selection = 0;
        return result;
    }

    result.requires_selection = 0;
    if (!u3_dungeon_valid_party_roster(party, party_length, roster, roster_length)) {
        result.status = U3_DUNGEON_INTERACTION_STATUS_INVALID_INPUT;
        return result;
    }
    if (!u3_dungeon_find_selected_character(party,
                                            roster,
                                            input.selected_active_slot,
                                            &roster_id,
                                            &record)) {
        result.status = U3_DUNGEON_INTERACTION_STATUS_INVALID_CHARACTER;
        result.active_slot = input.selected_active_slot;
        return result;
    }

    result.active_slot = input.selected_active_slot;
    result.roster_id = roster_id;
    result.status_before = record[U3_PARTY_ROSTER_STATUS_OFFSET];
    result.status_after = result.status_before;
    result.hit_points_before = u3_dungeon_read_u16(record, 26);
    result.hit_points_after = result.hit_points_before;
    result.max_hit_points = u3_dungeon_read_u16(record, 28);
    result.mark_before = record[14];
    result.mark_after = result.mark_before;

    if (!u3_dungeon_roster_slot_living(record)) {
        result.status = U3_DUNGEON_INTERACTION_STATUS_INCAPACITATED;
        return result;
    }

    switch (result.kind) {
    case U3_DUNGEON_INTERACTION_KIND_FOUNTAIN:
        switch (result.fountain_variant) {
        case 0:
            record[U3_PARTY_ROSTER_STATUS_OFFSET] = 'P';
            result.status = U3_DUNGEON_INTERACTION_STATUS_FOUNTAIN_POISON;
            result.status_after = record[U3_PARTY_ROSTER_STATUS_OFFSET];
            result.message_id = 154;
            result.sound_id = U3_AUDIO_SOUND_HIT;
            return result;
        case 1:
            u3_dungeon_write_u16(record, 26, result.max_hit_points);
            result.status = U3_DUNGEON_INTERACTION_STATUS_FOUNTAIN_HEAL;
            result.hit_points_after = result.max_hit_points;
            result.message_id = 155;
            result.sound_id = U3_AUDIO_SOUND_HEAL;
            return result;
        case 2:
            (void)u3_dungeon_subtract_hp(record, 25);
            result.status = U3_DUNGEON_INTERACTION_STATUS_FOUNTAIN_DAMAGE;
            result.status_after = record[U3_PARTY_ROSTER_STATUS_OFFSET];
            result.hit_points_after = u3_dungeon_read_u16(record, 26);
            result.message_id = 156;
            result.sound_id = U3_AUDIO_SOUND_HIT;
            return result;
        default:
            if (record[U3_PARTY_ROSTER_STATUS_OFFSET] == 'P')
                record[U3_PARTY_ROSTER_STATUS_OFFSET] = 'G';
            result.status = U3_DUNGEON_INTERACTION_STATUS_FOUNTAIN_CURE;
            result.status_after = record[U3_PARTY_ROSTER_STATUS_OFFSET];
            result.message_id = 157;
            result.sound_id = U3_AUDIO_SOUND_HEAL;
            return result;
        }
    case U3_DUNGEON_INTERACTION_KIND_MARK:
        record[14] = (uint8_t)(record[14] | (1 << ((input.x & 0x03) + 4)));
        (void)u3_dungeon_subtract_hp(record, 50);
        result.status = U3_DUNGEON_INTERACTION_STATUS_MARK;
        result.status_after = record[U3_PARTY_ROSTER_STATUS_OFFSET];
        result.hit_points_after = u3_dungeon_read_u16(record, 26);
        result.mark_after = record[14];
        result.message_id = 162;
        result.sound_id = U3_AUDIO_SOUND_HIT;
        return result;
    case U3_DUNGEON_INTERACTION_KIND_CHEST: {
        uint16_t gold;
        uint8_t reward_roll;

        if (input.chest_trap_roll > 127) {
            result.chest_trap_kind = (uint8_t)((input.chest_trap_roll & 0x03) + 1);
            if ((uint8_t)input.chest_trap_roll > u3_dungeon_disarm_factor(record)) {
                switch (result.chest_trap_kind) {
                case U3_DUNGEON_CHEST_TRAP_POISON:
                case U3_DUNGEON_CHEST_TRAP_POISON_CLOUD:
                    record[U3_PARTY_ROSTER_STATUS_OFFSET] = 'P';
                    result.status_after = record[U3_PARTY_ROSTER_STATUS_OFFSET];
                    result.message_id = result.chest_trap_kind == U3_DUNGEON_CHEST_TRAP_POISON ? 43 : 45;
                    result.sound_id = U3_AUDIO_SOUND_HIT;
                    break;
                case U3_DUNGEON_CHEST_TRAP_BOMB:
                    result.chest_trap_damage = (uint16_t)((input.chest_trap_roll & 0x77) + 8);
                    (void)u3_dungeon_subtract_hp(record, result.chest_trap_damage);
                    result.status_after = record[U3_PARTY_ROSTER_STATUS_OFFSET];
                    result.hit_points_after = u3_dungeon_read_u16(record, 26);
                    result.message_id = 44;
                    result.sound_id = U3_AUDIO_SOUND_HIT;
                    break;
                default:
                    result.chest_trap_damage = (uint16_t)(input.chest_trap_roll & 0x37);
                    if (result.chest_trap_damage == 0)
                        result.chest_trap_damage = 8;
                    (void)u3_dungeon_subtract_hp(record, result.chest_trap_damage);
                    result.status_after = record[U3_PARTY_ROSTER_STATUS_OFFSET];
                    result.hit_points_after = u3_dungeon_read_u16(record, 26);
                    result.message_id = 42;
                    result.sound_id = U3_AUDIO_SOUND_HIT;
                    break;
                }
            } else {
                result.chest_trap_disarmed = 1;
                result.message_id = 46;
                result.sound_id = U3_AUDIO_SOUND_OUCH;
            }
        }

        gold = (uint16_t)((input.chest_gold_roll & 0xff) % 101);
        if (gold < 30)
            gold = (uint16_t)(gold + 30);
        (void)u3_dungeon_add_gold(record, gold, &result);
        reward_roll = (uint8_t)(input.chest_gold_roll >> 8);
        if (reward_roll > 0 && reward_roll < 128) {
            uint8_t weapon = (uint8_t)(reward_roll & 0x07);

            if (weapon != 0) {
                result.weapon_reward = weapon;
                result.weapon_before = record[48 + weapon];
                result.weapon_after = u3_dungeon_add_item(record, (uint8_t)(48 + weapon));
            }
        } else if (reward_roll >= 128) {
            uint8_t armour = (uint8_t)(reward_roll & 0x03);

            if (armour != 0) {
                result.armour_reward = armour;
                result.armour_before = record[40 + armour];
                result.armour_after = u3_dungeon_add_item(record, (uint8_t)(40 + armour));
            }
        }
        result.status = U3_DUNGEON_INTERACTION_STATUS_CHEST_OPENED;
        result.clear_current_tile = 1;
        if (result.sound_id == 0)
            result.sound_id = U3_AUDIO_SOUND_CREAK;
        return result;
    }
    default:
        result.status = U3_DUNGEON_INTERACTION_STATUS_UNSUPPORTED;
        return result;
    }
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
