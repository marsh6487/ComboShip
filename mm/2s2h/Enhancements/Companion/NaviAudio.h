#pragma once

#include "MidnaAudio.h"

#ifdef __cplusplus
extern "C" {
#endif

bool MMCompanionAudio_TryPlay(MMMidnaAudioEvent event);
void MMCompanionAudio_Reset(void);

#ifdef __cplusplus
}

void MMOptionalSfx_Init();
void MMOptionalSfx_Mix(int16_t* output, size_t frames);
void MMOptionalSfx_Reset();
#endif
