#ifndef SAVING_ENHANCEMENTS_H
#define SAVING_ENHANCEMENTS_H

#include <stdbool.h>

struct PlayState;

#ifdef __cplusplus
extern "C" {
#endif

void SavingEnhancements_PersistSaveEntranceInfo();
void SavingEnhancements_ClearSaveEntranceInfo();
bool SavingEnhancements_CanSave();
void SavingEnhancements_AdvancePlaytime();
void SavingEnhancements_WriteOwlSnapshot(struct PlayState* play, bool immediate);
void SavingEnhancements_AutosaveOnReset();

#ifdef __cplusplus
}
#endif

#endif // SAVING_ENHANCEMENTS_H
