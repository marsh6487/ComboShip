#pragma once

#include "ComboMmKeyRingPolicy.h"
#include <array>
#include <cmath>

extern "C" {
bool NeiGi_CanDrawLayers(PlayState* play, size_t matrices, size_t opa, size_t xlu);
void NeiGi_ReportBlackMarketModel(const char* owner, const char* opaque, const char* translucent);
void NeiGi_ReportBlackMarketVertices(const Vtx* vertices, size_t count);
}

// TU glue: native engine/GBI headers precede this header. Both hosts execute
// the same geometry and pose; only their native matrix/setup entrypoints differ.
namespace ComboMmKeyRing {
// OPEN_DISPS declares these inside the current namespace. Bind those native
// marker declarations to the hosts' existing C symbols rather than inventing
// namespaced C++ symbols that pass object compilation but fail at link time.
extern "C" {
void FrameInterpolation_RecordOpenChild(const void*, int);
void FrameInterpolation_RecordCloseChild(void);
}
inline void Setup(GraphicsContext* graphics) {
#ifdef COMBO_MM_KEY_RING_HOST_OOT
    Gfx_SetupDL_25Opa(graphics);
#else
    Gfx_SetupDL25_Opa(graphics);
#endif
}
inline void Rotate(float angle, Vec3f* axis) {
#ifdef COMBO_MM_KEY_RING_HOST_OOT
    Matrix_RotateAxis(angle, axis, MTXMODE_APPLY);
#else
    Matrix_RotateAxisF(angle, axis, MTXMODE_APPLY);
#endif
}
inline Gfx* LoadMatrix(Gfx* packet, GraphicsContext* graphics) {
#ifdef COMBO_MM_KEY_RING_HOST_OOT
    gSPMatrix(packet, Matrix_NewMtx(graphics, (char*)__FILE__, __LINE__), G_MTX_MODELVIEW | G_MTX_LOAD);
#else
    MATRIX_FINALIZE_AND_LOAD(packet, graphics);
#endif
    return packet + 1;
}

// Native main-ring frame, measured from the existing mesh. The thin connector
// avoids intersecting the emblems that fill the authored keys' metal apertures.
inline constexpr float kGrid = 16.0f;
inline constexpr int kSegments = 24, kSides = 4, kBatchSegments = 4;
inline constexpr int kBatchVertices = (kBatchSegments + 1) * kSides;
inline constexpr int kBatches = kSegments / kBatchSegments;
inline const std::array<Vtx, kBatches * kBatchVertices>& Connector() {
    static const auto vertices = [] {
        std::array<Vtx, kBatches * kBatchVertices> result{};
        constexpr double pi = 3.14159265358979323846;
        constexpr double d[3] = { .909376957512118, -.382015475576536, -.1646138680853972 };
        constexpr double a[3] = { .04550093073105725, -.30200617759954484, .9522194778486311 };
        constexpr double n[3] = { .4134769817639321, .8734165358588908, .2572553992429386 };
        constexpr double c[3] = { 17.79921848970371, 7.019537631305841, -4.821428619290147 };
        for (int batch = 0; batch < kBatches; ++batch) {
            for (int row = 0; row <= kBatchSegments; ++row) {
                const double t = 2 * pi * (batch * kBatchSegments + row) / kSegments;
                const double ct = std::cos(t), st = std::sin(t);
                const double length = std::sqrt(ct * ct / 64.0 + st * st / 100.0);
                for (int side = 0; side < kSides; ++side) {
                    const double p = 2 * pi * side / kSides;
                    auto& vertex = result[batch * kBatchVertices + row * kSides + side];
                    for (int axis = 0; axis < 3; ++axis) {
                        const double outward = (d[axis] * ct / 8.0 + a[axis] * st / 10.0) / length;
                        const double normal = outward * std::cos(p) + n[axis] * std::sin(p);
                        const double position = c[axis] + 8 * ct * d[axis] + 10 * st * a[axis] + .22 * normal;
                        vertex.v.ob[axis] = static_cast<short>(std::lround(position * kGrid));
                        vertex.v.cn[axis] = static_cast<uint8_t>(static_cast<int8_t>(std::lround(normal * 127)));
                    }
                    vertex.v.cn[3] = 255;
                }
            }
        }
        return result;
    }();
    return vertices;
}

inline void DrawConnector(PlayState* play, const uint8_t* color) {
    const auto& vertices = Connector();
    Matrix_Push();
    Matrix_Scale(1.0f / kGrid, 1.0f / kGrid, 1.0f / kGrid, MTXMODE_APPLY);
    NeiGi_ReportBlackMarketVertices(vertices.data(), vertices.size());
    OPEN_DISPS(play->state.gfxCtx);
    Setup(play->state.gfxCtx);
    gSPGrayscale(POLY_OPA_DISP++, false);
    gSPTexture(POLY_OPA_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    gSPClearGeometryMode(POLY_OPA_DISP++, G_FOG | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gSPSetGeometryMode(POLY_OPA_DISP++, G_LIGHTING | G_SHADE | G_SHADING_SMOOTH);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, color[0], color[1], color[2], 255);
    gDPSetCombineLERP(POLY_OPA_DISP++, SHADE, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, SHADE, 0, PRIMITIVE, 0, 0, 0, 0,
                      PRIMITIVE);
    POLY_OPA_DISP = LoadMatrix(POLY_OPA_DISP, play->state.gfxCtx);
    for (int batch = 0; batch < kBatches; ++batch) {
        gSPVertex(POLY_OPA_DISP++, reinterpret_cast<uintptr_t>(vertices.data() + batch * kBatchVertices),
                  kBatchVertices, 0);
        for (int row = 0; row < kBatchSegments; ++row) {
            for (int side = 0; side < kSides; ++side) {
                const int next = (side + 1) % kSides;
                const int v0 = row * kSides + side, v1 = row * kSides + next;
                const int v2 = (row + 1) * kSides + next, v3 = (row + 1) * kSides + side;
                gSP2Triangles(POLY_OPA_DISP++, v0, v1, v2, 0, v0, v2, v3, 0);
            }
        }
    }
    gDPPipeSync(POLY_OPA_DISP++);
    Setup(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
    Matrix_Pop();
}

inline bool Draw(PlayState* play, int owner, const char* metal, const char* emblem, const char* ring,
                 const uint8_t* metalColor, const uint8_t* emblemColor, const uint8_t* ringColor) {
    const int count = ComboMmKeyRing_KeyCount(owner);
    if (!play || count == 0 || !metal || !emblem || !ring || !metalColor || !emblemColor || !ringColor ||
        !NeiGi_CanDrawLayers(play, 1 + 2 * count, 16 + 140 * count, 2))
        return false;
    static Vec3f axis = { .0455009307f, -.3020061776f, .9522194778f };
    static Vec3f keyAxis = { .1274444974f, -.1362319999f, .9824452872f };
    static constexpr float anchor[4][2] = { { 24.5f, 24.5f }, { 57, 31 }, { 37.5f, 61.75f }, { 21, 21.25f } };
    OPEN_DISPS(play->state.gfxCtx);
    Setup(play->state.gfxCtx);
    gSPGrayscale(POLY_OPA_DISP++, false);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x80, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_OPA_DISP++, ringColor[0], ringColor[1], ringColor[2], 255);
    NeiGi_ReportBlackMarketModel("mm", ring, nullptr);
    POLY_OPA_DISP = LoadMatrix(POLY_OPA_DISP, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)ring);
    CLOSE_DISPS(play->state.gfxCtx);
    for (int i = 0; i < count; ++i) {
        Matrix_Push();
        Matrix_Translate(-2.012f, 16.440f, -4.871f, MTXMODE_APPLY);
        // Stone Tower's broad head needs a wider fan than Snowhead's key.
        const float spacing = owner == 3 ? .8203047484f : .6981317008f;
        Rotate((i - (count - 1) * .5f) * spacing, &axis);
        Matrix_Translate(2.012f, -16.440f, 4.871f, MTXMODE_APPLY);
        DrawConnector(play, metalColor);
        Matrix_Translate(24.87564391f, 3.198162797f, -3.221023161f, MTXMODE_APPLY);
        Rotate(1.952986905f, &keyAxis);
        Matrix_Scale(.125f, .125f, .125f, MTXMODE_APPLY);
        Matrix_Translate(-anchor[owner][0], -anchor[owner][1], 0, MTXMODE_APPLY);
        OPEN_DISPS(play->state.gfxCtx);
        Setup(play->state.gfxCtx);
        gSPGrayscale(POLY_OPA_DISP++, false);
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x80, 255, 255, 255, 255);
        gDPSetEnvColor(POLY_OPA_DISP++, metalColor[0], metalColor[1], metalColor[2], 255);
        NeiGi_ReportBlackMarketModel("mm", metal, emblem);
        POLY_OPA_DISP = LoadMatrix(POLY_OPA_DISP, play->state.gfxCtx);
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)metal);
        gDPPipeSync(POLY_OPA_DISP++);
        gDPSetEnvColor(POLY_OPA_DISP++, emblemColor[0], emblemColor[1], emblemColor[2], 255);
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)emblem);
        CLOSE_DISPS(play->state.gfxCtx);
        Matrix_Pop();
    }
    OPEN_DISPS(play->state.gfxCtx);
    Setup(play->state.gfxCtx);
    gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
    CLOSE_DISPS(play->state.gfxCtx);
    return true;
}
} // namespace ComboMmKeyRing
