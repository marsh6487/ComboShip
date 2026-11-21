#include "SavingEnhancements.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "BenPort.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include "2s2h/CustomMessage/CustomMessage.h"

extern "C" {
#include <variables.h>
#include <functions.h>
}

#define CVAR_AUTOSAVE_NAME "gEnhancements.Saving.Autosave"
#define CVAR_AUTOSAVE CVarGetInteger(CVAR_AUTOSAVE_NAME, 0)

static uint32_t autosaveInterval = 0;
static uint32_t iconTimer = 0;
static uint64_t currentTimestamp = 0;
static uint64_t lastSaveTimestamp = GetUnixTimestamp();

void DrawAutosaveIcon() {
    // 5 seconds (100 frames) of showing the owl save icon to signify autosave has happened.
    if (iconTimer != 0) {
        float opacity = 255.0;
        // Fade in icon
        if (iconTimer > 80) {
            opacity = 255.0 - (((iconTimer - 80.0) / 20.0) * 255);
            // Fade out icon
        } else if (iconTimer < 20) {
            opacity = (iconTimer / 20.0) * 255.0;
        }
        Interface_DrawAutosaveIcon(gPlayState, uint16_t(opacity));
        iconTimer--;
    }
}

static void WriteAutosave(PlayState* play, bool resetting) {
    const bool currentOwlSaveState = gSaveContext.save.isOwlSave;
    gSaveContext.save.isOwlSave = true;
    SavingEnhancements_PersistSaveEntranceInfo();
    SavingEnhancements_AdvancePlaytime();
    Play_SaveCycleSceneFlags(play);
    gSaveContext.save.saveInfo.playerData.savedSceneId = play->sceneId;
    func_8014546C(&play->sramCtx);

    const int flashSlot = gSaveContext.fileNum * FLASH_SAVE_MAIN_MULTIPLIER;
    if (resetting) {
        // The game is about to leave PlayState. Finish the write without starting the
        // in-game save timer. Solo MM also writes the native backup page.
        SysFlashrom_WriteDataSync(play->sramCtx.saveBuf, gFlashOwlSaveStartPages[flashSlot],
                                  gFlashOwlSaveNumPages[flashSlot]);
        if (!Combo_IsCombinedGame()) {
            SysFlashrom_WriteDataSync(play->sramCtx.saveBuf, gFlashOwlSaveStartPages[flashSlot] + 0x80,
                                      gFlashOwlSaveNumPages[flashSlot]);
        }
    } else {
        Sram_SetFlashPagesOwlSave(&play->sramCtx, gFlashOwlSaveStartPages[flashSlot], gFlashOwlSaveNumPages[flashSlot]);
        Sram_StartWriteToFlashOwlSave(&play->sramCtx);
    }
    gSaveContext.save.isOwlSave = currentOwlSaveState;
    SavingEnhancements_ClearSaveEntranceInfo();
}

extern "C" void SavingEnhancements_AutosaveOnReset() {
    PlayState* play = gPlayState;
    if (!CVAR_AUTOSAVE || !CVarGetInteger("gEnhancements.Saving.RememberSaveLocation", 0) || !play ||
        gGameState != &play->state || !play->state.running || gSaveContext.gameMode != GAMEMODE_NORMAL ||
        gSaveContext.fileNum < 0 || gSaveContext.fileNum > 2 || !play->sramCtx.saveBuf ||
        play->gameOverCtx.state != GAMEOVER_INACTIVE || play->transitionMode != TRANS_MODE_OFF ||
        play->transitionTrigger != TRANS_TRIGGER_OFF || !SavingEnhancements_CanSave())
        return;

#ifdef COMBO_BUILD
    if (Combo_IsCombinedGame())
        Combo_MM_FlushAccumulators();
#endif
    WriteAutosave(play, true);
}

void HandleAutoSave() {
    // Check if the interval has passed in minutes.
    autosaveInterval = CVarGetInteger("gEnhancements.Saving.AutosaveInterval", 5) * 60000;
    currentTimestamp = GetUnixTimestamp();
    if ((currentTimestamp - lastSaveTimestamp) < autosaveInterval) {
        return;
    }

    Player* player = GET_PLAYER(gPlayState);
    if (player == NULL) {
        return;
    }

    // Time must be moving for autosave to work. Fixes an issue with Time Moves When You Move.
    if (gSaveContext.save.timeSpeedOffset == -R_TIME_SPEED && R_TIME_SPEED != 0) {
        return;
    }

    // If owl save available to create, do it and reset the interval.
    if (SavingEnhancements_CanSave() && gPlayState->pauseCtx.state == 0) {

        // Reset timestamp, set icon timer to show autosave icon for 5 seconds (100 frames)
        lastSaveTimestamp = GetUnixTimestamp();
        iconTimer = 100;

        WriteAutosave(gPlayState, false);
    }
}

static RegisterShipInitFunc registerAutosave(
    []() {
        COND_HOOK(OnGameStateUpdate, CVAR_AUTOSAVE, []() {
            if (gPlayState == nullptr) {
                return;
            }

            HandleAutoSave();
        });

        COND_HOOK(OnGameStateDrawFinish, CVAR_AUTOSAVE, []() {
            if (gPlayState == nullptr) {
                return;
            }

            DrawAutosaveIcon();
        });
    },
    { CVAR_AUTOSAVE_NAME });
