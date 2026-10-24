#ifdef COMBO_BUILD
#include "global.h"
#include "soh/frame_interpolation.h"
#include "actors/RegionalPassagePresentation.h"
#include "soh/Enhancements/Gameplay/ComboRegionalPassages.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/randomizer/NeiGiRender.h"
#include "soh/ShipInit.hpp"
#include "mods/transformation_masks/assets/mm_asset_loader.h"

namespace {
// Display-list macros redeclare these helpers at block scope in this namespace.
extern "C" void FrameInterpolation_RecordOpenChild(const void* a, int b);
extern "C" void FrameInterpolation_RecordCloseChild(void);
namespace RP = RegionalPassagePresentation;
// The audio bank retains this address. It belongs to the module, never a
// stack-local point or an actor that can be freed before an audio callback.
Vec3f sWaveEmitter{};
bool sWavesPlaying = false;

bool RegionalPresentationSafe(PlayState* play) {
    return play && play->state.running && play->state.gfxCtx && play->pauseCtx.state == 0 &&
           play->csCtx.state == CS_STATE_IDLE && play->transitionTrigger == TRANS_TRIGGER_OFF &&
           play->transitionMode == TRANS_MODE_OFF && play->gameOverCtx.state == GAMEOVER_INACTIVE;
}

void DrawRegionalShell(PlayState* play, int route) {
    static const auto vertices = RP::NativeShell<Vtx>();
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gSPComboRMPush(POLY_OPA_DISP++, "oot");
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPClearGeometryMode(POLY_OPA_DISP++, G_LIGHTING | G_CULL_BOTH);
    gSPTexture(POLY_OPA_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEI, G_CC_MODULATEI);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
    gDPLoadTextureBlock(POLY_OPA_DISP++, RP::Texture(RP::Host::Oot, route), G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0,
                        G_TX_WRAP, G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD);
    for (size_t i = 0; i < RP::kShellQuads; ++i) {
        gSPVertex(POLY_OPA_DISP++, (uintptr_t)&vertices[i * 4], 4, 0);
        gSP2Triangles(POLY_OPA_DISP++, 0, 1, 2, 0, 0, 2, 3, 0);
    }
    gDPPipeSync(POLY_OPA_DISP++);
    gSPComboRMPop(POLY_OPA_DISP++);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}

void DrawRegionalPassage() {
    auto* play = gPlayState;
    int route;
    Vec3f anchor;
    s16 yaw;
    if (!RegionalPresentationSafe(play) || !ComboRegionalPassagesOot_Get(play, &route, &anchor, &yaw) ||
        !RP::KnownRoute(route))
        return;
    const bool grave = route == COMBO_PORTAL_IKANA;
    if (!grave && !NeiGi_CanDrawLayers(play, 1, RP::kShellOpaCommands, RP::kShellXluCommands))
        return;
    Matrix_Push();
    Matrix_Translate(anchor.x, anchor.y, anchor.z, MTXMODE_NEW);
    Matrix_RotateY(yaw * (NeiGi::Tau / 65536.f), MTXMODE_APPLY);
    if (!grave)
        DrawRegionalShell(play, route);
    const auto camera = NeiGi_CameraBasis(play);
    const auto mesh =
        grave ? RP::GraveShimmer(play->gameplayFrames, camera) : RP::Shimmer(play->gameplayFrames, route, camera);
    NeiGi_DrawWorldParticles(play, mesh);
    Matrix_Pop();
}

void StopRegionalWaves() {
    if (sWavesPlaying)
        MmSfx_StopAtPos(RP::kContinuousWave, &sWaveEmitter);
    sWavesPlaying = false;
}

void UpdateRegionalWaves() {
    auto* play = gPlayState;
    int route;
    Vec3f anchor;
    s16 yaw;
    if (!RegionalPresentationSafe(play) || !ComboRegionalPassagesOot_Get(play, &route, &anchor, &yaw)) {
        StopRegionalWaves();
        return;
    }
    auto* player = GET_PLAYER(play);
    if (!player) {
        StopRegionalWaves();
        return;
    }
    const float dx = player->actor.world.pos.x - anchor.x, dz = player->actor.world.pos.z - anchor.z;
    if (!RP::WavesInRange(route, dx * dx + dz * dz, player->actor.world.pos.y - anchor.y) || !MmSfx_IsAvailable()) {
        StopRegionalWaves();
        return;
    }
    float inverseW;
    Actor_ProjectPos(play, &anchor, &sWaveEmitter, &inverseW);
    // Native MM gAudioEnvironmentalSfx uses NA_SE_EV_WAVE - SFX_FLAG.
    // Refresh one spatial emitter, without touching BGM or room ambience.
    if (!MmSfx_PlayAtPos(RP::kContinuousWave, &sWaveEmitter)) {
        StopRegionalWaves();
        return;
    }
    sWavesPlaying = true;
}

void RegisterRegionalPresentation() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawEnd>([] {
        DrawRegionalPassage();
        UpdateRegionalWaves();
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s16) { StopRegionalWaves(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(StopRegionalWaves);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnExitGame>([](int32_t) { StopRegionalWaves(); });
}
RegisterShipInitFunc init(RegisterRegionalPresentation);
} // namespace
#endif
