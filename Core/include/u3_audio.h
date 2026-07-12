#ifndef U3_AUDIO_H
#define U3_AUDIO_H

#include <stdint.h>

#define U3_AUDIO_QUEUE_CAPACITY 16

#define U3_AUDIO_EVENT_NONE 0
#define U3_AUDIO_EVENT_PLAY_SOUND 1
#define U3_AUDIO_EVENT_PLAY_MUSIC 2
#define U3_AUDIO_EVENT_STOP_MUSIC 3
#define U3_AUDIO_EVENT_SET_SOUND_VOLUME 4
#define U3_AUDIO_EVENT_SET_MUSIC_VOLUME 5

#define U3_AUDIO_SOUND_ERROR1 1
#define U3_AUDIO_SOUND_STEP 2
#define U3_AUDIO_SOUND_BUMP 3
#define U3_AUDIO_SOUND_TORCH_IGNITE 4
#define U3_AUDIO_SOUND_HIT 5
#define U3_AUDIO_SOUND_OUCH 6

#define U3_AUDIO_MUSIC_SONG_1 1

typedef struct u3_audio_event {
    uint8_t kind;
    uint16_t resource_id;
    uint8_t volume_percent;
    uint8_t async;
} u3_audio_event;

typedef struct u3_audio_queue {
    uint8_t length;
    uint8_t overflowed;
    u3_audio_event events[U3_AUDIO_QUEUE_CAPACITY];
} u3_audio_queue;

void u3_audio_queue_init(u3_audio_queue *queue);
uint8_t u3_audio_queue_push(u3_audio_queue *queue, u3_audio_event event);
uint8_t u3_audio_queue_pop(u3_audio_queue *queue, u3_audio_event *event);
uint8_t u3_audio_queue_push_sound(u3_audio_queue *queue, uint16_t sound_id, uint8_t async);
uint8_t u3_audio_queue_push_music(u3_audio_queue *queue, uint16_t music_id);
uint8_t u3_audio_queue_push_stop_music(u3_audio_queue *queue);
uint8_t u3_audio_queue_push_sound_volume(u3_audio_queue *queue, uint8_t volume_percent);
uint8_t u3_audio_queue_push_music_volume(u3_audio_queue *queue, uint8_t volume_percent);

#endif
