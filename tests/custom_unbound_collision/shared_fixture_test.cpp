#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <type_traits>
#include "functions.h"
#include "variables.h"
#include "macros.h"
#ifdef this
#undef this
#endif
#include "combo/BlackMarketDoorCollision.h"
#include "combo/BlackMarketShelvesNative.h"

uintptr_t gSegments[16]{};
extern "C" void gSPVertex(Gfx* packet, uintptr_t vertices, int count, int start) {
    __gSPVertex(packet, vertices, count, start);
}

static void ActorUpdate(Actor*, PlayState*) {}

int main() {
    static PlayState play{};
    static CollisionHeader scene{};
    auto& dyna = play.colCtx.dyna;
#ifdef TEST_OOT
    dyna.bgActorMax = 64;
    dyna.bgActors = static_cast<BgActor*>(std::calloc(64, sizeof(BgActor)));
    dyna.bgActorFlags = static_cast<u16*>(std::calloc(64, sizeof(u16)));
    // Dynamic geometry grows at setup; fixtures must register before buffers
    // exist and even when all current slots are occupied.
    for (int i = 0; i < 64; ++i)
        dyna.bgActorFlags[i] = 1;
#else
    static CollisionPoly polygons[1024];
    static Vec3s vertices[1024];
    static SSNode nodes[1024];
    dyna.polyList = polygons;
    dyna.vtxList = vertices;
    dyna.polyNodes.tbl = nodes;
    dyna.polyListMax = dyna.vtxListMax = 1024;
    dyna.polyNodes.maxNodes = 1024;
#endif
    play.colCtx.colHeader = &scene;
    Actor door{};
    door.update = door.draw = ActorUpdate;
    door.home.pos = { 60, -4, 90 };
    BlackMarketDoorCollision::Fixture<
#ifdef TEST_OOT
        true
#else
        false
#endif
        > fixture;
    assert(fixture.Ensure(&play, &door));
    const int owned = fixture.OwnedBgId(&play);
#ifdef TEST_OOT
    assert(owned == 64 && dyna.bgActorMax == 128);
#else
    assert(owned == 0);
#endif
    const auto* mesh = dyna.bgActors[owned].colHeader;
    assert(mesh && mesh->numVertices == 4 && mesh->numPolygons == 4);
    const int expected[4][3] = { { -30, -8, 0 }, { 30, -8, 0 }, { 30, 110, 0 }, { -30, 110, 0 } };
    for (int v = 0; v < 4; ++v) {
        assert(mesh->vtxList[v].x == expected[v][0]);
        assert(mesh->vtxList[v].y == expected[v][1]);
        assert(mesh->vtxList[v].z == expected[v][2]);
        const auto& poly = mesh->polyList[v];
        assert(COLPOLY_VIA_FLAG_TEST(poly.flags_vIA, 1));
        assert(!COLPOLY_VIA_FLAG_TEST(poly.flags_vIA, 2 | 4));
        assert(COLPOLY_VTX_INDEX(poly.flags_vIA) < 4);
    }
#ifdef TEST_OOT
    assert(mesh->surfaceTypeList[0].wallType == 1);
    assert(mesh->surfaceTypeList[0].camera == 0 && mesh->surfaceTypeList[0].exit == 0);
#else
    assert(mesh->surfaceTypeList[0].data[0] == (1u << 21));
#endif
    assert(fixture.Ensure(&play, &door) && fixture.OwnedBgId(&play) == owned);
    fixture.Release(&play);
    assert(!fixture.Owns(&play.colCtx, owned));

    BlackMarketShelvesNative::Fixture shelves;
    assert(shelves.Ensure(&play));
    const CollisionHeader* shelf = nullptr;
    const int capacity =
#ifdef TEST_OOT
        dyna.bgActorMax;
#else
        BG_ACTOR_MAX;
#endif
    int shelfId = -1;
    for (int i = 0; i < capacity; ++i)
        if ((dyna.bgActorFlags[i] & 1) && !(dyna.bgActorFlags[i] & 2) && dyna.bgActors[i].colHeader &&
            dyna.bgActors[i].colHeader != mesh) {
            shelf = dyna.bgActors[i].colHeader;
            shelfId = i;
        }
    assert(shelf && shelf->numVertices == 16 && shelf->numPolygons == 24);
    for (int bank = 0; bank < 2; ++bank) {
        const auto corners = BlackMarketShelves::Corners(BlackMarketShelves::kCollision[bank]);
        for (int i = 0; i < 8; ++i) {
            const auto& actual = shelf->vtxList[bank * 8 + i];
            assert(actual.x == corners[i].x && actual.y == corners[i].y && actual.z == corners[i].z);
        }
    }
    for (int p = 0; p < 24; ++p)
        for (int v = 0; v < 3; ++v) {
            const auto& poly = shelf->polyList[p];
            const auto& point = shelf->vtxList[poly.vtxData[v]];
            const float plane = (float(poly.normal.x) * point.x + float(poly.normal.y) * point.y +
                                 float(poly.normal.z) * point.z) / 32767.f + poly.dist;
            assert(std::fabs(plane) < .51f);
        }
    // Registration state belongs to the current collision table, not a bool
    // retained by the fixture after a native teardown/reuse of the context.
    dyna.bgActorFlags[shelfId] = 0;
    dyna.bgActors[shelfId] = {};
    assert(shelves.Ensure(&play));
    assert(dyna.bgActors[shelfId].colHeader == shelf);
    shelves.Reset();
    fixture.Reset();
#ifdef TEST_OOT
    std::free(dyna.bgActors);
    std::free(dyna.bgActorFlags);
#else
    for (auto& bg : dyna.bgActors)
        bg = {};
    for (auto& flags : dyna.bgActorFlags)
        flags = 1;
    assert(!fixture.Ensure(&play, &door));
    assert(!shelves.Ensure(&play));
#endif
    std::puts("PASS shared fixtures: authored geometry, camera/player flags, slot growth and re-registration");
}
