#pragma once
#include "z64.h"
#include <stdbool.h>
#include <stddef.h>
#include "partner_control.h"
#ifdef __cplusplus
extern "C" {
#endif
extern PlayState* gPlayState;
bool Partners_Register(const NeiPartnerV1*);
bool Partners_RigAvailable(const char*, unsigned);
bool Partners_IsActive(void);
bool Partners_SongOwned(void);
bool Partners_CojiroEnabled(void);
void Partners_UpdateCojiroUnlock(void);
bool Partners_CanControl(PlayState*, Actor*, bool);
bool Partners_MustRelease(PlayState*, Actor*, bool, bool);
int Partners_RegisterActor(const char*, const char*, int, unsigned int, size_t, ActorFunc, ActorFunc, ActorFunc,
                           ActorFunc);
void Partners_Filter(Player*, Input*);
void Partners_Notes(u8, f32, s8);
void Partners_SongRecognized(void);
void Partners_ControlTick(void);
void Partners_Reset(void);
void Partners_DrawHint(void);
void NativeMalon_Register(void);
void NativeMalon_Tick(void);
void NativeMalon_Reset(void);
void NativeCojiro_Register(void);
void NativeCojiro_Tick(void);
void NativeCojiro_Reset(void);
void NativeCojiro_ClearRecovery(void);
void NativeCojiro_PlayerUpdated(void);
void Partners_ToggleCojiro(void);
bool Partners_CojiroOwned(void);
#ifdef __cplusplus
}
#endif
