#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "u3_tick.h"

#define ASSERT_EQ_INT(expected, actual) assert_eq_int((expected), (actual), __FILE__, __LINE__)
#define ASSERT_TRUE(actual) assert_true((actual), __FILE__, __LINE__)
#define ASSERT_FALSE(actual) assert_false((actual), __FILE__, __LINE__)

static void assert_eq_int(int expected, int actual, const char *file, int line)
{
    if (expected != actual) {
        fprintf(stderr, "%s:%d: expected %d, got %d\n", file, line, expected, actual);
        exit(1);
    }
}

static void assert_true(int actual, const char *file, int line)
{
    if (!actual) {
        fprintf(stderr, "%s:%d: expected true\n", file, line);
        exit(1);
    }
}

static void assert_false(int actual, const char *file, int line)
{
    if (actual) {
        fprintf(stderr, "%s:%d: expected false\n", file, line);
        exit(1);
    }
}

static void test_initializes_empty_tick_state(void)
{
    u3_tick_state state;

    u3_tick_state_init(&state);

    ASSERT_EQ_INT(0, (int)state.tick_count);
    ASSERT_EQ_INT(0, state.phase);
    ASSERT_EQ_INT(0, (int)state.consumed_input_count);
    ASSERT_EQ_INT(0, (int)state.dispatched_audio_count);
}

static void test_advances_tick_phase_and_requests_render_refresh(void)
{
    u3_tick_state state;
    u3_tick_result result;

    u3_tick_state_init(&state);

    ASSERT_TRUE(u3_tick_advance(&state, 0, &result));
    ASSERT_EQ_INT(1, (int)state.tick_count);
    ASSERT_EQ_INT(1, state.phase);
    ASSERT_EQ_INT(1, (int)result.tick_count);
    ASSERT_EQ_INT(1, result.phase);
    ASSERT_FALSE(result.consumed_input);
    ASSERT_FALSE(result.dispatched_audio);
    ASSERT_TRUE(result.render_refresh_requested);

    ASSERT_TRUE(u3_tick_advance(&state, 0, &result));
    ASSERT_TRUE(u3_tick_advance(&state, 0, &result));
    ASSERT_TRUE(u3_tick_advance(&state, 0, &result));
    ASSERT_EQ_INT(4, (int)state.tick_count);
    ASSERT_EQ_INT(0, state.phase);
}

static void test_tracks_consumed_input_and_dispatched_audio(void)
{
    u3_tick_state state;
    u3_tick_input input;
    u3_tick_result result;

    u3_tick_state_init(&state);
    input.consumed_input = 1;
    input.dispatched_audio = 1;

    ASSERT_TRUE(u3_tick_advance(&state, &input, &result));
    ASSERT_EQ_INT(1, (int)state.consumed_input_count);
    ASSERT_EQ_INT(1, (int)state.dispatched_audio_count);
    ASSERT_TRUE(result.consumed_input);
    ASSERT_TRUE(result.dispatched_audio);
}

static void test_rejects_missing_state_or_result(void)
{
    u3_tick_state state;
    u3_tick_result result;

    u3_tick_state_init(&state);
    ASSERT_FALSE(u3_tick_advance(0, 0, &result));
    ASSERT_FALSE(u3_tick_advance(&state, 0, 0));
}

int main(void)
{
    test_initializes_empty_tick_state();
    test_advances_tick_phase_and_requests_render_refresh();
    test_tracks_consumed_input_and_dispatched_audio();
    test_rejects_missing_state_or_result();

    printf("tick smoke characterization passed\n");
    return 0;
}
