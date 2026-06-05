#include "u3_render.h"

#include <stddef.h>
#include <string.h>

static u3_render_color u3_render_color_make(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    u3_render_color color;

    color.red = red;
    color.green = green;
    color.blue = blue;
    color.alpha = alpha;
    return color;
}

void u3_render_frame_init(u3_render_frame *frame)
{
    if (frame == NULL)
        return;

    memset(frame, 0, sizeof(*frame));
    frame->logical_width = U3_RENDER_LOGICAL_WIDTH;
    frame->logical_height = U3_RENDER_LOGICAL_HEIGHT;
}

uint8_t u3_render_frame_add_command(u3_render_frame *frame, const u3_render_command *command)
{
    if (frame == NULL || command == NULL)
        return 0;

    if (frame->command_count >= U3_RENDER_MAX_COMMANDS) {
        frame->overflowed = 1;
        return 0;
    }

    frame->commands[frame->command_count] = *command;
    frame->command_count++;
    return 1;
}

uint8_t u3_render_frame_add_clear(u3_render_frame *frame, u3_render_color color)
{
    u3_render_command command;

    memset(&command, 0, sizeof(command));
    command.kind = U3_RENDER_COMMAND_CLEAR;
    command.width = U3_RENDER_LOGICAL_WIDTH;
    command.height = U3_RENDER_LOGICAL_HEIGHT;
    command.fill = color;
    return u3_render_frame_add_command(frame, &command);
}

uint8_t u3_render_frame_add_rect(u3_render_frame *frame,
                                 int16_t x,
                                 int16_t y,
                                 int16_t width,
                                 int16_t height,
                                 u3_render_color fill,
                                 u3_render_color stroke)
{
    u3_render_command command;

    memset(&command, 0, sizeof(command));
    command.kind = U3_RENDER_COMMAND_RECT;
    command.x = x;
    command.y = y;
    command.width = width;
    command.height = height;
    command.fill = fill;
    command.stroke = stroke;
    return u3_render_frame_add_command(frame, &command);
}

uint8_t u3_render_frame_add_tile(u3_render_frame *frame,
                                 uint8_t tile,
                                 int16_t x,
                                 int16_t y)
{
    u3_render_command command;

    memset(&command, 0, sizeof(command));
    command.kind = U3_RENDER_COMMAND_TILE;
    command.x = x;
    command.y = y;
    command.width = 1;
    command.height = 1;
    command.value = tile;
    command.fill = u3_render_color_make((uint8_t)(32 + ((tile * 23) % 176)),
                                        (uint8_t)(48 + ((tile * 47) % 160)),
                                        (uint8_t)(40 + ((tile * 71) % 168)),
                                        255);
    command.stroke = u3_render_color_make(10, 12, 14, 255);
    return u3_render_frame_add_command(frame, &command);
}

uint8_t u3_render_frame_add_tile_grid(u3_render_frame *frame,
                                      const uint8_t *tiles,
                                      uint16_t tile_count,
                                      int16_t origin_x,
                                      int16_t origin_y)
{
    uint16_t index;
    uint16_t count = tile_count;

    if (frame == NULL || tiles == NULL)
        return 0;

    if (count > U3_RENDER_TILE_COUNT)
        count = U3_RENDER_TILE_COUNT;

    for (index = 0; index < count; index++) {
        int16_t x = (int16_t)(origin_x + (index % U3_RENDER_TILE_GRID_WIDTH));
        int16_t y = (int16_t)(origin_y + (index / U3_RENDER_TILE_GRID_WIDTH));

        if (!u3_render_frame_add_tile(frame, tiles[index], x, y))
            return 0;
    }

    return tile_count <= U3_RENDER_TILE_COUNT;
}

u3_render_frame u3_render_make_synthetic_tile_frame(void)
{
    u3_render_frame frame;
    uint8_t tiles[U3_RENDER_TILE_COUNT];
    uint16_t index;

    u3_render_frame_init(&frame);
    u3_render_frame_add_clear(&frame, u3_render_color_make(8, 10, 12, 255));
    u3_render_frame_add_rect(&frame,
                             0,
                             0,
                             U3_RENDER_LOGICAL_WIDTH,
                             U3_RENDER_LOGICAL_HEIGHT,
                             u3_render_color_make(0, 0, 0, 0),
                             u3_render_color_make(42, 184, 154, 255));

    for (index = 0; index < U3_RENDER_TILE_COUNT; index++)
        tiles[index] = (uint8_t)((index * 5 + (index / U3_RENDER_TILE_GRID_WIDTH) * 9) & 0x7f);

    u3_render_frame_add_tile_grid(&frame, tiles, U3_RENDER_TILE_COUNT, 1, 1);
    return frame;
}
