#pragma once

#ifdef COMBO_BUILD
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "entrances/SoloForest.h"

extern "C" s16 sSceneCutsceneCount;

// Exploration headers omit the prologue's actor-cutscene list and spawn chain.
// Supply just its native unscripted ocarina entry before Player_Init assigns IDs.
static inline void ComboForestOcarina_PrepareScene(PlayState* play) {
    if (!play || !play->linkActorEntry || play->curSpawn != 0 || sSceneCutsceneCount != 0 ||
        (play->linkActorEntry->rot.y & 0x7F) != 0x7F)
        return;
    const bool soloForest = ComboSoloForest_IsMmVisit(play->sceneId, 0, gSaveContext.gameMode, gSaveContext.fileNum,
                                                      gSaveContext.sceneLayer, gSaveContext.save.cutsceneIndex) &&
                            gSaveContext.save.entrance == COMBO_SOLO_FOREST_MM_ENTRANCE;
    if (!ComboOpeningForest_IsExploration(play) && !soloForest)
        return;

    static ActorEntry spawn;
    static ActorCutscene ocarina;
    spawn = *play->linkActorEntry;
    spawn.rot.y &= ~0x7F; // Keep packed heading; start this one-entry chain at zero.
    // Native Lost Woods entry zero, with the omitted successor chain terminated.
    ocarina = { 700,
                -1,
                CS_CAM_ID_GLOBAL_ITEM_OCARINA,
                CS_SCRIPT_ID_NONE,
                CS_ID_NONE,
                CS_END_SFX_NONE,
                255,
                CS_HUD_VISIBILITY_NONE,
                CS_END_CAM_0,
                27 };
    play->linkActorEntry = &spawn;
    CutsceneManager_Init(play, &ocarina, 1);
}
#endif
