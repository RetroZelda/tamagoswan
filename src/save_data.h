#ifndef __SAVE_DATA_H__
#define __SAVE_DATA_H__

#include <tamalib/cpu.h>
#include "hal_ws.h"

typedef struct
{
    uint8_t ws_iram* pixels;
    hal_ws_icon ws_iram* icons;
    bool ws_iram* play_audio;
    uint32_t ws_iram* audio_frequency;
} ts_hal_state;

/*
 * we will save/load the internal state of tamalib's cpu directly
 * as well as the state of the hal
 */

bool ts_has_save_data();
bool ts_save(const state_t* cpu_state, const ts_hal_state* hal_state);
bool ts_load(state_t* cpu_state, ts_hal_state* hal_state);

#endif // __SAVE_DATA_H__