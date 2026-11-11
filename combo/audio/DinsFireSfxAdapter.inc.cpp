// Included once per host after engine headers and CW_DINS_AUDIO_MM selection.
#include "DinsFireSfx.h"
#include "DinsFireSfxMixer.h"
#include "DinsFireSfxResources.inc.cpp"
#include <libultraship/bridge/consolevariablebridge.h>

namespace {
CwDinsAudio::Mixer sDinsAudio;
CwDinsAudio::Context DinsContext(PlayState* play) {
    CwDinsAudio::Context result;
    result.play = play;
    if (!play)
        return result;
    Player* player = GET_PLAYER(play);
#ifdef CW_DINS_AUDIO_MM
    result.scene = play->sceneId;
    result.paused = IS_PAUSED(&play->pauseCtx) || play->pauseCtx.bombersNotebookOpen;
    const bool healthy = gSaveContext.save.saveInfo.playerData.health > 0;
    const float fallbackMaster = std::clamp(CVarGetInteger("gSettings.Volume.Master", 40), 0, 100) / 100.0f;
    const float fallbackSfx = std::clamp(CVarGetInteger("gSettings.Volume.SFX", 100), 0, 100) / 100.0f;
    result.gain = CwDinsAudio::ClampGain(CVarGetFloat("gSettings.Audio.MasterVolume", fallbackMaster)) *
                  CwDinsAudio::ClampGain(CVarGetFloat("gSettings.Audio.SoundEffectsVolume", fallbackSfx));
    const auto& at = play->view.at;
#else
    result.scene = play->sceneNum;
    result.paused = play->pauseCtx.state || play->pauseCtx.debugState;
    const bool healthy = gSaveContext.health > 0;
    result.gain = std::clamp(CVarGetInteger("gSettings.Volume.Master", 40), 0, 100) / 100.0f *
                  (std::clamp(CVarGetInteger("gSettings.Volume.SFX", 100), 0, 100) / 100.0f);
    const auto& at = play->view.lookAt;
#endif
    result.live = play->state.running && player && player->actor.update && healthy &&
                  gSaveContext.gameMode == GAMEMODE_NORMAL && play->gameOverCtx.state == GAMEOVER_INACTIVE &&
                  !play->transitionTrigger && !play->transitionMode;
    result.eye = { play->view.eye.x, play->view.eye.y, play->view.eye.z };
    result.at = { at.x, at.y, at.z };
    return result;
}
} // namespace
extern "C" uint32_t DinsFireSfx_Begin(PlayState* play, float x, float y, float z) {
    const auto context = DinsContext(play);
    const CwDinsAudio::Point position{ x, y, z };
    sDinsAudio.Publish(context);
    if (!context.live || context.paused || !context.play || !CwDinsAudio::Finite(position))
        return 0;
    // Archive I/O and decoding happen on this game-thread path before Begin
    // acquires the mixer lock. Invalid casts never consult the resource system.
    return sDinsAudio.Begin(play, position, DinsLoadOverrideBank());
}
extern "C" uint32_t DinsFireSfx_Attach(PlayState* play, const Actor* actor) {
    return sDinsAudio.Attach(play, actor);
}
extern "C" void DinsFireSfx_Contact(uint32_t id) {
    sDinsAudio.Contact(id);
}
extern "C" void DinsFireSfx_Release(uint32_t id) {
    sDinsAudio.Release(id);
}
extern "C" void DinsFireSfx_UpdateActor(uint32_t id, float x, float y, float z, int frame) {
    sDinsAudio.Update(id, { x, y, z }, frame);
}
extern "C" void DinsFireSfx_CancelCast(uint32_t id) {
    sDinsAudio.Cancel(id);
}
extern "C" void DinsFireSfx_Stop(uint32_t id) {
    sDinsAudio.Stop(id);
}
extern "C" void DinsFireSfx_UpdateContext(PlayState* play) {
    sDinsAudio.Publish(DinsContext(play));
}
extern "C" void DinsFireSfx_MixInto(int16_t* output, uint32_t frames) {
    sDinsAudio.Mix(output, frames);
}
