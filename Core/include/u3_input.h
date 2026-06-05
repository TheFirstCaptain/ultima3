#ifndef U3_INPUT_H
#define U3_INPUT_H

#include <stdint.h>

#define U3_INPUT_QUEUE_CAPACITY 32

#define U3_INPUT_EVENT_NONE 0
#define U3_INPUT_EVENT_KEYBOARD 1
#define U3_INPUT_EVENT_MOUSE_DOWN 2
#define U3_INPUT_EVENT_MENU_COMMAND 3
#define U3_INPUT_EVENT_MACRO_KEY 4

#define U3_INPUT_MENU_NEW_GAME 1
#define U3_INPUT_MENU_SAVE 2

#define U3_INPUT_MOUSE_BUTTON_PRIMARY 1

typedef struct u3_input_event {
    uint8_t kind;
    uint16_t command;
    int16_t x;
    int16_t y;
    uint8_t button;
} u3_input_event;

typedef struct u3_input_queue {
    uint8_t length;
    uint8_t overflowed;
    u3_input_event events[U3_INPUT_QUEUE_CAPACITY];
} u3_input_queue;

void u3_input_queue_init(u3_input_queue *queue);
uint8_t u3_input_queue_push(u3_input_queue *queue, u3_input_event event);
uint8_t u3_input_queue_pop(u3_input_queue *queue, u3_input_event *event);
uint8_t u3_input_queue_push_keyboard(u3_input_queue *queue, uint8_t key);
uint8_t u3_input_queue_push_mouse_down(u3_input_queue *queue, int16_t x, int16_t y, uint8_t button);
uint8_t u3_input_queue_push_menu_command(u3_input_queue *queue, uint16_t command);
uint8_t u3_input_queue_push_macro_keys(u3_input_queue *queue, const char *commands, uint8_t length);

#endif
