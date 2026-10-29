#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>

using s32 = int32_t;
using u32 = uint32_t;
using u64 = uint64_t;
using f32 = float;
#define COLPOLY_VTX_INDEX_MASK 0x1FFFFFFFu
#define UNBOUND_STATIC_NODES_MIN 4096
#define UNBOUND_DYNA_POLY_MAX 16384
#define UNBOUND_DYNA_VTX_MAX 16384
#define UNBOUND_DYNA_NODES_INITIAL 16384
#define osSyncPrintf(...) ((void)0)
#define LOG_HUNGUP_THREAD() ((void)0)
struct Refused {};
[[noreturn]] void TestAbort() { throw Refused{}; }
static unsigned allocationCalls, writes;
static s32 allocatedNodes, allocatedPolygons;
void* TestAlloc(size_t size) { ++allocationCalls; assert(size <= 4096); return std::malloc(size); }
struct Vec3i { s32 x, y, z; };
struct Vec3f { f32 x, y, z; };
struct CollisionPoly { uint8_t bytes[32]; };
struct CollisionHeader { u32 numPolygons = 0, numVertices = 0; Vec3i minBounds{}, maxBounds{}; };
struct SSNode { s32 polyId; u32 next; };
struct StaticLookup { u32 floor, wall, ceiling; };
struct SSNodeList {};
struct DynaContext { s32 polyListMax, vtxListMax, polyNodesMax; };
struct CollisionContext {
    CollisionHeader* colHeader = reinterpret_cast<CollisionHeader*>(1);
    Vec3i subdivAmount{};
    StaticLookup* lookupTbl = nullptr;
    Vec3f minBounds{}, maxBounds{}, subdivLength{}, subdivLengthInv{};
    DynaContext dyna{};
    SSNodeList polyNodes;
};
struct PlayState {};
void BgCheck_BeginOwnership(CollisionContext* context) { *context = CollisionContext{}; }
void BgCheck_SetVanillaSubdivisions(CollisionContext* c, PlayState*) { c->subdivAmount = { 2, 2, 2 }; }
void BgCheck_ScaleSubdivisionsForPolyCount(CollisionContext*, u32) {}
void Math_Vec3i_ToVec3f(Vec3f*, Vec3i*) {}
void BgCheck_SetSubdivisionDimension(f32, s32, f32*, f32*, f32*) {}
void SSNodeList_Initialize(SSNodeList*) {}
void SSNodeList_Alloc(PlayState*, SSNodeList*, s32 nodes, s32 polygons) { allocatedNodes = nodes; allocatedPolygons = polygons; }
void BgCheck_InitializeStaticLookup(CollisionContext*, PlayState*, StaticLookup*) { ++writes; }
void DynaPoly_Init(PlayState*, DynaContext*) {}
void DynaPoly_Alloc(PlayState*, DynaContext*) {}
#define malloc(size) static_cast<StaticLookup*>(TestAlloc(size))
#define abort TestAbort
/* ACTUAL STATIC ALLOCATION */
#undef malloc
#undef abort

int main() {
    PlayState play;
    for (auto header : { CollisionHeader{ 0x80000000u, 1 }, CollisionHeader{ 1, 0x20000001u } }) {
        CollisionContext context;
        allocationCalls = writes = 0;
        try { BgCheck_Allocate(&context, &play, &header); assert(false); } catch (const Refused&) {}
        assert(context.colHeader == reinterpret_cast<CollisionHeader*>(1) && writes == 0 && allocationCalls == 0);
    }
    CollisionHeader normal{ 100, 300 };
    CollisionContext context;
    BgCheck_Allocate(&context, &play, &normal);
    assert(allocatedNodes == UNBOUND_STATIC_NODES_MIN && allocatedPolygons == 100 && writes == 1);
    std::free(context.lookupTbl);
    if (SIZE_MAX / sizeof(CollisionPoly) >= 0x60000000u) {
        CollisionHeader large{ 0x60000000u, 1 };
        BgCheck_Allocate(&context, &play, &large);
        assert(allocatedNodes == INT32_MAX && allocatedPolygons == 0x60000000);
        std::free(context.lookupTbl);
    }
}
