#include "u3_input.h"

#include <stddef.h>
#include <string.h>

void u3_input_queue_init(u3_input_queue *queue)
{
    if (queue == NULL)
        return;

    memset(queue, 0, sizeof(*queue));
}

uint8_t u3_input_queue_push(u3_input_queue *queue, u3_input_event event)
{
    if (queue == NULL)
        return 0;

    if (queue->length >= U3_INPUT_QUEUE_CAPACITY) {
        queue->overflowed = 1;
        return 0;
    }

    queue->events[queue->length] = event;
    queue->length++;
    return 1;
}

uint8_t u3_input_queue_pop(u3_input_queue *queue, u3_input_event *event)
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

uint8_t u3_input_queue_push_keyboard(u3_input_queue *queue, uint8_t key)
{
    u3_input_event event;

    memset(&event, 0, sizeof(event));
    event.kind = U3_INPUT_EVENT_KEYBOARD;
    event.command = key;
    return u3_input_queue_push(queue, event);
}

uint8_t u3_input_queue_push_mouse_down(u3_input_queue *queue, int16_t x, int16_t y, uint8_t button)
{
    u3_input_event event;

    memset(&event, 0, sizeof(event));
    event.kind = U3_INPUT_EVENT_MOUSE_DOWN;
    event.command = U3_INPUT_EVENT_MOUSE_DOWN;
    event.x = x;
    event.y = y;
    event.button = button;
    return u3_input_queue_push(queue, event);
}

uint8_t u3_input_queue_push_menu_command(u3_input_queue *queue, uint16_t command)
{
    u3_input_event event;

    memset(&event, 0, sizeof(event));
    event.kind = U3_INPUT_EVENT_MENU_COMMAND;
    event.command = command;
    return u3_input_queue_push(queue, event);
}

uint8_t u3_input_queue_push_macro_keys(u3_input_queue *queue, const char *commands, uint8_t length)
{
    uint8_t index;

    if (queue == NULL || commands == NULL)
        return 0;

    if (length > U3_INPUT_QUEUE_CAPACITY - queue->length) {
        queue->overflowed = 1;
        return 0;
    }

    for (index = 0; index < length; index++) {
        u3_input_event event;

        memset(&event, 0, sizeof(event));
        event.kind = U3_INPUT_EVENT_MACRO_KEY;
        event.command = (uint8_t)commands[index];
        if (!u3_input_queue_push(queue, event))
            return 0;
    }

    return 1;
}
