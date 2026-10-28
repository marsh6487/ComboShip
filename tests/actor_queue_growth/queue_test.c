static void alive(void) {}
typedef s32 (*SetCollider)(PlayState*, CollisionCheckContext*, Collider*);
static SetCollider setters[] = { CollisionCheck_SetAT, CollisionCheck_SetAC, CollisionCheck_SetOC };
static s32 count(CollisionCheckContext* ctx, int kind) {
    return kind == 0 ? ctx->colATCount : kind == 1 ? ctx->colACCount : ctx->colOCCount;
}
static Collider** list(CollisionCheckContext* ctx, int kind) {
    return kind == 0 ? ctx->colAT : kind == 1 ? ctx->colAC : ctx->colOC;
}
static s32 capacity(CollisionCheckContext* ctx, int kind) {
    return kind == 0 ? ctx->colATCapacity : kind == 1 ? ctx->colACCapacity : ctx->colOCCapacity;
}
static void cylinder(ColliderCylinder* col, Actor* actor, float x) {
    memset(col, 0, sizeof(*col));
    memset(actor, 0, sizeof(*actor));
    actor->update = alive;
    actor->colChkInfo.mass = 10;
    col->base.actor = actor;
    col->base.shape = COLSHAPE_CYLINDER;
    col->base.atFlags = AT_ON | AT_TYPE_PLAYER;
    col->base.acFlags = AC_ON | AC_TYPE_PLAYER;
    col->base.ocFlags1 = OC1_ON | OC1_TYPE_ALL;
    col->base.ocFlags2 = OC2_TYPE_1;
    col->info.toucherFlags = TOUCH_ON;
    col->info.bumperFlags = BUMP_ON;
    col->info.ocElemFlags = OCELEM_ON;
    col->info.toucher.dmgFlags = DMG_SLASH_MASTER;
    col->info.bumper.dmgFlags = DMG_SLASH_MASTER;
    col->dim = (Cylinder16){ 10, 20, 0, { x, 0, 0 } };
}

static void growth(void) {
    PlayState play;
    CollisionCheck_InitContext(&play, &play.colChkCtx);
    Actor actors[257]; ColliderCylinder colliders[257];
    for (int i = 0; i < 257; ++i) cylinder(&colliders[i], &actors[i], 100.f * i);
    for (int kind = 0; kind < 3; ++kind) {
        int initial = capacity(&play.colChkCtx, kind), calls = allocationCalls;
        assert(initial >= (kind == 1 ? 60 : 50));
        for (int i = 0; i < 257; ++i) {
            assert(setters[kind](&play, &play.colChkCtx, &colliders[i].base) == i);
            assert(count(&play.colChkCtx, kind) == i + 1);
            assert(capacity(&play.colChkCtx, kind) >= i + 1);
            if (i < initial) assert(allocationCalls == calls);
            if (i == initial) assert(allocationCalls == calls + 1);
            for (int j = 0; j <= i; ++j) assert(list(&play.colChkCtx, kind)[j] == &colliders[j].base);
        }
    }
    assert(acNotes == 257 && lastAcNoted == &colliders[256].base);
    Collider** at = play.colChkCtx.colAT; Collider** ac = play.colChkCtx.colAC; Collider** oc = play.colChkCtx.colOC;
    int calls = allocationCalls;
    CollisionCheck_ClearContext(&play, &play.colChkCtx);
    for (int kind = 0; kind < 3; ++kind) {
        assert(!count(&play.colChkCtx, kind));
        for (int i = 0; i < 257; ++i) assert(list(&play.colChkCtx, kind)[i] == NULL);
        for (int i = 0; i < 257; ++i) assert(setters[kind](&play, &play.colChkCtx, &colliders[i].base) == i);
    }
    assert(play.colChkCtx.colAT == at && play.colChkCtx.colAC == ac && play.colChkCtx.colOC == oc);
    assert(allocationCalls == calls);
    CollisionCheck_DestroyContext(&play, &play.colChkCtx);
    assert(!liveAllocations && teardownNotes == 1);
    for (int kind = 0; kind < 3; ++kind)
        assert(!count(&play.colChkCtx, kind) && !list(&play.colChkCtx, kind) && !capacity(&play.colChkCtx, kind));
}

static void sac(void) {
    PlayState play;
    CollisionCheck_InitContext(&play, &play.colChkCtx);
    Actor actor[2]; ColliderCylinder col[2];
    for (int i = 0; i < 2; ++i) cylinder(&col[i], &actor[i], 0);
    assert(CollisionCheck_SetAT_SAC(&play, &play.colChkCtx, &col[0].base, 123) == 0);
    assert(CollisionCheck_SetAC_SAC(&play, &play.colChkCtx, &col[0].base, 123) == 0);
    for (int i = 0; i < 137; ++i) assert(CollisionCheck_SetOC_SAC(&play, &play.colChkCtx, &col[0].base, -100) == i);
    CollisionCheck_EnableSAC(&play, &play.colChkCtx);
    assert(CollisionCheck_SetOC_SAC(&play, &play.colChkCtx, &col[1].base, 136) == 136);
    assert(play.colChkCtx.colOC[136] == &col[1].base && play.colChkCtx.colAT[0] == &col[0].base);
    assert(CollisionCheck_SetAT_SAC(&play, &play.colChkCtx, &col[1].base, 0) == 0);
    assert(CollisionCheck_SetAC_SAC(&play, &play.colChkCtx, &col[1].base, 0) == 0);
    assert(CollisionCheck_SetOC_SAC(&play, &play.colChkCtx, &col[1].base, -1) == -1);
    assert(CollisionCheck_SetAT_SAC(&play, &play.colChkCtx, &col[1].base, -1) == -1);
    assert(CollisionCheck_SetAC_SAC(&play, &play.colChkCtx, &col[1].base, -1) == -1);
    assert(CollisionCheck_SetOC_SAC(&play, &play.colChkCtx, &col[1].base, 137) == -1);
    for (int kind = 0; kind < 3; ++kind) assert(setters[kind](&play, &play.colChkCtx, &col[0].base) == -1);
    CollisionCheck_ClearContext(&play, &play.colChkCtx);
    assert(play.colChkCtx.colOCCount == 137 && play.colChkCtx.colOC[136] == &col[1].base);
    CollisionCheck_DisableSAC(&play, &play.colChkCtx);
    CollisionCheck_ClearContext(&play, &play.colChkCtx);
    assert(!play.colChkCtx.colOCCount && !play.colChkCtx.colOC[136]);
    CollisionCheck_DestroyContext(&play, &play.colChkCtx);
    assert(!liveAllocations);
}

static void lifecycle(void) {
    PlayState play[2];
    memset(play, 0xA5, sizeof(play));
    Actor actor; ColliderCylinder col; cylinder(&col, &actor, 0);
    for (int i = 0; i < 2; ++i) {
        CollisionCheck_InitContext(&play[i], &play[i].colChkCtx);
        for (int j = 0; j < 257; ++j) assert(CollisionCheck_SetOC(&play[i], &play[i].colChkCtx, &col.base) == j);
    }
    Collider** other = play[1].colChkCtx.colOC;
    CollisionCheck_InitContext(&play[0], &play[0].colChkCtx);
    assert(!play[0].colChkCtx.colOCCount && !play[0].colChkCtx.sacFlags);
    assert(play[1].colChkCtx.colOCCount == 257 && play[1].colChkCtx.colOC == other);
    CollisionCheck_DestroyContext(&play[1], &play[1].colChkCtx);
    CollisionCheck_DestroyContext(&play[1], &play[1].colChkCtx);
    memset(&play[1], 0x5A, sizeof(play[1]));
    CollisionCheck_InitContext(&play[1], &play[1].colChkCtx);
    assert(CollisionCheck_SetOC(&play[1], &play[1].colChkCtx, &col.base) == 0);
    assert(CollisionCheck_SetAT(&play[0], &play[0].colChkCtx, &col.base) == 0);
    CollisionCheck_DestroyContext(&play[0], &play[0].colChkCtx);
    CollisionCheck_DestroyContext(&play[1], &play[1].colChkCtx);
    assert(!liveAllocations);
}

static void failure(void) {
    PlayState play;
    failAllocation = 3;
    CollisionCheck_InitContext(&play, &play.colChkCtx);
    Actor actor; ColliderCylinder col; cylinder(&col, &actor, 0);
    CollisionCheck_AT(&play, &play.colChkCtx);
    CollisionCheck_OC(&play, &play.colChkCtx);
    CollisionCheck_AC(&play, &play.colChkCtx, &col.base);
    CollisionCheck_SetHitEffects(&play, &play.colChkCtx);
    Vec3f origin = {0};
    assert(!CollisionCheck_LineOC(&play, &play.colChkCtx, &origin, &origin, NULL, 0));
    CollisionCheck_ClearContext(&play, &play.colChkCtx);
    for (int kind = 0; kind < 3; ++kind) {
        failAllocation = 1;
        assert(setters[kind](&play, &play.colChkCtx, &col.base) == -1);
        assert(!count(&play.colChkCtx, kind) && !list(&play.colChkCtx, kind));
    }
    failAllocation = 0;
    for (int kind = 0; kind < 3; ++kind) {
        int oldLimit = kind == 1 ? 60 : 50;
        for (int i = 0; i < oldLimit; ++i) assert(setters[kind](&play, &play.colChkCtx, &col.base) == i);
        Collider** before = list(&play.colChkCtx, kind);
        int notes = acNotes;
        failAllocation = 1;
        assert(setters[kind](&play, &play.colChkCtx, &col.base) == -1);
        assert(count(&play.colChkCtx, kind) == oldLimit && list(&play.colChkCtx, kind) == before);
        assert(acNotes == notes);
        for (int i = 0; i < oldLimit; ++i) assert(before[i] == &col.base);
        assert(setters[kind](&play, &play.colChkCtx, &col.base) == oldLimit);
    }
    CollisionCheck_DestroyContext(&play, &play.colChkCtx);
    assert(!liveAllocations);
}

static void guards(void) {
    PlayState play; Actor actor; ColliderCylinder col;
    CollisionCheck_InitContext(&play, &play.colChkCtx); cylinder(&col, &actor, 0);
    for (int gate = 0; gate < 2; ++gate) {
        secondUpdate = gate == 0; frameAdvance = gate == 1;
        for (int kind = 0; kind < 3; ++kind) {
            col.base.atFlags |= AT_HIT; col.base.acFlags |= AC_HIT; col.base.ocFlags1 |= OC1_HIT;
            assert(setters[kind](&play, &play.colChkCtx, &col.base) == -1);
            assert(!count(&play.colChkCtx, kind));
            assert(col.base.atFlags & AT_HIT); assert(col.base.acFlags & AC_HIT); assert(col.base.ocFlags1 & OC1_HIT);
        }
        assert(CollisionCheck_SetAT_SAC(&play, &play.colChkCtx, &col.base, 0) == -1);
        assert(CollisionCheck_SetAC_SAC(&play, &play.colChkCtx, &col.base, 0) == -1);
        assert(CollisionCheck_SetOC_SAC(&play, &play.colChkCtx, &col.base, 0) == -1);
    }
    secondUpdate = frameAdvance = false;
    actor.update = NULL;
    for (int kind = 0; kind < 3; ++kind) assert(setters[kind](&play, &play.colChkCtx, &col.base) == -1);
    assert(!(col.base.atFlags & AT_HIT) && !(col.base.acFlags & AC_HIT) && !(col.base.ocFlags1 & OC1_HIT));
    assert(!acNotes);
    CollisionCheck_DestroyContext(&play, &play.colChkCtx);
}

static void contacts(void) {
    PlayState play; Actor actors[75]; ColliderCylinder cols[75];
    CollisionCheck_InitContext(&play, &play.colChkCtx);
    for (int i = 0; i < 75; ++i) cylinder(&cols[i], &actors[i], 1000 + 100 * i);
    /* The late token collides with the player through the same native OC route. */
    cols[0].dim.pos.x = 0; cols[0].base.ocFlags2 = OC2_TYPE_PLAYER;
    cols[74].dim.pos.x = 15;
    for (int i = 0; i < 75; ++i) assert(CollisionCheck_SetOC(&play, &play.colChkCtx, &cols[i].base) == i);
    CollisionCheck_OC(&play, &play.colChkCtx);
    assert(cols[74].base.ocFlags2 & OC2_HIT_PLAYER);
    assert(cols[74].base.oc == &actors[0] && cols[0].base.oc == &actors[74]);
    assert(actors[0].colChkInfo.displacement.x == -2.5f && actors[74].colChkInfo.displacement.x == 2.5f);
    CollisionCheck_ClearContext(&play, &play.colChkCtx);
    /* Late attacks and receivers reach native AC dispatch after both old limits. */
    for (int i = 0; i < 75; ++i) cylinder(&cols[i], &actors[i], 1000 + 100 * i);
    cols[0].dim.pos.x = 0; cols[74].dim.pos.x = 5;
    assert(CollisionCheck_SetAT(&play, &play.colChkCtx, &cols[0].base) == 0);
    for (int i = 1; i < 75; ++i) assert(CollisionCheck_SetAC(&play, &play.colChkCtx, &cols[i].base) == i - 1);
    CollisionCheck_AT(&play, &play.colChkCtx);
    assert(cols[74].info.acHit == &cols[0].base && cols[0].info.atHit == &cols[74].base);
    CollisionCheck_ClearContext(&play, &play.colChkCtx);
    for (int i = 0; i < 75; ++i) cylinder(&cols[i], &actors[i], 1000 + 100 * i);
    cols[0].dim.pos.x = 0; cols[74].dim.pos.x = 5;
    for (int i = 1; i < 75; ++i) assert(CollisionCheck_SetAT(&play, &play.colChkCtx, &cols[i].base) == i - 1);
    assert(CollisionCheck_SetAC(&play, &play.colChkCtx, &cols[0].base) == 0);
    CollisionCheck_AT(&play, &play.colChkCtx);
    assert(cols[0].info.acHit == &cols[74].base && cols[74].info.atHit == &cols[0].base);
    CollisionCheck_ClearContext(&play, &play.colChkCtx);
    /* In native registration order the last compatible contact owns atHit. */
    for (int i = 0; i < 4; ++i) cylinder(&cols[i], &actors[i], i * 2.f);
    cols[1].base.acFlags |= AC_HARD;
    cols[2].info.bumper.dmgFlags = DMG_ARROW_NORMAL;
    assert(CollisionCheck_SetAT(&play, &play.colChkCtx, &cols[0].base) == 0);
    for (int i = 1; i < 4; ++i) assert(CollisionCheck_SetAC(&play, &play.colChkCtx, &cols[i].base) == i - 1);
    CollisionCheck_AT(&play, &play.colChkCtx);
    assert(championNotes == 3 && (cols[0].base.atFlags & AT_BOUNCED));
    assert(cols[0].info.atHit == &cols[3].base && cols[3].info.acHit == &cols[0].base);
    assert(cols[1].base.acFlags & AC_HIT); assert(!(cols[2].base.acFlags & AC_HIT));
    CollisionCheck_ClearContext(&play, &play.colChkCtx);
    /* The original OC pair order determines each collider's last contact. */
    for (int i = 0; i < 3; ++i) {
        cylinder(&cols[i], &actors[i], 2.f * i);
        cols[i].base.ocFlags1 |= OC1_NO_PUSH;
        assert(CollisionCheck_SetOC(&play, &play.colChkCtx, &cols[i].base) == i);
    }
    CollisionCheck_OC(&play, &play.colChkCtx);
    assert(cols[0].base.oc == &actors[2] && cols[1].base.oc == &actors[2] && cols[2].base.oc == &actors[1]);
    CollisionCheck_ClearContext(&play, &play.colChkCtx);
    /* Native OC masks, disabled elements, horse exclusions and same actor gates. */
    for (int i = 0; i < 4; ++i) cylinder(&cols[i], &actors[i], 0);
    cols[0].base.ocFlags1 = OC1_ON | OC1_TYPE_PLAYER;
    cols[1].base.ocFlags2 = OC2_TYPE_1;
    cols[2].info.ocElemFlags = 0;
    cols[3].base.actor = &actors[0];
    for (int i = 0; i < 4; ++i) assert(CollisionCheck_SetOC(&play, &play.colChkCtx, &cols[i].base) == i);
    CollisionCheck_OC(&play, &play.colChkCtx);
    assert(!(cols[0].base.ocFlags1 & OC1_HIT));
    assert(!(cols[2].info.ocElemFlags & OCELEM_HIT));
    cols[0].base.ocFlags1 = OC1_ON | OC1_TYPE_ALL; cols[0].base.ocFlags2 |= OC2_UNK1;
    cols[1].base.ocFlags2 |= OC2_UNK2;
    assert(CollisionCheck_Incompatible(&cols[0].base, &cols[1].base) == 1);
    CollisionCheck_DestroyContext(&play, &play.colChkCtx);
    assert(!liveAllocations);
}

static void wide(void) {
    PlayState play; Actor actor; ColliderCylinder col;
    CollisionCheck_InitContext(&play, &play.colChkCtx); cylinder(&col, &actor, 0);
    for (int i = 0; i < 40000; ++i) assert(CollisionCheck_SetAT(&play, &play.colChkCtx, &col.base) == i);
    assert(play.colChkCtx.colATCount == 40000 && play.colChkCtx.colAT[39999] == &col.base);
    int calls = allocationCalls;
    play.colChkCtx.colATCount = INT32_MAX;
    assert(CollisionCheck_SetAT(&play, &play.colChkCtx, &col.base) == -1);
    assert(play.colChkCtx.colATCount == INT32_MAX && allocationCalls == calls);
    CollisionCheck_DestroyContext(&play, &play.colChkCtx);
    assert(!liveAllocations);
}

int main(int argc, char** argv) {
    assert(argc == 2);
    if (!strcmp(argv[1], "growth")) growth();
    else if (!strcmp(argv[1], "sac")) sac();
    else if (!strcmp(argv[1], "lifecycle")) lifecycle();
    else if (!strcmp(argv[1], "failure")) failure();
    else if (!strcmp(argv[1], "guards")) guards();
    else if (!strcmp(argv[1], "contacts")) contacts();
    else if (!strcmp(argv[1], "wide")) wide();
    else assert(false);
    return 0;
}
