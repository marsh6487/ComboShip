#include "global.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
/* The unused aiming path has this mod-level declaration in its caller. */
s32 Player_IsZTargeting(Player* player);
#include "mods/items/helpers/grappling_helper.c"

uintptr_t gSegments[16];

int main(void) {
    static PlayState play;
    CollisionHeader header = { 0 };
    SurfaceType surface = { 0 };
    CollisionPoly local[2] = { 0 };
    Vec3i* wide = calloc(70004, sizeof(Vec3i));
    GrappleTarget result;
    local[0].flags_vIA = 70000;
    local[0].flags_vIB = 70001;
    local[0].vIC = 70002;
    local[0].normal.z = 32767;
    local[1] = local[0];
    local[1].flags_vIB = 70002;
    local[1].vIC = 70003;
    wide[70000] = (Vec3i){ 180000, 0, 20 };
    wide[70001] = (Vec3i){ 180100, 0, 20 };
    wide[70002] = (Vec3i){ 180100, 10, 20 };
    wide[70003] = (Vec3i){ 180000, 20, 20 };
    header.numVertices = 70004;
    header.vtxList = wide;
    header.numPolygons = 2;
    header.polyList = local;
    header.surfaceTypeList = &surface;
    play.colCtx.colHeader = &header;
    Grapple_AnalyzeSurface(&play, &local[0], BGCHECK_SCENE, NULL, &result);
    assert(result.dims[0] == 0 && result.dims[1] == 20 && result.dims[2] == 100);

    DynaCollisionContext* dyna = &play.colCtx.dyna;
    dyna->bgActorMax = 96;
    dyna->bgActors = calloc(96, sizeof(BgActor));
    dyna->bgActorFlags = calloc(96, sizeof(u16));
    dyna->vtxList = wide;
    dyna->vtxListMax = 70004;
    dyna->polyList = calloc(5, sizeof(CollisionPoly));
    dyna->polyListMax = 5;
    dyna->polyList[3] = local[0];
    dyna->polyList[4] = local[1];
    /* The registered header remains model-space. Neighbor analysis must visit
       the corresponding current baked polygons, not mix these local indices
       with the dynamic world-space vertex pool. */
    local[0].flags_vIA = 0;
    local[0].flags_vIB = 1;
    local[0].vIC = 2;
    local[1].flags_vIA = 0;
    local[1].flags_vIB = 2;
    local[1].vIC = 3;
    header.numVertices = 4;
    dyna->bgActorFlags[95] = 1;
    dyna->bgActors[95].colHeader = &header;
    dyna->bgActors[95].dynaLookup.polyStartIndex = 3;
    Grapple_AnalyzeSurface(&play, &dyna->polyList[3], 95, NULL, &result);
    assert(result.dims[0] == 0 && result.dims[1] == 20 && result.dims[2] == 100);
    dyna->polyList[3].vIC = 70004;
    assert(!Grapple_AnalyzeSurface(&play, &dyna->polyList[3], 95, NULL, &result));
    free(dyna->polyList);
    free(dyna->bgActors);
    free(dyna->bgActorFlags);
    free(wide);
    puts("PASS grapple: wide coordinates/indices and current dynamic polygon range, with invalid-index rejection");
}
