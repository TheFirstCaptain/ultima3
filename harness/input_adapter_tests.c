#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "u3_input.h"

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

static u3_input_event pop_required(u3_input_queue *queue)
{
    u3_input_event event;

    ASSERT_TRUE(u3_input_queue_pop(queue, &event));
    return event;
}

static void test_initializes_empty_queue(void)
{
    u3_input_queue queue;

    u3_input_queue_init(&queue);

    ASSERT_EQ_INT(0, queue.length);
    ASSERT_FALSE(queue.overflowed);
}

static void test_keyboard_mouse_and_menu_events_share_queue(void)
{
    u3_input_queue queue;
    u3_input_event event;

    u3_input_queue_init(&queue);

    ASSERT_TRUE(u3_input_queue_push_keyboard(&queue, 'A'));
    ASSERT_TRUE(u3_input_queue_push_mouse_down(&queue, 12, 23, U3_INPUT_MOUSE_BUTTON_PRIMARY));
    ASSERT_TRUE(u3_input_queue_push_menu_command(&queue, U3_INPUT_MENU_SAVE));
    ASSERT_EQ_INT(3, queue.length);

    event = pop_required(&queue);
    ASSERT_EQ_INT(U3_INPUT_EVENT_KEYBOARD, event.kind);
    ASSERT_EQ_INT('A', event.command);

    event = pop_required(&queue);
    ASSERT_EQ_INT(U3_INPUT_EVENT_MOUSE_DOWN, event.kind);
    ASSERT_EQ_INT(12, event.x);
    ASSERT_EQ_INT(23, event.y);
    ASSERT_EQ_INT(U3_INPUT_MOUSE_BUTTON_PRIMARY, event.button);

    event = pop_required(&queue);
    ASSERT_EQ_INT(U3_INPUT_EVENT_MENU_COMMAND, event.kind);
    ASSERT_EQ_INT(U3_INPUT_MENU_SAVE, event.command);

    ASSERT_FALSE(u3_input_queue_pop(&queue, &event));
}

static void test_macro_events_preserve_execution_order(void)
{
    u3_input_queue queue;
    u3_input_event event;
    const char commands[] = {'A', 'C', '4'};

    u3_input_queue_init(&queue);

    ASSERT_TRUE(u3_input_queue_push_macro_keys(&queue, commands, 3));
    ASSERT_EQ_INT(3, queue.length);

    event = pop_required(&queue);
    ASSERT_EQ_INT(U3_INPUT_EVENT_MACRO_KEY, event.kind);
    ASSERT_EQ_INT('A', event.command);

    event = pop_required(&queue);
    ASSERT_EQ_INT(U3_INPUT_EVENT_MACRO_KEY, event.kind);
    ASSERT_EQ_INT('C', event.command);

    event = pop_required(&queue);
    ASSERT_EQ_INT(U3_INPUT_EVENT_MACRO_KEY, event.kind);
    ASSERT_EQ_INT('4', event.command);
}

static void test_reports_overflow_without_reordering_events(void)
{
    u3_input_queue queue;
    u3_input_event event;
    int index;

    u3_input_queue_init(&queue);

    for (index = 0; index < U3_INPUT_QUEUE_CAPACITY; index++)
        ASSERT_TRUE(u3_input_queue_push_keyboard(&queue, (uint8_t)('A' + (index % 26))));

    ASSERT_FALSE(u3_input_queue_push_keyboard(&queue, 'Z'));
    ASSERT_TRUE(queue.overflowed);
    ASSERT_EQ_INT(U3_INPUT_QUEUE_CAPACITY, queue.length);

    event = pop_required(&queue);
    ASSERT_EQ_INT('A', event.command);
}

static void test_macro_overflow_does_not_partially_enqueue(void)
{
    u3_input_queue queue;
    u3_input_event event;
    const char commands[] = {'A', 'C', '4'};
    int index;

    u3_input_queue_init(&queue);

    for (index = 0; index < U3_INPUT_QUEUE_CAPACITY - 1; index++)
        ASSERT_TRUE(u3_input_queue_push_keyboard(&queue, 'K'));

    ASSERT_FALSE(u3_input_queue_push_macro_keys(&queue, commands, 3));
    ASSERT_TRUE(queue.overflowed);
    ASSERT_EQ_INT(U3_INPUT_QUEUE_CAPACITY - 1, queue.length);

    for (index = 0; index < U3_INPUT_QUEUE_CAPACITY - 1; index++) {
        event = pop_required(&queue);
        ASSERT_EQ_INT(U3_INPUT_EVENT_KEYBOARD, event.kind);
        ASSERT_EQ_INT('K', event.command);
    }

    ASSERT_FALSE(u3_input_queue_pop(&queue, &event));
}

int main(void)
{
    test_initializes_empty_queue();
    test_keyboard_mouse_and_menu_events_share_queue();
    test_macro_events_preserve_execution_order();
    test_reports_overflow_without_reordering_events();
    test_macro_overflow_does_not_partially_enqueue();

    printf("input adapter characterization passed\n");
    return 0;
}
