#pragma once
// Combined-only adapter fixtures do not link the native Solo MM save codec.
// These dormant session boundaries must never be reached by their test cases.
extern "C" int Combo_MM_SoloEntranceRuntimeEnabled(void) { return 0; }
extern "C" int Combo_MM_SoloEntranceBossEnabled(void) { return 0; }
extern "C" int Combo_MM_SoloEntranceTransition(int32_t*) { return 0; }
extern "C" void Combo_MM_SoloEntranceRequestBossDeathRecovery(void) {}
extern "C" void Combo_MM_SoloEntranceRestoreCheckpoint(int32_t) {}
extern "C" int32_t Combo_MM_SoloEntranceSavedCheckpoint(void) { return -1; }
