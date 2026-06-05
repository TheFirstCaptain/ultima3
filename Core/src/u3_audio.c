#include "u3_audio.h"

#include <stddef.h>
#include <string.h>

static uint8_t u3_audio_clamp_volume(uint8_t volume_percent)
{
    return volume_percent > 100 ? 100 : volume_percent;
}

void u3_audio_queue_init(u3_audio_queue *queue)
{
    if (queue == NULL)
        return;

    memset(queue, 0, sizeof(*queue));
}

uint8_t u3_audio_queue_push(u3_audio_queue *queue, u3_audio_event event)
{
    if (queue == NULL)
        return 0;

    if (queue->length >= U3_AUDIO_QUEUE_CAPACITY) {
        queue->overflowed = 1;
        return 0;
    }

    queue->events[queue->length] = event;
    queue->length++;
    return 1;
}

uint8_t u3_audio_queue_pop(u3_audio_queue *queue, u3_audio_event *event)
{
    uint8_t index;

    if (queue == NULL || event == NULL)
        return 0;

    if (queue->length == 0) {
        memset(event, 0, sizeof(*event));
        return 0;
    }

    *event = queue->events[0];
    for (index = 1; index < queue->length; index++)
        queue->events[index - 1] = queue->events[index];

    queue->length--;
    memset(&queue->events[queue->length], 0, sizeof(queue->events[queue->length]));
    return 1;
}

uint8_t u3_audio_queue_push_sound(u3_audio_queue *queue, uint16_t sound_id, uint8_t async)
{
    u3_audio_event event;

    memset(&event, 0, sizeof(event));
    event.kind = U3_AUDIO_EVENT_PLAY_SOUND;
    event.resource_id = sound_id;
    event.async = async ? 1 : 0;
    return u3_audio_queue_push(queue, event);
}

uint8_t u3_audio_queue_push_music(u3_audio_queue *queue, uint16_t music_id)
{
    u3_audio_event event;

    memset(&event, 0, sizeof(event));
    event.kind = U3_AUDIO_EVENT_PLAY_MUSIC;
    event.resource_id = music_id;
    return u3_audio_queue_push(queue, event);
}

uint8_t u3_audio_queue_push_stop_music(u3_audio_queue *queue)
{
    u3_audio_event event;

    memset(&event, 0, sizeof(event));
    event.kind = U3_AUDIO_EVENT_STOP_MUSIC;
    return u3_audio_queue_push(queue, event);
}

uint8_t u3_audio_queue_push_sound_volume(u3_audio_queue *queue, uint8_t volume_percent)
{
    u3_audio_event event;

    memset(&event, 0, sizeof(event));
    event.kind = U3_AUDIO_EVENT_SET_SOUND_VOLUME;
    event.volume_percent = u3_audio_clamp_volume(volume_percent);
    return u3_audio_queue_push(queue, event);
}

uint8_t u3_audio_queue_push_music_volume(u3_audio_queue *queue, uint8_t volume_percent)
{
    u3_audio_event event;

    memset(&event, 0, sizeof(event));
    event.kind = U3_AUDIO_EVENT_SET_MUSIC_VOLUME;
    event.volume_percent = u3_audio_clamp_volume(volume_percent);
    return u3_audio_queue_push(queue, event);
}
