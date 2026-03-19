
#include "hal_ws.h"

#include <tamalib/tamalib.h>
#include <tamalib/hal.h>

#include <wse/memory.h>
#include <wsx/console.h>
#include <ws/util.h>

#include <stdcountof.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "save_data.h"
#include "rom/rom.h"
#include "utility.h"

// #define USE_UART
#ifdef USE_UART
#include <ws/uart.h>
#endif

// functions that map into tamalib
static void         hal_ws_halt(void);
static bool_t       hal_ws_is_log_enabled(log_level_t level);
static void         hal_ws_log(log_level_t level, const char __wf_rom* buff, ...);
static void         hal_ws_sleep_until(timestamp_t ts);
static timestamp_t  hal_ws_get_timestamp(void);
static void         hal_ws_update_screen(void);
static void         hal_ws_set_lcd_matrix(u8_t x, u8_t y, bool_t val);
static void         hal_ws_set_lcd_icon(u8_t icon, bool_t val);
static void         hal_ws_set_frequency(u32_t freq);
static void         hal_ws_play_frequency(bool_t en);
static int          hal_ws_handler(void);

static hal_t ws_hal = 
{
    .malloc             = NULL, // Unneeded because we removed breakpoints 
    .free               = NULL, // Unneeded because we removed breakpoints 
    .halt               = hal_ws_halt,
    .is_log_enabled     = hal_ws_is_log_enabled,
    .log                = hal_ws_log,
    .sleep_until        = hal_ws_sleep_until,
    .get_timestamp      = hal_ws_get_timestamp,
    .update_screen      = hal_ws_update_screen,
    .set_lcd_matrix     = hal_ws_set_lcd_matrix,
    .set_lcd_icon       = hal_ws_set_lcd_icon,
    .set_frequency      = hal_ws_set_frequency,
    .play_frequency     = hal_ws_play_frequency,
    .handler            = hal_ws_handler,
};

#define SPRITE_ROWS_PER_ICON 3
#define SPRITE_COLS_PER_ICON 3

typedef struct 
{
    ws_sprite_t ws_iram* sprites[SPRITE_ROWS_PER_ICON][SPRITE_COLS_PER_ICON];
    uint8_t x;
    uint8_t y;
    uint8_t visible;
} hal_ws_icon;


#define BITS_TO_BYTES(bits)   (((bits) + 7) / 8)
static uint8_t ws_iram pixels[BITS_TO_BYTES(LCD_WIDTH*LCD_HEIGHT)]; // TODO: Not holding these as bits could be a major speedup in rendering... however rendering isnt a problem anymore so whatevs
static hal_ws_icon ws_iram* icons[ICON_NUM];

static uint16_t last_keys = 0;
static uint16_t curr_keys = 0;

static uint16_t g_loop_ticks = 0;
volatile uint16_t g_vblank_ticks = 0;
static uint8_t ws_iram g_screen_needs_update = 0;
static uint16_t ws_iram g_save_frequency_counter = 0;

#define SCREEN_OFFSET_X 6
#define SCREEN_OFFSET_Y 5

#ifdef ENABLE_LOGS
#define SPRINTF_BUFFER_SIZE 128
char ws_iram sprintf_dst_buffer[SPRINTF_BUFFER_SIZE];
static uint8_t enable_logs = true;

// strings for logging the level 
DEFINE_STRING(log_level_memory,   " MEM");
DEFINE_STRING(log_level_pixel,    "PIXL");
DEFINE_STRING(log_level_error,    " ERR");
DEFINE_STRING(log_level_info,     "INFO");
DEFINE_STRING(log_level_cpu,      " CPU");
DEFINE_STRING(log_level_int,      " IRQ");
DEFINE_STRING(log_level_op,       "  OP");
DEFINE_STRING(log_format,         "%s: ");

// strings for our HAL logs
DEFINE_STRING(log_game_save_success, "save success\n");
DEFINE_STRING(log_game_save_failed, "save failed\n");
DEFINE_STRING(log_game_load_success, "load success\n");
DEFINE_STRING(log_game_load_failed, "load failed\n");
DEFINE_STRING(log_pixel_write, "Pixel (%02d, %02d) %d\n");
DEFINE_STRING(log_icon_write, "Icon %02d %s\n");
DEFINE_STRING(log_halted, "Halted for %d\n");
DEFINE_STRING(log_generic_on, "on");
DEFINE_STRING(log_generic_off, "off");
#endif

#define TO_LCD_INDEX(x,y)  ((y) * LCD_WIDTH + (x))
#define TICK_RATE (WS_SYSTEM_CLOCK_HZ / CLOCKS_PER_SEC)
#define ICON_POSITION_X_OFFSET 12
#define ICON_POSITION_X_SPREAD 20
#define ICON_POSITION_Y_ON_TOP 3
#define ICON_POSITION_Y_ON_BOTTOM 115
#define ICON_POSITION_Y_OFF (8 * 18)

// helpers for dealing with pixels
#define IS_PIXEL_SET(arr, p)      ((arr)[(p)>>3] &   (1u << ((p)&7)))
#define SET_PIXEL_ON(arr, p)      ((arr)[(p)>>3] |=  (1u << ((p)&7)))
#define SET_PIXEL_OFF(arr, p)     ((arr)[(p)>>3] &= ~(1u << ((p)&7)))
#define TOGGLE_PIXEL(arr, p)      ((arr)[(p)>>3] ^=  (1u << ((p)&7)))
#define SET_PIXEL(arr, p, v)      ((arr)[(p)>>3] =   ((arr)[(p)>>3] & ~(1 << ((p)&7))) | ((!!(v)) << ((p)&7)))
#define GET_TILE_INDEX(a, b,  c, d) ((a) ? 1 : 0) | ((b) ? 2 : 0) | ((c) ? 4 : 0) | ((d) ? 8 : 0)
#define GET_SCREEN_TILE_FROM_PIXEL(x, y) ((x >> 1) + (x & 1) * LCD_WIDTH) + ((y >> 1) + (y & 1))

// buffers to optimize screen draw
static uint8_t ws_iram modified_tiles[(LCD_WIDTH * LCD_HEIGHT) >> 2];
static const uint16_t __wf_rom tile_lookup[16] =
{
    (WS_SCREEN_ATTR_TILE(0x0) & WS_SCREEN_ATTR_TILE_MASK) | (WS_SCREEN_ATTR_PALETTE(0x7) & WS_SCREEN_ATTR_PALETTE_MASK),
    (WS_SCREEN_ATTR_TILE(0x1) & WS_SCREEN_ATTR_TILE_MASK) | (WS_SCREEN_ATTR_PALETTE(0x7) & WS_SCREEN_ATTR_PALETTE_MASK),
    (WS_SCREEN_ATTR_TILE(0x2) & WS_SCREEN_ATTR_TILE_MASK) | (WS_SCREEN_ATTR_PALETTE(0x7) & WS_SCREEN_ATTR_PALETTE_MASK),
    (WS_SCREEN_ATTR_TILE(0x3) & WS_SCREEN_ATTR_TILE_MASK) | (WS_SCREEN_ATTR_PALETTE(0x7) & WS_SCREEN_ATTR_PALETTE_MASK),
    (WS_SCREEN_ATTR_TILE(0x4) & WS_SCREEN_ATTR_TILE_MASK) | (WS_SCREEN_ATTR_PALETTE(0x7) & WS_SCREEN_ATTR_PALETTE_MASK),
    (WS_SCREEN_ATTR_TILE(0x5) & WS_SCREEN_ATTR_TILE_MASK) | (WS_SCREEN_ATTR_PALETTE(0x7) & WS_SCREEN_ATTR_PALETTE_MASK),
    (WS_SCREEN_ATTR_TILE(0x6) & WS_SCREEN_ATTR_TILE_MASK) | (WS_SCREEN_ATTR_PALETTE(0x7) & WS_SCREEN_ATTR_PALETTE_MASK),
    (WS_SCREEN_ATTR_TILE(0x7) & WS_SCREEN_ATTR_TILE_MASK) | (WS_SCREEN_ATTR_PALETTE(0x7) & WS_SCREEN_ATTR_PALETTE_MASK),
    (WS_SCREEN_ATTR_TILE(0x8) & WS_SCREEN_ATTR_TILE_MASK) | (WS_SCREEN_ATTR_PALETTE(0x7) & WS_SCREEN_ATTR_PALETTE_MASK),
    (WS_SCREEN_ATTR_TILE(0x9) & WS_SCREEN_ATTR_TILE_MASK) | (WS_SCREEN_ATTR_PALETTE(0x7) & WS_SCREEN_ATTR_PALETTE_MASK),
    (WS_SCREEN_ATTR_TILE(0xA) & WS_SCREEN_ATTR_TILE_MASK) | (WS_SCREEN_ATTR_PALETTE(0x7) & WS_SCREEN_ATTR_PALETTE_MASK),
    (WS_SCREEN_ATTR_TILE(0xB) & WS_SCREEN_ATTR_TILE_MASK) | (WS_SCREEN_ATTR_PALETTE(0x7) & WS_SCREEN_ATTR_PALETTE_MASK),
    (WS_SCREEN_ATTR_TILE(0xC) & WS_SCREEN_ATTR_TILE_MASK) | (WS_SCREEN_ATTR_PALETTE(0x7) & WS_SCREEN_ATTR_PALETTE_MASK),
    (WS_SCREEN_ATTR_TILE(0xD) & WS_SCREEN_ATTR_TILE_MASK) | (WS_SCREEN_ATTR_PALETTE(0x7) & WS_SCREEN_ATTR_PALETTE_MASK),
    (WS_SCREEN_ATTR_TILE(0xE) & WS_SCREEN_ATTR_TILE_MASK) | (WS_SCREEN_ATTR_PALETTE(0x7) & WS_SCREEN_ATTR_PALETTE_MASK),
    (WS_SCREEN_ATTR_TILE(0xF) & WS_SCREEN_ATTR_TILE_MASK) | (WS_SCREEN_ATTR_PALETTE(0x7) & WS_SCREEN_ATTR_PALETTE_MASK)
}; 

void vblank_wait(void) 
{
    uint16_t vblank_ticks_last = g_vblank_ticks;
    while (g_vblank_ticks == vblank_ticks_last) 
    {
        ia16_halt();
    }
}

__attribute__((interrupt))
static void vblank_int_handler(void) 
{
    ++g_vblank_ticks;
    ws_int_ack(WS_INT_ACK_VBLANK);
}

void hal_ws_initize()
{
    uint8_t sprite_x, sprite_y;
    uint8_t counter;
    uint8_t icon_index;
    uint16_t tile_index;
    ws_sprite_t ws_iram* sprite;

    const u12_t __wf_rom* rom = (const u12_t __wf_rom*)tamagochi_rom_p1_swapped_data;

	tamalib_register_hal(&ws_hal);
    tamalib_init(rom, WS_SYSTEM_CLOCK_HZ >> 8);

    g_loop_ticks = 0;

	last_keys = 0;
    curr_keys = 0;

    // setup icons
    // icons are 3x3 sprites where sprite tiles are loaded by each row
    // so contiguous tiles are s0s0s0 s1s1s1 s2s2s2 ... n0n1n2
    outportw(WS_SPR_FIRST_PORT, 0);
    outportw(WS_SPR_COUNT_PORT, (uint8_t)(ICON_NUM * SPRITE_ROWS_PER_ICON * SPRITE_COLS_PER_ICON));
    ws_display_set_screen_addresses(&wse_screen1, &wse_screen2); // NOTE: setting WS_SPR_COUNT_PORT resets the screen addresses for some reason.

    // clear our pixel bit buffers
    memset(pixels, 0x0, BITS_TO_BYTES(LCD_WIDTH*LCD_HEIGHT));

    counter = 0;
    for(icon_index = 0; icon_index < ICON_NUM; ++icon_index)
    {
        icons[icon_index]->x = ICON_POSITION_X_OFFSET + ((icon_index % 4) * SPRITE_COLS_PER_ICON * ICON_POSITION_X_SPREAD);
        icons[icon_index]->y = icon_index < 4 ? ICON_POSITION_Y_ON_TOP : ICON_POSITION_Y_ON_BOTTOM;
        icons[icon_index]->visible = 0;
        for(sprite_x = 0; sprite_x < SPRITE_COLS_PER_ICON; ++sprite_x)
        {
            for(sprite_y = 0; sprite_y < SPRITE_ROWS_PER_ICON; ++sprite_y)
            {
                sprite = wse_sprites1.entry + counter++;
                tile_index = 16 + (sprite_y * ICON_NUM * SPRITE_COLS_PER_ICON) +
                      (icon_index * SPRITE_COLS_PER_ICON) +
                      sprite_x;

                sprite->x = icons[icon_index]->x + (sprite_x * 8);
                sprite->y = icons[icon_index]->visible != 0 ? icons[icon_index]->y + (sprite_y * 8) : ICON_POSITION_Y_OFF;
                sprite->attr = ( WS_SPRITE_ATTR_TILE(tile_index) & WS_SPRITE_ATTR_TILE_MASK) | 
                                (WS_SPRITE_ATTR_PALETTE(0x7) & WS_SPRITE_ATTR_PALETTE_MASK);

                icons[icon_index]->sprites[sprite_x][sprite_y] = sprite;
            }

        }
    }

    // register our interrupts
	ws_int_set_handler(WS_INT_VBLANK, vblank_int_handler);
    ws_int_set_default_handler_key();
    ws_int_set_default_handler_hblank_timer(); // for the hal_ws_sleep_until timer
	ws_int_enable(WS_INT_ENABLE_VBLANK | WS_INT_ENABLE_KEY_SCAN);
	ia16_enable_irq();

    if(ts_load(cpu_get_state()) == true)
    {
        PRINT_LOG(LOG_INFO, log_game_load_success);
    }
    else
    {
        PRINT_LOG(LOG_INFO, log_game_load_failed);
    }

    vblank_wait();

    #ifdef USE_UART
    ws_uart_open(WS_UART_BAUD_RATE_9600);
    #endif
}

void hal_ws_loop(void)
{
    uint16_t vblank_ticks_last = g_vblank_ticks; 
    while (1) 
	{
        if(vblank_ticks_last != g_vblank_ticks)
        {
            if(g_screen_needs_update > 0)
            {
                g_hal->update_screen();
                g_screen_needs_update = 0;
            }
            else if(++g_save_frequency_counter > 500) // triggers a save every 1000 vblanks if we arent updating the screen
            {
                if(ts_save(cpu_get_state()) == true)
                {
                    PRINT_LOG(LOG_INFO, log_game_save_success);
                }
                else
                {
                    PRINT_LOG(LOG_INFO, log_game_save_failed);
                }
                g_save_frequency_counter = 0;
            }
            g_hal->handler();
            vblank_ticks_last = g_vblank_ticks;
            ++g_loop_ticks;
        }
        tamalib_step();
    }
}

void hal_ws_halt(void)
{

}

bool_t hal_ws_is_log_enabled(log_level_t level)
{
#ifdef ENABLE_LOGS
    if(!enable_logs)
    {
        return false;
    }

    switch(level)
    {
	case LOG_ERROR: return true;
	case LOG_INFO:  return true;
    case LOG_MEMORY:return false;
	case LOG_CPU:   return false;
	case LOG_INT:   return false;
	case LOG_OP:    return false;
	case LOG_PIXEL: return false;
    }
#endif // ENABLE_LOGS
    return false;
}

void hal_ws_log(log_level_t level, const char __wf_rom* buff, ...)
{
#ifdef ENABLE_LOGS
    if(!hal_ws_is_log_enabled(level))
    {
        return;
    }
    
    uint16_t bytes_written = 0;
    switch(level)
    {
        case LOG_ERROR:
            bytes_written = mini_snprintf(sprintf_dst_buffer, SPRINTF_BUFFER_SIZE, log_format, log_level_error);
            break;
        case LOG_INFO:
            bytes_written = mini_snprintf(sprintf_dst_buffer, SPRINTF_BUFFER_SIZE, log_format, log_level_info);
            break;
        case LOG_MEMORY:
            bytes_written = mini_snprintf(sprintf_dst_buffer, SPRINTF_BUFFER_SIZE, log_format, log_level_memory);
            break;
        case LOG_CPU:
            bytes_written = mini_snprintf(sprintf_dst_buffer, SPRINTF_BUFFER_SIZE, log_format, log_level_cpu);
            break;
        case LOG_INT:
            bytes_written = mini_snprintf(sprintf_dst_buffer, SPRINTF_BUFFER_SIZE, log_format, log_level_int);
            break;
        case LOG_OP:
            bytes_written = mini_snprintf(sprintf_dst_buffer, SPRINTF_BUFFER_SIZE, log_format, log_level_op);
            break;
        case LOG_PIXEL:
            bytes_written = mini_snprintf(sprintf_dst_buffer, SPRINTF_BUFFER_SIZE, log_format, log_level_pixel);
            break;
    }
    
    if(bytes_written > 0)
    {
        va_list args;
        va_start(args, buff);
        // vsnprintf(sprintf_dst_buffer, SPRINTF_BUFFER_SIZE, buff, args);
        mini_vsnprintf(sprintf_dst_buffer + bytes_written, 
                       SPRINTF_BUFFER_SIZE - bytes_written, buff, args);
        va_end(args);

        const char* p = sprintf_dst_buffer;

        #ifdef USE_UART
        if(ws_uart_is_opened())
        {
            while(*p)
            {
                wsx_console_putc(*p);

                while(!ws_uart_is_tx_ready()){}
                ws_uart_putc(*p);
                ++p;
            }
        }
        else
        #endif
        {
            while(*p)
            {
                wsx_console_putc(*p);
                ++p;
            }
        }
    }
#endif // ENABLE_LOGS
}

void hal_ws_sleep_until(timestamp_t ts)
{
	ws_int_enable(WS_INT_ENABLE_HBL_TIMER);
    ws_timer_hblank_start_once((uint16_t)ts - g_loop_ticks);
    PRINT_LOG(LOG_INFO, log_halted, (uint16_t)ts - g_loop_ticks);
    while(!ws_int_is_requested(WS_INT_STATUS_HBL_TIMER))
    {
        ia16_halt();
    }
    g_loop_ticks = (uint16_t)ts;
}

timestamp_t hal_ws_get_timestamp(void)
{
    return (timestamp_t)g_loop_ticks;
}

void hal_ws_update_screen(void)
{      
    uint8_t row = 0;
    uint8_t col = 0;
    uint8_t dbl_row = 0;
    uint8_t dbl_col = 0;
    for(; row < LCD_HEIGHT >> 1; ++row)
    {
        col = 0;
        for(; col < LCD_WIDTH >> 1; ++col)
        {
            if(modified_tiles[(col * (LCD_HEIGHT > 1)) + row] == 0)
            {
                continue;
            }
            
            dbl_row = row << 1;
            dbl_col = col << 1;
            uint16_t tile_index = GET_TILE_INDEX(IS_PIXEL_SET(pixels, ((dbl_col + 1) * LCD_HEIGHT) + (dbl_row + 0)), 
                                                    IS_PIXEL_SET(pixels, ((dbl_col + 0) * LCD_HEIGHT) + (dbl_row + 0)), 
                                                    IS_PIXEL_SET(pixels, ((dbl_col + 1) * LCD_HEIGHT) + (dbl_row + 1)), 
                                                    IS_PIXEL_SET(pixels, ((dbl_col + 0) * LCD_HEIGHT) + (dbl_row + 1)));

            ws_screen_put_tile(&wse_screen1, tile_lookup[tile_index], SCREEN_OFFSET_X + col, SCREEN_OFFSET_Y + row);
        }
    }
    memset(modified_tiles, 0x0, countof(modified_tiles));
}

void hal_ws_set_lcd_matrix(u8_t x, u8_t y, bool_t val)
{
    uint16_t target_bit = (x * LCD_HEIGHT) + y;
    SET_PIXEL(pixels, target_bit, val != 0 ? 0xF : 0x0);
    modified_tiles[(x * (LCD_HEIGHT > 1)) + y] |= 0xFF;
    ++g_screen_needs_update;
    PRINT_LOG(LOG_PIXEL, log_pixel_write, x, y, val != 0 ? 1 : 0);
}

void hal_ws_set_lcd_icon(u8_t icon, bool_t val)
{
    icons[icon]->visible = val;
    for(uint8_t sprite_x = 0; sprite_x < SPRITE_COLS_PER_ICON; ++sprite_x)
    {
        for(uint8_t sprite_y = 0; sprite_y < SPRITE_ROWS_PER_ICON; ++sprite_y)
        {
            icons[icon]->sprites[sprite_x][sprite_y]->x = icons[icon]->x + (sprite_x * 8);
            icons[icon]->sprites[sprite_x][sprite_y]->y = val != 0 ? icons[icon]->y + (sprite_y * 8) : ICON_POSITION_Y_OFF;
        }
    }
    
    // PRINT_LOG(LOG_INFO, log_icon_write, icon, val != 0 ? log_generic_on : log_generic_off);
}

void hal_ws_set_frequency(u32_t freq)
{

}

void hal_ws_play_frequency(bool_t en)
{

}

int hal_ws_handler(void)
{
	last_keys = curr_keys;
    curr_keys = ws_keypad_scan();
    uint16_t pressed_keys = curr_keys & ~last_keys;
    uint16_t released_keys = ~curr_keys & last_keys;

#ifdef ENABLE_LOGS

    // releasing all the Y keys will toggle the logs
    #define LOG_TOGGLE_MASK (WS_KEY_Y4 | WS_KEY_Y3 | WS_KEY_Y2 | WS_KEY_Y1)
    if((released_keys & LOG_TOGGLE_MASK) == LOG_TOGGLE_MASK)
    {
        enable_logs = !enable_logs;
        // wsx_console_clear();
        // hackily replace the start char for better clears
        uint16_t tile =   (WS_SCREEN_ATTR_TILE(0x4) & WS_SCREEN_ATTR_TILE_MASK)
                        | (WS_SCREEN_ATTR_PALETTE(0xC) & WS_SCREEN_ATTR_PALETTE_MASK)
                        | (WS_SCREEN_ATTR_BANK(0) & WS_SCREEN_ATTR_BANK_MASK) // bank
                        | (0)  // WS_SCREEN_ATTR_FLIP_H
                        | (0); // WS_SCREEN_ATTR_FLIP_V

        ws_screen_fill_tiles(&wse_screen2, tile, 0, 0, 32, 32);
    }

#endif // ENABLE_LOGS
    
    if( pressed_keys & WS_KEY_X4) tamalib_set_button(BTN_LEFT,   BTN_STATE_PRESSED);
    if( pressed_keys & WS_KEY_X3) tamalib_set_button(BTN_MIDDLE, BTN_STATE_PRESSED);
    if( pressed_keys & WS_KEY_X2) tamalib_set_button(BTN_RIGHT,  BTN_STATE_PRESSED);
    if( pressed_keys & WS_KEY_A ) tamalib_set_button(BTN_TAP,    BTN_STATE_PRESSED);

    if(released_keys & WS_KEY_X4) tamalib_set_button(BTN_LEFT,   BTN_STATE_RELEASED);
    if(released_keys & WS_KEY_X3) tamalib_set_button(BTN_MIDDLE, BTN_STATE_RELEASED);
    if(released_keys & WS_KEY_X2) tamalib_set_button(BTN_RIGHT,  BTN_STATE_RELEASED);
    if(released_keys & WS_KEY_A ) tamalib_set_button(BTN_TAP,    BTN_STATE_RELEASED);

    return 0;
}
