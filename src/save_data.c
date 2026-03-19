#include "save_data.h"

#include <string.h> // memcpy

#include <tamalib/hw.h>
#include <wonderful.h>
#include <ws.h>

typedef struct 
{
	u13_t pc;
	u12_t x;
	u12_t y;
	u4_t a;
	u4_t b;
	u5_t np;
	u8_t sp;
	u4_t flags;

	u32_t tick_counter;
	u32_t clk_timer_2hz_timestamp;
	u32_t clk_timer_4hz_timestamp;
	u32_t clk_timer_8hz_timestamp;
	u32_t clk_timer_16hz_timestamp;
	u32_t clk_timer_32hz_timestamp;
	u32_t clk_timer_64hz_timestamp;
	u32_t clk_timer_128hz_timestamp;
	u32_t clk_timer_256hz_timestamp;
	u32_t prog_timer_timestamp;
	bool_t prog_timer_enabled;
	u8_t prog_timer_data;
	u8_t prog_timer_rld;

	interrupt_t interrupts[INT_SLOT_NUM];

	bool_t cpu_halted;
    
	MEM_BUFFER_TYPE memory[MEM_BUFFER_SIZE];
} ts_cpu_data;


#define BITS_TO_BYTES(bits)   (((bits) + 7) / 8)
typedef struct
{
    uint8_t pixels[BITS_TO_BYTES(LCD_WIDTH*LCD_HEIGHT)];
    hal_ws_icon icons[ICON_NUM];
} ts_hal_data;

typedef struct
{
	ts_cpu_data cpu_data;
	ts_hal_data hal_data;
} ts_save_buffer;

typedef struct 
{
	 // used to see if save data exists
	union
	{
		const char header_data[4];
		uint32_t header_value;
	} verification_header;
	
	uint8_t num_buffer;
	uint8_t current_buffer;
	ts_save_buffer buffer_data[];
} ts_save_data;


#define NUM_DATA_BUFFERS 2
#define HEADER_VALUE 0x84657765 // TAMA

static inline uint8_t get_next_buffer_index(uint8_t in_index)
{
    if(in_index >= (NUM_DATA_BUFFERS - 1))
    {
        return 0;
    }
    return in_index + 1;
}

bool ts_save(const state_t* cpu_state, const ts_hal_state* hal_state)
{
    outportw(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_ENABLE);

	ts_save_data __wf_sram* save_data = (ts_save_data __wf_sram*)WS_SRAM_MEM;

	// check if this is the first write
	if(save_data->verification_header.header_value != HEADER_VALUE)
	{
		save_data->verification_header.header_value = HEADER_VALUE;
		save_data->num_buffer = NUM_DATA_BUFFERS;
		save_data->current_buffer = 0xFF;
		memset(save_data->buffer_data, 0x0, sizeof(ts_save_buffer) * NUM_DATA_BUFFERS);
	}


    // we only want to write into the next buffer slot, so if we lose power we can resume from the previous buffer's data
    uint8_t next_buffer = get_next_buffer_index(save_data->current_buffer);
    ts_save_buffer __wf_sram* destination_buffer = save_data->buffer_data + next_buffer;

	// write the TamaLiB state
	if(cpu_state != NULL)
	{
		destination_buffer->cpu_data.pc = *cpu_state->pc;
		destination_buffer->cpu_data.x = *cpu_state->x;
		destination_buffer->cpu_data.y = *cpu_state->y;
		destination_buffer->cpu_data.a = *cpu_state->a;
		destination_buffer->cpu_data.b = *cpu_state->b;
		destination_buffer->cpu_data.np = *cpu_state->np;
		destination_buffer->cpu_data.sp = *cpu_state->sp;
		destination_buffer->cpu_data.flags = *cpu_state->flags;
		destination_buffer->cpu_data.tick_counter = *cpu_state->tick_counter;
		destination_buffer->cpu_data.clk_timer_2hz_timestamp = *cpu_state->clk_timer_2hz_timestamp;
		destination_buffer->cpu_data.clk_timer_4hz_timestamp = *cpu_state->clk_timer_4hz_timestamp;
		destination_buffer->cpu_data.clk_timer_8hz_timestamp = *cpu_state->clk_timer_8hz_timestamp;
		destination_buffer->cpu_data.clk_timer_16hz_timestamp = *cpu_state->clk_timer_16hz_timestamp;
		destination_buffer->cpu_data.clk_timer_32hz_timestamp = *cpu_state->clk_timer_32hz_timestamp;
		destination_buffer->cpu_data.clk_timer_64hz_timestamp = *cpu_state->clk_timer_64hz_timestamp;
		destination_buffer->cpu_data.clk_timer_128hz_timestamp = *cpu_state->clk_timer_128hz_timestamp;
		destination_buffer->cpu_data.clk_timer_256hz_timestamp = *cpu_state->clk_timer_256hz_timestamp;
		destination_buffer->cpu_data.prog_timer_timestamp = *cpu_state->prog_timer_timestamp;
		destination_buffer->cpu_data.prog_timer_enabled = *cpu_state->prog_timer_enabled;
		destination_buffer->cpu_data.prog_timer_data = *cpu_state->prog_timer_data;
		destination_buffer->cpu_data.prog_timer_rld = *cpu_state->prog_timer_rld;
		destination_buffer->cpu_data.cpu_halted = *cpu_state->cpu_halted;
		memcpy(destination_buffer->cpu_data.interrupts, cpu_state->interrupts, INT_SLOT_NUM * sizeof(interrupt_t));    
		memcpy(destination_buffer->cpu_data.memory, cpu_state->memory, MEM_BUFFER_SIZE * sizeof(MEM_BUFFER_TYPE));  
	}  

	// write the HAL state
	if(hal_state != NULL)
	{
    	memcpy(destination_buffer->hal_data.pixels, hal_state->pixels, BITS_TO_BYTES(LCD_WIDTH*LCD_HEIGHT));    
    	memcpy(destination_buffer->hal_data.icons, hal_state->icons, ICON_NUM * sizeof(hal_ws_icon));   
	}

    // now we can write the valid buffer index before closing the bus
    save_data->current_buffer = next_buffer;
    outportw(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);

	// re-enable to verify the write succeeded
    outportw(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_ENABLE);
	bool success = true;
	save_data = (ts_save_data __wf_sram*)WS_SRAM_MEM;
	if(save_data->verification_header.header_value != HEADER_VALUE)
	{
		success = false;
	}
	if(save_data->current_buffer != next_buffer)
	{
		success = false;
	}
    outportw(WS_CART_BANK_FLASH_PORT, WS_CART_BANK_FLASH_DISABLE);
    return success;
}

bool ts_load(state_t* cpu_state, ts_hal_state* hal_state)
{
    outportw(WS_CART_BANK_FLASH_ENABLE, WS_CART_EXTBANK_RAM_PORT);

	ts_save_data __wf_sram* save_data = (ts_save_data __wf_sram*)WS_SRAM_MEM;

	// check if the data is valid
	if(save_data->verification_header.header_value != HEADER_VALUE)
	{
    	outportw(WS_CART_BANK_FLASH_DISABLE, WS_CART_EXTBANK_RAM_PORT);
        return false;
	}

    const ts_save_buffer __wf_sram* source_buffer = save_data->buffer_data + save_data->current_buffer;

	// load the TamaLiB state
	if(cpu_state != NULL)
	{
		*cpu_state->pc = source_buffer->cpu_data.pc;
		*cpu_state->x = source_buffer->cpu_data.x;
		*cpu_state->y = source_buffer->cpu_data.y;
		*cpu_state->a = source_buffer->cpu_data.a;
		*cpu_state->b = source_buffer->cpu_data.b;
		*cpu_state->np = source_buffer->cpu_data.np;
		*cpu_state->sp = source_buffer->cpu_data.sp;
		*cpu_state->flags = source_buffer->cpu_data.flags;
		*cpu_state->tick_counter = source_buffer->cpu_data.tick_counter;
		*cpu_state->clk_timer_2hz_timestamp = source_buffer->cpu_data.clk_timer_2hz_timestamp;
		*cpu_state->clk_timer_4hz_timestamp = source_buffer->cpu_data.clk_timer_4hz_timestamp;
		*cpu_state->clk_timer_8hz_timestamp = source_buffer->cpu_data.clk_timer_8hz_timestamp;
		*cpu_state->clk_timer_16hz_timestamp = source_buffer->cpu_data.clk_timer_16hz_timestamp;
		*cpu_state->clk_timer_32hz_timestamp = source_buffer->cpu_data.clk_timer_32hz_timestamp;
		*cpu_state->clk_timer_64hz_timestamp = source_buffer->cpu_data.clk_timer_64hz_timestamp;
		*cpu_state->clk_timer_128hz_timestamp = source_buffer->cpu_data.clk_timer_128hz_timestamp;
		*cpu_state->clk_timer_256hz_timestamp = source_buffer->cpu_data.clk_timer_256hz_timestamp;
		*cpu_state->prog_timer_timestamp = source_buffer->cpu_data.prog_timer_timestamp;
		*cpu_state->prog_timer_enabled = source_buffer->cpu_data.prog_timer_enabled;
		*cpu_state->prog_timer_data = source_buffer->cpu_data.prog_timer_data;
		*cpu_state->prog_timer_rld = source_buffer->cpu_data.prog_timer_rld;
		*cpu_state->cpu_halted = source_buffer->cpu_data.cpu_halted;
		memcpy(cpu_state->interrupts, source_buffer->cpu_data.interrupts, INT_SLOT_NUM * sizeof(interrupt_t));    
		memcpy(cpu_state->memory, source_buffer->cpu_data.memory, MEM_BUFFER_SIZE * sizeof(MEM_BUFFER_TYPE));  
	}

	// load the HAL state
	if(hal_state != NULL)
	{
    	memcpy(hal_state->pixels, source_buffer->hal_data.pixels, BITS_TO_BYTES(LCD_WIDTH*LCD_HEIGHT));    
    	memcpy(hal_state->icons, source_buffer->hal_data.icons, ICON_NUM * sizeof(hal_ws_icon));   
	}

    outportw(WS_CART_BANK_FLASH_DISABLE, WS_CART_EXTBANK_RAM_PORT);

    return true;
}