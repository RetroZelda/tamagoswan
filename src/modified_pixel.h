#ifndef __MODIFIED_PIXELS_H__
#define __MODIFIED_PIXELS_H__

#include <stdbool.h>
#include <stdint.h>

#define PIXEL_OFF 0x00
#define PIXEL_ON  0xFF

typedef struct
{
    uint8_t x;
    uint8_t y;
    uint8_t pixel;
    uint8_t __padding;
} ts_modified_pixel;

bool ts_pixel_push(uint8_t x, uint8_t y, uint8_t state);
const ts_modified_pixel* ts_pixel_pop();

#endif // __MODIFIED_PIXELS_H__