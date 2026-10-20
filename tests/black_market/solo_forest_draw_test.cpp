#include "global.h"
#include "entrances/SoloForest.h"
#include "mm/2s2h/Enhancements/Graphics/ComboForestPortalEffect.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

PlayState* gPlayState;
static bool visit = true, safe = true, allocationAvailable = true, sForestTitle;
static char kForestTitle[] = "__OTR__custom/skiejer_forest/title";
static std::array<Vtx, ComboForestPortalEffect::kVertexCount> arena;
static std::vector<std::string> events;
struct Batch { const Vtx* vertices; int count; std::vector<std::array<int, 3>> triangles; };
static std::vector<Batch> batches;
static int clears, renderFirst, renderSecond;
bool ForestVisit(const PlayState* play) { return play && visit; }
bool ForestSafe(const PlayState* play) { return play && safe; }
int ComboPlaceTitleCard_Show(PlayState*, void*, int width, int height) {
    assert(width == 144 && height == 24); events.emplace_back("title"); return 1;
}
void* AllocateVertices(size_t size) { assert(size == sizeof(arena)); return allocationAvailable ? arena.data() : nullptr; }
void Vertices(uintptr_t vertices, int count, int start) {
    assert(start == 0); batches.push_back({ reinterpret_cast<const Vtx*>(vertices), count, {} });
}
void Triangles(int a, int b, int c, int flagA, int d, int e, int f, int flagB) {
    assert(!flagA && !flagB && !batches.empty());
    batches.back().triangles.push_back({ a, b, c }); batches.back().triangles.push_back({ d, e, f });
}
// Only the native graphics boundary is replaced. The complete production draw
// body, shared animated geometry and actual host vertex/title types execute.
#undef GRAPH_ALLOC
#define GRAPH_ALLOC(context, size) AllocateVertices(size)
#define Graph_Alloc(context, size) AllocateVertices(size)
#undef OPEN_DISPS
#define OPEN_DISPS(context) events.emplace_back("open")
#undef CLOSE_DISPS
#define CLOSE_DISPS(context) events.emplace_back("close")
#define Gfx_SetupDL25_Xlu(context) events.emplace_back("setup")
#define Gfx_SetupDL_25Xlu(context) events.emplace_back("setup")
#define Matrix_Push() events.emplace_back("push")
#define Matrix_Pop() events.emplace_back("pop")
#define Matrix_Translate(x,y,z,mode) assert((x)==COMBO_SOLO_FOREST_STUMP_X && (z)==COMBO_SOLO_FOREST_STUMP_Z)
#define Matrix_RotateYS(yaw,mode) assert((yaw)==COMBO_SOLO_FOREST_STUMP_YAW)
#define Matrix_RotateY(yaw,mode) ((void)0)
#undef MATRIX_FINALIZE_AND_LOAD
#define MATRIX_FINALIZE_AND_LOAD(...) ((void)0)
#undef gSPMatrix
#ifdef OOT_FOREST_TEST
// Evaluate the production call against the real legacy char* declaration.
Mtx* Matrix_NewMtx(GraphicsContext*, char* file, s32 line) {
    assert(file && file[0] && line > 0);
    static Mtx matrix{};
    return &matrix;
}
#define gSPMatrix(packet,matrix,flags) ((void)(matrix))
#else
#define gSPMatrix(...) ((void)0)
#endif
#undef gSPClearGeometryMode
#define gSPClearGeometryMode(packet,mask) (clears=(mask))
#undef gSPTexture
#define gSPTexture(...) ((void)0)
#undef gDPSetRenderMode
#define gDPSetRenderMode(packet,first,second) (renderFirst=(first),renderSecond=(second))
#undef gDPSetCombineMode
#define gDPSetCombineMode(...) ((void)0)
#undef gSPVertex
#define gSPVertex(packet,vertices,count,start) Vertices(vertices,count,start)
#undef gSP2Triangles
#define gSP2Triangles(packet,a,b,c,f1,d,e,f,f2) Triangles(a,b,c,f1,d,e,f,f2)
#undef gDPPipeSync
#define gDPPipeSync(packet) events.emplace_back("sync")
/* PRODUCTION */

int main(int argc, char** argv) {
    PlayState play{}; gPlayState = &play; play.roomCtx.curRoom.num = 0; play.gameplayFrames = 75;
    DrawForestPortal();
    const std::string selected = argc > 1 ? argv[1] : "all";
    using namespace ComboForestPortalEffect;
    if (selected != "state") {
        assert(batches.size() == kRimSegments + kGlints);
        std::array<Vertex, kVertexCount> expected; Build(play.gameplayFrames, true, expected);
        for (int i = 0; i < kVertexCount; ++i) {
            assert(arena[i].v.ob[0] == static_cast<s16>(expected[i].x));
            assert(arena[i].v.ob[1] == static_cast<s16>(expected[i].y) && arena[i].v.ob[2] == 10);
            assert(arena[i].v.cn[0] == expected[i].r && arena[i].v.cn[1] == expected[i].g);
            assert(arena[i].v.cn[2] == expected[i].b && arena[i].v.cn[3] == expected[i].a);
        }
        const std::vector<std::array<int, 3>> quad = { {0,1,2}, {0,2,3} };
        const std::vector<std::array<int, 3>> fan = { {0,1,2}, {0,2,3}, {0,3,4}, {0,4,1} };
        for (int i = 0; i < kRimSegments; ++i)
            assert(batches[i].count == 4 && batches[i].vertices == arena.data() + i * 4 && batches[i].triangles == quad);
        for (int i = 0; i < kGlints; ++i)
            assert(batches[kRimSegments+i].count == 5 && batches[kRimSegments+i].vertices == arena.data()+kRimSegments*4+i*5 &&
                   batches[kRimSegments+i].triangles == fan && "every glint requires the complete four-triangle diamond");
    }
    if (selected != "fan") {
        assert((clears & (G_LIGHTING | G_FOG | G_CULL_BOTH)) == (G_LIGHTING | G_FOG | G_CULL_BOTH));
        assert(renderFirst == G_RM_PASS && renderSecond == G_RM_AA_ZB_XLU_SURF2);
        const std::vector<std::string> expected = { "open", "setup", "push", "pop", "sync", "setup", "close" };
        assert(events == expected && "restore native pipeline/setup before closing the display list");
    }
    for (int blocked = 0; blocked < 5; ++blocked) {
        events.clear(); batches.clear();
        visit = blocked != 0; safe = blocked != 1; allocationAvailable = blocked != 2;
        play.roomCtx.curRoom.num = blocked == 3 ? 1 : 0;
        gPlayState = blocked == 4 ? nullptr : &play;
        DrawForestPortal(); assert(events.empty() && batches.empty());
    }
    visit = safe = allocationAvailable = true; play.roomCtx.curRoom.num = 0; gPlayState = &play;
    sForestTitle = true; DrawForestPortal(); assert(!sForestTitle);
#ifdef OOT_FOREST_TEST
    assert(play.actorCtx.titleCtx.width == 144 && play.actorCtx.titleCtx.height == 24);
    assert(play.actorCtx.titleCtx.durationTimer == 80 && play.actorCtx.titleCtx.delayTimer == 20);
#endif
    std::puts("PASS native forest portal submission: complete animated glints, restored XLU state, guards and local title dimensions");
}
