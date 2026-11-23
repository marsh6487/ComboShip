#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void SOHTwilightPrincessTargeting_PlayCamera(void);
void MMTwilightPrincessTargeting_PlayCamera(void);

#ifdef __cplusplus
}

void SOHTwilightPrincessTargeting_Init();
void SOHTwilightPrincessTargeting_Mix(int16_t* output, size_t frames);
void SOHTwilightPrincessTargeting_Reset();
#endif
