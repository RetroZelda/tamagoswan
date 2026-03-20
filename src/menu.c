#include "menu.h"

#include <wse/memory.h>
#include <wonderful.h>
#include <ws.h>

#include "utility.h"
#include "save_data.h"
#include "images/menu.h"

#include <stdcountof.h>
#include <wsx/console.h>

typedef enum
{
    MENU,
    CREDITS
} ts_menu_state;

DEFINE_STRING_LOCAL(menu_item_new,        "New Game");
DEFINE_STRING_LOCAL(menu_item_continue,   "Continue");
DEFINE_STRING_LOCAL(menu_item_credits,    "Credits");
DEFINE_STRING_LOCAL(menu_item_selector,    ">");
DEFINE_STRING_LOCAL(menu_item_empty,       " ");

DEFINE_STRING_LOCAL(menu_credits_line_0,     "Special Thanks");
DEFINE_STRING_LOCAL(menu_credits_line_1,     "TamaLib.............Jcrona");
DEFINE_STRING_LOCAL(menu_credits_line_2,     "Wonderful...........Asie");
DEFINE_STRING_LOCAL(menu_credits_exit,       "Press any key to return");

static ts_menu_result ws_iram selected_menu_item;
static uint8_t ws_iram has_save_data;

static uint16_t last_keys = 0;
static uint16_t curr_keys = 0;
static ts_menu_state menu_state;

extern ws_display_tile_t ws_iram* menu_header_tiles;
extern ws_display_tile_t ws_iram* menu_footer_tiles;
extern wsx_console_config_t console_config; // should find the condfig in main.c

#define MENU_OPTION_START_X 10
#define MENU_OPTION_START_Y 8

#define CREDITS_START_X 1
#define CREDITS_START_Y 6

ts_menu_result ts_get_menu_result()
{
    return selected_menu_item;
}

static void clear_menu()
{
    uint16_t tile = (WS_SCREEN_ATTR_TILE(console_config.tile_offset) & WS_SCREEN_ATTR_TILE_MASK)
                    | (WS_SCREEN_ATTR_PALETTE(console_config.palette) & WS_SCREEN_ATTR_PALETTE_MASK);
    ws_screen_fill_tiles(&wse_screen2, tile, 0, 0, 32, 32);
}

static void clear_screen()
{
    uint16_t tile = (WS_SCREEN_ATTR_TILE(console_config.tile_offset) & WS_SCREEN_ATTR_TILE_MASK)
                    | (WS_SCREEN_ATTR_PALETTE(console_config.palette) & WS_SCREEN_ATTR_PALETTE_MASK);
    ws_screen_fill_tiles(&wse_screen1, tile, 0, 0, 32, 32);
}

static void draw_menu()
{
    ts_utility_screen_print(&wse_screen2, MENU_OPTION_START_X, MENU_OPTION_START_Y + 0, menu_item_new);
    if(has_save_data)
    {
        ts_utility_screen_print(&wse_screen2, MENU_OPTION_START_X, MENU_OPTION_START_Y + 1, menu_item_continue);
    }
    ts_utility_screen_print(&wse_screen2, MENU_OPTION_START_X, MENU_OPTION_START_Y + 2, menu_item_credits);
    
    ws_display_set_control( WS_DISPLAY_CTRL_SCR1_ENABLE | 
                            WS_DISPLAY_CTRL_SCR2_ENABLE | 
                            WS_DISPLAY_CTRL_SPR_ENABLE);
}

static void draw_selection()
{
    switch(selected_menu_item)
    {
        case MENU_RESULT_NEW:
            ts_utility_screen_print(&wse_screen2, MENU_OPTION_START_X - 1, MENU_OPTION_START_Y + 0, menu_item_selector);
            ts_utility_screen_print(&wse_screen2, MENU_OPTION_START_X - 1, MENU_OPTION_START_Y + 1, menu_item_empty);
            ts_utility_screen_print(&wse_screen2, MENU_OPTION_START_X - 1, MENU_OPTION_START_Y + 2, menu_item_empty);
        break;
        case MENU_RESULT_CONTINUE:
            ts_utility_screen_print(&wse_screen2, MENU_OPTION_START_X - 1, MENU_OPTION_START_Y + 0, menu_item_empty);
            ts_utility_screen_print(&wse_screen2, MENU_OPTION_START_X - 1, MENU_OPTION_START_Y + 1, menu_item_selector);
            ts_utility_screen_print(&wse_screen2, MENU_OPTION_START_X - 1, MENU_OPTION_START_Y + 2, menu_item_empty);
        break;
        case MENU_RESULT_CREDITS:
            ts_utility_screen_print(&wse_screen2, MENU_OPTION_START_X - 1, MENU_OPTION_START_Y + 0, menu_item_empty);
            ts_utility_screen_print(&wse_screen2, MENU_OPTION_START_X - 1, MENU_OPTION_START_Y + 1, menu_item_empty);
            ts_utility_screen_print(&wse_screen2, MENU_OPTION_START_X - 1, MENU_OPTION_START_Y + 2, menu_item_selector);
        break;
    }
}

static void draw_credits()
{
    ts_utility_screen_print(&wse_screen2, CREDITS_START_X + 6, CREDITS_START_Y - 1, menu_credits_line_0);
    ts_utility_screen_print(&wse_screen2, CREDITS_START_X, CREDITS_START_Y + 1, menu_credits_line_1);
    ts_utility_screen_print(&wse_screen2, CREDITS_START_X, CREDITS_START_Y + 4, menu_credits_line_2);
}

void ts_menu_setup()
{
    has_save_data = ts_has_save_data();
    if(has_save_data)
    {
        selected_menu_item = MENU_RESULT_CONTINUE;
    }
    else
    {
        selected_menu_item = MENU_RESULT_NEW;
    }

    uint16_t tile;
    uint16_t tile_index;
    uint8_t tile_row = 0;
    uint8_t tile_col = 0;

    // the header is 28x4
    tile_index = menu_header_tiles - WS_TILE_MEM(0);
    for(; tile_row < 4; ++tile_row)
    {
        tile_col = 0;
        for(; tile_col < 28; ++tile_col)
        {        
            tile = (WS_SCREEN_ATTR_TILE(tile_index++) & WS_SCREEN_ATTR_TILE_MASK)
                | (WS_SCREEN_ATTR_PALETTE(0xC) & WS_SCREEN_ATTR_PALETTE_MASK);
            ws_screen_put_tile(&wse_screen1, tile, tile_col, tile_row);
        }
    }

    // the footer is 28x3 and starts at row 15
    tile_index = menu_footer_tiles - WS_TILE_MEM(0);
    tile_row = 0;
    for(; tile_row < 3; ++tile_row)
    {
        tile_col = 0;
        for(; tile_col < 28; ++tile_col)
        {        
            tile = (WS_SCREEN_ATTR_TILE(tile_index++) & WS_SCREEN_ATTR_TILE_MASK)
                | (WS_SCREEN_ATTR_PALETTE(0xC) & WS_SCREEN_ATTR_PALETTE_MASK);
            ws_screen_put_tile(&wse_screen1, tile, tile_col, 15 + tile_row);
        }
    }

    
    ws_display_scroll_screen2_to(0, 0);
    menu_state = MENU;
    draw_menu();
}

void ts_menu_main()
{
    uint16_t pressed_keys = 0;
    uint16_t released_keys = 0;
    uint16_t ticks_in_credits = 0;
    bool in_menu = true;

    ws_int_set_default_handler_key();
    ws_int_set_default_handler_vblank();
	ws_int_enable(WS_INT_ENABLE_VBLANK | WS_INT_ENABLE_KEY_SCAN);
    while(in_menu)
    {
        ia16_halt(); // will pass on interrrupt(vblank)
        
        last_keys = curr_keys;
        curr_keys = ws_keypad_scan();

        pressed_keys = curr_keys & ~last_keys;
        released_keys = ~curr_keys & last_keys;

        switch(menu_state)
        {
            case MENU:
            {
                if(pressed_keys & WS_KEY_X1)
                {            
                    if(selected_menu_item > MENU_RESULT_NEW)
                    {
                        --selected_menu_item;
                        if(selected_menu_item == MENU_RESULT_CONTINUE && !has_save_data)
                        {
                            --selected_menu_item;
                        }
                    }
                }
                if(pressed_keys & WS_KEY_X3)
                {            
                    if(selected_menu_item < MENU_RESULT_CREDITS)
                    {
                        ++selected_menu_item;
                        if(selected_menu_item == MENU_RESULT_CONTINUE && !has_save_data)
                        {
                            ++selected_menu_item;
                        }
                    }
                }
                if(released_keys & WS_KEY_A)
                {            
                    if(selected_menu_item == MENU_RESULT_CREDITS)
                    {
                        clear_menu();
                        draw_credits();
                        menu_state = CREDITS;
                        ticks_in_credits = 0;
                    }
                    else
                    {
                        // we can leave the menu
                        in_menu = false;
                    }
                }
                else
                {
                    draw_selection();
                }
            } break;
            case CREDITS:
            {
                if(ticks_in_credits < 20000) // arbituary waiting
                {
                    if(++ticks_in_credits >= 20000)
                    {
                        ts_utility_screen_print(&wse_screen2, 3, 14, menu_credits_exit);
                    }
                }
                else
                {
                    if(released_keys != 0)
                    {
                        clear_menu();
                        draw_menu();
                        menu_state = MENU;
                    }
                }
            } break;
        }

    }

    clear_menu();
    clear_screen();
}
