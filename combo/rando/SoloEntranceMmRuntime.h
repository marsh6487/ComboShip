#pragma once
#include <stdint.h>
#include "../ComboGameExport.h"

#ifdef __cplusplus
extern "C" {
#endif
COMBO_MM_EXPORT int MM_SetSoloEntranceRuntimeSeed(const char* raw);
int Combo_MM_SoloEntranceRuntimeEnabled(void);
int Combo_MM_SoloEntranceBossEnabled(void);
int Combo_MM_SoloEntranceBossSource(int physicalBoss);
int Combo_MM_SoloEntranceTransition(int32_t* index);
void Combo_MM_SoloEntranceRequestBossDeathRecovery(void);
void Combo_MM_SoloEntranceOnSaveLoad(void);
void Combo_MM_SoloEntranceOnPlayDestroy(void);
void Combo_MM_SoloEntranceObserveScene(int scene);
void Combo_MM_SoloEntranceRestoreCheckpoint(int32_t index);
void Combo_MM_SoloEntranceRestoreWarpCheckpoint(int32_t index);
int32_t Combo_MM_SoloEntranceSavedCheckpoint(void);
#ifdef __cplusplus
}
#endif
