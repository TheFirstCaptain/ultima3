CC ?= clang
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -pedantic
BUILD_DIR ?= build
TESTS = \
	$(BUILD_DIR)/map_math_accessors_tests \
	$(BUILD_DIR)/pascal_string_helpers_tests \
	$(BUILD_DIR)/combat_predicates_tests

.PHONY: test clean

test: $(TESTS)
	@for test in $(TESTS); do ./$$test; done

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/map_math_accessors_tests: map_math_accessors_tests.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/pascal_string_helpers_tests: pascal_string_helpers_tests.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/combat_predicates_tests: combat_predicates_tests.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)
