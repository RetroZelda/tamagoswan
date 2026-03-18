#ifndef __PIXEL_TILES_H__
#define __PIXEL_TILES_H__

#include <wonderful.h>
#include <stdint.h>

// we want to hold hte pixel state of 4 pixels in 1 tile
// so we need a tile for every permutation:
//      ________  ____XXXX XXXX____ XXXXXXXX
//      ________  ________ ________ ________
//
//      ________  ____XXXX XXXX____ XXXXXXXX
//      ____XXXX  ____XXXX ____XXXX ____XXXX
//
//      ________  ____XXXX XXXX____ XXXXXXXX
//      XXXX____  XXXX____ XXXX____ XXXX____
//
//      ________  ____XXXX XXXX____ XXXXXXXX
//      XXXXXXXX  XXXXXXXX XXXXXXXX XXXXXXXX

#define gfx_pixel_tiles_size (16)
#define NUM_PIXEL (8)

#define ONL  ((uint8_t)0x00F0)
#define ONR  ((uint8_t)0x000F)
#define ONA  ((uint8_t)0x00FF)
#define OFF ((uint8_t)0x0000)
#define PIXEL(left, right) (uint8_t)((left << 0x4) | (right))

// should load into a ws_display_tile_t
ws_display_tile_t __wf_rom gfx_pixel_tiles[gfx_pixel_tiles_size] =
{
    {{.row = { OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF } }},
    {{.row = { ONR, ONR, ONR, ONR, OFF, OFF, OFF, OFF } }},
    {{.row = { ONL, ONL, ONL, ONL, OFF, OFF, OFF, OFF } }},
    {{.row = { ONA, ONA, ONA, ONA, OFF, OFF, OFF, OFF } }},    
    {{.row = { OFF, OFF, OFF, OFF, ONR, ONR, ONR, ONR } }},
    {{.row = { ONR, ONR, ONR, ONR, ONR, ONR, ONR, ONR } }},
    {{.row = { ONL, ONL, ONL, ONL, ONR, ONR, ONR, ONR } }},
    {{.row = { ONA, ONA, ONA, ONA, ONR, ONR, ONR, ONR } }},
    {{.row = { OFF, OFF, OFF, OFF, ONL, ONL, ONL, ONL } }},
    {{.row = { ONR, ONR, ONR, ONR, ONL, ONL, ONL, ONL } }},
    {{.row = { ONL, ONL, ONL, ONL, ONL, ONL, ONL, ONL } }},
    {{.row = { ONA, ONA, ONA, ONA, ONL, ONL, ONL, ONL } }},
    {{.row = { OFF, OFF, OFF, OFF, ONA, ONA, ONA, ONA } }},
    {{.row = { ONR, ONR, ONR, ONR, ONA, ONA, ONA, ONA } }},
    {{.row = { ONL, ONL, ONL, ONL, ONA, ONA, ONA, ONA } }},
    {{.row = { ONA, ONA, ONA, ONA, ONA, ONA, ONA, ONA } }},
};

#undef ON
#undef OFF
#undef PIXEL

#endif // __PIXEL_TILES_H__