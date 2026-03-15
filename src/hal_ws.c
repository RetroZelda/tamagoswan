
#include "hal_ws.h"

#include <tamalib/tamalib.h>
#include <tamalib/hal.h>

#include <wse/memory.h>
#include <wsx/console.h>
#include <ws/util.h>

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "modified_pixel.h"
#include "rom/rom.h"
#include "utility.h"
#include "memory.h"

// #define USE_UART
#ifdef USE_UART
#include <ws/uart.h>
#endif

// strings for logging the level 
DEFINE_STRING_LOCAL(log_level_memory,   " MEM");
DEFINE_STRING_LOCAL(log_level_pixel,    "PIXL");
DEFINE_STRING_LOCAL(log_level_error,    " ERR");
DEFINE_STRING_LOCAL(log_level_info,     "INFO");
DEFINE_STRING_LOCAL(log_level_cpu,      " CPU");
DEFINE_STRING_LOCAL(log_level_int,      " IRQ");
DEFINE_STRING_LOCAL(log_level_op,       "  OP");
DEFINE_STRING_LOCAL(log_format,         "%s: ");

// strings for our HAL logs
DEFINE_STRING_LOCAL(log_pixel_write, "Pixel (%02d, %02d) %d\n");
DEFINE_STRING_LOCAL(log_icon_write, "Icon %02d %s\n");
DEFINE_STRING_LOCAL(log_halted, "Halted for %d\n");
DEFINE_STRING_LOCAL(log_generic_on, "on");
DEFINE_STRING_LOCAL(log_generic_off, "off");

// functions that map into tamalib
static void*        hal_ws_malloc(u32_t size);
static void         hal_ws_free(void* ptr);
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
    .malloc             = hal_ws_malloc,
    .free               = hal_ws_free,
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

typedef struct
{
    uint8_t x;
    uint8_t y;
    uint8_t pixel;
} hal_modified_pixel;


static uint8_t ws_iram icon_buffer[ICON_NUM];

static uint16_t last_keys = 0;
static uint16_t curr_keys = 0;

static timestamp_t g_ticks = 0;

#ifdef ENABLE_LOGS
#define SPRINTF_BUFFER_SIZE 128
char ws_iram sprintf_dst_buffer[SPRINTF_BUFFER_SIZE];
#endif

#define CLOCK (WS_SYSTEM_CLOCK_HZ >> 16)
#define TO_LCD_INDEX(x,y)  ((y) * LCD_WIDTH + (x))
#define TICK_RATE (CLOCK / CLOCKS_PER_SEC)

void hal_ws_initize()
{
    memset(icon_buffer, 0x00, ICON_NUM);

    const u12_t __wf_rom* rom = (const u12_t __wf_rom*)tamagochi_rom_p1_swapped_data;
	tamalib_register_hal(&ws_hal);
    tamalib_init(rom, NULL, CLOCK);

    g_ticks = 0;

	last_keys = 0;
    curr_keys = 0;

    #ifdef USE_UART
    ws_uart_open(WS_UART_BAUD_RATE_9600);
    #endif
}

void* hal_ws_malloc(u32_t size)
{
    return NULL; // ts_memory_malloc(size);
}

void hal_ws_free(void* ptr)
{
    // ts_memory_free(ptr);
}

void hal_ws_halt(void)
{

}

bool_t hal_ws_is_log_enabled(log_level_t level)
{
#ifdef ENABLE_LOGS
    switch(level)
    {
	case LOG_ERROR: return true;
	case LOG_INFO:  return true;
    case LOG_MEMORY:return false;
	case LOG_CPU:   return false;
	case LOG_INT:   return true;
	case LOG_OP:    return false;
	case LOG_PIXEL: return true;
    }
    return true;
#else
    return false;
#endif // ENABLE_LOGS
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
    ws_timer_hblank_start_once(ts - g_ticks);
    PRINT_LOG(LOG_INFO, log_halted, ts - g_ticks);
    while(!ws_int_is_requested(WS_INT_STATUS_HBL_TIMER))
    {
        ia16_halt();
    }
    g_ticks = ts;
}

timestamp_t hal_ws_get_timestamp(void)
{
    return g_ticks;
}

void hal_ws_update_screen(void)
{        
    const ts_modified_pixel* pixel = ts_pixel_pop();
    while(pixel)
    {
        uint16_t tile = (WS_SCREEN_ATTR_TILE(0x0) & WS_SCREEN_ATTR_TILE_MASK) // tile index
                        | (WS_SCREEN_ATTR_PALETTE(pixel->pixel & 0x2) & WS_SCREEN_ATTR_PALETTE_MASK)
                        | (WS_SCREEN_ATTR_BANK(0) & WS_SCREEN_ATTR_BANK_MASK) // bank
                        | (0)  // WS_SCREEN_ATTR_FLIP_H
                        | (0); // WS_SCREEN_ATTR_FLIP_V
        ws_screen_put_tile(&wse_screen1, tile, pixel->x, pixel->y);
        pixel = ts_pixel_pop();
    }    
}

void hal_ws_set_lcd_matrix(u8_t x, u8_t y, bool_t val)
{
    ts_pixel_push(x, y, val != 0 ? 0xFF : 0x0);
    PRINT_LOG(LOG_PIXEL, log_pixel_write, x, y, val);
}

void hal_ws_set_lcd_icon(u8_t icon, bool_t val)
{
    icon_buffer[icon] = val != 0 ? 0xFF : 0x0;
    PRINT_LOG(LOG_INFO, log_icon_write, icon, val != 0 ? log_generic_on : log_generic_off);
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
