#ifndef U3_COMBAT_H
#define U3_COMBAT_H

#include <stdint.h>

#include "u3_audio.h"

#define U3_COMBAT_CHARACTER_COUNT 4
#define U3_COMBAT_MONSTER_COUNT 8
#define U3_COMBAT_NO_SLOT 255
#define U3_COMBAT_TILE_COUNT 128
#define U3_COMBAT_EXPERIENCE_COUNT 17
#define U3_COMBAT_LORD_BRITISH_TYPE 0x26
#define U3_COMBAT_MONSTER_DEFEATED_MESSAGE 130
#define U3_COMBAT_ATTACK_HIT_MESSAGE 146
#define U3_COMBAT_ATTACK_MISS_MESSAGE 147
#define U3_COMBAT_ATTACK_HIT_TILE 0x7A
#define U3_COMBAT_MONSTER_ATTACK_MESSAGE 141
#define U3_COMBAT_MONSTER_MISS_MESSAGE 142
#define U3_COMBAT_MONSTER_HIT_MESSAGE 143
#define U3_COMBAT_CHARACTER_DIED_MESSAGE 144
#define U3_COMBAT_PILFER_MESSAGE 148
#define U3_COMBAT_POISON_MESSAGE 149
#define U3_COMBAT_STATUS_GOOD 'G'
#define U3_COMBAT_STATUS_DEAD 'D'
#define U3_COMBAT_STATUS_ASH 'A'
#define U3_COMBAT_STATUS_POISONED 'P'
#define U3_COMBAT_EXPERIENCE_MAX 9899

#define U3_COMBAT_COMMAND_NORTH 1
#define U3_COMBAT_COMMAND_SOUTH 2
#define U3_COMBAT_COMMAND_WEST 3
#define U3_COMBAT_COMMAND_EAST 4
#define U3_COMBAT_COMMAND_PASS 32
#define U3_COMBAT_COMMAND_ATTACK 65

#define U3_COMBAT_PLAYER_STATUS_NONE 0
#define U3_COMBAT_PLAYER_STATUS_MOVED 1
#define U3_COMBAT_PLAYER_STATUS_BLOCKED 2
#define U3_COMBAT_PLAYER_STATUS_PASSED 3
#define U3_COMBAT_PLAYER_STATUS_ATTACK_DIRECTION_REQUIRED 4
#define U3_COMBAT_PLAYER_STATUS_ATTACK_CANCELLED 5
#define U3_COMBAT_PLAYER_STATUS_ATTACK_MISSED 6
#define U3_COMBAT_PLAYER_STATUS_ATTACK_HIT 7
#define U3_COMBAT_PLAYER_STATUS_ATTACK_DEFERRED 8
#define U3_COMBAT_PLAYER_STATUS_UNSUPPORTED 9

#define U3_COMBAT_MONSTER_TURN_STATUS_NONE 0
#define U3_COMBAT_MONSTER_TURN_STATUS_MOVED 1
#define U3_COMBAT_MONSTER_TURN_STATUS_ATTACK_MISSED 2
#define U3_COMBAT_MONSTER_TURN_STATUS_ATTACK_HIT 3
#define U3_COMBAT_MONSTER_TURN_STATUS_SHOT 4
#define U3_COMBAT_MONSTER_TURN_STATUS_SPELL_DEFERRED 5
#define U3_COMBAT_MONSTER_TURN_STATUS_NO_ACTION 6

typedef struct u3_combat_state {
    int16_t monster_type;
    uint8_t character_x[U3_COMBAT_CHARACTER_COUNT];
    uint8_t character_y[U3_COMBAT_CHARACTER_COUNT];
    uint8_t character_tile[U3_COMBAT_CHARACTER_COUNT];
    uint8_t character_shape[U3_COMBAT_CHARACTER_COUNT];
    uint8_t character_status[U3_COMBAT_CHARACTER_COUNT];
    uint8_t character_armour[U3_COMBAT_CHARACTER_COUNT];
    uint8_t character_weapon[U3_COMBAT_CHARACTER_COUNT];
    uint16_t character_hp[U3_COMBAT_CHARACTER_COUNT];
    uint16_t character_experience[U3_COMBAT_CHARACTER_COUNT];
    uint8_t character_armour_inventory[U3_COMBAT_CHARACTER_COUNT][8];
    uint8_t character_weapon_inventory[U3_COMBAT_CHARACTER_COUNT][16];
    uint8_t monster_x[U3_COMBAT_MONSTER_COUNT];
    uint8_t monster_y[U3_COMBAT_MONSTER_COUNT];
    uint8_t monster_tile[U3_COMBAT_MONSTER_COUNT];
    uint8_t monster_hp[U3_COMBAT_MONSTER_COUNT];
    uint8_t tile_array[U3_COMBAT_TILE_COUNT];
} u3_combat_state;

typedef struct u3_combat_damage_result {
    uint8_t ignored_lord_british;
    uint8_t damaged;
    uint8_t defeated;
    uint8_t award_experience;
    uint8_t redraw_tiles;
    uint8_t restore_tile;
    uint8_t character;
    uint8_t message_id;
    uint8_t experience_awarded;
    uint8_t monster_x;
    uint8_t monster_y;
    uint8_t restored_tile;
} u3_combat_damage_result;

typedef struct u3_combat_experience_award_result {
    uint8_t applied;
    uint8_t level_increased;
    uint8_t character;
    uint8_t amount;
    uint16_t experience_before;
    uint16_t experience_after;
} u3_combat_experience_award_result;

typedef struct u3_combat_victory_result {
    uint8_t checked;
    uint8_t victorious;
    uint8_t live_monsters;
    uint8_t defeated_monsters;
} u3_combat_victory_result;

typedef struct u3_combat_attack_input {
    uint8_t character;
    int8_t direction_x;
    int8_t direction_y;
    uint8_t weapon;
    uint8_t weapon_quantity;
    uint8_t strength;
    uint8_t dexterity;
    uint8_t projectile_monster;
    uint8_t exodus_castle_result;
    uint8_t hit_chance_roll;
    uint8_t hit_dexterity_roll;
    uint8_t damage_roll;
} u3_combat_attack_input;

typedef struct u3_combat_attack_result {
    uint8_t cancelled;
    uint8_t miss;
    uint8_t hit;
    uint8_t used_projectile;
    uint8_t used_thrown_dagger;
    uint8_t mutated_inventory;
    uint8_t unequipped_weapon;
    uint8_t swish_sound;
    uint8_t print_hit_message;
    uint8_t play_hit_sound;
    uint8_t redraw_tiles;
    uint8_t pause_after_hit;
    uint8_t target_monster;
    uint8_t message_id;
    uint8_t weapon_after;
    uint8_t weapon_quantity_after;
    uint8_t hit_x;
    uint8_t hit_y;
    uint8_t hit_tile;
    uint8_t damage_amount;
    u3_combat_damage_result damage_result;
} u3_combat_attack_result;

typedef struct u3_combat_player_command_input {
    uint16_t command;
    uint8_t character;
    int8_t attack_direction_x;
    int8_t attack_direction_y;
    uint8_t weapon;
    uint8_t weapon_quantity;
    uint8_t strength;
    uint8_t dexterity;
    uint8_t projectile_monster;
    uint8_t exodus_castle_result;
    uint8_t hit_chance_roll;
    uint8_t hit_dexterity_roll;
    uint8_t damage_roll;
} u3_combat_player_command_input;

typedef struct u3_combat_player_command_result {
    uint8_t handled;
    uint8_t moved;
    uint8_t blocked;
    uint8_t passed;
    uint8_t attack_direction_required;
    uint8_t attacked;
    uint8_t unsupported;
    uint8_t redraw;
    uint16_t sound_id;
    uint8_t status;
    uint8_t character;
    uint8_t x;
    uint8_t y;
    uint8_t target_x;
    uint8_t target_y;
    uint8_t target_tile;
    u3_combat_attack_result attack_result;
} u3_combat_player_command_result;

typedef struct u3_combat_monster_action_input {
    uint8_t monster;
    uint8_t party_size;
    uint8_t target_character;
    uint8_t target_distance;
    uint8_t move_x;
    uint8_t move_y;
    int8_t shoot_delta_x;
    int8_t shoot_delta_y;
    uint8_t shoot_choice_roll;
    uint8_t magic_choice_roll;
    uint8_t magic_target_character;
    uint8_t projectile_hit_character;
    uint8_t monster_hp_start;
    uint8_t monster_tile_value;
    uint8_t poison_roll;
    uint8_t pilfer_branch_roll;
    uint8_t pilfer_item_roll;
    uint8_t exodus_castle_active;
    uint8_t exodus_damage_flags;
    uint8_t armour_hit_roll;
    uint8_t damage_roll;
} u3_combat_monster_action_input;

typedef struct u3_combat_monster_action_result {
    uint8_t no_action;
    uint8_t moved;
    uint8_t shot;
    uint8_t projectile_missed;
    uint8_t cast_spell;
    uint8_t attacked;
    uint8_t miss;
    uint8_t hit;
    uint8_t poisoned;
    uint8_t pilfered;
    uint8_t pilfered_weapon;
    uint8_t pilfered_armour;
    uint8_t character_died;
    uint8_t redraw_tiles;
    uint8_t pause_after_projectile;
    uint8_t pause_after_hit;
    uint8_t play_attack_sound;
    uint8_t play_hit_sound;
    uint8_t play_ouch_sound;
    uint8_t play_shoot_sound;
    uint8_t play_monster_spell_sound;
    uint8_t target_character;
    uint8_t message_id;
    uint8_t attack_message_id;
    uint8_t status_message_id;
    uint8_t outcome_message_id;
    uint8_t death_message_id;
    uint8_t pilfer_item;
    uint8_t hit_x;
    uint8_t hit_y;
    uint8_t hit_tile;
    uint8_t damage_amount;
    uint8_t moved_from_x;
    uint8_t moved_from_y;
    uint8_t moved_to_x;
    uint8_t moved_to_y;
} u3_combat_monster_action_result;

typedef struct u3_combat_monster_turn_input {
    uint8_t party_size;
    uint8_t starting_monster;
    uint8_t shoot_choice_roll;
    uint8_t magic_choice_roll;
    uint8_t magic_target_character;
    uint8_t projectile_hit_character;
    uint8_t poison_roll;
    uint8_t pilfer_branch_roll;
    uint8_t pilfer_item_roll;
    uint8_t exodus_castle_active;
    uint8_t exodus_damage_flags;
    uint8_t armour_hit_roll;
    uint8_t damage_roll;
} u3_combat_monster_turn_input;

typedef struct u3_combat_monster_turn_result {
    uint8_t handled;
    uint8_t no_action;
    uint8_t monster;
    uint8_t next_starting_monster;
    uint8_t target_character;
    uint8_t redraw;
    uint16_t sound_id;
    uint8_t status;
    u3_combat_monster_action_result action_result;
} u3_combat_monster_turn_result;

uint8_t u3_combat_valid_move(const u3_combat_state *state, int16_t tile);
uint8_t u3_combat_monster_here(const u3_combat_state *state, int16_t x, int16_t y);
uint8_t u3_combat_character_here(const u3_combat_state *state, int16_t x, int16_t y);
uint8_t u3_combat_projectile_monster(const u3_combat_state *state,
                                      uint8_t character,
                                      int8_t direction_x,
                                      int8_t direction_y);
u3_combat_damage_result u3_combat_damage_monster(u3_combat_state *state,
                                                  const uint8_t experience[U3_COMBAT_EXPERIENCE_COUNT],
                                                  int16_t monster,
                                                  int16_t damage,
                                                  int16_t character);
u3_combat_experience_award_result u3_combat_apply_experience_award(u3_combat_state *state,
                                                                    const u3_combat_damage_result *damage_result);
u3_combat_victory_result u3_combat_check_victory(const u3_combat_state *state);
u3_combat_attack_result u3_combat_attack(u3_combat_state *state,
                                          const uint8_t experience[U3_COMBAT_EXPERIENCE_COUNT],
                                          const u3_combat_attack_input *input);
u3_combat_player_command_result u3_combat_player_command(u3_combat_state *state,
                                                          const uint8_t experience[U3_COMBAT_EXPERIENCE_COUNT],
                                                          const u3_combat_player_command_input *input);
u3_combat_monster_action_result u3_combat_monster_action(u3_combat_state *state,
                                                          const u3_combat_monster_action_input *input);
u3_combat_monster_turn_result u3_combat_monster_turn(u3_combat_state *state,
                                                      const u3_combat_monster_turn_input *input);

#endif
