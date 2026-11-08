#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Game-thread lazy load. Immutable mono PCM at 32000 Hz, valid for process lifetime.
// action is 0..31 (native OoT adult SFX ID minus 0x6800); variant wraps.
// Missing OoT ownership/assets or invalid action returns null and zero frames.
const int16_t* OotAdultVoice_GetClip(unsigned action, unsigned variant, uint32_t* frames);
#ifdef __cplusplus
}
#endif
