#include "memory.h"

#include <wonderful.h>

typedef enum
{
    MEMORY_FLAG_NONE = 0x0,
    MEMORY_FLAG_USED = 0x1,
} ts_memory_flags;

typedef struct ts_memory_block ts_memory_block;
typedef struct 
{
    ts_memory_block* PrevBlock;
    uint16_t DataSize;
    uint8_t Flags;
} ts_memory_block_header;

typedef uint8_t ts_memory_block_data;

typedef struct __attribute__((packed)) ts_memory_block
{
    ts_memory_block_header header;
    ts_memory_block_data Data[0];
} ts_memory_block;

static void* memory = NULL; 
static size_t capacity = 0;

static inline ts_memory_block* memory_get_first_block()
{
    return (ts_memory_block*)memory;
}

static inline bool memory_is_block_valid(const ts_memory_block* const block)
{
    return block && (void*)block >= memory && (void*)block < (memory + capacity);
}

static inline bool memory_is_block_used(const ts_memory_block* const block)
{
    return (block->header.Flags & MEMORY_FLAG_USED) == MEMORY_FLAG_USED;
}

static inline bool memory_is_block_free(const ts_memory_block* const block)
{
    return (block->header.Flags & MEMORY_FLAG_USED) == MEMORY_FLAG_NONE;
}

static inline ts_memory_block* memory_get_block_from_data(void* data)
{
    return (ts_memory_block*)(data - sizeof(ts_memory_block_header));
}

static inline ts_memory_block* memory_get_next_block(const ts_memory_block* const block)
{
    ts_memory_block* next_block = (ts_memory_block*)((uint8_t*)block + sizeof(ts_memory_block_header) + block->header.DataSize);
    if(memory_is_block_valid(next_block))
    {
        return next_block;
    }
    return NULL;
}

static inline ts_memory_block* memory_get_prev_block(const ts_memory_block* const block)
{
    ts_memory_block* prev_block = block->header.PrevBlock;
    if(memory_is_block_valid(prev_block))
    {
        return prev_block;
    }
    return NULL;
}

void ts_memory_init(void* address, size_t size)
{
    memory = address;
    capacity = size;

    ts_memory_block* head_block = memory_get_first_block();
    head_block->header.PrevBlock = NULL;
    head_block->header.DataSize = size;
    head_block->header.Flags = MEMORY_FLAG_NONE;
}

void* ts_memory_malloc(size_t size)
{
    size_t total_size = size + sizeof(ts_memory_block_header);
    ts_memory_block* block = memory_get_first_block();
    while(memory_is_block_valid(block))
    {
        // if the block fits we canmark it as used and split it in 2
        if(memory_is_block_free(block) && block->header.DataSize >= total_size)
        {
            // split hte block to create a new block
            ts_memory_block* new_block = (ts_memory_block*)((uint8_t*)block + total_size);
            new_block->header.Flags = MEMORY_FLAG_NONE;
            new_block->header.DataSize = block->header.DataSize - total_size;
            new_block->header.PrevBlock = block;

            // have the next block point to the new block
            ts_memory_block* next_block = memory_get_next_block(block);
            if(memory_is_block_valid(next_block))
            {
                next_block->header.PrevBlock = new_block;
            }

            // flag the block to return
            block->header.Flags |= MEMORY_FLAG_USED;
            block->header.DataSize = size;
            return block->Data;
        }
        block = (ts_memory_block*)((uint8_t*)block + sizeof(ts_memory_block_header) + block->header.DataSize);
    }
    return NULL;
}

void ts_memory_free(void* data)
{
    ts_memory_block* block = memory_get_block_from_data(data);

    // merge with the previous block if its free
    ts_memory_block* prev_block = memory_get_prev_block(block);
    if(prev_block && memory_is_block_free(prev_block))
    {
        prev_block->header.DataSize += block->header.DataSize + sizeof(ts_memory_block_header);
        block = prev_block; // so we can merge with the next block
    }
    else
    {
        // prev block isnt free(or doesnt exist) so we can jsut flag as free
        block->header.Flags &= ~MEMORY_FLAG_USED;
    }

    // merge with the next block if its free and ensure it points to the correct prev block
    ts_memory_block* next_block = memory_get_prev_block(block);
    if(memory_is_block_valid(next_block))
    {
        if(next_block && memory_is_block_free(next_block))
        {
            block->header.DataSize += next_block->header.DataSize;
        }
        else
        {
            next_block->header.PrevBlock = block;
        }
    }
}

void ts_memory_get_stats(size_t* out_used, size_t* out_capacity)
{
    if(out_used)
    {
        *out_used = 0;
        ts_memory_block* block = memory_get_first_block();
        while(memory_is_block_valid(block))
        {
            if(memory_is_block_used(block))
            {
                *out_used += block->header.DataSize;
            }
            block = memory_get_next_block(block);
        }
    }
    if(out_capacity)
    {
        *out_capacity = capacity;
    }
}
