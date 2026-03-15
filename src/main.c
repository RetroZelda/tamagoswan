

#include <wse/memory.h>
#include <wsx/console.h>
#include <ws/display.h>
#include "wsx/zx0.h"

#include <tamalib/tamalib.h>
#include "hal_ws.h"
#include "memory.h"

// defined as a part of libwsx
// https://github.com/WonderfulToolchain/target-wswan-syslibs/blob/main/libwsx/assets/wsx_console_font_default.lua
extern const uint8_t __wf_rom wsx_console_font_default[];

WSE_RESERVE_TILES(512, 1024);

volatile uint16_t g_vblank_ticks = 0;
volatile uint16_t g_input_ticks = 0;

__attribute__((interrupt))
static void vblank_int_handler(void) 
{
    ++g_vblank_ticks;
}

__attribute__((interrupt))
static void key_scan_int_handler(void)
{
    ++g_input_ticks;
};

void vblank_wait(void) 
{
    uint16_t vblank_ticks_last = g_vblank_ticks;
    while (g_vblank_ticks == vblank_ticks_last) 
    {
        ia16_halt();
    }
}

void setup_console()
{
    wsx_console_config_t config;
    config.tile_offset = 512 - 96;
    config.char_start = 32;
    config.char_count = 96;
    config.screen = WSX_CONSOLE_SCREEN2;
    config.palette = 4; // On a "mono" model, only palettes 4-7 and 12-15 are translucent.

    outportw(WS_DISPLAY_CTRL_PORT, 0);
    wsx_zx0_decompress(WS_TILE_MEM(config.tile_offset), wsx_console_font_default);
    wsx_console_init(&config);
}

void main(void)
{    
    // setup the display and tiles
	ws_display_set_control(0);
	ws_display_set_screen_addresses(&wse_screen1, &wse_screen2);

	ws_display_scroll_screen1_to(16, -8);
	ws_display_scroll_screen2_to(0, 0);

    // setup the tiles and palettes
    ws_display_set_shade_lut(WS_DISPLAY_SHADE_LUT_DEFAULT);
    outportw(WS_SCR_PAL_0_PORT, WS_DISPLAY_MONO_PALETTE(0x00, 0x00, 0x00, 0x00));
    outportw(WS_SCR_PAL_1_PORT, WS_DISPLAY_MONO_PALETTE(0x02, 0x02, 0x02, 0x02));
    outportw(WS_SCR_PAL_2_PORT, WS_DISPLAY_MONO_PALETTE(0x04, 0x04, 0x04, 0x04));
    outportw(WS_SCR_PAL_3_PORT, WS_DISPLAY_MONO_PALETTE(0x0F, 0x0F, 0x0F, 0x0F));
    outportw(WS_SCR_PAL_4_PORT, WS_DISPLAY_MONO_PALETTE(0, 0x7, 0, 0));
    memset(WS_TILE_MEM(0), 0xFF, sizeof(ws_display_tile_t));
    memset(WS_TILE_MEM(1), 0x00, sizeof(ws_display_tile_t));
    
    setup_console();

    // enable screen 1 for the tamagochi and screen 2 for logging
	ws_display_set_control(WS_DISPLAY_CTRL_SCR1_ENABLE);

    hal_ws_initize();

    // register our interrupts
	ws_int_set_handler(WS_INT_VBLANK, vblank_int_handler);
    ws_int_set_handler(WS_INT_KEY_SCAN, key_scan_int_handler);
    ws_int_set_default_handler_hblank_timer(); // for the hal_ws_sleep_until timer
	ws_int_enable(WS_INT_ENABLE_VBLANK | WS_INT_ENABLE_HBL_TIMER | WS_INT_ENABLE_KEY_SCAN);
	ia16_enable_irq();

    vblank_wait();

    uint16_t vblank_ticks_last = g_vblank_ticks;
    uint16_t input_ticks_last = g_input_ticks;    
    while (1) 
	{
        if(vblank_ticks_last != g_vblank_ticks)
        {
            g_hal->update_screen();
            vblank_ticks_last = g_vblank_ticks;
            ws_int_ack(WS_INT_ACK_VBLANK);
        }
        if(input_ticks_last != g_input_ticks)
        {
            g_hal->handler();
            input_ticks_last = g_input_ticks;
            ws_int_ack(WS_INT_ACK_KEY_SCAN);
        }
        tamalib_step();
    }
}
