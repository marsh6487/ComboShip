#include "BetterSaveMenu.h"
#include "SavingEnhancements.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
#include "functions.h"
#include "z64horse.h"

extern s32 gSceneEntranceTableCount;
extern SceneEntranceTableEntry sSceneEntranceTable[];
}

static s32 BetterSaveMenu_GetSceneEntrance(const PlayState* play) {
    // save.entrance is the resolved arrival, including the spawn chosen by a
    // shuffled entrance. The low nibble selects a header, not a spawn.
    const s32 entrance = gSaveContext.save.entrance & ~0xF;
    const u32 scene = static_cast<u32>(entrance) >> 9;
    const u32 spawn = (entrance >> 4) & 0x1F;
    if (scene >= static_cast<u32>(gSceneEntranceTableCount)) {
        return -1;
    }
    const auto& entrances = sSceneEntranceTable[scene];
    if (!entrances.table || spawn >= static_cast<u32>(entrances.tableCount) || !entrances.table[spawn] ||
        Entrance_GetSceneIdAbsolute(entrance) != play->sceneId) {
        return -1;
    }
    return entrance;
}

static void BetterSaveMenu_OpenPrompt(PlayState* play, const char* text) {
    CustomMessage::SetActiveCustomMessage(text, {});
    Message_StartTextbox(play, CUSTOM_MESSAGE_ID, nullptr);
    play->msgCtx.choiceIndex = 0;
}

static bool BetterSaveMenu_CanOpen(PlayState* play) {
    return CVarGetInteger("gEnhancements.Saving.BetterSaveMenu", 0) && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           gSaveContext.fileNum >= 0 && gSaveContext.fileNum <= 2 && play->sramCtx.saveBuf != nullptr &&
           play->sramCtx.status == 0 && play->gameOverCtx.state == GAMEOVER_INACTIVE &&
           play->transitionTrigger == TRANS_TRIGGER_OFF && play->transitionMode == TRANS_MODE_OFF &&
           play->pauseCtx.debugEditor == DEBUG_EDITOR_NONE && !play->pauseCtx.itemDescriptionOn &&
           SavingEnhancements_CanSave();
}

extern "C" MmBetterSaveMenuResult BetterSaveMenu_Update(PlayState* play) {
    PauseContext* pause = &play->pauseCtx;
    if (pause->state != PAUSE_STATE_MAIN) {
        return MM_BETTER_SAVE_INACTIVE;
    }

    const u16 buttons = CONTROLLER1(&play->state)->press.button;
    u16& menu = pause->savePromptState;
    if (menu == PAUSE_SAVEPROMPT_STATE_BETTER_SAVING) {
        if (play->sramCtx.status == 0 && play->msgCtx.msgMode == MSGMODE_NONE) {
            BetterSaveMenu_OpenPrompt(play,
                                      BetterSaveMenu_GetSceneEntrance(play) >= 0
                                          ? "Game saved. Continue playing?\n%gContinue\nReturn to Scene Entrance\n"
                                            "Return to Clock Town\xC3"
                                          : "Game saved. Continue playing?\n%gContinue\nReturn to Clock Town\xC2");
            menu = PAUSE_SAVEPROMPT_STATE_BETTER_DESTINATION;
        }
        return MM_BETTER_SAVE_HANDLED;
    }

    if (menu != PAUSE_SAVEPROMPT_STATE_BETTER_CONFIRM && menu != PAUSE_SAVEPROMPT_STATE_BETTER_DESTINATION) {
        if (!(buttons & BTN_B) ||
            (pause->mainState != PAUSE_MAIN_STATE_IDLE && pause->mainState != PAUSE_MAIN_STATE_IDLE_CURSOR_ON_SONG &&
             pause->mainState != PAUSE_MAIN_STATE_SONG_PROMPT) ||
            !BetterSaveMenu_CanOpen(play)) {
            return MM_BETTER_SAVE_INACTIVE;
        }
        if (pause->mainState != PAUSE_MAIN_STATE_IDLE) {
            AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
        }
        pause->mainState = PAUSE_MAIN_STATE_IDLE;
        Interface_SetAButtonDoAction(play, DO_ACTION_NONE);
        Audio_PlaySfx_MessageDecide();
        BetterSaveMenu_OpenPrompt(play, "Would you like to save?\n%gYes\nNo\xC2");
        menu = PAUSE_SAVEPROMPT_STATE_BETTER_CONFIRM;
        return MM_BETTER_SAVE_HANDLED;
    }

    if (play->msgCtx.currentTextId != CUSTOM_MESSAGE_ID || play->msgCtx.msgMode == MSGMODE_NONE) {
        menu = PAUSE_SAVEPROMPT_STATE_APPEARING;
        return MM_BETTER_SAVE_CLOSE_PAUSE;
    }
    if (buttons & (BTN_START | BTN_B)) {
        Message_CloseTextbox(play);
        menu = PAUSE_SAVEPROMPT_STATE_APPEARING;
        return MM_BETTER_SAVE_CLOSE_PAUSE;
    }
    if (Message_GetState(&play->msgCtx) != TEXT_STATE_CHOICE || !Message_ShouldAdvance(play)) {
        return MM_BETTER_SAVE_HANDLED;
    }

    const u8 choice = play->msgCtx.choiceIndex;
    const bool showScene = play->msgCtx.textboxEndType == TEXTBOX_ENDTYPE_THREE_CHOICE;
    Message_CloseTextbox(play);
    if (menu == PAUSE_SAVEPROMPT_STATE_BETTER_CONFIRM && choice == 0) {
        Audio_PlaySfx(NA_SE_SY_PIECE_OF_HEART);
        SavingEnhancements_WriteOwlSnapshot(play, false);
        menu = PAUSE_SAVEPROMPT_STATE_BETTER_SAVING;
        return MM_BETTER_SAVE_HANDLED;
    }

    if (menu == PAUSE_SAVEPROMPT_STATE_BETTER_DESTINATION && choice != 0) {
        if (showScene && choice == 1) {
            menu = PAUSE_SAVEPROMPT_STATE_BETTER_SCENE;
            return MM_BETTER_SAVE_CLOSE_PAUSE;
        }
        if (choice == (showScene ? 2 : 1)) {
            menu = PAUSE_SAVEPROMPT_STATE_BETTER_CLOCK_TOWN;
            return MM_BETTER_SAVE_CLOSE_PAUSE;
        }
    }
    menu = PAUSE_SAVEPROMPT_STATE_APPEARING;
    return MM_BETTER_SAVE_CLOSE_PAUSE;
}

extern "C" void BetterSaveMenu_OnUnpause(PlayState* play) {
    u16& menu = play->pauseCtx.savePromptState;
    if (menu != PAUSE_SAVEPROMPT_STATE_BETTER_SCENE && menu != PAUSE_SAVEPROMPT_STATE_BETTER_CLOCK_TOWN) {
        return;
    }
    const s32 entrance = menu == PAUSE_SAVEPROMPT_STATE_BETTER_SCENE ? BetterSaveMenu_GetSceneEntrance(play)
                                                                     : ENTRANCE(SOUTH_CLOCK_TOWN, 0);
    menu = PAUSE_SAVEPROMPT_STATE_APPEARING;
    if (entrance < 0 || play->transitionMode != TRANS_MODE_OFF || play->transitionTrigger != TRANS_TRIGGER_OFF) {
        return;
    }

    Player* player = GET_PLAYER(play);
    if (gHorseIsMounted && player && player->rideActor && CHECK_QUEST_ITEM(QUEST_SONG_EPONA)) {
        auto& horse = gSaveContext.save.saveInfo.horseData;
        horse.sceneId = play->sceneId;
        horse.pos.x = player->rideActor->world.pos.x;
        horse.pos.y = player->rideActor->world.pos.y;
        horse.pos.z = player->rideActor->world.pos.z;
        horse.yaw = player->rideActor->shape.rot.y;
    }
    gHorseIsMounted = false;
    Play_SaveCycleSceneFlags(play);
    gSaveContext.respawnFlag = 0;
    gSaveContext.save.cutsceneIndex = 0;
    gSaveContext.nextCutsceneIndex = 0xFFEF;
    gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK;
    play->nextEntrance = entrance;
    play->transitionType = TRANS_TYPE_FADE_BLACK;
    play->transitionTrigger = TRANS_TRIGGER_START;
    menu = PAUSE_SAVEPROMPT_STATE_BETTER_TRANSITION;
}

extern "C" bool BetterSaveMenu_ConsumeDirectTransition(PlayState* play) {
    if (play->pauseCtx.savePromptState != PAUSE_SAVEPROMPT_STATE_BETTER_TRANSITION) {
        return false;
    }
    play->pauseCtx.savePromptState = PAUSE_SAVEPROMPT_STATE_APPEARING;
    return true;
}
