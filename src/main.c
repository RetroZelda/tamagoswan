

#include <wse/memory.h>
#include <wsx/console.h>
#include <ws/display.h>
#include "wsx/zx0.h"

#include "pixel_tiles.h"
#include "hal_ws.h"
#include "memory.h"

#include "icons/icons.h"

#define NUM_FONT_TILES 96
#define NUM_ICON_TILES gfx_color_icons_tiles_size / sizeof(ws_display_tile_t)
#define TOTAL_TILES (gfx_pixel_tiles_size + NUM_ICON_TILES + NUM_FONT_TILES)
WSE_RESERVE_TILES(256, 0);

// defined as a part of libwsx
// https://github.com/WonderfulToolchain/target-wswan-syslibs/blob/main/libwsx/assets/wsx_console_font_default.lua
extern const uint8_t __wf_rom wsx_console_font_default[];

static void setup_console()
{
    wsx_console_config_t config;
    config.tile_offset = TOTAL_TILES - NUM_FONT_TILES; // WSE_RESERVE_TILES - char_count
    config.char_start = 32;
    config.char_count = NUM_FONT_TILES;
    config.screen = WSX_CONSOLE_SCREEN2;
    config.palette = 0xC; // On a "mono" model, only palettes 4-7 and 12-15 are translucent.

    outportw(WS_DISPLAY_CTRL_PORT, 0);
    wsx_zx0_decompress(WS_TILE_MEM(config.tile_offset), wsx_console_font_default);
    wsx_console_init(&config);

    // hackily replace the start char for better clears
    uint16_t tile = (WS_SCREEN_ATTR_TILE(config.tile_offset) & WS_SCREEN_ATTR_TILE_MASK)
                    | (WS_SCREEN_ATTR_PALETTE(config.palette) & WS_SCREEN_ATTR_PALETTE_MASK)
                    | (WS_SCREEN_ATTR_BANK(0) & WS_SCREEN_ATTR_BANK_MASK) // bank
                    | (0)  // WS_SCREEN_ATTR_FLIP_H
                    | (0); // WS_SCREEN_ATTR_FLIP_V

    ws_screen_fill_tiles(&wse_screen2, tile, 0, 0, 32, 32);
}

void main(void)
{   
    // setup the display and tiles
    ws_display_set_control(0);;
    ws_display_set_sprite_address(&wse_sprites1);
    ws_display_set_screen_addresses(&wse_screen1, &wse_screen2);

    ws_display_scroll_screen1_to(0, 0);
    ws_display_scroll_screen2_to(0, 0);

    // 2bpp on wsc for the extra ram 
    if(ws_system_set_mode(WS_MODE_COLOR))
    {
        // get a bunch of grayscale colors 
        uint8_t col = 0;
        for(uint8_t u = 0; u < 0x0f; ++u)
        {
            col = 0xf - u;
            for(uint8_t v = 0; v < 0x10; ++v)
            {
                *(WS_SCREEN_COLOR_MEM(u) + (v)) = WS_RGB(col, col, col);
            }
        }

        // and make some that are more what we need
        *(WS_SCREEN_COLOR_MEM(0xf) + (0)) = WS_RGB(0xF, 0xF, 0xF);
        *(WS_SCREEN_COLOR_MEM(0xf) + (1)) = WS_RGB(0x0, 0x0, 0x0);
    }
    else
    {
        // setup the tiles and palettes
        ws_display_set_shade_lut(WS_DISPLAY_SHADE_LUT_DEFAULT);
        outportw(WS_SCR_PAL_0_PORT, WS_DISPLAY_MONO_PALETTE(0x00, 0x00, 0x00, 0x00));
        outportw(WS_SCR_PAL_1_PORT, WS_DISPLAY_MONO_PALETTE(0x02, 0x02, 0x02, 0x02));
        outportw(WS_SCR_PAL_2_PORT, WS_DISPLAY_MONO_PALETTE(0x04, 0x04, 0x04, 0x04));
        outportw(WS_SCR_PAL_3_PORT, WS_DISPLAY_MONO_PALETTE(0x0F, 0x0F, 0x0F, 0x0F));
        outportw(WS_SCR_PAL_4_PORT, WS_DISPLAY_MONO_PALETTE(0x00, 0x07, 0x00, 0x00));
    }

    memcpy(WS_TILE_MEM(0), gfx_pixel_tiles, gfx_pixel_tiles_size * sizeof(ws_display_tile_t));
    memcpy(WS_TILE_MEM(gfx_pixel_tiles_size), gfx_color_icons_tiles, gfx_color_icons_tiles_size);

    setup_console();

    hal_ws_initize(false);

    // NOTE: enabling hte screens last due to some ports used during init (e.g. WS_SPR_COUNT_PORT, WS_CART_BANK_FLASH_PORT)
    //       affecting some of the display port values.  This might be a bug in Mesen tho, however I dont have a flashcart to test on hardware
#ifdef ENABLE_LOGS
    // enable screen 1 for the tamagochi and screen 2 for logging
	ws_display_set_control( WS_DISPLAY_CTRL_SCR1_ENABLE | 
                            WS_DISPLAY_CTRL_SCR2_ENABLE | 
                            WS_DISPLAY_CTRL_SPR_ENABLE);
#else
    // enable screen 1 for the tamagochi
	ws_display_set_control( WS_DISPLAY_CTRL_SCR1_ENABLE |  
                            WS_DISPLAY_CTRL_SPR_ENABLE);
#endif // ENABLE_LOGS

    hal_ws_loop();
}
