#include "../../../../combo/audio/NightMusic.h"
#include "AudioCollection.h"
#include "AudioEditor.h"
#include "soh/ShipInit.hpp"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include <libultraship/bridge/consolevariablebridge.h>
extern "C" {
#include "functions.h"
#include "variables.h"
#include "night_bgm_bridge.h"
extern PlayState* gPlayState;
}

static ComboNightMusic::Lifecycle sSceneNight;
static bool IsPlaying() {
    return sSceneNight.Owns() && Audio_IsSceneNightBgmActive();
}
uint16_t SceneNightMusic_CurrentEntry() {
    return IsPlaying() ? sSceneNight.Source() : 0;
}
bool SceneNightMusic_RequestRefresh() {
    sSceneNight.Refresh();
    return sSceneNight.Owns();
}
static void ResetSceneNightMusic() {
    Audio_StopSceneNightBgm();
    sSceneNight.Reset();
    Audio_RegisterSceneNightBgm(NA_BGM_DISABLED);
}
static void UpdateSceneNightMusic() {
    PlayState* play = gPlayState;
    if (!play) {
        ResetSceneNightMusic();
        return;
    }
    const auto main = func_800FA0B4(SEQ_PLAYER_BGM_MAIN);
    const auto sub = func_800FA0B4(SEQ_PLAYER_BGM_SUB);
    const auto source = play->sequenceCtx.seqId;
    const bool nativePending = Audio_HasPendingSubBgmCommand() != 0;
    const bool combat = Audio_IsSceneNightMusicInCombat() != 0;
    const bool playing = IsPlaying() && !nativePending && !combat;
    const bool ranchSelection =
        play->sceneNum == SCENE_LON_LON_RANCH && CVarGetInteger("gAudioEditor.DisableRanchProximityMusic", 0) != 0;
    const bool dedicated =
        play->sceneNum == SCENE_HYRULE_FIELD && CVarGetInteger("gAudioEditor.HyruleFieldNightMusic", 0) != 0;
    const bool nativeMain = main == NA_BGM_NATURE_AMBIENCE || main == NA_BGM_DISABLED;
    const bool blocked = play->csCtx.state != CS_STATE_IDLE || gSaveContext.forcedSeqId != NA_BGM_GENERAL_SFX ||
                         CVarGetInteger("gAudioEditor.Playing", 0) != 0 ||
                         func_800FA0B4(SEQ_PLAYER_FANFARE) != NA_BGM_DISABLED || !nativeMain || nativePending || combat;
    uint16_t row = source;
#ifdef COMBO_BUILD
    if (const auto sceneRow = AudioEditor_GetSceneMusicRow(source))
        row = sceneRow;
#endif
    const auto action = sSceneNight.Update({
        .scene = play->sceneNum,
        .source = row,
        .enabled = CVarGetInteger("gAudioEditor.EnableNightMusic", 0) != 0 || ranchSelection,
        .night = gSaveContext.dayTime <= 0x4AAA || gSaveContext.dayTime >= 0xB71D,
        // Debug Warp uses file 0xFF; the game-mode guard still excludes title demos.
        .eligible = gSaveContext.gameMode == GAMEMODE_NORMAL &&
                    (gSaveContext.fileNum < 3 || gSaveContext.fileNum == 0xFF) && source != NA_BGM_NO_MUSIC &&
                    play->sequenceCtx.natureAmbienceId < NATURE_ID_NONE && !dedicated,
        .blocked = blocked,
        // Talking also sets Player_InCsMode. Defer starts/edits while controls
        // are locked; existing playback and dawn/disable cleanup still run.
        .canStart = !Player_InCsMode(play) && nativeMain && (sub == NA_BGM_DISABLED || playing),
        .playing = playing,
        .suspendWhenBlocked = true,
        .stopOnDay = true,
    });
    if (action == ComboNightMusic::Action::Start) {
        if (!AudioCollection::Instance->HasSequenceNum(row))
            return;
        const auto sequence = AudioCollection::Instance->GetReplacementSequence(row);
        if (!AudioCollection::Instance->HasSequenceNum(sequence))
            return;
        // Resolve exactly once. Nature and all its channel IO remain on MAIN.
        Audio_SetVolScale(SEQ_PLAYER_BGM_SUB, 3, 0x7F, 1);
        Audio_QueueResolvedSeqCmd(SEQ_PLAYER_BGM_SUB, sequence, 30);
        Audio_RegisterSceneNightBgm(sequence);
        sSceneNight.DidStart(sequence);
    } else if (action == ComboNightMusic::Action::Stop) {
        Audio_StopSceneNightBgm();
    } else if (!sSceneNight.Owns()) {
        Audio_RegisterSceneNightBgm(NA_BGM_DISABLED);
    }
}
static void RegisterSceneNightMusic() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(UpdateSceneNightMusic);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(ResetSceneNightMusic);
}
static RegisterShipInitFunc initSceneNight(RegisterSceneNightMusic);
