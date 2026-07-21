#ifndef U3_COMBAT_RENDER_H
#define U3_COMBAT_RENDER_H

#include <stdint.h>

#include "u3_combat.h"
#include "u3_render.h"

#define U3_COMBAT_SCREEN_TILE_BYTES U3_RENDER_TILE_COUNT
#define U3_COMBAT_SCREEN_FULL_LENGTH 0xB0
#define U3_COMBAT_RENDER_MONSTER_BASE 0xE0
#define U3_COMBAT_RENDER_PARTY_BASE 0xF0

#define U3_COMBAT_SCREEN_STATUS_OK 1
#define U3_COMBAT_SCREEN_STATUS_TILE_ONLY 2
#define U3_COMBAT_SCREEN_STATUS_INVALID 3

#define U3_COMBAT_PRESENTATION_NONE 0
#define U3_COMBAT_PRESENTATION_HIT_FLASH 1
#define U3_COMBAT_PRESENTATION_PROJECTILE_TRACE 2

typedef struct u3_combat_screen_init_result {
    uint8_t status;
    uint8_t copied_tiles;
    uint8_t initialized_monsters;
    uint8_t initialized_characters;
} u3_combat_screen_init_result;

typedef struct u3_combat_render_result {
    uint8_t rendered;
    uint8_t terrain_commands;
    uint8_t monster_commands;
    uint8_t party_commands;
    uint8_t invalid_occupants;
} u3_combat_render_result;

typedef struct u3_combat_presentation_result {
    uint8_t applied;
    uint8_t effect_kind;
    uint8_t trace_commands;
    uint8_t flash_commands;
    uint8_t overflowed;
} u3_combat_presentation_result;

u3_combat_screen_init_result u3_combat_state_init_from_screen(u3_combat_state *state,
                                                              const uint8_t *screen,
                                                              uint32_t screen_length,
                                                              uint8_t monster_type,
                                                              uint8_t monster_count,
                                                              uint8_t party_size);

u3_render_frame u3_combat_make_frame(const u3_combat_state *state,
                                     u3_combat_render_result *result);
u3_combat_presentation_result u3_combat_render_add_player_presentation(u3_render_frame *frame,
                                                                       const u3_combat_player_command_result *result);
u3_combat_presentation_result u3_combat_render_add_monster_presentation(u3_render_frame *frame,
                                                                        const u3_combat_monster_turn_result *result);

#endif
