#include "global.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static int sFailSnapshotAllocation;
static void* SnapshotRealloc(void* previous, size_t bytes) {
    return sFailSnapshotAllocation ? NULL : realloc(previous, bytes);
}
#define realloc SnapshotRealloc
#include "expansions/sm64/sm64_mario_surfaces.c"
#undef realloc

/* Native surface accessors are compiled from z_bgcheck.c by the runner. */
uintptr_t gSegments[16];
static void UpdateActor(Actor* actor, PlayState* play) { (void)actor; (void)play; }

int main(void) {
    static PlayState play;
    CollisionPoly poly = { 0 };
    Vec3i vertices[3] = { { 80000, 20, 40 }, { 80010, 20, 40 }, { 80000, 20, 50 } };
    SurfaceType surface = { 0 };
    CollisionHeader scene = { 0 };
    u32 count;
    struct SM64Surface* extracted;
    poly.flags_vIA = 0;
    poly.flags_vIB = 1;
    poly.vIC = 2;
    poly.normal.y = 32767;
    scene.numVertices = 3;
    scene.vtxList = vertices;
    scene.numPolygons = 1;
    scene.polyList = &poly;
    scene.surfaceTypeList = &surface;
    play.colCtx.colHeader = &scene;
    extracted = Sm64Surfaces_ExtractStatic(&play, &count);
    assert(extracted && count == 1 && extracted[0].vertices[0][0] == 320000);
    assert(extracted[0].vertices[1][0] == 320040 && extracted[0].vertices[2][2] == 200);
    free(extracted);

    /* Register through the real grown OC queue, preserving admission order.
       More than 64 blockers need a complete snapshot, never a 1024-triangle prefix. */
    CollisionCheck_InitContext(&play, &play.colChkCtx);
    Actor actors[96] = { 0 };
    ColliderCylinder cylinders[96] = { 0 };
    for (int i = 0; i < 96; ++i) {
        actors[i].update = UpdateActor;
        actors[i].category = ACTORCAT_PROP;
        cylinders[i].base.actor = &actors[i];
        cylinders[i].base.shape = COLSHAPE_CYLINDER;
        cylinders[i].base.ocFlags1 = OC1_ON | OC1_TYPE_PLAYER;
        cylinders[i].dim.pos.x = i * 10;
        cylinders[i].dim.radius = 4;
        cylinders[i].dim.height = 40;
        assert(CollisionCheck_SetOC(&play, &play.colChkCtx, &cylinders[i].base) == i);
    }
    assert(play.colChkCtx.colOCCount == 96 && play.colChkCtx.colOCCapacity >= 96);
    Sm64Surfaces_RefreshActorColliders(&play);
    extracted = Sm64Surfaces_ExtractStatic(&play, &count);
    assert(extracted && count == 1537);
    assert(extracted[count - 1].vertices[0][0] > 3700);
    free(extracted);
    Sm64Surfaces_RefreshActorColliders(NULL);
    extracted = Sm64Surfaces_ExtractStatic(&play, &count);
    assert(extracted && count == 1);
    free(extracted);
    sFailSnapshotAllocation = 1;
    Sm64Surfaces_RefreshActorColliders(&play);
    extracted = Sm64Surfaces_ExtractStatic(&play, &count);
    assert(extracted == NULL && count == 0); /* No partial player collision. */
    sFailSnapshotAllocation = 0;
    Sm64Surfaces_RefreshActorColliders(&play);
    extracted = Sm64Surfaces_ExtractStatic(&play, &count);
    assert(extracted && count == 1537);
    free(extracted);
    extracted = Sm64Surfaces_ExtractFiltered(&play, &count, 1);
    assert(extracted && count == 1); /* Vanish mode retains floors, excludes actor walls. */
    free(extracted);
    Sm64Surfaces_RefreshActorColliders(NULL);
    CollisionCheck_DestroyContext(&play, &play.colChkCtx);
    poly.normal.y = 0;
    extracted = Sm64Surfaces_ExtractFiltered(&play, &count, 1);
    assert(extracted == NULL && count == 0); /* Empty extraction owns no leaked allocation. */
    surface.exit = 128;
    extracted = Sm64Surfaces_ExtractFiltered(&play, &count, 1);
    assert(extracted && count == 1); /* Wide exit walls preserve native progression in vanish mode. */
    free(extracted);
    poly.normal.y = 32767;
    surface.exit = 0;

    /* A dynamic slot beyond vanilla's 50, with a baked vertex index beyond
       16 bits, must be copied using the current context's actual capacities. */
    DynaCollisionContext* dyna = &play.colCtx.dyna;
    dyna->bgActorMax = 96;
    dyna->bgActors = calloc(96, sizeof(BgActor));
    dyna->bgActorFlags = calloc(96, sizeof(u16));
    dyna->polyListMax = 4;
    dyna->vtxListMax = 70003;
    dyna->polyList = calloc(4, sizeof(CollisionPoly));
    dyna->vtxList = calloc(70003, sizeof(Vec3i));
    dyna->bgActorFlags[95] = 1;
    dyna->bgActors[95].colHeader = &scene;
    dyna->bgActors[95].dynaLookup.polyStartIndex = 3;
    dyna->polyList[3] = poly;
    dyna->polyList[3].flags_vIA = 70000;
    dyna->polyList[3].flags_vIB = 70001;
    dyna->polyList[3].vIC = 70002;
    for (int i = 0; i < 3; ++i)
        dyna->vtxList[70000 + i] = vertices[i];
    extracted = Sm64Surfaces_ExtractStatic(&play, &count);
    assert(extracted && count == 2 && extracted[1].vertices[1][0] == 320040);
    free(extracted);
    dyna->polyList[3].flags_vIA |= 2u << COLPOLY_VIA_FLAGS_SHIFT;
    extracted = Sm64Surfaces_ExtractStatic(&play, &count);
    assert(extracted && count == 1);
    free(extracted);
    dyna->polyList[3].flags_vIA = 70003; /* Invalid vertex: omit without an OOB read. */
    extracted = Sm64Surfaces_ExtractStatic(&play, &count);
    assert(extracted && count == 1);
    free(extracted);
    free(dyna->bgActors);
    free(dyna->bgActorFlags);
    free(dyna->polyList);
    free(dyna->vtxList);
    puts("PASS SM64: wide geometry, real 96-entry OC queue snapshot, allocation failure/retry, teardown, bounds and flags");
}
