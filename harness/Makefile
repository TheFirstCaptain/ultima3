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
	$(BUILD_DIR)/autocombat_targeting_tests \
	$(BUILD_DIR)/dungeon_navigation_tests \
	$(BUILD_DIR)/resource_fixture_tests

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

$(BUILD_DIR)/autocombat_targeting_tests: autocombat_targeting_tests.c $(CORE_SRC)/u3_autocombat.c $(CORE_INCLUDE)/u3_autocombat.h $(CORE_SRC)/u3_combat.c $(CORE_INCLUDE)/u3_combat.h $(CORE_SRC)/u3_map_math.c $(CORE_INCLUDE)/u3_map_math.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) autocombat_targeting_tests.c $(CORE_SRC)/u3_autocombat.c $(CORE_SRC)/u3_combat.c $(CORE_SRC)/u3_map_math.c -o $@

$(BUILD_DIR)/dungeon_navigation_tests: dungeon_navigation_tests.c $(CORE_SRC)/u3_dungeon.c $(CORE_INCLUDE)/u3_dungeon.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) dungeon_navigation_tests.c $(CORE_SRC)/u3_dungeon.c -o $@

$(BUILD_DIR)/resource_fixture_tests: resource_fixture_tests.c $(CORE_SRC)/u3_resource.c $(CORE_INCLUDE)/u3_resource.h ../Resources/English.lproj/MainResources.rsrc | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(CORE_INCLUDE) resource_fixture_tests.c $(CORE_SRC)/u3_resource.c -o $@

clean:
	rm -rf $(BUILD_DIR)
