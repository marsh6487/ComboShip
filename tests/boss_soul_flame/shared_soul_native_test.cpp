#include <nlohmann/json_fwd.hpp>
#include "global.h"
#include "ComboItemDrawABI.h"
#include "ComboCollectibleShimmer.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#if HOST_MM
#include "2s2h/Rando/SoulShuffle.h"
#include "2s2h/Rando/DrawFuncs.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#else
#include "soh/Enhancements/randomizer/SoulShuffle.h"
#include "soh/Enhancements/randomizer/draw.h"
#include "soh_assets.h"
#endif
#include "objects/object_gi_sutaru/object_gi_sutaru.h"

static PlayState play{};
static GraphicsContext graphics{};
PlayState* gPlayState = &play;
uintptr_t gSegments[NUM_SEGMENTS]{};
static Gfx opa[2048], xlu[2048], scratch[64];
static Mtx matrixStorage{};
static int matrixDepth;
static std::vector<std::string> submitted;
static std::array<int, 4> scrollSize{};
extern "C" {
void FrameInterpolation_RecordOpenChild(const void*, int) {}
void FrameInterpolation_RecordCloseChild(void) {}
#if HOST_MM
void Graph_OpenDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {}
void Graph_CloseDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {}
void Gfx_SetupDL25_Opa(GraphicsContext*) {}
void Gfx_SetupDL25_Xlu(GraphicsContext*) {}
Mtx* Matrix_Finalize(GraphicsContext*) { return &matrixStorage; }
void Matrix_Scale(f32, f32, f32, MatrixMode) {}
void Matrix_Translate(f32, f32, f32, MatrixMode) {}
f32 Rand_ZeroOne() { return 0.25f; }
#else
void Graph_OpenDisps(Gfx**, GraphicsContext*, const char*, s32) {}
void Graph_CloseDisps(Gfx**, GraphicsContext*, const char*, s32) {}
void Gfx_SetupDL_25Opa(GraphicsContext*) {}
void Gfx_SetupDL_25Xlu(GraphicsContext*) {}
Mtx* Matrix_NewMtx(GraphicsContext*, char*, s32) { return &matrixStorage; }
void Matrix_Scale(f32, f32, f32, u8) {}
void Matrix_Translate(f32, f32, f32, u8) {}
#endif
void* Graph_Alloc(GraphicsContext*, size_t size) { assert(size <= sizeof(scratch)); return scratch; }
void Matrix_Push() { ++matrixDepth; }
void Matrix_Pop() { assert(matrixDepth > 0); --matrixDepth; }
void Matrix_ReplaceRotation(MtxF*) {}
void gSPDisplayList(Gfx* packet, Gfx* list) {
    submitted.emplace_back(reinterpret_cast<const char*>(list));
    gDma1p(packet, G_DL_OTR_FILEPATH, list, 0, G_DL_PUSH);
}
void gSPSegment(void* packet, int segment, uintptr_t target) {
    __gSPSegment(static_cast<Gfx*>(packet), segment, target);
}
Gfx* Gfx_TwoTexScrollEx(GraphicsContext*, s32, u32, u32, s32 width1, s32 height1,
                       s32, u32, u32, s32 width2, s32 height2, s32, s32, s32, s32) {
    scrollSize = {width1, height1, width2, height2};
    return scratch;
}
}
/* PRODUCTION */
static void Reset(u32 frame) {
    submitted.clear(); matrixDepth = 0; scrollSize = {};
    graphics.polyOpa.p = opa; graphics.polyOpa.d = std::end(opa);
    graphics.polyXlu.p = xlu; graphics.polyXlu.d = std::end(xlu);
    play.state.gfxCtx = &graphics; play.state.frames = frame;
}
static void CheckDraw(const char* expectedBody, const char* expectedFlame) {
    assert(submitted.size() == 2 && submitted[0] == expectedBody && submitted[1] == expectedFlame);
    assert(matrixDepth == 0 && (scrollSize == std::array<int,4>{16,32,16,32}));
    int depth = 0;
    for (Gfx* command = xlu; command < graphics.polyXlu.p; ++command) {
        const auto opcode = command->words.w0 >> 24;
        if (opcode == G_COMBO_RM_PUSH) ++depth;
        if (opcode == G_COMBO_RM_POP) --depth;
        assert(depth >= 0);
    }
    assert(depth == 0);
}
int main() {
    static_assert(CW_DRAW_KIND_MM_KEY_RING == 43 && CW_DRAW_KIND_SHUFFLED_SOUL == 44);
    assert(CwMinDlistsForKind(CW_DRAW_KIND_SHUFFLED_SOUL) == 2);
    const char* body = "__OTR__objects/object_gi_sutaru/gGiSkulltulaTokenDL";
#if HOST_MM
    const char* flame = "__OTR__objects/gameplay_keep/gameplay_keep_DL_01ACF0";
    const char* foreignBody = "__OTR__@oot:objects/object_gi_sutaru/gGiSkulltulaTokenDL";
#else
    const char* flame = "__OTR__objects/object_boss_soul/gBossSoulFlameDL";
    const char* foreignBody = "__OTR__@mm:objects/object_gi_sutaru/gGiSkulltulaTokenDL";
#endif
    // All native catalogue IDs are real. None may enter boss-index arithmetic.
    for (const auto& soul : Rando::SoulShuffle::Catalog) {
        CwItemDrawInfo recipe{};
        assert(DescribeAddedSoul(soul.item, &recipe));
        assert(recipe.drawKind == CW_DRAW_KIND_SHUFFLED_SOUL && recipe.dlistCount == 2);
        assert(!strcmp(recipe.dlists[0], body) && !strcmp(recipe.dlists[1], flame));
        assert(recipe.xluStartIndex == 1 && recipe.xluSeg8TexScroll == 1);
        assert(recipe.primColorXlu[0] == 155 && recipe.primColorXlu[1] == 155 &&
               recipe.primColorXlu[2] == 155 && recipe.primColorXlu[3] == 255);
        for (u32 frame : {0u, 1u, 127u, 128u, 0xffffffffu}) {
            Reset(frame);
#if HOST_MM
            DrawShuffledSoul();
#else
            GetItemEntry entry{}; entry.getItemId = soul.item;
            Randomizer_DrawShuffledSoul(&play, &entry);
#endif
            CheckDraw(body, flame);
            Reset(frame);
#if HOST_MM
            ComboForeignDrawInfoOOT foreign{};
            foreign.dls[0] = foreignBody;
            std::memcpy(foreign.primColorXlu, recipe.primColorXlu, 4);
            MM_DrawForeignShuffledSoul(&foreign);
#else
            ComboForeignDrawInfo foreign{};
            foreign.dls[0] = foreignBody;
            std::memcpy(foreign.primColorXlu, recipe.primColorXlu, 4);
            OOT_DrawForeignShuffledSoul(&play, &foreign);
#endif
            CheckDraw(foreignBody, flame);
        }
    }
    CwItemDrawInfo ordinary{};
    assert(!DescribeAddedSoul(static_cast<decltype(Rando::SoulShuffle::Catalog[0].item)>(0), &ordinary));
    std::printf("PASS %zu native soul identities: token bodies, native+foreign boss flame root/scroll, owner brackets, stable ABI\n",
                std::size(Rando::SoulShuffle::Catalog));
}
