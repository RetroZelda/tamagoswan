
#include "utility.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

DEFINE_STRING_LOCAL(digits_upper, "0123456789ABCDEF");
DEFINE_STRING_LOCAL(digits_lower, "0123456789abcdef");
DEFINE_STRING_LOCAL(null_text, "(null)");

uint16_t mini_snprintf(char* buffer, uint16_t max_len, const char __wf_rom *format, ...)
{
    va_list args;
    va_start(args, format);

    uint16_t written = mini_vsnprintf(buffer, max_len, format, args);
    
    va_end(args);

    return written;
}

uint16_t mini_vsnprintf(char *buffer, uint16_t max_len, const char __wf_rom *format, va_list args)
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