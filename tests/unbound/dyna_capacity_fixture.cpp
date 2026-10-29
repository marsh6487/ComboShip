#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

using s32 = int32_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
#define DYNAPOLY_INVALIDATE_LOOKUP 1
#define COLPOLY_VTX_INDEX_MASK 0x1FFFFFFFu
#define BGACTOR_NEG_ONE -1
#define VT_FGCOL(...) ""
#define VT_RST ""
#define osSyncPrintf(...) ((void)0)
static unsigned writes, allocationCalls, freeCalls;
static unsigned failAllocation;
static size_t largestAllocation;
void* TestAlloc(size_t size) {
    ++allocationCalls;
    largestAllocation = std::max(largestAllocation, size);
    if (allocationCalls == failAllocation || size > 4096) return nullptr;
    return std::malloc(size);
}
void TestFree(void* ptr) { if (ptr) { ++freeCalls; std::free(ptr); } }
struct CollisionPoly { uint8_t bytes[32]; };
struct Vec3i { s32 x, y, z; };
struct CollisionHeader { u32 numPolygons, numVertices; };
struct Actor { void* update = reinterpret_cast<void*>(1); int bgId = 0; };
using DynaPolyActor = Actor;
struct DynaLookup {};
struct BgActor { Actor* actor = nullptr; CollisionHeader* colHeader = nullptr; DynaLookup dynaLookup; };
struct Nodes { s32 count = 0; };
struct DynaCollisionContext {
    s32 bgActorMax = 0;
    std::vector<u16> bgActorFlags;
    std::vector<BgActor> bgActors;
    CollisionPoly* polyList = nullptr;
    Vec3i* vtxList = nullptr;
    s32 polyListMax = 2, vtxListMax = 2;
    std::vector<void*> trackedBuffers;
    u32 bitFlag = 0;
    Nodes polyNodes;
};
struct PlayState { struct {} colCtx; };
void DynaSSNodeList_ResetCount(Nodes* nodes) { nodes->count = 0; }
void DynaLookup_ResetLists(DynaLookup*) {}
void BgActor_Initialize(PlayState*, BgActor* actor) { *actor = {}; }
Actor* DynaPoly_GetActor(void*, s32) { return nullptr; }
void DynaPoly_ExpandSRT(PlayState*, DynaCollisionContext*, s32, s32*, s32*) { ++writes; }
static bool failTracking;
s32 BgCheck_TrackDynaGeometry(DynaCollisionContext* d, void* p, size_t, void* v, size_t) {
    if (failTracking) return false;
    if (p) d->trackedBuffers.push_back(p);
    if (v) d->trackedBuffers.push_back(v);
    return true;
}
void DynaPoly_AllocPolyList(PlayState*, CollisionPoly** out, s32 count) { *out = static_cast<CollisionPoly*>(TestAlloc((size_t)count * sizeof(CollisionPoly))); }
void DynaPoly_AllocVtxList(PlayState*, Vec3i** out, s32 count) { *out = static_cast<Vec3i*>(TestAlloc((size_t)count * sizeof(Vec3i))); }
#define malloc TestAlloc
#define free TestFree
/* ACTUAL DYNA CAPACITY */
#undef malloc
#undef free

int main() {
    u64 next = 99;
    u64 vertexLimit = BgCheck_TableCapacityLimit((u64)COLPOLY_VTX_INDEX_MASK + 1, sizeof(Vec3i));
    assert(BgCheck_NextTableCapacity(vertexLimit, vertexLimit, (u64)COLPOLY_VTX_INDEX_MASK + 1, sizeof(Vec3i), &next));
    assert(next == vertexLimit);
    assert(!BgCheck_NextTableCapacity(vertexLimit + 1, vertexLimit, (u64)COLPOLY_VTX_INDEX_MASK + 1, sizeof(Vec3i), &next));
    assert(!BgCheck_NextTableCapacity(UINT64_MAX, 0, INT32_MAX, sizeof(CollisionPoly), &next));
    PlayState play;
    Actor actor;
    CollisionPoly oldPolys[2]{};
    Vec3i oldVertices[2]{};
    auto make = [&](std::vector<CollisionHeader>& headers) {
        DynaCollisionContext d;
        d.bgActorMax = headers.size(); d.bgActorFlags.assign(headers.size(), 1);
        for (auto& header : headers) d.bgActors.push_back(BgActor{ &actor, &header, {} });
        d.polyList = oldPolys; d.vtxList = oldVertices;
        return d;
    };
    auto refused = [&](std::vector<CollisionHeader> headers, s32 polyMax = 2, s32 vertexMax = 2, unsigned fail = 0, bool trackingFailure = false) {
        auto d = make(headers); d.polyListMax = polyMax; d.vtxListMax = vertexMax;
        failTracking = trackingFailure;
        writes = allocationCalls = freeCalls = 0; failAllocation = fail; largestAllocation = 0;
        DynaPoly_Setup(&play, &d);
        assert(writes == 0 && d.polyList == oldPolys && d.vtxList == oldVertices);
        assert(d.polyListMax == polyMax && d.vtxListMax == vertexMax && d.trackedBuffers.empty());
        assert(d.bitFlag & DYNAPOLY_INVALIDATE_LOOKUP);
        return allocationCalls;
    };
    assert(refused({ { 0x80000000u, 1 } }) == 0);
    assert(refused({ { 0x40000000u, 1 }, { 0x40000000u, 1 } }) == 0);
    assert(refused({ { 1, 0x20000001u } }) == 0);
    assert(refused({ { 1, 0x10000001u }, { 1, 0x10000000u } }) == 0);
    assert(refused({ { 3, 3 } }, 2, 2, 0, true) == 2 && freeCalls == 2);
    assert(refused({ { 3, 3 } }, 2, 2, 1) == 1 && freeCalls == 0);
    // A valid signed capacity doubles above INT32_MAX: clamp to the actual representation limit.
    if (SIZE_MAX / sizeof(CollisionPoly) >= INT32_MAX) {
        assert(refused({ { 0x60000000u, 1 } }, 0x50000000, 2) == 1);
        assert(largestAllocation == (size_t)INT32_MAX * sizeof(CollisionPoly));
    } else {
        assert(refused({ { 0x60000000u, 1 } }, 0x50000000, 2) == 0);
    }
    // A failed second allocation must not publish or retire a successful first allocation.
    assert(refused({ { 3, 3 } }, 2, 2, 2) == 2 && freeCalls == 1);
    std::vector<CollisionHeader> normal{ { 3, 3 } };
    auto d = make(normal); writes = allocationCalls = freeCalls = 0; failAllocation = 0; failTracking = false;
    DynaPoly_Setup(&play, &d);
    assert(writes == 1 && d.polyListMax == 4 && d.vtxListMax == 4 && d.trackedBuffers.size() == 2);
    assert(d.trackedBuffers[0] == d.polyList && d.trackedBuffers[1] == d.vtxList);
    assert(!(d.bitFlag & DYNAPOLY_INVALIDATE_LOOKUP));
    TestFree(d.polyList); TestFree(d.vtxList);
}
