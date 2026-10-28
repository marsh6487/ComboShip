#include "global.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
PlayState* gPlayState;
#include "actor_adapters.inc"

static void ActorUpdate(Actor* actor, PlayState* play) { (void)actor; (void)play; }

int main(void) {
    static PlayState play;
    DynaPolyActor body = { 0 };
    Actor* actor = &body.actor;
    CollisionHeader authored = { 0 };
    s32 id = -1;
    f32 top = 0;
    DynaCollisionContext* dyna = &play.colCtx.dyna;
    gPlayState = &play;
    dyna->bgActorMax = 96;
    dyna->bgActors = calloc(96, sizeof(BgActor));
    dyna->bgActorFlags = calloc(96, sizeof(u16));
    actor->id = -1;
    actor->scale = (Vec3f){ 1, 2, 1 };
    actor->world.pos.y = 10;
    actor->update = ActorUpdate;
    body.bgId = 95;
    authored.minBounds = (Vec3i){ -400, -100, -300 };
    authored.maxBounds = (Vec3i){ 400, 600, 300 };
    dyna->bgActors[95].actor = actor;
    dyna->bgActors[95].colHeader = &authored;
    dyna->bgActorFlags[95] = 1;
    assert(Stasis_FindBg(&play, actor, &id) == &authored && id == 95);
    assert(Pacci_UhTooBig(actor));
    assert(Pacci_UhDynaTopY(&play, actor, &top) && top == 1210);
    assert(SwitchMagnet_DynaTopY(&play, actor, &top) && top == 1210);
    assert(SwitchMagnet_BodyBottom(&play, actor) == -200);
    WandSand_Measure(&play, actor);
    assert(sSandMeasured && sSandTopOffset == 30 && sSandReach == 15);
    StasisRing rings[STASIS_MAX_RINGS];
    StasisStack stacks[STASIS_MAX_STACKS];
    s32 stackCount;
    authored.maxBounds.y = 80000;
    assert(Stasis_GetProfile(actor, rings, stacks, &stackCount) == 2);
    assert(rings[0].y == -200 && rings[1].y == 160000 && rings[0].radius == 400);
    authored.maxBounds.y = 600;
    static Player player;
    Vec3f impact = { 180000.25f, -90000.5f, 70000.75f };
    Pacci_BurstSpawn(&play, &player, &impact, 1);
    assert(sPacciBurst.dim.pos.x == impact.x && sPacciBurst.dim.pos.y == impact.y &&
           sPacciBurst.dim.pos.z == impact.z);
    assert(sPacciBurst.base.actor == &player.actor && sPacciBurst.info.toucher.dmgFlags == DMG_HAMMER);
    assert(sPacciBurst.dim.radius == 34 && sPacciBurstTimer == 3);

    /* Freed/deleting slots can retain header storage. Consumers must use the
       active current registration instead of classifying stale geometry. */
    dyna->bgActorFlags[95] = 0;
    assert(Stasis_FindBg(&play, actor, &id) == NULL);
    assert(!Pacci_UhTooBig(actor));
    assert(!Pacci_UhDynaTopY(&play, actor, &top));
    dyna->bgActorFlags[95] = 1 | 2;
    assert(Stasis_FindBg(&play, actor, &id) == NULL);
    assert(!Pacci_UhTooBig(actor));
    assert(!SwitchMagnet_DynaTopY(&play, actor, &top));

    /* A full authored tree profile registers even before the growable native
       polygon/vertex buffers have been allocated for this frame. */
    Actor tree = { 0 };
    tree.id = ACTOR_EN_WOOD02;
    tree.scale = (Vec3f){ 1, 1, 1 };
    tree.update = ActorUpdate;
    const int shell = Stasis_BuildCollision(&play, &tree, 1);
    assert(shell >= 0 && shell < dyna->bgActorMax);
    const CollisionHeader* mesh = dyna->bgActors[shell].colHeader;
    assert(mesh && mesh->numVertices == 32 && mesh->numPolygons == 44);
    assert(mesh->minBounds.x == -180 && mesh->maxBounds.x == 180 && mesh->maxBounds.y == 480);
    assert(mesh->vtxList[0].x == 12 && mesh->vtxList[0].y == 0);
    assert(mesh->vtxList[8].x == 6 && mesh->vtxList[8].y == 480);
    assert(mesh->vtxList[16].x == 180 && mesh->vtxList[16].y == 108);
    assert(mesh->vtxList[24].x == 4 && mesh->vtxList[24].y == 480);
    assert(mesh->surfaceTypeList[0].wallType == 4);
    assert(mesh->surfaceTypeList[0].camera == 0 && mesh->surfaceTypeList[0].exit == 0);
    assert(mesh->cameraDataList && mesh->cameraDataListLen == 1);
    assert(dyna->bgActors[shell].actor != &tree);
    for (u32 p = 0; p < mesh->numPolygons; ++p)
        for (int v = 0; v < 3; ++v)
            assert(COLPOLY_VTX_INDEX(mesh->polyList[p].vtxData[v]) < 32);
    DynaPoly_DeleteBgActor(&play, dyna, shell);
    const int secondShell = Stasis_BuildCollision(&play, &tree, 0);
    assert(secondShell >= 0);
    assert(sStasisSurfPool[0].wallType == 0);
    sStasis.bgId = 95;
    sStasis.bgIsOurs = 1;
    dyna->bgActorFlags[95] = 1;
    Stasis_DropCollision();
    assert(dyna->bgActors[95].actor == actor); /* A reused id is not our carrier. */
    assert(sStasis.bgId == -1 && !sStasis.bgIsOurs);
    sStasis.bgId = secondShell;
    sStasis.bgIsOurs = 1;
    Stasis_DropCollision();
    assert(dyna->bgActors[secondShell].actor == NULL);
    free(dyna->bgActors);
    free(dyna->bgActorFlags);
    puts("PASS actors: slot95 dimensions, inactive/deleting lifetime guards and unchanged Stasis tree/camera/climb flags");
}
