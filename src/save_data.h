#ifndef __SAVE_DATA_H__
#define __SAVE_DATA_H__

#include <tamalib/cpu.h>

/*
 * we will save/load the internal state of tamalib's cpu directly
 */

bool ts_save(const state_t* cpu_state);
bool ts_load(state_t* cpu_state);

#endif // __SAVE_DATA_H__