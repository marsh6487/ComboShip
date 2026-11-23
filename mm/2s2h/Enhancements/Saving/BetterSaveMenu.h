#ifndef MM_BETTER_SAVE_MENU_H
#define MM_BETTER_SAVE_MENU_H

#include <stdbool.h>

struct PlayState;

typedef enum MmBetterSaveMenuResult {
    MM_BETTER_SAVE_INACTIVE,
    MM_BETTER_SAVE_HANDLED,
    MM_BETTER_SAVE_CLOSE_PAUSE,
} MmBetterSaveMenuResult;

#ifdef __cplusplus
extern "C" {
#endif

MmBetterSaveMenuResult BetterSaveMenu_Update(struct PlayState* play);
void BetterSaveMenu_OnUnpause(struct PlayState* play);
bool BetterSaveMenu_ConsumeDirectTransition(struct PlayState* play);

#ifdef __cplusplus
}
#endif

#endif
