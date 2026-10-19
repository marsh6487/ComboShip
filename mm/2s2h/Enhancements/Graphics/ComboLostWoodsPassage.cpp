// A separate tree passage between the entry clearing's maze corridors.
// Drawn in the active MM resource manager, so the native bark can resolve through Alt Assets.
#ifdef COMBO_BUILD
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include "../../../../combo/entrances/LostWoodsEntrance.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "2s2h/Enhancements/Gameplay/ComboSoloForest.h"
#include "ComboForestPortalEffect.h"
#include <array>
#include <cmath>

#include "global.h"

namespace {
constexpr int kArchSegments = 10;
constexpr int kQuadCount = 2 + kArchSegments * 2 + 1;
std::array<Vtx, kQuadCount * 4> sVertices;

void Vertex(Vtx& vertex, float x, float y, float z, unsigned char shade) {
    vertex = {};
    vertex.v.ob[0] = (s16)x;
    vertex.v.ob[1] = (s16)y;
    vertex.v.ob[2] = (s16)z;
    vertex.v.tc[0] = (s16)(x * 32);
    vertex.v.tc[1] = (s16)(-y * 32);
    vertex.v.cn[0] = vertex.v.cn[1] = vertex.v.cn[2] = shade;
    vertex.v.cn[3] = 255;
}

void BuildPassage() {
    int index = 0;
    auto quad = [&](float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, float dx,
                    float dy, float dz, unsigned char shade) {
        Vertex(sVertices[index++], ax, ay, az, shade);
        Vertex(sVertices[index++], bx, by, bz, shade);
        Vertex(sVertices[index++], cx, cy, cz, shade);
        Vertex(sVertices[index++], dx, dy, dz, shade);
    };
    // Two roots form the jambs. The curved roof and its dark inner reveal form a short tunnel.
    quad(-68, 0, 0, -44, 0, 0, -44, 65, 0, -68, 65, 0, 200);
    quad(44, 0, 0, 68, 0, 0, 68, 65, 0, 44, 65, 0, 200);
    for (int i = 0; i < kArchSegments; ++i) {
        const float a = i * 3.14159265f / kArchSegments;
        const float b = (i + 1) * 3.14159265f / kArchSegments;
        const float ax = std::cos(a), ay = std::sin(a), bx = std::cos(b), by = std::sin(b);
        quad(ax * 44, 65 + ay * 44, 0, ax * 68, 65 + ay * 68, 0, bx * 68, 65 + by * 68, 0, bx * 44, 65 + by * 44, 0,
             200);
        quad(ax * 44, 65 + ay * 44, 0, bx * 44, 65 + by * 44, 0, bx * 44, 65 + by * 44, -45, ax * 44, 65 + ay * 44, -45,
             65);
    }
    // Black mouth, behind the frame. The native wall/collision beyond stays intact.
    quad(-44, 0, -2, 44, 0, -2, 44, 110, -2, -44, 110, -2, 0);
}
} // namespace

static void DrawPassage() {
    if (!Combo_IsCombinedGame() && !ComboSoloForestMm_PassageAvailable())
        return;
    if (ComboOpeningForest_UsesPackTunnel(gPlayState))
        return; // The active MM3D mesh owns the doorway; never draw a second arch.
    const bool openingForest = ComboOpeningForest_IsExploration(gPlayState) && gPlayState->roomCtx.curRoom.num == 0 &&
                               gPlayState->linkActorEntry;
    if (!openingForest && (!gPlayState || gSaveContext.gameMode != GAMEMODE_NORMAL ||
                           gPlayState->sceneId != SCENE_26SARUNOMORI || gPlayState->roomCtx.curRoom.num != 1))
        return;
    static const bool built = (BuildPassage(), true);
    (void)built;
    OPEN_DISPS(gPlayState->state.gfxCtx);
    Gfx_SetupDL25_Opa(gPlayState->state.gfxCtx);
    Matrix_Push();
    if (openingForest) {
        Matrix_Translate(COMBO_OPENING_FOREST_STUMP_X, 0.0f, COMBO_OPENING_FOREST_STUMP_Z, MTXMODE_NEW);
        Matrix_RotateYS(COMBO_OPENING_FOREST_STUMP_YAW, MTXMODE_APPLY);
    } else {
        Matrix_Translate(160.0f, 0.0f, -160.0f, MTXMODE_NEW);
        Matrix_RotateYS(-8192, MTXMODE_APPLY);
    }
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gPlayState->state.gfxCtx);
    gSPClearGeometryMode(POLY_OPA_DISP++, G_LIGHTING | G_CULL_BOTH);
    gSPTexture(POLY_OPA_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEI, G_CC_MODULATEI);
    gDPLoadTextureBlock(POLY_OPA_DISP++, "__OTR__scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0", G_IM_FMT_RGBA,
                        G_IM_SIZ_16b, 32, 32, 0, G_TX_WRAP, G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD);
    // Arrival remains in the clearing, away from the stump's black mouth.
    for (int i = 0; i < kQuadCount; ++i) {
        gSPVertex(POLY_OPA_DISP++, (uintptr_t)&sVertices[i * 4], 4, 0);
        gSP2Triangles(POLY_OPA_DISP++, 0, 1, 2, 0, 0, 2, 3, 0);
    }
    Matrix_Pop();
    gDPPipeSync(POLY_OPA_DISP++);
    Gfx_SetupDL25_Opa(gPlayState->state.gfxCtx);
    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

static void DrawPortalFinish() {
    PlayState* play = gPlayState;
    if (!ComboOpeningForest_IsExploration(play) || play->roomCtx.curRoom.num != 0 || !play->linkActorEntry ||
        play->csCtx.state != CS_STATE_IDLE)
        return;
    const int version = ComboOpeningForest_ReturnPackVersion(play);
    if (version == 1 || version == 2)
        return; // Older meshes retain their own visible location and treatment.
    using namespace ComboForestPortalEffect;
    std::array<ComboForestPortalEffect::Vertex, kVertexCount> shape;
    Build(play->gameplayFrames, version >= 3, shape);
    // Each submitted frame owns its vertices until its display lists finish.
    auto* vertices = static_cast<Vtx*>(GRAPH_ALLOC(play->state.gfxCtx, sizeof(Vtx) * kVertexCount));
    if (!vertices)
        return;
    for (int i = 0; i < kVertexCount; ++i) {
        vertices[i] = {};
        vertices[i].v.ob[0] = static_cast<s16>(shape[i].x);
        vertices[i].v.ob[1] = static_cast<s16>(shape[i].y);
        vertices[i].v.ob[2] = version >= 3 ? 10 : 1;
        vertices[i].v.cn[0] = shape[i].r;
        vertices[i].v.cn[1] = shape[i].g;
        vertices[i].v.cn[2] = shape[i].b;
        vertices[i].v.cn[3] = shape[i].a;
    }
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    Matrix_Push();
    const Vec3f anchor = ComboOpeningForest_ReturnAnchor(play);
    Matrix_Translate(anchor.x, anchor.y, anchor.z, MTXMODE_NEW);
    Matrix_RotateYS(ComboOpeningForest_ReturnYaw(play), MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING | G_FOG | G_CULL_BOTH);
    gSPTexture(POLY_XLU_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetRenderMode(POLY_XLU_DISP++, G_RM_PASS, G_RM_AA_ZB_XLU_SURF2);
    gDPSetCombineMode(POLY_XLU_DISP++, G_CC_SHADE, G_CC_SHADE);
    for (int i = 0; i < kRimSegments; ++i) {
        gSPVertex(POLY_XLU_DISP++, reinterpret_cast<uintptr_t>(&vertices[i * 4]), 4, 0);
        gSP2Triangles(POLY_XLU_DISP++, 0, 1, 2, 0, 0, 2, 3, 0);
    }
    for (int i = 0; i < kGlints; ++i) {
        gSPVertex(POLY_XLU_DISP++, reinterpret_cast<uintptr_t>(&vertices[kRimSegments * 4 + i * 5]), 5, 0);
        gSP2Triangles(POLY_XLU_DISP++, 0, 1, 2, 0, 0, 2, 3, 0);
        gSP2Triangles(POLY_XLU_DISP++, 0, 3, 4, 0, 0, 4, 1, 0);
    }
    Matrix_Pop();
    gDPPipeSync(POLY_XLU_DISP++);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}

namespace {
void RegisterPassage() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawWorldEnd>([] {
        DrawPassage();
        DrawPortalFinish();
    });
}
static RegisterShipInitFunc initFunc(RegisterPassage, {});
} // namespace
#endif
