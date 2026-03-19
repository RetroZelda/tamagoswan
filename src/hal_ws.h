#ifndef __HAL_WS__
#define __HAL_WS__

#include <ws/memory.h>
#include <ws/display.h>

/*
 * Run the main HAL driver for tamalib
 */

#define SPRITE_ROWS_PER_ICON 3
#define SPRITE_COLS_PER_ICON 3

typedef struct 
{
    ws_sprite_t ws_iram* sprites[SPRITE_ROWS_PER_ICON][SPRITE_COLS_PER_ICON];
    uint8_t x;
    uint8_t y;
    uint8_t visible;
} hal_ws_icon;

void hal_ws_initize();
void hal_ws_loop();


#endif // __HAL_WS__