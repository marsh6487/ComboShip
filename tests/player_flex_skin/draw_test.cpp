#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>
#include <vector>
#include "global.h"
#include "combo/actors/PlayerFlexSkin.h"
#include "combo/actors/PlayerFlexSkinPolicy.h"
static MtxF current;
static std::vector<MtxF> stack;
static std::map<Mtx*, MtxF> values;
static Mtx storage[256];
static size_t used, lastAllocation;
static int planFlag, failure, postCount;
static void** target;
static bool nested;
static PlayState* activePlay;
static void** nestedSkeleton;
static Vec3s* activeJoints;
static std::vector<Gfx*> commands;
static std::vector<uintptr_t> segments;
static Gfx rigid[1], weighted[1], equipment[1];
extern "C" {
#ifdef PLAYER_SKIN_MM
void SkelAnime_DrawFlexLodWithXlu(PlayState*, void**, Vec3s*, s32, OverrideLimbDrawFlex, PostLimbDrawFlex, Actor*, s32);
#else
void SkelAnime_DrawFlexLodWithXlu(PlayState*, void**, Vec3s*, s32, OverrideLimbDrawOpa, PostLimbDrawOpa, void*, s32);
const void* NativeNpcPose_GetSkinPlan(void**, s32) { return nullptr; }
s32 NativeNpcPose_SkinMatrixCount(const void*) { return 0; }
void NativeNpcPose_BlendSkinMatrix(const void*, s32, const MtxF*, MtxF*) { assert(false); }
float NativeNpcPose_GetDrawScale(void**) { return 1; }
#endif
const void* PlayerFlexSkin_GetPlan(void** skeleton, s32 count) {
    return planFlag && skeleton == target && count == 2 ? &planFlag : nullptr;
}
s32 PlayerFlexSkin_MatrixCount(const void* plan) { return plan ? 1 : 0; }
Gfx* PlayerFlexSkin_SelectLimb(const void* plan, s32, Gfx* selected, Gfx* original) {
    return plan && selected && selected == original ? weighted : selected;
}
void PlayerFlexSkin_BlendMatrix(const void*, s32 index, const MtxF* native, MtxF* out) {
    assert(index == 0);
    PlayerFlexSkin::Plan plan;
    plan.inverse.resize(2);
    for (auto& matrix : plan.inverse) matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1;
    plan.blends.push_back({0, 1, 8});
    if (failure == 1 || failure == 5) plan.inverse[0][0] = 1e7f;
    if (failure == 2) plan.inverse[0][0] = std::numeric_limits<float>::quiet_NaN();
    PlayerFlexSkin::Matrix matrices[2];
    std::memcpy(matrices[0].data(), &native[0].xx, sizeof(MtxF));
    std::memcpy(matrices[1].data(), &native[1].xx, sizeof(MtxF));
    auto result = PlayerFlexSkin::Blend(plan, 0, matrices);
    std::memcpy(&out->xx, result.data(), sizeof(MtxF));
}
void* Graph_Alloc(GraphicsContext*, size_t size) {
    assert(size % sizeof(Mtx) == 0 && used + size / sizeof(Mtx) <= 256);
    lastAllocation = size;
    auto* result = &storage[used]; used += size / sizeof(Mtx); return result;
}
#ifdef PLAYER_SKIN_MM
void Graph_OpenDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {}
void Graph_CloseDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {}
#else
void Graph_OpenDisps(Gfx**, GraphicsContext*, const char*, s32) {}
void Graph_CloseDisps(Gfx**, GraphicsContext*, const char*, s32) {}
#endif
void gSPSegment(void*, int segment, uintptr_t address) { assert(segment == 13); segments.push_back(address); }
void gSPDisplayList(Gfx* command, Gfx* display) {
    command->words.w1 = reinterpret_cast<uintptr_t>(display); commands.push_back(command);
}
void FrameInterpolation_RecordOpenChild(const void*, int) {}
void FrameInterpolation_RecordCloseChild(void) {}
void Matrix_Push(void) { stack.push_back(current); }
void Matrix_Pop(void) { assert(!stack.empty()); current = stack.back(); stack.pop_back(); }
void Matrix_Put(MtxF* matrix) { current = *matrix; }
void Matrix_TranslateRotateZYX(Vec3f* pos, Vec3s* rot) {
    assert(rot->x == 0 && rot->y == 0 && rot->z == 0);
    current.xw += pos->x; current.yw += pos->y; current.zw += pos->z;
}
#ifdef PLAYER_SKIN_MM
void Matrix_Scale(f32, f32, f32, MatrixMode) { assert(false); }
#else
void Matrix_Scale(f32, f32, f32, u8) { assert(false); }
#endif
void Matrix_MtxToMtxF(Mtx* matrix, MtxF* out) { assert(values.contains(matrix)); *out = values.at(matrix); }
#ifdef PLAYER_SKIN_MM
Mtx* Matrix_ToMtx(Mtx* matrix) {
#else
Mtx* Matrix_ToMtx(Mtx* matrix, char*, s32) {
#endif
    // If a bad extra matrix reaches native packing the safety guard failed.
    assert(std::isfinite(current.xw) && std::abs(current.xw) <= 32768);
    values[matrix] = current; return matrix;
}
int32_t CVarGetInteger(const char*, int32_t fallback) { return fallback; }
void osSyncPrintfUnused(const char*, ...) {}
void lusprintf(const char*, int32_t, int32_t, const char*, ...) {}
#ifndef PLAYER_SKIN_MM
s32 osRecvMesg(OSMesgQueue*, OSMesg*, s32) { assert(false); return 0; }
#else
void* Lib_SegmentedToVirtual(void* pointer) { return pointer; }
#endif
f32 Math_SinS(s16) { assert(false); return 0; }
f32 Math_CosS(s16) { assert(false); return 0; }
}

#ifdef PLAYER_SKIN_MM
static s32 Override(PlayState*, s32 index, Gfx** dl, Vec3f*, Vec3s*, Actor*) {
#else
static s32 Override(PlayState*, s32 index, Gfx** dl, Vec3f*, Vec3s*, void*) {
#endif
    if (index == 2 && (failure == 3 || failure == 5)) *dl = equipment;
    if (index == 2 && failure == 4) *dl = nullptr;
    return false;
}
#ifdef PLAYER_SKIN_MM
static void Post(PlayState*, s32 index, Gfx**, Gfx**, Vec3s*, Actor*) {
#else
static void Post(PlayState*, s32 index, Gfx**, Vec3s*, void*) {
#endif
    ++postCount;
    if (nested && index == 1)
        SkelAnime_DrawFlexLodWithXlu(activePlay, nestedSkeleton, activeJoints, 2, nullptr, nullptr, nullptr, 0);
}
int main() {
    PlayState play{}; GraphicsContext gfx{}; Gfx opa[4096]{}, xlu[128]{};
    LodLimb root{{0, 0, 0}, 1, LIMB_DONE, {rigid, rigid}};
    LodLimb child{{0, 20, 0}, LIMB_DONE, LIMB_DONE, {rigid, rigid}};
    void* skeleton[2]{&root, &child}; void* other[2]{&root, &child};
    Vec3s joints[3]{{0, 10, 0}, {}, {}};
    target = skeleton; nestedSkeleton = other; activeJoints = joints; activePlay = &play;
    play.state.gfxCtx = &gfx;
    for (int test = 0; test < 9; ++test) {
        current = {}; current.xx = current.yy = current.zz = current.ww = 1;
        current.xw = 100; current.yw = 200;
        const MtxF before = current;
        values.clear(); commands.clear(); segments.clear(); used = 0; postCount = 0; stack.clear();
        gfx.polyOpa.p = opa; gfx.polyOpa.d = opa + 4096;
        gfx.polyXlu.p = xlu; gfx.polyXlu.d = xlu + 128;
        planFlag = test != 0;
        failure = test == 7 ? 1 : test == 8 ? 5 : test <= 4 ? test : 0;
        nested = test == 6 || test == 7;
        if (test == 5) gfx.polyOpa.d = opa + (2 * sizeof(Mtx) + 32 * sizeof(Gfx)) / sizeof(Gfx);
        const uintptr_t initialTail = reinterpret_cast<uintptr_t>(gfx.polyOpa.d);
        SkelAnime_DrawFlexLodWithXlu(&play, skeleton, joints, 2, Override, Post, nullptr, 0);
        assert(stack.empty() && std::memcmp(&current, &before, sizeof(current)) == 0 && postCount == 2);
        assert(segments.size() == (nested ? 4 : 2));
#ifdef PLAYER_SKIN_MM
        used = (initialTail - reinterpret_cast<uintptr_t>(gfx.polyOpa.d)) / sizeof(Mtx);
        const size_t outerCount = test == 0 || test == 5 ? 2 : 3;
        assert(segments[0] == initialTail - outerCount * sizeof(Mtx) && segments[0] == segments[1]);
        if (nested) assert(segments[2] == initialTail - 5 * sizeof(Mtx) && segments[2] == segments[3]);
#else
        assert(segments[0] == reinterpret_cast<uintptr_t>(&storage[0]) && segments[0] == segments[1]);
        if (nested) assert(segments[2] == reinterpret_cast<uintptr_t>(&storage[3]) && segments[2] == segments[3]);
#endif
        int visibleWeighted = 0, visibleEquipment = 0, visibleRigid = 0;
        for (Gfx* command = opa; command < gfx.polyOpa.p; ++command) {
            visibleWeighted += command->words.w1 == reinterpret_cast<uintptr_t>(weighted);
            visibleEquipment += command->words.w1 == reinterpret_cast<uintptr_t>(equipment);
            visibleRigid += command->words.w1 == reinterpret_cast<uintptr_t>(rigid);
        }
        if (test == 0 || test == 1 || test == 2 || test == 5 || test == 7 || test == 8) assert(visibleWeighted == 0);
        else if (test == 3 || test == 4) assert(visibleWeighted == 1);
        else assert(visibleWeighted == 2);
        if (test == 3 || test == 8) assert(visibleEquipment == 1);
        if (test == 0 || test == 5) assert(used == 2);
        if (test == 1 || test == 2) assert(visibleRigid == 2);
        if (nested) assert(visibleRigid == (test == 7 ? 4 : 2) && used == 5);
        if (test == 6) {
            assert(std::abs(values.at(reinterpret_cast<Mtx*>(segments[0]) + 2).yw - 220.f) < 1e-5);
            // Repeat a nested draw with a bad palette: only this frame's
            // weighted commands roll back; the nested rigid body stays intact.
            failure = 1;
        }
    }
    std::puts("PASS native player FlexLOD: palette slots, callback/pose preservation, opaque+XLU binding, arena fallback, packing rollback, nested scope");
}
