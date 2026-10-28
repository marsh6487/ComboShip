#include "global.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "wide_motion.inc"

int main(void) {
    static PlayState play;
    DynaCollisionContext* dyna = &play.colCtx.dyna;
    dyna->bgActorMax = 96;
    dyna->bgActors = calloc(96, sizeof(BgActor));
    dyna->bgActorFlags = calloc(96, sizeof(u16));
    dyna->bgActorFlags[95] = 1;
    dyna->bgActors[95].curTransform.pos = (Vec3f){ 4000, 20, -10 };
    dyna->bgActors[95].curTransform.rot.y = (s16)0x8000;
    assert(ProductionDynaSignature(&play) == 206950173u);
    dyna->bgActorFlags[95] = 0;
    assert(ProductionDynaSignature(&play) == 0);
    free(dyna->bgActors);
    free(dyna->bgActorFlags);

    Vec3f delta = { 40000.25f, -50000.5f, .75f };
    ColliderCylinder cylinder = { 0 };
    cylinder.base.shape = COLSHAPE_CYLINDER;
    SwitchHook_ShiftCollider(&cylinder.base, &delta);
    assert(cylinder.dim.pos.x == 40000.25f && cylinder.dim.pos.y == -50000.5f && cylinder.dim.pos.z == .75f);
    ColliderJntSphElement element = { 0 };
    ColliderJntSph sphere = { 0 };
    sphere.base.shape = COLSHAPE_JNTSPH;
    sphere.count = 1;
    sphere.elements = &element;
    SwitchHook_ShiftCollider(&sphere.base, &delta);
    assert(element.dim.worldSphere.center.x == 40000.25f && element.dim.worldSphere.center.y == -50000.5f &&
           element.dim.worldSphere.center.z == .75f);
    ColliderQuad quad = { 0 };
    quad.base.shape = COLSHAPE_QUAD;
    SwitchHook_ShiftCollider(&quad.base, &delta);
    for (int i = 0; i < 4; ++i)
        assert(quad.dim.quad[i].x == 40000.25f && quad.dim.quad[i].y == -50000.5f && quad.dim.quad[i].z == .75f);
    assert(quad.dim.dcMid.x == 40000.25f && quad.dim.dcMid.y == -50000.5f && quad.dim.dcMid.z == .75f);
    assert(quad.dim.baMid.x == 40000.25f && quad.dim.baMid.y == -50000.5f && quad.dim.baMid.z == .75f);
    ColliderTrisElement triangle = { 0 };
    ColliderTris tris = { 0 };
    tris.base.shape = COLSHAPE_TRIS;
    tris.count = 1;
    tris.elements = &triangle;
    triangle.dim.plane.normal = (Vec3f){ 0, 1, 0 };
    SwitchHook_ShiftCollider(&tris.base, &delta);
    assert(triangle.dim.plane.originDist == 50000.5f);
    for (int i = 0; i < 3; ++i)
        assert(triangle.dim.vtx[i].x == 40000.25f && triangle.dim.vtx[i].y == -50000.5f &&
               triangle.dim.vtx[i].z == .75f);
    puts("PASS motion: slot95 SM64 refresh signature and precise wide/fractional Switch Hook shape translation");
}
