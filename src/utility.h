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
uint16_t mini_snprintf(char* buffer, uint16_t max_len, const char __wf_rom *format, ...);
uint16_t mini_vsnprintf(char *buffer, uint16_t max_len, const char __wf_rom *format, va_list args);

#endif // __UTILITY_H__