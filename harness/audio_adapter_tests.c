#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "u3_audio.h"

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

static u3_audio_event pop_required(u3_audio_queue *queue)
{
    u3_audio_event event;

    ASSERT_TRUE(u3_audio_queue_pop(queue, &event));
    return event;
}

static void test_initializes_empty_queue(void)
{
    u3_audio_queue queue;

    u3_audio_queue_init(&queue);

    ASSERT_EQ_INT(0, queue.length);
    ASSERT_FALSE(queue.overflowed);
}

static void test_sound_music_and_volume_events_share_queue(void)
{
    u3_audio_queue queue;
    u3_audio_event event;

    u3_audio_queue_init(&queue);

    ASSERT_TRUE(u3_audio_queue_push_sound(&queue, U3_AUDIO_SOUND_ERROR1, 1));
    ASSERT_TRUE(u3_audio_queue_push_sound(&queue, U3_AUDIO_SOUND_BUMP, 1));
    ASSERT_TRUE(u3_audio_queue_push_sound(&queue, U3_AUDIO_SOUND_TORCH_IGNITE, 1));
    ASSERT_TRUE(u3_audio_queue_push_music(&queue, U3_AUDIO_MUSIC_SONG_1));
    ASSERT_TRUE(u3_audio_queue_push_sound_volume(&queue, 75));
    ASSERT_TRUE(u3_audio_queue_push_stop_music(&queue));

    event = pop_required(&queue);
    ASSERT_EQ_INT(U3_AUDIO_EVENT_PLAY_SOUND, event.kind);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_ERROR1, event.resource_id);
    ASSERT_EQ_INT(1, event.async);

    event = pop_required(&queue);
    ASSERT_EQ_INT(U3_AUDIO_EVENT_PLAY_SOUND, event.kind);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_BUMP, event.resource_id);
    ASSERT_EQ_INT(1, event.async);

    event = pop_required(&queue);
    ASSERT_EQ_INT(U3_AUDIO_EVENT_PLAY_SOUND, event.kind);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_TORCH_IGNITE, event.resource_id);
    ASSERT_EQ_INT(1, event.async);

    event = pop_required(&queue);
    ASSERT_EQ_INT(U3_AUDIO_EVENT_PLAY_MUSIC, event.kind);
    ASSERT_EQ_INT(U3_AUDIO_MUSIC_SONG_1, event.resource_id);

    event = pop_required(&queue);
    ASSERT_EQ_INT(U3_AUDIO_EVENT_SET_SOUND_VOLUME, event.kind);
    ASSERT_EQ_INT(75, event.volume_percent);

    event = pop_required(&queue);
    ASSERT_EQ_INT(U3_AUDIO_EVENT_STOP_MUSIC, event.kind);

    ASSERT_FALSE(u3_audio_queue_pop(&queue, &event));
}

static void test_volume_is_clamped(void)
{
    u3_audio_queue queue;
    u3_audio_event event;

    u3_audio_queue_init(&queue);

    ASSERT_TRUE(u3_audio_queue_push_music_volume(&queue, 255));
    event = pop_required(&queue);
    ASSERT_EQ_INT(U3_AUDIO_EVENT_SET_MUSIC_VOLUME, event.kind);
    ASSERT_EQ_INT(100, event.volume_percent);
}

static void test_reports_overflow_without_reordering_events(void)
{
    u3_audio_queue queue;
    u3_audio_event event;
    int index;

    u3_audio_queue_init(&queue);

    for (index = 0; index < U3_AUDIO_QUEUE_CAPACITY; index++)
        ASSERT_TRUE(u3_audio_queue_push_sound(&queue, U3_AUDIO_SOUND_STEP, 1));

    ASSERT_FALSE(u3_audio_queue_push_sound(&queue, U3_AUDIO_SOUND_ERROR1, 1));
    ASSERT_TRUE(queue.overflowed);
    ASSERT_EQ_INT(U3_AUDIO_QUEUE_CAPACITY, queue.length);

    event = pop_required(&queue);
    ASSERT_EQ_INT(U3_AUDIO_EVENT_PLAY_SOUND, event.kind);
    ASSERT_EQ_INT(U3_AUDIO_SOUND_STEP, event.resource_id);
}

int main(void)
{
    test_initializes_empty_queue();
    test_sound_music_and_volume_events_share_queue();
    test_volume_is_clamped();
    test_reports_overflow_without_reordering_events();

    printf("audio adapter characterization passed\n");
    return 0;
}
