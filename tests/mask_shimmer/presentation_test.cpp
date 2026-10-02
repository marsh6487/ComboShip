#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
extern "C" {
#ifdef HOST_MM
#include "global.h"
#else
#include "z64.h"
#include "macros.h"
#include "functions.h"
#endif
#include "objects/gameplay_keep/gameplay_keep.h"
}

static Gfx opa[512], xlu[512], resource;
static GraphicsContext gfx;
static PlayState play;
static Mtx matrix;
static int depth;
static bool missingSparkle;
struct Transform { float x = 0, y = 0, z = 0, scale = 1; };
static Transform transform;
static std::vector<Transform> stack, sparkleTransforms;
static std::vector<std::string> paths;

extern "C" {
void gSPDisplayList(Gfx* p, Gfx* dl) {
    paths.emplace_back((const char*)dl);
    if (paths.back().find("gEffSparklesDL") != std::string::npos) {
        sparkleTransforms.push_back(transform);
    }
    __gSPDisplayList(p, dl);
}
Gfx* ResourceMgr_LoadGfxByName(const char* path) {
    return missingSparkle && strstr(path, "gEffSparklesDL") ? nullptr : &resource;
}
void Matrix_Push() { stack.push_back(transform); ++depth; }
void Matrix_Pop() { assert(depth > 0); transform = stack.back(); stack.pop_back(); --depth; }
#ifdef HOST_MM
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode) { assert(x == y && y == z); transform.scale *= x; }
void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode) {
    transform.x += x * transform.scale; transform.y += y * transform.scale; transform.z += z * transform.scale;
}
Mtx* Matrix_Finalize(GraphicsContext*) { return &matrix; }
void Gfx_SetupDL25_Xlu(GraphicsContext*) {}
void Graph_OpenDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {}
void Graph_CloseDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {}
#else
void Matrix_Scale(f32 x, f32 y, f32 z, u8) { assert(x == y && y == z); transform.scale *= x; }
void Matrix_Translate(f32 x, f32 y, f32 z, u8) {
    transform.x += x * transform.scale; transform.y += y * transform.scale; transform.z += z * transform.scale;
}
Mtx* Matrix_NewMtx(GraphicsContext*, char*, s32) { return &matrix; }
void Gfx_SetupDL_25Xlu(GraphicsContext*) {}
void Graph_OpenDisps(Gfx**, GraphicsContext*, const char*, s32) {}
void Graph_CloseDisps(Gfx**, GraphicsContext*, const char*, s32) {}
#endif
void Matrix_ReplaceRotation(MtxF*) {}
void FrameInterpolation_RecordOpenChild(const void*, int) {}
void FrameInterpolation_RecordCloseChild() {}
f32 Math_SinS(s16 a) { return sinf(a * 3.14159265358979323846f / 32768); }
f32 Math_CosS(s16 a) { return cosf(a * 3.14159265358979323846f / 32768); }
int DinFireShield_DrawItem(PlayState*, int16_t) { return 0; }
#ifdef HOST_MM
s16 Play_GetOriginalSceneId(s16 id) { return id; }
s32 GetItem_DrawDungeonItem(PlayState*, s16, s32) { return false; }
int32_t CVarGetInteger(const char*, int32_t value) { return value; }
static s32 GetItem_BottleShimmerColor(s16, Color_RGBA8*) { return false; }
static void GetItem_DrawBottleShimmer(PlayState*, s16) { assert(false); }
#endif
}

#ifdef HOST_MM
#define COMBO_MASK_SHIMMER_HOST_MM
#endif
#include "ComboMaskShimmer.h"

static void ModelDraw(PlayState*, s16 id) {
    gSPDisplayList(gfx.polyOpa.p++, (Gfx*)"__OTR__fixture/mask");
    // Model callbacks can leave scaling/translation behind (native remains do).
#ifdef HOST_MM
    if (id == GID_REMAINS_ODOLWA || id == GID_REMAINS_GOHT || id == GID_REMAINS_GYORG || id == GID_REMAINS_TWINMOLD) {
        Matrix_Scale(.02f, .02f, .02f, MTXMODE_APPLY);
        Matrix_Translate(5000, 5000, 5000, MTXMODE_APPLY);
    }
#endif
}
static struct { void (*drawFunc)(PlayState*, s16); void* drawResources[1]; } sDrawItemTable[256];
/* PRODUCTION_NATIVE */

static void Reset(int frame) {
    memset(opa, 0, sizeof(opa)); memset(xlu, 0, sizeof(xlu));
    gfx.polyOpa.p = opa; gfx.polyXlu.p = xlu;
    play.state.gfxCtx = &gfx; play.gameplayFrames = frame;
    paths.clear(); sparkleTransforms.clear(); stack.clear(); depth = 0; transform = {};
}
static void Check(const uint8_t color[4], bool foreign) {
    assert(depth == 0 && stack.empty() && transform.scale == 1 && transform.x == 0);
    assert(sparkleTransforms.size() == 5);
    for (const auto& t : sparkleTransforms) {
        assert(fabs(t.x) <= 26.01f && fabs(t.z) <= 26.01f && fabs(t.y) <= 24.01f);
        assert(t.scale >= .025f && t.scale <= .05001f);
    }
    int tint = 0, owners = 0, sparkles = 0;
    uintptr_t prim = 0, env = 0;
    for (Gfx* cmd = xlu; cmd < gfx.polyXlu.p; ++cmd) {
        unsigned op = cmd->words.w0 >> 24;
        if (op == G_SETGRAYSCALE) { tint = cmd->words.w1; assert(tint == 0); }
        if (op == G_COMBO_RM_PUSH) { assert(foreign && !strcmp((const char*)cmd->words.w1, "mm")); ++owners; }
        if (op == G_COMBO_RM_POP) --owners;
        if (op == G_SETPRIMCOLOR) prim = cmd->words.w1;
        if (op == G_SETENVCOLOR) env = cmd->words.w1;
        if (op == G_DL) {
            assert(owners == (foreign ? 1 : 0) && tint == 0);
            assert(env == ((uint32_t)color[0] << 24 | (uint32_t)color[1] << 16 | (uint32_t)color[2] << 8));
            if (color[0] == 0 && color[1] == 0 && color[2] == 0) assert((prim >> 8) == 0x505060);
            ++sparkles;
        }
    }
    assert(sparkles == 5 && owners == 0 && prim == 0xffffffff && env == 0xffffffff);
}
int main() {
    for (auto& row : sDrawItemTable) { row.drawFunc = ModelDraw; row.drawResources[0] = (void*)"__OTR__fixture/mask"; }
    int eligible = 0;
    for (int id = 0; id < 256; ++id) {
        uint8_t color[4]{};
        if (!GetItem_GetShimmerColor(id, color)) continue;
        ++eligible;
        Reset(47); GetItem_Draw(&play, id); assert(paths.size() == 6); Check(color, false);
        auto first = sparkleTransforms;
        Reset(47); GetItem_Draw(&play, id);
        assert(first.size() == sparkleTransforms.size());
        for (size_t i = 0; i < first.size(); ++i) assert(!memcmp(&first[i], &sparkleTransforms[i], sizeof(Transform)));
        missingSparkle = true; Reset(47); GetItem_Draw(&play, id);
        assert(paths.size() == 1 && sparkleTransforms.empty() && depth == 0);
        missingSparkle = false;
    }
#ifdef HOST_MM
    assert(eligible == 29); // 24 inventory masks, Sun Mask GI and four remains.
    const int special[] = { GID_MASK_DEKU, GID_MASK_GORON, GID_MASK_ZORA, GID_MASK_FIERCE_DEITY };
    const uint8_t expected[4][3] = { {50,220,90}, {240,64,64}, {64,144,255}, {0,0,0} };
    for (int i = 0; i < 4; ++i) { uint8_t color[4]; assert(GetItem_GetShimmerColor(special[i], color)); assert(!memcmp(color, expected[i], 3)); }
#else
    assert(eligible == 8);
#endif
    for (int profile = 0; profile < 9; ++profile) {
        uint8_t color[4]; ComboMaskShimmerColor(profile, color);
        Reset(47); ComboDrawMaskShimmer(&play, "__OTR__@mm:objects/gameplay_keep/gEffSparklesDL", color, "mm"); Check(color, true);
    }
    Reset(47); GetItem_Draw(&play, GID_BOMB); assert(paths.size() == 1 && sparkleTransforms.empty());
    puts("PASS real-header mask/remains shimmer: eligibility, transformation hex, black highlights, deterministic motes, missing-effect fallback and matrix/color/RM cleanup");
}
