
#include "hal_ws.h"

#include <tamalib/tamalib.h>
#include <tamalib/hal.h>

#include <wse/memory.h>
#include <wsx/console.h>
#include <ws/util.h>

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "rom/rom.h"
#include "utility.h"
#include "memory.h"

// #define USE_UART
#ifdef USE_UART
#include <ws/uart.h>
#endif

// functions that map into tamalib
static void*        __far hal_ws_malloc(u32_t size);
static void         __far hal_ws_free(void* ptr);
static void         __far hal_ws_halt(void);
static bool_t       __far hal_ws_is_log_enabled(log_level_t level);
static void         __far hal_ws_log(log_level_t level, const char __wf_rom* buff, ...);
static void         __far hal_ws_sleep_until(timestamp_t ts);
static timestamp_t  __far hal_ws_get_timestamp(void);
static void         __far hal_ws_update_screen(void);
static void         __far hal_ws_set_lcd_matrix(u8_t x, u8_t y, bool_t val);
static void         __far hal_ws_set_lcd_icon(u8_t icon, bool_t val);
static void         __far hal_ws_set_frequency(u32_t freq);
static void         __far hal_ws_play_frequency(bool_t en);
static int          __far hal_ws_handler(void);

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

// buffer that holds the statethat we will want to draw
static uint8_t ws_iram pixel_buffer[LCD_WIDTH*LCD_HEIGHT];
static uint8_t ws_iram icon_buffer[ICON_NUM];

static uint16_t last_keys = 0;
static uint16_t curr_keys = 0;

static timestamp_t g_ticks = 0;
static timestamp_t g_screen_ts = 0;
static exec_mode_t g_exec_mode = EXEC_MODE_RUN;

#define SPRINTF_BUFFER_SIZE 128
char ws_iram sprintf_dst_buffer[SPRINTF_BUFFER_SIZE];

#define TO_LCD_INDEX(x,y)  ((y) * LCD_WIDTH + (x))
#define TICK_RATE (WS_SYSTEM_CLOCK_HZ / CLOCKS_PER_SEC)

DEFINE_STRING(src_test, "TEST %d\n");
void hal_ws_initize()
{
    memset(pixel_buffer, 0x00, LCD_WIDTH * LCD_HEIGHT * sizeof(uint8_t));
    memset(icon_buffer, 0x00, ICON_NUM);

    const u12_t __wf_rom* rom = (const u12_t __wf_rom*)tamagochi_rom_p1_swapped_data;
	tamalib_register_hal(&ws_hal);
    tamalib_init(rom, NULL, WS_SYSTEM_CLOCK_HZ);
    tamalib_set_speed(0);

	tamalib_set_exec_mode(g_exec_mode);
    g_ticks = 0;

	last_keys = 0;
    curr_keys = 0;

    #ifdef USE_UART
    ws_uart_open(WS_UART_BAUD_RATE_9600);
    #endif
    // mini_snprintf(sprintf_dst_buffer, SPRINTF_BUFFER_SIZE, src_test, 0x69);
    // hal_ws_log(LOG_INFO, src_test, 0x69);
}

void hal_ws_loop()
{
    //if(g_ticks++ >= g_sleep_target)
    {
        // tamalib_mainloop();
        tamalib_set_exec_mode(g_exec_mode);
        g_hal->handler();
        tamalib_step();

        timestamp_t ts = g_hal->get_timestamp();
        //if ((ts - g_screen_ts) >= TICK_RATE) 
        {
            g_screen_ts = ts;
            g_hal->update_screen();
            //hal_ws_log(LOG_INFO, src_test, 0x69);
        }
    }
    // hal_ws_log(LOG_INFO, src_test, 0x69);
}

void* __far hal_ws_malloc(u32_t size)
{
    return NULL; // ts_memory_malloc(size);
}

void __far hal_ws_free(void* ptr)
{
    // ts_memory_free(ptr);
}

void __far hal_ws_halt(void)
{

}

bool_t __far hal_ws_is_log_enabled(log_level_t level)
{
    switch(level)
    {
	case LOG_ERROR: return true;
	case LOG_INFO:  return true;
    case LOG_MEMORY:return false;
	case LOG_CPU:   return false;
	case LOG_INT:   return true;
	case LOG_OP:    return false;
    }
    return true;
}

DEFINE_STRING(log_memory,   " MEM");
DEFINE_STRING(log_error,    " ERR");
DEFINE_STRING(log_info,     "INFO");
DEFINE_STRING(log_cpu,      " CPU");
DEFINE_STRING(log_int,      " IRQ");
DEFINE_STRING(log_op,       "  OP");
DEFINE_STRING(log_format,   "%s: ");
void ws_iram hal_ws_log(log_level_t level, const char __wf_rom* buff, ...)
{
    if(!hal_ws_is_log_enabled(level))
    {
        return;
    }
    
    uint16_t bytes_written = 0;
    switch(level)
    {
        case LOG_ERROR:
            bytes_written = mini_snprintf(sprintf_dst_buffer, SPRINTF_BUFFER_SIZE, log_format, log_error);
            break;
        case LOG_INFO:
            bytes_written = mini_snprintf(sprintf_dst_buffer, SPRINTF_BUFFER_SIZE, log_format, log_info);
            break;
        case LOG_MEMORY:
            bytes_written = mini_snprintf(sprintf_dst_buffer, SPRINTF_BUFFER_SIZE, log_format, log_memory);
            break;
        case LOG_CPU:
            bytes_written = mini_snprintf(sprintf_dst_buffer, SPRINTF_BUFFER_SIZE, log_format, log_cpu);
            break;
        case LOG_INT:
            bytes_written = mini_snprintf(sprintf_dst_buffer, SPRINTF_BUFFER_SIZE, log_format, log_int);
            break;
        case LOG_OP:
            bytes_written = mini_snprintf(sprintf_dst_buffer, SPRINTF_BUFFER_SIZE, log_format, log_op);
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
}

DEFINE_STRING(log_halted, "Halted for %d\n");
void __far hal_ws_sleep_until(timestamp_t ts)
{
    ws_timer_hblank_start_once(ts - g_ticks);
    ia16_halt();
    hal_ws_log(LOG_INFO, log_halted, ts - g_ticks);
    g_ticks = ts;
}

timestamp_t __far hal_ws_get_timestamp(void)
{
    return g_ticks;
}

static uint16_t off_index = 0;
void __far hal_ws_update_screen(void)
{    
    uint16_t row = 0;
    uint16_t col = 0;
    uint8_t pixel_state = 0x0;
    for(; row < LCD_HEIGHT; ++row)
    {
        col = 0;
        for(; col < LCD_WIDTH; col += 1)
        {
            // should_be_on = off_index == TO_LCD_INDEX(col, row) ? 0xFF : 0x0;w];
            pixel_state = 0xff;//pixel_buffer[TO_LCD_INDEX(col, row)];
            uint16_t tile = (WS_SCREEN_ATTR_TILE(0x0) & WS_SCREEN_ATTR_TILE_MASK) // tile index
                          | (WS_SCREEN_ATTR_PALETTE(pixel_state & 0x3) & WS_SCREEN_ATTR_PALETTE_MASK)
                          | (WS_SCREEN_ATTR_BANK(0) & WS_SCREEN_ATTR_BANK_MASK) // bank
                          | (WS_SCREEN_ATTR_FLIP_H) // H Flip
                          | (WS_SCREEN_ATTR_FLIP_V); // V Flip
            ws_screen_put_tile(&wse_screen1, tile, col, row);
        }
    }
    off_index = (off_index + 1) % (LCD_WIDTH * LCD_HEIGHT);
    
}

void __far hal_ws_set_lcd_matrix(u8_t x, u8_t y, bool_t val)
{
    pixel_buffer[TO_LCD_INDEX(x, y)] = val != 0 ? 0xFF : 0x0;
}

void __far hal_ws_set_lcd_icon(u8_t icon, bool_t val)
{
    icon_buffer[icon] = val != 0 ? 0xFF : 0x0;
}

void __far hal_ws_set_frequency(u32_t freq)
{

}

void __far hal_ws_play_frequency(bool_t en)
{

}

int __far hal_ws_handler(void)
{
	last_keys = curr_keys;
    curr_keys = ws_keypad_scan();
    uint16_t pressed_keys = curr_keys & ~last_keys;
    uint16_t released_keys = ~curr_keys & last_keys;
    
    if(pressed_keys & WS_KEY_X4) tamalib_set_button(BTN_LEFT,    BTN_STATE_PRESSED);
    if(pressed_keys & WS_KEY_X3) tamalib_set_button(BTN_MIDDLE,  BTN_STATE_PRESSED);
    if(pressed_keys & WS_KEY_X2) tamalib_set_button(BTN_TAP,     BTN_STATE_PRESSED);
    if(pressed_keys & WS_KEY_A ) tamalib_set_button(BTN_RIGHT,   BTN_STATE_PRESSED);

    if(released_keys & WS_KEY_X4) tamalib_set_button(BTN_LEFT,    BTN_STATE_RELEASED);
    if(released_keys & WS_KEY_X3) tamalib_set_button(BTN_MIDDLE,  BTN_STATE_RELEASED);
    if(released_keys & WS_KEY_X2) tamalib_set_button(BTN_TAP,     BTN_STATE_RELEASED);
    if(released_keys & WS_KEY_A ) tamalib_set_button(BTN_RIGHT,   BTN_STATE_RELEASED);

    return 0;
}
