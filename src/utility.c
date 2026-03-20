
#include "utility.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <ws.h>

DEFINE_STRING_LOCAL(digits_upper, "0123456789ABCDEF");
DEFINE_STRING_LOCAL(digits_lower, "0123456789abcdef");
DEFINE_STRING_LOCAL(null_text, "(null)");

uint16_t ts_utility_snprintf(char* buffer, uint16_t max_len, const char __wf_rom *format, ...)
{
    va_list args;
    va_start(args, format);

    uint16_t written = ts_utility_vsnprintf(buffer, max_len, format, args);
    
    va_end(args);

    return written;
}

uint16_t ts_utility_vsnprintf(char *buffer, uint16_t max_len, const char __wf_rom *format, va_list args)
{
    if (max_len == 0 || buffer == NULL) 
    {
        return 0;
    }

    char *p = buffer;
    const char __wf_rom* fmt = format;
    uint16_t written = 0;             // chars actually written
    uint16_t would_write = 0;         // total chars that would be written

    while (*fmt && written < (int)max_len - 1)  // -1 for null terminator
    {
        if (*fmt != '%')
        {
            *p++ = *fmt++;
            written++;
            would_write++;
            continue;
        }

        fmt++;  // skip %

        int width = 0;
        bool zero_pad = false;
        if (*fmt == '0')
        {
            zero_pad = true;
            fmt++;
        }
        while (*fmt >= '0' && *fmt <= '9')
        {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        char spec = *fmt++;
        bool upper = false;

        switch (spec)
        {
            case '%':
                if (written < (int)max_len - 1)
                {
                    *p++ = '%';
                    written++;
                }
                would_write++;
                break;

            case 'c':
            {
                int c = va_arg(args, int);
                if (written < (int)max_len - 1)
                {
                    *p++ = (char)c;
                    written++;
                }
                would_write++;
                break;
            }

            case 's':
            {
                const char __wf_rom* s = va_arg(args, const char __wf_rom*);
                if (!s) 
                {
                    s = null_text;
                }
                while (*s)
                {
                    if (written < (int)max_len - 1)
                    {
                        *p++ = *s;
                        written++;
                    }
                    s++;
                    would_write++;
                }
                break;
            }

            case 'd':
            case 'i':
            case 'u':
            case 'x':
            case 'X':
            {
                uint16_t val;
                bool negative = false;

                if (spec == 'd' || spec == 'i')
                {
                    int32_t v = va_arg(args, int32_t);
                    negative = (v < 0);
                    val = negative ? (uint16_t)(-v) : (uint16_t)v;
                }
                else
                {
                    val = (uint16_t)va_arg(args, uint32_t);
                }

                if (spec == 'X') upper = true;

                char buf[12];
                char *bp = buf + sizeof(buf);
                *--bp = '\0';

                if (val == 0)
                {
                    *--bp = '0';
                }
                else
                {
                    uint16_t base = (spec == 'x' || spec == 'X') ? 16 : 10;

                    while (val > 0)
                    {
                        *--bp = (upper?digits_upper:digits_lower)[val % base];
                        val /= base;
                    }
                }

                if (negative) *--bp = '-';

                int len = (buf + sizeof(buf) - 1) - bp;
                int pad = width - len;

                // Padding
                char pad_char = zero_pad ? '0' : ' ';
                while (pad-- > 0)
                {
                    if (written < (int)max_len - 1)
                    {
                        *p++ = pad_char;
                        written++;
                    }
                    would_write++;
                }

                // Content
                while (*bp)
                {
                    if (written < (int)max_len - 1)
                    {
                        *p++ = *bp++;
                        written++;
                    }
                    else
                    {
                        bp++;  // still count for return value
                    }
                    would_write++;
                }
                break;
            }

            default:
                if (written < (int)max_len - 1)
                {
                    *p++ = '%';
                    *p++ = spec;
                    written += 2;
                }
                would_write += 2;
                break;
        }
    }

    // Always null-terminate (if there's space)
    if (written < (int)max_len)
    {
        *p = '\0';
    }
    else
    {
        buffer[max_len - 1] = '\0';  // truncate
    }

    // Return value follows snprintf convention
    return would_write;
}

#include <wsx/console.h>
extern wsx_console_config_t console_config; // should find the condfig in main.c
void ts_utility_screen_print(ws_screen_t ws_iram* screen, uint16_t start_tile_x, uint16_t start_tile_y, const char __wf_rom* text)
{
    uint16_t tile;
    uint16_t tile_index;
    uint16_t pos_x = start_tile_x;
    const char __wf_rom* p = text;
    while(*p)
    {
        switch(*p)
        {
            case '\n':
                pos_x = start_tile_x;
                ++start_tile_y;
                ++p;
                continue;
            case '\t':
                ++pos_x;
            default:
            {
                if(*p < console_config.char_start || *p > console_config.char_start + console_config.char_count)
                {
                    ++pos_x;
                    ++p;
                    continue;
                }
            }
        }           
        
        tile_index = console_config.tile_offset + (*p - console_config.char_start);
        tile = (WS_SCREEN_ATTR_TILE(tile_index) & WS_SCREEN_ATTR_TILE_MASK)
             | (WS_SCREEN_ATTR_PALETTE(console_config.palette) & WS_SCREEN_ATTR_PALETTE_MASK);

        ws_screen_put_tile(screen, tile, pos_x++, start_tile_y);
        ++p;
    }
}