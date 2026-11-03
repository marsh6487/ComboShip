#include "../../../../combo/audio/NightMusic.h"
#include "AudioCollection.h"
#include "AudioEditor.h"
#include "2s2h/ShipInit.hpp"
#include "2s2h/GameInteractor/GameInteractor.h"
#include <libultraship/bridge/consolevariablebridge.h>
extern "C" {
#include "global.h"
extern PlayState* gPlayState;
void Audio_InvalidateSceneSequence(void);
}

static ComboNightMusic::Lifecycle sSceneNight;
static void ResetSceneNightMusic() {
    sSceneNight.Reset();
}
static bool IsPlaying() {
    return sSceneNight.Owns() && AudioSeq_GetActiveSeqId(SEQ_PLAYER_BGM_MAIN) == sSceneNight.Playback();
}
uint16_t SceneNightMusic_CurrentEntry() {
    return IsPlaying() ? sSceneNight.Source() : 0;
}
bool SceneNightMusic_RequestRefresh() {
    sSceneNight.Refresh();
    return sSceneNight.Owns();
}
static void UpdateSceneNightMusic() {
    PlayState* play = gPlayState;
    if (!play) {
        sSceneNight.Reset();
        return;
    }
    const auto main = AudioSeq_GetActiveSeqId(SEQ_PLAYER_BGM_MAIN);
    const auto source = play->sceneSequences.seqId;
    const bool pending = !AudioSeq_IsSeqCmdNotQueued(SEQ_PLAYER_BGM_MAIN << 24, 0xF0000000 | 0x0F000000);
    const bool playing = IsPlaying();
    uint16_t row = source;
#ifdef COMBO_BUILD
    if (const auto sceneRow = AudioEditor_GetSceneMusicRow(source))
        row = sceneRow;
#endif
    const bool blocked =
        play->csCtx.state != CS_STATE_IDLE || Player_InCsMode(play) || CutsceneManager_GetCurrentCsId() != CS_ID_NONE ||
        gSaveContext.forcedSeqId != NA_BGM_GENERAL_SFX || CVarGetInteger("gAudioEditor.Playing", 0) != 0 ||
        AudioSeq_GetActiveSeqId(SEQ_PLAYER_FANFARE) != NA_BGM_DISABLED ||
        AudioSeq_GetActiveSeqId(SEQ_PLAYER_BGM_SUB) != NA_BGM_DISABLED || Environment_IsFinalHours(play) || pending;
    const auto action = sSceneNight.Update({
        .scene = play->sceneId,
        .source = row,
        .enabled = CVarGetInteger("gAudioEditor.EnableNightMusic", 0) != 0,
        .night = CURRENT_TIME < CLOCK_TIME(6, 0) || CURRENT_TIME > CLOCK_TIME(17, 10),
        .eligible = gSaveContext.gameMode == GAMEMODE_NORMAL && gSaveContext.fileNum < 3 && source != NA_BGM_NO_MUSIC &&
                    play->sceneSequences.ambienceId != AMBIENCE_ID_13,
        .blocked = blocked,
        .canStart = main == NA_BGM_DISABLED || playing,
        .playing = playing,
    });
    if (action == ComboNightMusic::Action::Start) {
        // Keep the native request identity, clock-town/day IO and single replacement lookup.
        // This does not touch SEQ_PLAYER_AMBIENCE.
        u8 day = gSaveContext.save.day > 0 && gSaveContext.save.day <= 3 ? gSaveContext.save.day - 1 : 0;
        Audio_InvalidateSceneSequence();
        Audio_PlaySceneSequence(source, day);
        sSceneNight.DidStart(source);
    } else if (action == ComboNightMusic::Action::Stop) {
        SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 10);
    }
}
static void RegisterSceneNightMusic() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateUpdate>(UpdateSceneNightMusic);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(ResetSceneNightMusic);
}
static RegisterShipInitFunc initSceneNight(RegisterSceneNightMusic);
