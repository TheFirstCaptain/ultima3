#ifndef U3_TICK_H
#define U3_TICK_H

#include <stdint.h>

#define U3_TICK_PHASE_COUNT 4

typedef struct u3_tick_state {
    uint32_t tick_count;
    uint8_t phase;
    uint32_t consumed_input_count;
    uint32_t dispatched_audio_count;
} u3_tick_state;

typedef struct u3_tick_input {
    uint8_t consumed_input;
    uint8_t dispatched_audio;
} u3_tick_input;

typedef struct u3_tick_result {
    uint32_t tick_count;
    uint8_t phase;
    uint8_t consumed_input;
    uint8_t dispatched_audio;
    uint8_t render_refresh_requested;
} u3_tick_result;

void u3_tick_state_init(u3_tick_state *state);
uint8_t u3_tick_advance(u3_tick_state *state, const u3_tick_input *input, u3_tick_result *result);

#endif
