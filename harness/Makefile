CC ?= clang
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -pedantic
BUILD_DIR ?= build
CORE_INCLUDE = ../Core/include
CORE_SRC = ../Core/src
TESTS = \
	$(BUILD_DIR)/map_math_accessors_tests \
	$(BUILD_DIR)/pascal_string_helpers_tests \
	$(BUILD_DIR)/combat_predicates_tests \
	$(BUILD_DIR)/combat_damage_tests \
	$(BUILD_DIR)/combat_attack_tests \
	$(BUILD_DIR)/combat_monster_action_tests \
	$(BUILD_DIR)/combat_render_tests \
	$(BUILD_DIR)/autocombat_targeting_tests \
	$(BUILD_DIR)/dungeon_navigation_tests \
	$(BUILD_DIR)/dungeon_render_tests \
	$(BUILD_DIR)/dungeon_post_turn_tests \
	$(BUILD_DIR)/dungeon_special_tile_tests \
	$(BUILD_DIR)/resource_fixture_tests \
	$(BUILD_DIR)/save_fixture_tests \
	$(BUILD_DIR)/render_contract_tests \
	$(BUILD_DIR)/input_adapter_tests \
	$(BUILD_DIR)/audio_adapter_tests \
	$(BUILD_DIR)/tick_smoke_tests \
	$(BUILD_DIR)/party_roster_tests \
	$(BUILD_DIR)/overworld_movement_tests \
	$(BUILD_DIR)/location_transition_tests \
	$(BUILD_DIR)/location_navigation_tests \
	$(BUILD_DIR)/location_talk_tests \
	$(BUILD_DIR)/character_creation_tests

.PHONY: test clean

test: $(TESTS)
	@set -e; for test in $(TESTS); do ./$$test; done

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/map_math_accessors_tests: map_math_accessors_tests.c $(CORE_SRC)/u3_map_math.c $(CORE_INCLUDE)/u3_map_math.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) map_math_accessors_tests.c $(CORE_SRC)/u3_map_math.c -o $@

$(BUILD_DIR)/pascal_string_helpers_tests: pascal_string_helpers_tests.c $(CORE_SRC)/u3_pascal_string.c $(CORE_INCLUDE)/u3_pascal_string.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) pascal_string_helpers_tests.c $(CORE_SRC)/u3_pascal_string.c -o $@

$(BUILD_DIR)/combat_predicates_tests: combat_predicates_tests.c $(CORE_SRC)/u3_combat.c $(CORE_INCLUDE)/u3_combat.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) combat_predicates_tests.c $(CORE_SRC)/u3_combat.c -o $@

$(BUILD_DIR)/combat_damage_tests: combat_damage_tests.c $(CORE_SRC)/u3_combat.c $(CORE_INCLUDE)/u3_combat.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) combat_damage_tests.c $(CORE_SRC)/u3_combat.c -o $@

$(BUILD_DIR)/combat_attack_tests: combat_attack_tests.c $(CORE_SRC)/u3_combat.c $(CORE_INCLUDE)/u3_combat.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) combat_attack_tests.c $(CORE_SRC)/u3_combat.c -o $@

$(BUILD_DIR)/combat_monster_action_tests: combat_monster_action_tests.c $(CORE_SRC)/u3_combat.c $(CORE_INCLUDE)/u3_combat.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) combat_monster_action_tests.c $(CORE_SRC)/u3_combat.c -o $@

$(BUILD_DIR)/combat_render_tests: combat_render_tests.c $(CORE_SRC)/u3_combat_render.c $(CORE_INCLUDE)/u3_combat_render.h $(CORE_SRC)/u3_render.c $(CORE_INCLUDE)/u3_render.h $(CORE_INCLUDE)/u3_combat.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) combat_render_tests.c $(CORE_SRC)/u3_combat_render.c $(CORE_SRC)/u3_render.c -o $@

$(BUILD_DIR)/autocombat_targeting_tests: autocombat_targeting_tests.c $(CORE_SRC)/u3_autocombat.c $(CORE_INCLUDE)/u3_autocombat.h $(CORE_SRC)/u3_combat.c $(CORE_INCLUDE)/u3_combat.h $(CORE_SRC)/u3_map_math.c $(CORE_INCLUDE)/u3_map_math.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) autocombat_targeting_tests.c $(CORE_SRC)/u3_autocombat.c $(CORE_SRC)/u3_combat.c $(CORE_SRC)/u3_map_math.c -o $@

$(BUILD_DIR)/dungeon_navigation_tests: dungeon_navigation_tests.c $(CORE_SRC)/u3_dungeon.c $(CORE_INCLUDE)/u3_dungeon.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) dungeon_navigation_tests.c $(CORE_SRC)/u3_dungeon.c $(CORE_SRC)/u3_render.c -o $@

$(BUILD_DIR)/dungeon_render_tests: dungeon_render_tests.c $(CORE_SRC)/u3_dungeon.c $(CORE_INCLUDE)/u3_dungeon.h $(CORE_SRC)/u3_render.c $(CORE_INCLUDE)/u3_render.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) dungeon_render_tests.c $(CORE_SRC)/u3_dungeon.c $(CORE_SRC)/u3_render.c -o $@

$(BUILD_DIR)/dungeon_post_turn_tests: dungeon_post_turn_tests.c $(CORE_SRC)/u3_dungeon.c $(CORE_INCLUDE)/u3_dungeon.h $(CORE_SRC)/u3_render.c $(CORE_INCLUDE)/u3_render.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) dungeon_post_turn_tests.c $(CORE_SRC)/u3_dungeon.c $(CORE_SRC)/u3_render.c -o $@

$(BUILD_DIR)/dungeon_special_tile_tests: dungeon_special_tile_tests.c $(CORE_SRC)/u3_dungeon.c $(CORE_INCLUDE)/u3_dungeon.h $(CORE_SRC)/u3_render.c $(CORE_INCLUDE)/u3_render.h $(CORE_INCLUDE)/u3_audio.h $(CORE_INCLUDE)/u3_party.h $(CORE_INCLUDE)/u3_save.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) dungeon_special_tile_tests.c $(CORE_SRC)/u3_dungeon.c $(CORE_SRC)/u3_render.c -o $@

$(BUILD_DIR)/resource_fixture_tests: resource_fixture_tests.c $(CORE_SRC)/u3_resource.c $(CORE_INCLUDE)/u3_resource.h ../Resources/English.lproj/MainResources.rsrc | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) resource_fixture_tests.c $(CORE_SRC)/u3_resource.c -o $@

$(BUILD_DIR)/save_fixture_tests: save_fixture_tests.c $(CORE_SRC)/u3_resource.c $(CORE_INCLUDE)/u3_resource.h $(CORE_SRC)/u3_save.c $(CORE_INCLUDE)/u3_save.h ../Resources/English.lproj/MainResources.rsrc | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) save_fixture_tests.c $(CORE_SRC)/u3_resource.c $(CORE_SRC)/u3_save.c -o $@

$(BUILD_DIR)/render_contract_tests: render_contract_tests.c $(CORE_SRC)/u3_render.c $(CORE_INCLUDE)/u3_render.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) render_contract_tests.c $(CORE_SRC)/u3_render.c -o $@

$(BUILD_DIR)/input_adapter_tests: input_adapter_tests.c $(CORE_SRC)/u3_input.c $(CORE_INCLUDE)/u3_input.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) input_adapter_tests.c $(CORE_SRC)/u3_input.c -o $@

$(BUILD_DIR)/audio_adapter_tests: audio_adapter_tests.c $(CORE_SRC)/u3_audio.c $(CORE_INCLUDE)/u3_audio.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) audio_adapter_tests.c $(CORE_SRC)/u3_audio.c -o $@

$(BUILD_DIR)/tick_smoke_tests: tick_smoke_tests.c $(CORE_SRC)/u3_tick.c $(CORE_INCLUDE)/u3_tick.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) tick_smoke_tests.c $(CORE_SRC)/u3_tick.c -o $@

$(BUILD_DIR)/party_roster_tests: party_roster_tests.c $(CORE_SRC)/u3_party.c $(CORE_INCLUDE)/u3_party.h $(CORE_SRC)/u3_resource.c $(CORE_INCLUDE)/u3_resource.h $(CORE_SRC)/u3_save.c $(CORE_INCLUDE)/u3_save.h ../Resources/English.lproj/MainResources.rsrc | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) party_roster_tests.c $(CORE_SRC)/u3_party.c $(CORE_SRC)/u3_resource.c $(CORE_SRC)/u3_save.c -o $@

$(BUILD_DIR)/overworld_movement_tests: overworld_movement_tests.c $(CORE_SRC)/u3_overworld.c $(CORE_INCLUDE)/u3_overworld.h $(CORE_SRC)/u3_render.c $(CORE_INCLUDE)/u3_render.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) overworld_movement_tests.c $(CORE_SRC)/u3_overworld.c $(CORE_SRC)/u3_render.c -o $@

$(BUILD_DIR)/location_transition_tests: location_transition_tests.c $(CORE_SRC)/u3_location.c $(CORE_INCLUDE)/u3_location.h $(CORE_SRC)/u3_overworld.c $(CORE_INCLUDE)/u3_overworld.h $(CORE_SRC)/u3_render.c $(CORE_INCLUDE)/u3_render.h $(CORE_SRC)/u3_resource.c $(CORE_INCLUDE)/u3_resource.h ../Resources/English.lproj/MainResources.rsrc | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) location_transition_tests.c $(CORE_SRC)/u3_location.c $(CORE_SRC)/u3_overworld.c $(CORE_SRC)/u3_render.c $(CORE_SRC)/u3_resource.c -o $@

$(BUILD_DIR)/location_navigation_tests: location_navigation_tests.c $(CORE_SRC)/u3_location.c $(CORE_INCLUDE)/u3_location.h $(CORE_SRC)/u3_overworld.c $(CORE_INCLUDE)/u3_overworld.h $(CORE_SRC)/u3_render.c $(CORE_INCLUDE)/u3_render.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) location_navigation_tests.c $(CORE_SRC)/u3_location.c $(CORE_SRC)/u3_overworld.c $(CORE_SRC)/u3_render.c -o $@

$(BUILD_DIR)/location_talk_tests: location_talk_tests.c $(CORE_SRC)/u3_location_talk.c $(CORE_INCLUDE)/u3_location_talk.h $(CORE_SRC)/u3_resource.c $(CORE_INCLUDE)/u3_resource.h ../Resources/English.lproj/MainResources.rsrc | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) location_talk_tests.c $(CORE_SRC)/u3_location_talk.c $(CORE_SRC)/u3_resource.c -o $@

$(BUILD_DIR)/character_creation_tests: character_creation_tests.c $(CORE_SRC)/u3_character.c $(CORE_INCLUDE)/u3_character.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) character_creation_tests.c $(CORE_SRC)/u3_character.c -o $@

clean:
	rm -rf $(BUILD_DIR)
