#include "modified_pixel.h"

#include <tamalib/hw.h>
#include <wonderful.h>
#include <ws.h>

#define TOTAL_PIXELS LCD_WIDTH*LCD_HEIGHT
#define BUFFER_CAPACITY TOTAL_PIXELS

ts_modified_pixel ws_iram modified_pixels[BUFFER_CAPACITY];
static uint16_t ws_iram modified_pixels_head = 0;
static uint16_t ws_iram modified_pixels_tail = 0;

inline static uint16_t get_next_index(uint16_t in)
{
    if(in == TOTAL_PIXELS)
    {
        return 0;
    }
    return in + 1;
}

bool ts_pixel_push(uint8_t x, uint8_t y, uint8_t pixel)
{
    uint16_t next_tail = get_next_index(modified_pixels_tail);
    if(next_tail == modified_pixels_head)
    {
        // we are at capacity
        return false; 
    }

    ts_modified_pixel* tail_pixel = modified_pixels + modified_pixels_tail;
    tail_pixel->x = x;
    tail_pixel->y = y;
    tail_pixel->pixel = pixel;
    modified_pixels_tail = next_tail;
    return true;
}

const ts_modified_pixel* ts_pixel_pop()
{
    if(modified_pixels_head == modified_pixels_tail)
    {
        // we are empty
        return NULL;
    }

    ts_modified_pixel* head_pixel = modified_pixels + modified_pixels_head;
    modified_pixels_head = get_next_index(modified_pixels_head);
    return head_pixel;
}