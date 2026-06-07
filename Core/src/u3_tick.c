#include "u3_tick.h"

#include <string.h>

void u3_tick_state_init(u3_tick_state *state)
{
    if (state == 0)
        return;

    memset(state, 0, sizeof(*state));
}

uint8_t u3_tick_advance(u3_tick_state *state, const u3_tick_input *input, u3_tick_result *result)
{
    u3_tick_input empty_input;
    u3_tick_result tick_result;

    if (state == 0 || result == 0)
        return 0;

    memset(&empty_input, 0, sizeof(empty_input));
    if (input == 0)
        input = &empty_input;

    state->tick_count++;
    state->phase = (uint8_t)((state->phase + 1) % U3_TICK_PHASE_COUNT);
    if (input->consumed_input)
        state->consumed_input_count++;
    if (input->dispatched_audio)
        state->dispatched_audio_count++;

    memset(&tick_result, 0, sizeof(tick_result));
    tick_result.tick_count = state->tick_count;
    tick_result.phase = state->phase;
    tick_result.consumed_input = input->consumed_input ? 1 : 0;
    tick_result.dispatched_audio = input->dispatched_audio ? 1 : 0;
    tick_result.render_refresh_requested = 1;
    *result = tick_result;
    return 1;
}
