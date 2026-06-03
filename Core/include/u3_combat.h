#ifndef U3_COMBAT_H
#define U3_COMBAT_H

#include <stdint.h>

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

typedef struct u3_combat_state {
    int16_t monster_type;
    uint8_t character_x[U3_COMBAT_CHARACTER_COUNT];
    uint8_t character_y[U3_COMBAT_CHARACTER_COUNT];
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

uint8_t u3_combat_valid_move(const u3_combat_state *state, int16_t tile);
uint8_t u3_combat_monster_here(const u3_combat_state *state, int16_t x, int16_t y);
uint8_t u3_combat_character_here(const u3_combat_state *state, int16_t x, int16_t y);
u3_combat_damage_result u3_combat_damage_monster(u3_combat_state *state,
                                                  const uint8_t experience[U3_COMBAT_EXPERIENCE_COUNT],
                                                  int16_t monster,
                                                  int16_t damage,
                                                  int16_t character);
u3_combat_attack_result u3_combat_attack(u3_combat_state *state,
                                          const uint8_t experience[U3_COMBAT_EXPERIENCE_COUNT],
                                          const u3_combat_attack_input *input);

#endif
