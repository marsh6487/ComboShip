#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
uint8_t LinkVoice_TryPlay(uint16_t sfxId, const void* position);
void LinkVoice_EndFrame(void);
void LinkVoice_MixInto(int16_t* output, uint32_t frames);
void LinkVoice_Stop(uint16_t sfxId, const void* position);
uint8_t LinkVoice_IsPlaying(uint16_t sfxId);
#ifdef __cplusplus
}
#endif
