#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
uint8_t DinVoice_TryPlay(uint16_t sfxId, const void* position);
void DinVoice_EndFrame(void);
void DinVoice_MixInto(int16_t* output, uint32_t frames);
void DinVoice_Stop(uint16_t sfxId, const void* position);
uint8_t DinVoice_IsPlaying(uint16_t sfxId);
#ifdef __cplusplus
}
#endif
