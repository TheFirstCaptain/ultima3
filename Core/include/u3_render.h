#ifndef U3_RENDER_H
#define U3_RENDER_H

#include <stdint.h>

#define U3_RENDER_LOGICAL_WIDTH 40
#define U3_RENDER_LOGICAL_HEIGHT 24
#define U3_RENDER_TILE_GRID_WIDTH 11
#define U3_RENDER_TILE_GRID_HEIGHT 11
#define U3_RENDER_TILE_COUNT (U3_RENDER_TILE_GRID_WIDTH * U3_RENDER_TILE_GRID_HEIGHT)
#define U3_RENDER_MAX_COMMANDS 160

#define U3_RENDER_COMMAND_CLEAR 1
#define U3_RENDER_COMMAND_RECT 2
#define U3_RENDER_COMMAND_TILE 3

typedef struct u3_render_color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
} u3_render_color;

typedef struct u3_render_command {
    uint8_t kind;
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
    uint16_t value;
    u3_render_color fill;
    u3_render_color stroke;
} u3_render_command;

typedef struct u3_render_frame {
    uint16_t logical_width;
    uint16_t logical_height;
    uint16_t command_count;
    uint8_t overflowed;
    u3_render_command commands[U3_RENDER_MAX_COMMANDS];
} u3_render_frame;

void u3_render_frame_init(u3_render_frame *frame);
uint8_t u3_render_frame_add_command(u3_render_frame *frame, const u3_render_command *command);
uint8_t u3_render_frame_add_clear(u3_render_frame *frame, u3_render_color color);
uint8_t u3_render_frame_add_rect(u3_render_frame *frame,
                                 int16_t x,
                                 int16_t y,
                                 int16_t width,
                                 int16_t height,
                                 u3_render_color fill,
                                 u3_render_color stroke);
uint8_t u3_render_frame_add_tile(u3_render_frame *frame,
                                 uint8_t tile,
                                 int16_t x,
                                 int16_t y);
uint8_t u3_render_frame_add_tile_grid(u3_render_frame *frame,
                                      const uint8_t *tiles,
                                      uint16_t tile_count,
                                      int16_t origin_x,
                                      int16_t origin_y);
u3_render_frame u3_render_make_synthetic_tile_frame(void);

#endif
