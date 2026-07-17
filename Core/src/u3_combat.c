#include "u3_combat.h"

uint8_t u3_combat_valid_move(const u3_combat_state *state, int16_t tile)
{
    /* Legacy reference: Sources/UltimaSpellCombat.c CombatValidMove, $7E31. */
    if (state->monster_type < 0x16 || state->monster_type >= 0x20) {
        if (tile == 2)
            return 0;
        if (tile == 4)
            return 0;
        if (tile == 6)
            return 0;
        if (tile == 0x10)
            return 0;
        return 0xFF;
    } else {
        if (tile == 0)
            return 0;
        return 0xFF;
    }
}

uint8_t u3_combat_monster_here(const u3_combat_state *state, int16_t x, int16_t y)
{
    /* Legacy reference: Sources/UltimaSpellCombat.c CombatMonsterHere, $7D24. */
    int16_t mon;
    uint8_t result = U3_COMBAT_NO_SLOT;

    for (mon = U3_COMBAT_MONSTER_COUNT - 1; mon >= 0; mon--) {
        if (state->monster_hp[mon] != 0) {
            if (state->monster_x[mon] == x && state->monster_y[mon] == y)
                result = (uint8_t)mon;
        }
    }
    return result;
}

uint8_t u3_combat_character_here(const u3_combat_state *state, int16_t x, int16_t y)
{
    /* Legacy reference: Sources/UltimaSpellCombat.c CombatCharacterHere. */
    int16_t character;

    for (character = U3_COMBAT_CHARACTER_COUNT - 1; character >= 0; character--) {
        if (state->character_x[character] == x && state->character_y[character] == y)
            return (uint8_t)character;
    }
    return U3_COMBAT_NO_SLOT;
}

static void u3_combat_put_tile(u3_combat_state *state, int16_t value, int16_t x, int16_t y)
{
    /* Legacy reference: Sources/UltimaMisc.c PutXYTile. */
    if (x < 0 || x > 10 || y < 0 || y > 10)
        return;
    state->tile_array[(y * 11) + x] = (uint8_t)value;
}

static uint8_t u3_combat_get_tile(const u3_combat_state *state, int16_t x, int16_t y)
{
    /* Legacy reference: Sources/UltimaMisc.c GetXYTile. */
    if (x < 0 || x > 10 || y < 0 || y > 10)
        return 0;
    return state->tile_array[(y * 11) + x];
}

static uint8_t u3_combat_is_projectile_weapon(uint8_t weapon)
{
    return weapon == 3 || weapon == 5 || weapon == 9 || weapon == 13;
}

static uint8_t u3_combat_character_can_act(const u3_combat_state *state, uint8_t character)
{
    if (state->character_status[character] == U3_COMBAT_STATUS_DEAD ||
        state->character_status[character] == U3_COMBAT_STATUS_ASH)
        return 0;
    if (state->character_x[character] > 10 || state->character_y[character] > 10)
        return 0;
    return 1;
}

static uint8_t u3_combat_is_player_passable_tile(uint8_t tile)
{
    return tile == 2 || tile == 4 || tile == 6 || tile == 0x10;
}

static uint8_t u3_combat_command_delta(uint16_t command, int8_t *dx, int8_t *dy)
{
    *dx = 0;
    *dy = 0;
    switch (command) {
    case U3_COMBAT_COMMAND_NORTH:
        *dy = -1;
        return 1;
    case U3_COMBAT_COMMAND_SOUTH:
        *dy = 1;
        return 1;
    case U3_COMBAT_COMMAND_WEST:
        *dx = -1;
        return 1;
    case U3_COMBAT_COMMAND_EAST:
        *dx = 1;
        return 1;
    default:
        return 0;
    }
}

static void u3_combat_set_attack_miss(u3_combat_attack_result *result)
{
    result->miss = 1;
    result->message_id = U3_COMBAT_ATTACK_MISS_MESSAGE;
    result->redraw_tiles = 1;
}

static uint8_t u3_combat_is_magic_monster(uint8_t monster_type)
{
    return monster_type == 0x1A ||
           monster_type == 0x1C ||
           monster_type == 0x2C ||
           monster_type == 0x36 ||
           monster_type == 0x3A ||
           monster_type == 0x3C;
}

static uint8_t u3_combat_is_poison_monster(uint8_t monster_type)
{
    return monster_type == 0x1C || monster_type == 0x3C || monster_type == 0x38;
}

static uint8_t u3_combat_subtract_character_hp(u3_combat_state *state, uint8_t character, uint16_t amount)
{
    if (state->character_hp[character] < amount + 1) {
        state->character_hp[character] = 0;
        state->character_status[character] = U3_COMBAT_STATUS_DEAD;
        return 1;
    }
    state->character_hp[character] = (uint16_t)(state->character_hp[character] - amount);
    return 0;
}

u3_combat_damage_result u3_combat_damage_monster(u3_combat_state *state,
                                                  const uint8_t experience[U3_COMBAT_EXPERIENCE_COUNT],
                                                  int16_t monster,
                                                  int16_t damage,
                                                  int16_t character)
{
    /* Legacy reference: Sources/UltimaSpellCombat.c DamageMonster, $84C7. */
    u3_combat_damage_result result = {0};

    result.character = (uint8_t)character;

    if (state->monster_type == U3_COMBAT_LORD_BRITISH_TYPE) {
        result.ignored_lord_british = 1;
        return result;
    }

    if ((state->monster_hp[monster] - damage) < 1) {
        uint8_t expnum = (uint8_t)((state->monster_type / 2) & 0x0F);
        uint8_t x = state->monster_x[monster];
        uint8_t y = state->monster_y[monster];
        uint8_t restored_tile = state->monster_tile[monster];

        result.defeated = 1;
        result.award_experience = 1;
        result.redraw_tiles = 1;
        result.restore_tile = 1;
        result.message_id = U3_COMBAT_MONSTER_DEFEATED_MESSAGE;
        result.experience_awarded = experience[expnum];
        result.monster_x = x;
        result.monster_y = y;
        result.restored_tile = restored_tile;

        u3_combat_put_tile(state, restored_tile, x, y);
        state->monster_hp[monster] = 0;
    } else {
        result.damaged = 1;
        state->monster_hp[monster] = (uint8_t)(state->monster_hp[monster] - damage);
    }

    return result;
}

static void u3_combat_move_monster(u3_combat_state *state,
                                   const u3_combat_monster_action_input *input,
                                   u3_combat_monster_action_result *result)
{
    uint8_t monster = input->monster;
    uint8_t draw_tile = input->monster_tile_value;

    if (draw_tile == 0)
        draw_tile = (uint8_t)state->monster_type;

    result->moved = 1;
    result->redraw_tiles = 1;
    result->moved_from_x = state->monster_x[monster];
    result->moved_from_y = state->monster_y[monster];
    result->moved_to_x = input->move_x;
    result->moved_to_y = input->move_y;

    u3_combat_put_tile(state, state->monster_tile[monster], state->monster_x[monster], state->monster_y[monster]);
    state->monster_x[monster] = input->move_x;
    state->monster_y[monster] = input->move_y;
    state->monster_tile[monster] = u3_combat_get_tile(state, state->monster_x[monster], state->monster_y[monster]);
    u3_combat_put_tile(state, draw_tile, state->monster_x[monster], state->monster_y[monster]);
}

static void u3_combat_try_poison(u3_combat_state *state,
                                 const u3_combat_monster_action_input *input,
                                 u3_combat_monster_action_result *result,
                                 uint8_t target)
{
    if (!u3_combat_is_poison_monster((uint8_t)state->monster_type))
        return;
    if ((input->poison_roll & 0x03) != 0)
        return;
    if (state->character_status[target] != U3_COMBAT_STATUS_GOOD)
        return;

    state->character_status[target] = U3_COMBAT_STATUS_POISONED;
    result->poisoned = 1;
    result->status_message_id = U3_COMBAT_POISON_MESSAGE;
    result->play_ouch_sound = 1;
}

static void u3_combat_try_pilfer(u3_combat_state *state,
                                 const u3_combat_monster_action_input *input,
                                 u3_combat_monster_action_result *result,
                                 uint8_t target)
{
    uint8_t item = input->pilfer_item_roll;

    if (state->monster_type != 0x2E)
        return;

    if (input->pilfer_branch_roll < 128) {
        item &= 0x0F;
        if (item == 0)
            return;
        if (state->character_weapon[target] == item)
            return;
        if (state->character_weapon_inventory[target][item] == 0)
            return;
        state->character_weapon_inventory[target][item] = 0;
        result->pilfered_weapon = 1;
    } else {
        item &= 0x07;
        if (item == 0)
            return;
        if (state->character_armour[target] == item)
            return;
        if (state->character_armour_inventory[target][item] == 0)
            return;
        state->character_armour_inventory[target][item] = 0;
        result->pilfered_armour = 1;
    }

    result->pilfered = 1;
    result->pilfer_item = item;
    result->status_message_id = U3_COMBAT_PILFER_MESSAGE;
    result->play_ouch_sound = 1;
}

static void u3_combat_apply_monster_hit(u3_combat_state *state,
                                        const u3_combat_monster_action_input *input,
                                        u3_combat_monster_action_result *result,
                                        uint8_t target,
                                        uint8_t hit_tile,
                                        uint8_t emit_hit_message)
{
    uint16_t level = state->character_experience[target] / 100;
    uint16_t damage_max = (uint16_t)(((input->monster_hp_start / 8) + level) | 1);
    uint16_t damage = (uint16_t)((input->damage_roll % (damage_max + 1)) + 1);
    uint16_t exodus_damage = (uint16_t)((input->exodus_damage_flags & 0x03) * 16);

    damage = (uint16_t)(damage + exodus_damage);

    result->hit = 1;
    if (emit_hit_message) {
        result->message_id = U3_COMBAT_MONSTER_HIT_MESSAGE;
        result->outcome_message_id = U3_COMBAT_MONSTER_HIT_MESSAGE;
    }
    result->play_hit_sound = 1;
    result->pause_after_hit = 1;
    result->redraw_tiles = 1;
    result->hit_x = state->character_x[target];
    result->hit_y = state->character_y[target];
    result->hit_tile = hit_tile;
    result->damage_amount = (uint8_t)damage;

    result->character_died = u3_combat_subtract_character_hp(state, target, damage);
    u3_combat_put_tile(state, state->character_shape[target], state->character_x[target], state->character_y[target]);
    if (result->character_died) {
        result->death_message_id = U3_COMBAT_CHARACTER_DIED_MESSAGE;
        u3_combat_put_tile(state, state->character_tile[target], state->character_x[target], state->character_y[target]);
        state->character_x[target] = 0xFF;
        state->character_y[target] = 0xFF;
    }
}

u3_combat_monster_action_result u3_combat_monster_action(u3_combat_state *state,
                                                          const u3_combat_monster_action_input *input)
{
    /* Legacy reference: Sources/UltimaSpellCombat.c monster turn flow, $85A6-$878C. */
    u3_combat_monster_action_result result = {0};
    uint8_t target = input->target_character;
    uint8_t hit_tile = U3_COMBAT_ATTACK_HIT_TILE;

    result.target_character = U3_COMBAT_NO_SLOT;

    if (input->monster >= U3_COMBAT_MONSTER_COUNT || state->monster_hp[input->monster] == 0) {
        result.no_action = 1;
        return result;
    }
    if (input->target_distance != 0) {
        if (target < input->party_size &&
            target < U3_COMBAT_CHARACTER_COUNT &&
            state->monster_type == 0x3A &&
            input->shoot_choice_roll < 128) {
            result.shot = 1;
            result.play_shoot_sound = 1;
            target = input->projectile_hit_character;
            if (target >= input->party_size ||
                target >= U3_COMBAT_CHARACTER_COUNT ||
                state->character_status[target] == U3_COMBAT_STATUS_DEAD ||
                state->character_status[target] == U3_COMBAT_STATUS_ASH) {
                result.projectile_missed = 1;
                result.pause_after_projectile = 1;
                result.redraw_tiles = 1;
                return result;
            }
            result.target_character = target;
            u3_combat_apply_monster_hit(state, input, &result, target, U3_COMBAT_ATTACK_HIT_TILE, 0);
            return result;
        }

        if (input->magic_choice_roll > 127 && u3_combat_is_magic_monster((uint8_t)state->monster_type)) {
            target = input->magic_target_character;
            if (target < input->party_size &&
                target < U3_COMBAT_CHARACTER_COUNT &&
                state->character_status[target] == U3_COMBAT_STATUS_GOOD) {
                result.cast_spell = 1;
                result.play_monster_spell_sound = 1;
                hit_tile = 0x78;
            } else if (input->target_distance < 0x80) {
                u3_combat_move_monster(state, input, &result);
                return result;
            } else {
                result.no_action = 1;
                return result;
            }
        } else if (input->target_distance < 0x80) {
            u3_combat_move_monster(state, input, &result);
            return result;
        } else {
            result.no_action = 1;
            return result;
        }
    }

    if (target >= input->party_size || target >= U3_COMBAT_CHARACTER_COUNT) {
        result.no_action = 1;
        return result;
    }

    result.target_character = target;
    result.attacked = 1;
    result.play_attack_sound = 1;
    result.attack_message_id = U3_COMBAT_MONSTER_ATTACK_MESSAGE;

    u3_combat_try_poison(state, input, &result, target);
    u3_combat_try_pilfer(state, input, &result, target);

    if (!(input->exodus_castle_active && state->character_armour[target] != 7)) {
        if (input->armour_hit_roll >= 8) {
            result.miss = 1;
            result.message_id = U3_COMBAT_MONSTER_MISS_MESSAGE;
            result.outcome_message_id = U3_COMBAT_MONSTER_MISS_MESSAGE;
            return result;
        }
    }

    u3_combat_apply_monster_hit(state, input, &result, target, hit_tile, 1);
    return result;
}

u3_combat_attack_result u3_combat_attack(u3_combat_state *state,
                                          const uint8_t experience[U3_COMBAT_EXPERIENCE_COUNT],
                                          const u3_combat_attack_input *input)
{
    /* Legacy reference: Sources/UltimaSpellCombat.c CombatAttack, $8360. */
    u3_combat_attack_result result = {0};
    uint8_t weapon = input->weapon;
    uint8_t target = U3_COMBAT_NO_SLOT;
    uint8_t damage_weapon;
    uint8_t x;
    uint8_t y;

    result.target_monster = U3_COMBAT_NO_SLOT;
    result.weapon_after = input->weapon;
    result.weapon_quantity_after = input->weapon_quantity;

    if (input->direction_x == 0 && input->direction_y == 0) {
        result.cancelled = 1;
        return result;
    }

    result.swish_sound = input->character + 1;

    if (u3_combat_is_projectile_weapon(weapon)) {
        result.used_projectile = 1;
        target = input->projectile_monster;
    } else {
        target = u3_combat_monster_here(state,
                                        state->character_x[input->character] + input->direction_x,
                                        state->character_y[input->character] + input->direction_y);
        if (target == U3_COMBAT_NO_SLOT && weapon == 1) {
            result.used_projectile = 1;
            result.used_thrown_dagger = 1;
            result.mutated_inventory = 1;
            result.weapon_quantity_after = (uint8_t)(input->weapon_quantity - 1);
            if (result.weapon_quantity_after < 1 || result.weapon_quantity_after > 250) {
                result.weapon_after = 0;
                result.weapon_quantity_after = 0;
                result.unequipped_weapon = 1;
            }
            target = input->projectile_monster;
        }
    }

    if (target >= U3_COMBAT_MONSTER_COUNT || state->monster_hp[target] == 0) {
        u3_combat_set_attack_miss(&result);
        return result;
    }

    result.target_monster = target;

    if (input->exodus_castle_result == 0 && weapon != 15) {
        u3_combat_set_attack_miss(&result);
        return result;
    }

    if (input->hit_chance_roll < 128) {
        if (input->dexterity < input->hit_dexterity_roll) {
            u3_combat_set_attack_miss(&result);
            return result;
        }
    }

    x = state->monster_x[target];
    y = state->monster_y[target];
    damage_weapon = result.weapon_after;

    result.hit = 1;
    result.print_hit_message = 1;
    result.play_hit_sound = 1;
    result.redraw_tiles = 1;
    result.pause_after_hit = 1;
    result.message_id = U3_COMBAT_ATTACK_HIT_MESSAGE;
    result.hit_x = x;
    result.hit_y = y;
    result.hit_tile = U3_COMBAT_ATTACK_HIT_TILE;
    result.damage_amount = (uint8_t)(input->damage_roll + (input->strength / 2) + (damage_weapon * 3) + 4);

    u3_combat_put_tile(state, state->monster_type, x, y);
    result.damage_result = u3_combat_damage_monster(state,
                                                     experience,
                                                     target,
                                                     result.damage_amount,
                                                     input->character + 1);

    return result;
}

u3_combat_player_command_result u3_combat_player_command(u3_combat_state *state,
                                                          const uint8_t experience[U3_COMBAT_EXPERIENCE_COUNT],
                                                          const u3_combat_player_command_input *input)
{
    /* Legacy references: Sources/UltimaSpellCombat.c Combat player loop and HandleMove. */
    u3_combat_player_command_result result = {0};
    int8_t dx;
    int8_t dy;
    int16_t target_x;
    int16_t target_y;

    if (state == 0 || input == 0)
        return result;

    result.character = input->character;
    if (input->character >= U3_COMBAT_CHARACTER_COUNT) {
        result.handled = 1;
        result.unsupported = 1;
        result.status = U3_COMBAT_PLAYER_STATUS_UNSUPPORTED;
        return result;
    }

    result.x = state->character_x[input->character];
    result.y = state->character_y[input->character];
    if (!u3_combat_character_can_act(state, input->character)) {
        result.handled = 1;
        result.unsupported = 1;
        result.status = U3_COMBAT_PLAYER_STATUS_UNSUPPORTED;
        return result;
    }

    if (input->command == U3_COMBAT_COMMAND_PASS) {
        result.handled = 1;
        result.passed = 1;
        result.status = U3_COMBAT_PLAYER_STATUS_PASSED;
        return result;
    }

    if (input->command == U3_COMBAT_COMMAND_ATTACK) {
        u3_combat_attack_input attack;

        result.handled = 1;
        if (input->attack_direction_x == 0 && input->attack_direction_y == 0) {
            result.attack_direction_required = 1;
            result.status = U3_COMBAT_PLAYER_STATUS_ATTACK_DIRECTION_REQUIRED;
            return result;
        }

        if (u3_combat_is_projectile_weapon(input->weapon)) {
            result.unsupported = 1;
            result.status = U3_COMBAT_PLAYER_STATUS_ATTACK_DEFERRED;
            return result;
        }

        if (input->weapon == 1 &&
            u3_combat_monster_here(state,
                                   state->character_x[input->character] + input->attack_direction_x,
                                   state->character_y[input->character] + input->attack_direction_y) == U3_COMBAT_NO_SLOT) {
            result.unsupported = 1;
            result.status = U3_COMBAT_PLAYER_STATUS_ATTACK_DEFERRED;
            return result;
        }

        attack.character = input->character;
        attack.direction_x = input->attack_direction_x;
        attack.direction_y = input->attack_direction_y;
        attack.weapon = input->weapon;
        attack.weapon_quantity = input->weapon_quantity;
        attack.strength = input->strength;
        attack.dexterity = input->dexterity;
        attack.projectile_monster = input->projectile_monster;
        attack.exodus_castle_result = input->exodus_castle_result;
        attack.hit_chance_roll = input->hit_chance_roll;
        attack.hit_dexterity_roll = input->hit_dexterity_roll;
        attack.damage_roll = input->damage_roll;

        result.attack_result = u3_combat_attack(state, experience, &attack);
        result.attacked = 1;
        result.redraw = result.attack_result.redraw_tiles;
        if (result.attack_result.play_hit_sound)
            result.sound_id = U3_AUDIO_SOUND_HIT;
        if (result.attack_result.hit) {
            result.status = U3_COMBAT_PLAYER_STATUS_ATTACK_HIT;
        } else if (result.attack_result.cancelled) {
            result.status = U3_COMBAT_PLAYER_STATUS_ATTACK_CANCELLED;
        } else {
            result.status = U3_COMBAT_PLAYER_STATUS_ATTACK_MISSED;
        }
        return result;
    }

    if (!u3_combat_command_delta(input->command, &dx, &dy)) {
        result.handled = 1;
        result.unsupported = 1;
        result.status = U3_COMBAT_PLAYER_STATUS_UNSUPPORTED;
        return result;
    }

    result.handled = 1;
    target_x = (int16_t)state->character_x[input->character] + dx;
    target_y = (int16_t)state->character_y[input->character] + dy;
    result.target_x = (uint8_t)target_x;
    result.target_y = (uint8_t)target_y;

    if (target_x < 0 || target_x > 10 || target_y < 0 || target_y > 10 ||
        !u3_combat_is_player_passable_tile(u3_combat_get_tile(state, target_x, target_y)) ||
        u3_combat_monster_here(state, target_x, target_y) != U3_COMBAT_NO_SLOT ||
        u3_combat_character_here(state, target_x, target_y) != U3_COMBAT_NO_SLOT) {
        result.blocked = 1;
        result.status = U3_COMBAT_PLAYER_STATUS_BLOCKED;
        result.sound_id = U3_AUDIO_SOUND_BUMP;
        if (target_x >= 0 && target_x <= 10 && target_y >= 0 && target_y <= 10)
            result.target_tile = u3_combat_get_tile(state, target_x, target_y);
        return result;
    }

    result.target_tile = u3_combat_get_tile(state, target_x, target_y);
    state->character_x[input->character] = (uint8_t)target_x;
    state->character_y[input->character] = (uint8_t)target_y;
    state->character_tile[input->character] = result.target_tile;
    result.moved = 1;
    result.redraw = 1;
    result.sound_id = U3_AUDIO_SOUND_STEP;
    result.status = U3_COMBAT_PLAYER_STATUS_MOVED;
    result.x = state->character_x[input->character];
    result.y = state->character_y[input->character];
    return result;
}
