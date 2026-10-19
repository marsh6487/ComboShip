#include "global.h"
#include "combo/BlackMarketLock.h"
#include "rando/BlackMarket.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace ComboRando::BlackMarket;
PlayState* gPlayState;
static PlayState play;
static GraphicsContext graphics;
static std::array<Gfx, 1024> commands;
static std::array<std::array<Vtx, 64>, 8> arenas;
static std::array<Actor, 8> sDisplays;
static constexpr Vec3f kShelves[8] = {
    {72,64,48},{72,104,48},{120,64,48},{120,104,48},
    {-72,64,48},{-72,104,48},{-120,64,48},{-120,104,48}
};
static int sSelected = 3, allocations, setups, draws, pushDepth;
static int shelfDraws;
void DrawShelves(PlayState*) { ++shelfDraws; }
static bool sTalking = true, sTitlePending, active = true, allocate = true, report = true;
static std::vector<Vec3f> translations;
static std::vector<float> scales;
static Mtx testMatrix;
// These are fresh arena vertices, so the bridge's native raw-pointer packet
// applies directly; no resource-path resolution is involved.
extern "C" void gSPVertex(Gfx* packet, uintptr_t vertices, int count, int start) {
    __gSPVertex(packet, vertices, count, start);
}
bool Active() { return active; }
static bool seals = true;
int CVarGetInteger(const char* name,int fallback) {
    return std::strcmp(name,BlackMarketVisual::kSealsCvar)==0 ? seals : fallback;
}
bool Safe(PlayState*) { return false; }
void PresentTitle() { assert(false); }
const Item* Offer(int slot) { return slot == 7 ? nullptr : &Catalog()[slot]; }
PurchaseResult Status(int slot) { return slot == 0 || slot == 2 ? PurchaseResult::Locked : PurchaseResult::Ready; }
void SubmitOffer(const Item& item) {
    const int slot = int(&item - Catalog().data());
    const bool locked = Status(slot) == PurchaseResult::Locked;
    assert(BlackMarketVisual::Active() == locked && "only locked offers should query geometry");
    if (report) BlackMarketVisual::Report({10.f + slot,20,30}, {16.f + slot,40,34},
                                        [](BlackMarketVisual::Point point) { return point; });
    ++draws;
}
void DrawOffer(PlayState*, const Item& item) { SubmitOffer(item); }
namespace ComboBlackMarketItems { int Id(const Item& item) { return int(&item - Catalog().data()); } }
namespace Rando { void DrawResolvedItem(int id, int, Actor*) { SubmitOffer(Catalog()[id]); } }
void* Allocate(size_t bytes) {
    assert(bytes == sizeof(arenas[0]));
    return allocate ? arenas[allocations++].data() : nullptr;
}
void Setup(GraphicsContext* ctx) {
    ++setups;
    // SETUPDL_25 is opaque even when appended to the XLU stream. Poison LUT
    // too, so the actual effect must explicitly establish every required state.
    gDPSetRenderMode(ctx->polyXlu.p++, G_RM_FOG_SHADE_A, G_RM_AA_ZB_OPA_SURF2);
    gDPSetTextureLUT(ctx->polyXlu.p++, G_TT_RGBA16);
    gSPTexture(ctx->polyXlu.p++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
}
#undef GRAPH_ALLOC
#define GRAPH_ALLOC(ctx, bytes) Allocate(bytes)
#define Graph_Alloc(ctx, bytes) Allocate(bytes)
#undef OPEN_DISPS
#define OPEN_DISPS(ctx) { GraphicsContext* __gfxCtx = (ctx);
#undef CLOSE_DISPS
#define CLOSE_DISPS(ctx) }
#define Gfx_SetupDL25_Xlu(ctx) Setup(ctx)
#define Gfx_SetupDL_25Xlu(ctx) Setup(ctx)
#define Matrix_Push() (++pushDepth)
#define Matrix_Pop() assert(--pushDepth >= 0)
#define Matrix_Translate(x,y,z,mode) translations.push_back({x,y,z})
#define Matrix_Scale(x,y,z,mode) scales.push_back(x)
#define Matrix_RotateY(...) ((void)0)
#define Matrix_RotateYS(...) ((void)0)
#define Matrix_NewMtx(...) (&testMatrix)
#undef MATRIX_FINALIZE_AND_LOAD
#define MATRIX_FINALIZE_AND_LOAD(...) ((void)0)
/* PRODUCTION */

void Reset() {
    play = {}; graphics = {}; commands = {}; arenas = {};
    gPlayState = &play; play.state.running = true; play.state.gfxCtx = &graphics;
    graphics.polyXlu.p = commands.data(); play.gameplayFrames = 31;
    allocations = setups = draws = pushDepth = shelfDraws = 0;
    translations.clear(); scales.clear(); active = allocate = report = true;
    sTalking = true; sTitlePending = false;
}
void CheckPackets(int effects) {
    int vertexBatches = 0, triangles = 0, syncs = 0;
    uint32_t geometry = G_LIGHTING | G_FOG | G_CULL_BOTH, low = 0, high = 0;
    bool texture = false;
    Gfx combine{};
    gDPSetCombineMode(&combine, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    uintptr_t combine0 = 0, combine1 = 0, image = 0;
    for (const Gfx* cmd = commands.data(); cmd != graphics.polyXlu.p; ++cmd) {
        const uint32_t op = uint32_t(cmd->words.w0) >> 24;
        if (op == G_GEOMETRYMODE) geometry = (geometry & (cmd->words.w0 & 0xffffff)) | cmd->words.w1;
        if (op == G_SETOTHERMODE_H || op == G_SETOTHERMODE_L) {
            const unsigned length = (cmd->words.w0 & 255) + 1;
            const unsigned shift = 32 - ((cmd->words.w0 >> 8) & 255) - length;
            const uint32_t mask = uint32_t((uint64_t(1) << length) - 1) << shift;
            auto& target = op == G_SETOTHERMODE_H ? high : low;
            target = (target & ~mask) | (cmd->words.w1 & mask);
        }
        if (op == G_TEXTURE) texture = (cmd->words.w0 & 255) != 0;
        if (op == G_SETTIMG) image = cmd->words.w1;
        if (op == G_SETCOMBINE) { combine0 = cmd->words.w0; combine1 = cmd->words.w1; }
        if (op == G_RDPPIPESYNC) ++syncs;
        if (op == G_VTX) {
            assert((geometry & (G_LIGHTING | G_FOG | G_CULL_BOTH)) == 0);
            const uint32_t mask = ~((1u << G_MDSFT_RENDERMODE) - 1);
            assert((low & mask) == uint32_t(G_RM_PASS | G_RM_AA_ZB_XLU_SURF2));
            assert((low & Z_UPD) == 0 && (low & FORCE_BL) != 0);
            assert((high & (3u << G_MDSFT_TEXTLUT)) == G_TT_NONE && texture);
            assert(combine0 == combine.words.w0 && combine1 == combine.words.w1);
            assert(image == uintptr_t(BlackMarketVisual::kLockTexture.data()));
            assert(((cmd->words.w0 >> 12) & 255) == 32);
            assert(cmd->words.w1 == uintptr_t(arenas[vertexBatches / 2].data() + (vertexBatches % 2) * 32));
            ++vertexBatches;
        }
        if (op == G_TRI2) ++triangles;
    }
    assert(vertexBatches == effects * 2 && triangles == effects * 16);
    assert(syncs >= effects && setups == effects * 2 && !texture && pushDepth == 0);
}
int main() {
    Reset();
    const BlackMarketVisual::Frame frame{{10,20,30},{16,40,34},true};
    DrawSeal(&play, frame, true);
    assert(allocations == 1 && translations.size() == 1);
    assert(translations[0].x == 13 && translations[0].y == 30 && translations[0].z == 32);
    assert(scales.size() == 1 && scales[0] == 1.f / BlackMarketVisual::kLockVertexScale);
    const auto expected = BlackMarketVisual::BuildLock(frame, play.gameplayFrames, true);
    for (int i = 0; i < 64; ++i) {
        const auto& actual = arenas[0][i].v; const auto& want = expected.vertices[i];
        assert(actual.ob[0] == want.x && actual.ob[1] == want.y && actual.ob[2] == want.z);
        assert(actual.tc[0] == want.s && actual.tc[1] == want.t && actual.cn[3] == 255);
    }
    CheckPackets(1);
    Reset(); DrawSeal(&play, {}, false);
    assert(allocations == 0 && graphics.polyXlu.p == commands.data() && translations.empty());
    allocate = false; DrawSeal(&play, frame, false);
    assert(allocations == 0 && graphics.polyXlu.p == commands.data() && translations.empty());
    Reset(); DrawMarket();
    assert(shelfDraws == 1 && "native shop must draw physical shelves in the shared room-appearance path");
    assert(draws == 7 && allocations == 2 && !BlackMarketVisual::Active());
    assert(translations.size() == 9 && translations[1].x == 13 && translations[4].x == 15);
    CheckPackets(2);
    Reset(); report = false; DrawMarket();
    assert(draws == 7 && allocations == 0 && graphics.polyXlu.p == commands.data());
    Reset(); seals = false; DrawMarket();
    assert(shelfDraws == 1 && "hiding seals must not hide shelves");
    assert(draws == 7 && allocations == 0 && !BlackMarketVisual::Active());
    seals = true;
    Reset(); active = false; DrawMarket(); assert(draws == 0 && allocations == 0 && shelfDraws == 0);
    Reset(); play.state.running = false; DrawMarket(); assert(draws == 0 && allocations == 0);
    std::puts("PASS native market X: real GBI blend/depth/LUT/texture/combiner, 64 fitted vertices, locked-only capture and restored state");
}
