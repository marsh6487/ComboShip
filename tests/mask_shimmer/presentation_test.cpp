#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <map>
#include <cstdlib>
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
/* PRODUCTION_DRAW_PRELUDE */
#include "soh/Enhancements/randomizer/NeiGiRender.h"
#include "soh/Enhancements/randomizer/NeiGiEnergyTexture.h"

static Gfx opa[2048], xlu[2048], resource;
static GraphicsContext gfx;
static PlayState play;
static Mtx matrix;
static int depth;
static bool missingSparkle, allocationFailure;
static int resourceLoads;
static std::vector<std::vector<Vtx>> arena;
static std::map<Gfx*, std::vector<Vtx>> vertexLoads;
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
    if (strstr(path, "gEffSparklesDL")) ++resourceLoads;
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
void Matrix_Get(MtxF* m) {
    *m = {}; m->xx = m->yy = m->zz = transform.scale;
    m->xw = transform.x; m->yw = transform.y; m->zw = transform.z;
}
void* Graph_Alloc(GraphicsContext*, size_t size) {
    if (allocationFailure) return nullptr;
    assert(size <= 1536 * sizeof(Vtx) && size % sizeof(Vtx) == 0);
    arena.emplace_back(size / sizeof(Vtx)); return arena.back().data();
}
void gSPVertex(Gfx* cmd, uintptr_t data, int n, int v0) {
    assert(n > 0 && n + v0 <= 32);
    auto* vertices = reinterpret_cast<const Vtx*>(data);
    vertexLoads[cmd] = std::vector<Vtx>(vertices, vertices + n);
    __gSPVertex(cmd, data, n, v0);
}
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
/* PRODUCTION_FOREIGN_LINKAGE */
using NeiGi::Kind;
static bool HasResource(const char*) { return false; }
extern "C" {
#ifdef HOST_MM
#define Gfx_SetupDL_25Xlu Gfx_SetupDL25_Xlu
#define Matrix_NewMtx(ctx, file, line) Matrix_Finalize(ctx)
#endif
#include "soh/Enhancements/randomizer/NeiGiMeshRenderer.inc"
#ifdef HOST_MM
#undef Gfx_SetupDL_25Xlu
#undef Matrix_NewMtx
#endif
}

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
    arena.clear(); vertexLoads.clear(); resourceLoads = 0;
    play.billboardMtxF = {}; play.billboardMtxF.xx = play.billboardMtxF.yy = play.billboardMtxF.zz = 1;
}
static std::vector<Vtx> ExpandedVertices(bool foreign, const char* owner = "mm") {
    std::vector<Vtx> result, cache;
    int owners = 0, grayscale = 1;
    for (Gfx* cmd = xlu; cmd < gfx.polyXlu.p; ++cmd) {
        unsigned op = cmd->words.w0 >> 24;
        if (op == G_COMBO_RM_PUSH) { assert(foreign && !strcmp((const char*)cmd->words.w1, owner)); ++owners; }
        if (op == G_COMBO_RM_POP) --owners;
        if (op == G_SETGRAYSCALE) { grayscale = cmd->words.w1; assert(grayscale == 0); }
        if (op == G_VTX) cache = vertexLoads.at(cmd);
        auto triangle = [&](uintptr_t bits) {
            assert(owners == (foreign ? 1 : 0) && grayscale == 0);
            for (int shift : {16, 8, 0}) result.push_back(cache.at(((bits >> shift) & 255) / 2));
        };
        if (op == G_TRI1 || op == G_TRI2) triangle(cmd->words.w0);
        if (op == G_TRI2) triangle(cmd->words.w1);
    }
    assert(owners == 0 && grayscale == 0);
    return result;
}
extern "C" void Fixture_DrawCShimmer(PlayState*, const uint8_t color[4]);
static std::vector<Vtx> Check(const uint8_t color[4], bool foreign, const char* owner = "mm") {
    assert(depth == 0 && stack.empty() && transform.scale == 1 && transform.x == 0 && transform.y == 0 && transform.z == 0);
    assert(sparkleTransforms.empty());
    const auto actual = ExpandedVertices(foreign, owner);
    const auto expected = NeiGi::SampleShimmer(play.gameplayFrames, true, NeiGi_CameraBasis(&play));
    assert(actual.size() == expected.count && actual.size() == 360);
    const bool ordinary = color[0] == 220 && color[1] == 225 && color[2] == 240;
    const bool dark = color[0] == 0 && color[1] == 0 && color[2] == 0;
    const uint32_t tint = uint32_t(color[0]) << 16 | uint32_t(color[1]) << 8 | color[2];
    for (size_t i = 0; i < actual.size(); ++i) {
        const auto& vertex = expected.vertices[i];
        assert(actual[i].v.ob[0] == std::lround(vertex.p.x * 16));
        assert(actual[i].v.ob[1] == std::lround(vertex.p.y * 16));
        assert(actual[i].v.ob[2] == std::lround(vertex.p.z * 16));
        uint32_t rgb = vertex.rgb;
        if (!ordinary) rgb = vertex.rgb == 0xFFFFFF ? (dark ? 0x505060 : 0xFFFFFF) : tint;
        assert(actual[i].v.cn[0] == (rgb >> 16));
        assert(actual[i].v.cn[1] == ((rgb >> 8) & 255));
        assert(actual[i].v.cn[2] == (rgb & 255));
        assert(actual[i].v.cn[3] == vertex.alpha);
    }
    uintptr_t prim = 0, env = 0;
    for (Gfx* cmd = xlu; cmd < gfx.polyXlu.p; ++cmd) {
        if ((cmd->words.w0 >> 24) == G_SETPRIMCOLOR) prim = cmd->words.w1;
        if ((cmd->words.w0 >> 24) == G_SETENVCOLOR) env = cmd->words.w1;
    }
    assert(prim == 0xffffffff && env == 0xffffffff);
    return actual;
}
int main() {
    for (auto& row : sDrawItemTable) { row.drawFunc = ModelDraw; row.drawResources[0] = (void*)"__OTR__fixture/mask"; }
    int eligible = 0;
    for (int id = 0; id < 256; ++id) {
        uint8_t color[4]{};
        if (!GetItem_GetShimmerColor(id, color)) continue;
        ++eligible;
        Reset(47); GetItem_Draw(&play, id); auto first = Check(color, false); assert(paths.size() == 1);
        Reset(47); GetItem_Draw(&play, id);
        auto second = Check(color, false);
        assert(first.size() == second.size());
        for (size_t i = 0; i < first.size(); ++i) assert(!memcmp(&first[i], &second[i], sizeof(Vtx)));
        missingSparkle = true; Reset(47); GetItem_Draw(&play, id);
        Check(color, false); assert(resourceLoads == 0);
        missingSparkle = false;
        allocationFailure = true; Reset(47); GetItem_Draw(&play, id);
        assert(paths.size() == 1 && vertexLoads.empty() && depth == 0 && stack.empty());
        assert(ExpandedVertices(false).empty());
        allocationFailure = false;
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
    uint8_t ordinary[4]; ComboMaskShimmerColor(0, ordinary);
    Reset(47); ComboDrawMaskShimmer(&play, nullptr, ordinary, "oot"); Check(ordinary, true, "oot");
    allocationFailure = true; Reset(47); ComboDrawMaskShimmer(&play, nullptr, ordinary, "oot");
    assert(ExpandedVertices(true, "oot").empty() && depth == 0 && stack.empty()); allocationFailure = false;
    for (int frame : {0, 47, 179, 180, 65535}) {
        Reset(frame); Fixture_DrawCShimmer(&play, ordinary); Check(ordinary, false);
        Reset(frame); play.billboardMtxF.xx = play.billboardMtxF.yy = 0;
        play.billboardMtxF.xy = -1; play.billboardMtxF.yx = 1;
        Fixture_DrawCShimmer(&play, ordinary); Check(ordinary, false);
    }
    Reset(47); GetItem_Draw(&play, GID_BOMB); assert(paths.size() == 1 && sparkleTransforms.empty());
    puts("PASS real-header mask/remains shimmer: eligibility, transformation hex, black highlights, exact NEI shimmer geometry, deterministic sampling, texture independence, allocation fallback and matrix/color/RM cleanup");
}
