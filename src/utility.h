#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <wonderful.h>
#include <stddef.h>
#include <stdarg.h>

#define DEFINE_STRING_LOCAL(name, value) static const char __wf_rom name[] = value
#define DEFINE_STRING(name, value) const char __wf_rom name[] = value

// Safe sprintf-like function with buffer size limit
//   buffer    - output destination
//   max_len   - maximum bytes writable (including null terminator)
//   format    - format string
// Returns:
//   >= 0 : number of characters that would have been written (excluding \0)
//   < 0  : error (buffer too small or invalid)
uint16_t ts_utility_snprintf(char* buffer, uint16_t max_len, const char __wf_rom *format, ...);
uint16_t ts_utility_vsnprintf(char *buffer, uint16_t max_len, const char __wf_rom *format, va_list args);

// will use the font from <wsx/console.h> to print text to the screen at a given position
void ts_utility_screen_print(ws_screen_t ws_iram* screen, uint16_t start_tile_x, uint16_t start_tile_y, const char* text);

#endif // __UTILITY_H__