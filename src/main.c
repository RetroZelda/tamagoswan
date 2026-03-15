

#include <wse/memory.h>
#include <wsx/console.h>
#include <ws/display.h>
#include "wsx/zx0.h"

#include "hal_ws.h"
#include "memory.h"

// defined as a part of libwsx
// https://github.com/WonderfulToolchain/target-wswan-syslibs/blob/main/libwsx/assets/wsx_console_font_default.lua
extern const uint8_t __wf_rom wsx_console_font_default[];

WSE_RESERVE_TILES(512, 1024);

volatile uint16_t g_vblank_ticks = 0;

__attribute__((interrupt))
static void __wf_rom vblank_int_handler(void) 
{
    g_vblank_ticks++;

    // suppressess unnessisary warning:
    //      calling assume_ss_data function when %ss may not point to data segment [-Wmaybe-uninitialized]
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmaybe-uninitialized" 
    ws_int_ack(WS_INT_ACK_VBLANK);
    #pragma GCC diagnostic pop
}

void vblank_wait(void) 
{
    uint16_t g_vblank_ticks_last = g_vblank_ticks;
    while (g_vblank_ticks == g_vblank_ticks_last) 
    //while(!ws_int_is_requested(WS_INT_STATUS_VBLANK))
    {
        ia16_halt();
    }
    ws_int_ack(WS_INT_ACK_VBLANK);
}

void setup_console()
{
    wsx_console_config_t config;
    config.tile_offset = 512 - 96;
    config.char_start = 32;
    config.char_count = 96;
    config.screen = WSX_CONSOLE_SCREEN2;
    config.palette = 0;

    outportw(WS_DISPLAY_CTRL_PORT, 0);

    if (ws_system_is_color_active()) {
        WS_DISPLAY_COLOR_MEM(0)[0] = 0xFFF;
        WS_DISPLAY_COLOR_MEM(0)[1] = 0x000;
    } else {
        ws_display_set_shade_lut(WS_DISPLAY_SHADE_LUT_DEFAULT);
        outportw(WS_SCR_PAL_0_PORT, WS_DISPLAY_MONO_PALETTE(0, 7, 0, 0));
    }

    wsx_zx0_decompress(WS_TILE_MEM(config.tile_offset), wsx_console_font_default);
    wsx_console_init(&config);
}

void main(void)
{    
    // setup the display and tiles
	ws_display_set_control(0);
	ws_display_set_screen_addresses(&wse_screen1, &wse_screen2);

	ws_display_scroll_screen1_to(0, 0);
	ws_display_scroll_screen2_to(0, 0);

    // setup the tiles and palettes
    ws_display_set_shade_lut(WS_DISPLAY_SHADE_LUT_DEFAULT);
    // ws_screen_fill_tiles(&wse_screen1, 0x3A10, 0, 0, 32, 32);
    // ws_screen_fill_tiles(&wse_screen2, 0x3A10, 0, 0, 32, 32);
    
    outportw(WS_SCR_PAL_0_PORT, 0x00);
    outportw(WS_SCR_PAL_1_PORT, 0x02);
    outportw(WS_SCR_PAL_2_PORT, 0x04);
    outportw(WS_SCR_PAL_3_PORT, 0x0F);
    memset(WS_TILE_MEM(0), 0xFF, sizeof(ws_display_tile_t));
    memset(WS_TILE_MEM(1), 0x00, sizeof(ws_display_tile_t));
    
    setup_console();

    // enable screen 1 for the tamagochi and screen 2 for logging
	ws_display_set_control(WS_DISPLAY_CTRL_SCR1_ENABLE | WS_DISPLAY_CTRL_SCR2_ENABLE);

    // register our interrupts
	ws_int_set_handler(WS_INT_VBLANK, vblank_int_handler);
    ws_int_set_default_handler_hblank_timer(); // for the hal_ws_sleep_until timer
	ws_int_enable(WS_INT_ENABLE_VBLANK | WS_INT_ENABLE_HBL_TIMER);
	ia16_enable_irq(); // we need to enable cpu interrupts to get all interrupts

    // the work ram that can be used for tamalib breakpoint
    //uint8_t* iram = WS_IRAM_MEM + 0x0400;
    //ts_memory_init(iram, 0x1600);
    hal_ws_initize();

    while (1) 
	{
        vblank_wait();
        hal_ws_loop();
    }
}
