#ifndef __MEMORY_H_
#define __MEMORY_H_

#include <stddef.h>

/*

    simple manager to simulate dynamic memory calls

*/

void ts_memory_init(void* address, size_t size);
void* ts_memory_malloc(size_t size);
void ts_memory_free(void* data);
void ts_memory_get_stats(size_t* out_used, size_t* out_capacity);


#endif // __MEMORY_H_