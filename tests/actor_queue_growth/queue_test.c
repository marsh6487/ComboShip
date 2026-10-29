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

/* Teardown must use authoritative ownership even if a raw heap rewind restored freed pointers. */
static void rewind_teardown(void) {
    PlayState play; Actor actor; ColliderCylinder col;
    CollisionCheck_InitContext(&play, &play.colChkCtx); cylinder(&col, &actor, 0);
    CollisionCheckContext saved = play.colChkCtx;
    moveAllocations = 1;
    for (int kind = 0; kind < 3; ++kind) {
        int initial = capacity(&saved, kind);
        for (int i = 0; i <= initial; ++i) assert(setters[kind](&play, &play.colChkCtx, &col.base) == i);
        assert(list(&play.colChkCtx, kind) != list(&saved, kind));
    }
    memcpy(&play.colChkCtx, &saved, sizeof(saved));
    CollisionCheck_DestroyContext(&play, &play.colChkCtx);
    assert(!liveAllocations);
}

static void snapshot_rebind(void) {
    PlayState play, other;
    Actor actors[145]; ColliderCylinder cols[145]; OcLine lines[COLLISION_CHECK_OC_LINE_MAX] = {0};
    Collider* expected[3][137];
    CollisionCheck_InitContext(&play, &play.colChkCtx);
    CollisionCheck_InitContext(&other, &other.colChkCtx);
    for (int i = 0; i < 145; ++i) cylinder(&cols[i], &actors[i], 100.f * i);
    for (int kind = 0; kind < 3; ++kind) {
        for (int i = 0; i < 137; ++i) {
            expected[kind][i] = &cols[(i + kind * 3) % 145].base;
            assert(setters[kind](&play, &play.colChkCtx, expected[kind][i]) == i);
        }
    }
    for (int i = 0; i < COLLISION_CHECK_OC_LINE_MAX; ++i) {
        lines[i].ocFlags = OCLINE_HIT;
        assert(CollisionCheck_SetOCLine(&play, &play.colChkCtx, &lines[i]) == i);
    }
    play.colChkCtx.sacFlags = 0xA4;
    CollisionCheck_EnableSAC(&play, &play.colChkCtx);
    CollisionCheckContext saved = play.colChkCtx;
    uint64_t epoch = CollisionCheck_GetContextEpoch(&play.colChkCtx);
    assert(epoch != 0);
    CollisionCheckQueueSnapshot* snapshot = CollisionCheck_CaptureQueueSnapshot(&play.colChkCtx);
    assert(snapshot);
    // Mutate every saved list through the direct APIs, then move all allocations.
    assert(CollisionCheck_SetAT_SAC(&play, &play.colChkCtx, &cols[144].base, 136) == 136);
    assert(CollisionCheck_SetAC_SAC(&play, &play.colChkCtx, &cols[144].base, 136) == 136);
    assert(CollisionCheck_SetOC_SAC(&play, &play.colChkCtx, &cols[144].base, 136) == 136);
    CollisionCheck_DisableSAC(&play, &play.colChkCtx);
    moveAllocations = 1;
    for (int kind = 0; kind < 3; ++kind) {
        int oldCapacity = capacity(&saved, kind);
        for (int i = 137; i <= oldCapacity; ++i)
            assert(setters[kind](&play, &play.colChkCtx, &cols[144].base) == i);
        assert(list(&play.colChkCtx, kind) != list(&saved, kind));
    }
    // Reorder the external registry between save and load. No restored link may
    // remove or redirect another context's owner record.
    CollisionCheck_DestroyContext(&other, &other.colChkCtx);
    CollisionCheck_InitContext(&other, &other.colChkCtx);
    assert(CollisionCheck_SetOC(&other, &other.colChkCtx, &cols[144].base) == 0);
    Collider** otherQueue = other.colChkCtx.colOC;
    for (int load = 0; load < 4; ++load) {
        CollisionCheckQueueRestore* restore = CollisionCheck_PrepareQueueRestore(&play.colChkCtx, snapshot);
        assert(restore && CollisionCheck_ValidateQueueRestore(restore));
        int calls = allocationCalls;
        memcpy(&play.colChkCtx, &saved, sizeof(saved)); // Same raw heap operation as SaveState.
        failAllocation = 1; // Commit must not allocate, even when allocation would fail.
        assert(CollisionCheck_CommitQueueRestore(restore));
        assert(allocationCalls == calls && failAllocation == 1);
        failAllocation = 0;
        assert(!CollisionCheck_ValidateQueueRestore(restore) && !CollisionCheck_CommitQueueRestore(restore));
        CollisionCheck_FreeQueueRestore(restore);
        assert(CollisionCheck_GetContextEpoch(&play.colChkCtx) == epoch);
        assert(play.colChkCtx.sacFlags == saved.sacFlags && play.colChkCtx.colLineCount == COLLISION_CHECK_OC_LINE_MAX);
        for (int i = 0; i < COLLISION_CHECK_OC_LINE_MAX; ++i) assert(play.colChkCtx.colLine[i] == &lines[i]);
        for (int kind = 0; kind < 3; ++kind) {
            assert(count(&play.colChkCtx, kind) == 137 && capacity(&play.colChkCtx, kind) == capacity(&saved, kind));
            for (int i = 0; i < 137; ++i) assert(list(&play.colChkCtx, kind)[i] == expected[kind][i]);
            for (int i = 137; i < capacity(&saved, kind); ++i) assert(!list(&play.colChkCtx, kind)[i]);
        }
        assert(other.colChkCtx.colOC == otherQueue && other.colChkCtx.colOC[0] == &cols[144].base);
        CollisionCheck_ClearContext(&play, &play.colChkCtx); // SAC survives restore.
        assert(play.colChkCtx.colOCCount == 137);
        CollisionCheck_DisableSAC(&play, &play.colChkCtx);
        CollisionCheck_ClearContext(&play, &play.colChkCtx);
        for (int kind = 0; kind < 3; ++kind) {
            assert(!count(&play.colChkCtx, kind));
            assert(setters[kind](&play, &play.colChkCtx, &cols[144].base) == 0);
        }
    }
    CollisionCheck_FreeQueueSnapshot(snapshot);
    CollisionCheck_DestroyContext(&other, &other.colChkCtx);
    CollisionCheck_ClearContext(&play, &play.colChkCtx);
    // Real native contact dispatch still reaches a late OC and AC after restore.
    for (int i = 0; i < 75; ++i) cylinder(&cols[i], &actors[i], 1000 + 100 * i);
    cols[0].dim.pos.x = 0; cols[0].base.ocFlags2 = OC2_TYPE_PLAYER; cols[74].dim.pos.x = 15;
    for (int i = 0; i < 75; ++i) assert(CollisionCheck_SetOC(&play, &play.colChkCtx, &cols[i].base) == i);
    CollisionCheck_OC(&play, &play.colChkCtx);
    assert(cols[74].base.ocFlags2 & OC2_HIT_PLAYER);
    assert(cols[74].base.oc == &actors[0] && cols[0].base.oc == &actors[74]);
    CollisionCheck_ClearContext(&play, &play.colChkCtx);
    for (int i = 0; i < 75; ++i) cylinder(&cols[i], &actors[i], 1000 + 100 * i);
    cols[0].dim.pos.x = 0; cols[74].dim.pos.x = 5;
    assert(CollisionCheck_SetAT(&play, &play.colChkCtx, &cols[0].base) == 0);
    for (int i = 1; i < 75; ++i) assert(CollisionCheck_SetAC(&play, &play.colChkCtx, &cols[i].base) == i - 1);
    CollisionCheck_AT(&play, &play.colChkCtx);
    assert(cols[74].info.acHit == &cols[0].base && cols[0].info.atHit == &cols[74].base);
    CollisionCheck_DestroyContext(&play, &play.colChkCtx);
    assert(!CollisionCheck_GetContextEpoch(&play.colChkCtx) && !liveAllocations);
}

static void snapshot_oom(void) {
    PlayState play; Actor actor; ColliderCylinder col;
    CollisionCheck_InitContext(&play, &play.colChkCtx); cylinder(&col, &actor, 0);
    for (int kind = 0; kind < 3; ++kind)
        for (int i = 0; i < 137; ++i) assert(setters[kind](&play, &play.colChkCtx, &col.base) == i);
    CollisionCheckContext original = play.colChkCtx;
    int baseline = liveAllocations;
    // Snapshot object plus each of the three queue copies may independently fail.
    for (int fail = 1; fail <= 4; ++fail) {
        failAtAllocation = allocationCalls + fail;
        assert(!CollisionCheck_CaptureQueueSnapshot(&play.colChkCtx));
        assert(!memcmp(&play.colChkCtx, &original, sizeof(original)) && liveAllocations == baseline);
    }
    failAtAllocation = 0;
    CollisionCheckQueueSnapshot* snapshot = CollisionCheck_CaptureQueueSnapshot(&play.colChkCtx);
    assert(snapshot);
    baseline = liveAllocations;
    // Each prepared allocation fails before the world heap is touched. Original
    // queues and the reusable snapshot must remain intact after partial cleanup.
    for (int fail = 1; fail <= 4; ++fail) {
        failAtAllocation = allocationCalls + fail;
        assert(!CollisionCheck_PrepareQueueRestore(&play.colChkCtx, snapshot));
        assert(!memcmp(&play.colChkCtx, &original, sizeof(original)) && liveAllocations == baseline);
        for (int kind = 0; kind < 3; ++kind)
            for (int i = 0; i < 137; ++i) assert(list(&play.colChkCtx, kind)[i] == &col.base);
    }
    failAtAllocation = 0;
    CollisionCheckQueueRestore* abandoned = CollisionCheck_PrepareQueueRestore(&play.colChkCtx, snapshot);
    assert(abandoned && CollisionCheck_ValidateQueueRestore(abandoned));
    CollisionCheck_FreeQueueRestore(abandoned);
    assert(!memcmp(&play.colChkCtx, &original, sizeof(original)) && liveAllocations == baseline);
    CollisionCheckQueueRestore* independent = CollisionCheck_PrepareQueueRestore(&play.colChkCtx, snapshot);
    assert(independent);
    CollisionCheck_FreeQueueSnapshot(snapshot); // The prepared payload is independent.
    memcpy(&play.colChkCtx, &original, sizeof(original));
    assert(CollisionCheck_CommitQueueRestore(independent));
    CollisionCheck_FreeQueueRestore(independent);
    CollisionCheck_DestroyContext(&play, &play.colChkCtx);
    assert(!liveAllocations);
    // Owner and each initial reservation can also fail and recover on admission.
    for (int fail = 1; fail <= 4; ++fail) {
        memset(&play, 0xA5, sizeof(play));
        failAtAllocation = allocationCalls + fail;
        CollisionCheck_InitContext(&play, &play.colChkCtx);
        failAtAllocation = 0;
        CollisionCheckContext empty;
        memcpy(&empty, &play.colChkCtx, sizeof(empty));
        CollisionCheckQueueSnapshot* emptySnapshot = CollisionCheck_CaptureQueueSnapshot(&play.colChkCtx);
        assert((emptySnapshot != NULL) == (fail != 1)); // No owner exists if its allocation failed.
        for (int kind = 0; kind < 3; ++kind) assert(setters[kind](&play, &play.colChkCtx, &col.base) == 0);
        if (emptySnapshot != NULL) {
            CollisionCheckQueueRestore* emptyRestore = CollisionCheck_PrepareQueueRestore(&play.colChkCtx, emptySnapshot);
            assert(emptyRestore);
            memcpy(&play.colChkCtx, &empty, sizeof(empty));
            assert(CollisionCheck_CommitQueueRestore(emptyRestore));
            CollisionCheck_FreeQueueRestore(emptyRestore);
            CollisionCheck_FreeQueueSnapshot(emptySnapshot);
            for (int kind = 0; kind < 3; ++kind) {
                assert(!count(&play.colChkCtx, kind) && capacity(&play.colChkCtx, kind) == capacity(&empty, kind));
                assert(setters[kind](&play, &play.colChkCtx, &col.base) == 0);
            }
        }
        CollisionCheck_DestroyContext(&play, &play.colChkCtx);
        assert(!liveAllocations);
    }
    CollisionCheck_FreeQueueSnapshot(NULL);
    CollisionCheck_FreeQueueRestore(NULL);
}

static void snapshot_epochs(void) {
    PlayState play[3]; Actor actor; ColliderCylinder col; OcLine line = {0};
    memset(play, 0xA5, sizeof(play)); cylinder(&col, &actor, 0);
    for (int i = 0; i < 3; ++i) {
        CollisionCheck_InitContext(&play[i], &play[i].colChkCtx);
        assert(CollisionCheck_SetOC(&play[i], &play[i].colChkCtx, &col.base) == 0);
    }
    CollisionCheckQueueSnapshot* snapshot[3];
    uint64_t epochs[3];
    for (int i = 0; i < 3; ++i) {
        epochs[i] = CollisionCheck_GetContextEpoch(&play[i].colChkCtx);
        snapshot[i] = CollisionCheck_CaptureQueueSnapshot(&play[i].colChkCtx);
        assert(epochs[i] && snapshot[i]);
        for (int j = 0; j < i; ++j) assert(epochs[i] != epochs[j]);
    }
    int calls = allocationCalls;
    CollisionCheckContext other = play[1].colChkCtx;
    assert(!CollisionCheck_PrepareQueueRestore(&play[1].colChkCtx, snapshot[0]));
    assert(allocationCalls == calls && !memcmp(&other, &play[1].colChkCtx, sizeof(other)));
    assert(!CollisionCheck_CaptureQueueSnapshot(NULL) && !CollisionCheck_GetContextEpoch(NULL));
    assert(!CollisionCheck_PrepareQueueRestore(&play[0].colChkCtx, NULL));
    assert(!CollisionCheck_ValidateQueueRestore(NULL) && !CollisionCheck_CommitQueueRestore(NULL));
    CollisionCheckQueueRestore* plan = CollisionCheck_PrepareQueueRestore(&play[0].colChkCtx, snapshot[0]);
    assert(plan);
    // Destruction/address reuse of another owner changes list ordering only.
    CollisionCheck_DestroyContext(&play[1], &play[1].colChkCtx);
    assert(!CollisionCheck_GetContextEpoch(&play[1].colChkCtx));
    CollisionCheck_InitContext(&play[1], &play[1].colChkCtx);
    assert(CollisionCheck_GetContextEpoch(&play[1].colChkCtx) != epochs[1]);
    assert(CollisionCheck_SetOC(&play[1], &play[1].colChkCtx, &col.base) == 0);
    assert(CollisionCheck_ValidateQueueRestore(plan));
    calls = allocationCalls;
    assert(!CollisionCheck_PrepareQueueRestore(&play[1].colChkCtx, snapshot[1]) && allocationCalls == calls);
    assert(CollisionCheck_CommitQueueRestore(plan));
    CollisionCheck_FreeQueueRestore(plan);
    // Every native mutation of this owner's queue state invalidates a staged plan.
    for (int change = 0; change < 10; ++change) {
        CollisionCheck_DisableSAC(&play[0], &play[0].colChkCtx);
        CollisionCheck_ClearContext(&play[0], &play[0].colChkCtx);
        for (int kind = 0; kind < 3; ++kind) assert(setters[kind](&play[0], &play[0].colChkCtx, &col.base) == 0);
        if ((change >= 4 && change <= 6) || change == 9) CollisionCheck_EnableSAC(&play[0], &play[0].colChkCtx);
        plan = CollisionCheck_PrepareQueueRestore(&play[0].colChkCtx, snapshot[0]);
        assert(plan && CollisionCheck_ValidateQueueRestore(plan));
        if (change == 0) CollisionCheck_ClearContext(&play[0], &play[0].colChkCtx);
        else if (change <= 3) assert(setters[change - 1](&play[0], &play[0].colChkCtx, &col.base) == 1);
        else if (change == 4) assert(CollisionCheck_SetAT_SAC(&play[0], &play[0].colChkCtx, &col.base, 0) == 0);
        else if (change == 5) assert(CollisionCheck_SetAC_SAC(&play[0], &play[0].colChkCtx, &col.base, 0) == 0);
        else if (change == 6) assert(CollisionCheck_SetOC_SAC(&play[0], &play[0].colChkCtx, &col.base, 0) == 0);
        else if (change == 7) assert(CollisionCheck_SetOCLine(&play[0], &play[0].colChkCtx, &line) == 0);
        else if (change == 8) CollisionCheck_EnableSAC(&play[0], &play[0].colChkCtx);
        else CollisionCheck_DisableSAC(&play[0], &play[0].colChkCtx);
        CollisionCheckContext current = play[0].colChkCtx;
        assert(!CollisionCheck_ValidateQueueRestore(plan) && !CollisionCheck_CommitQueueRestore(plan));
        assert(!memcmp(&current, &play[0].colChkCtx, sizeof(current)));
        CollisionCheck_FreeQueueRestore(plan);
    }
    plan = CollisionCheck_PrepareQueueRestore(&play[0].colChkCtx, snapshot[0]);
    assert(plan);
    CollisionCheck_DestroyContext(&play[0], &play[0].colChkCtx);
    assert(!CollisionCheck_ValidateQueueRestore(plan) && !CollisionCheck_CommitQueueRestore(plan));
    memset(&play[0], 0x5A, sizeof(play[0]));
    CollisionCheck_InitContext(&play[0], &play[0].colChkCtx);
    calls = allocationCalls;
    CollisionCheckContext current = play[0].colChkCtx;
    assert(!CollisionCheck_PrepareQueueRestore(&play[0].colChkCtx, snapshot[0]));
    assert(!CollisionCheck_CommitQueueRestore(plan));
    assert(allocationCalls == calls && !memcmp(&current, &play[0].colChkCtx, sizeof(current)));
    CollisionCheck_FreeQueueRestore(plan); // Frees staged memory after its owner died.
    plan = CollisionCheck_PrepareQueueRestore(&play[2].colChkCtx, snapshot[2]);
    assert(plan);
    Collider** reused = play[2].colChkCtx.colOC;
    CollisionCheck_InitContext(&play[2], &play[2].colChkCtx); // Repeated Init also changes epoch.
    assert(play[2].colChkCtx.colOC == reused && CollisionCheck_GetContextEpoch(&play[2].colChkCtx) != epochs[2]);
    assert(!CollisionCheck_ValidateQueueRestore(plan) && !CollisionCheck_CommitQueueRestore(plan));
    calls = allocationCalls;
    assert(!CollisionCheck_PrepareQueueRestore(&play[2].colChkCtx, snapshot[2]) && allocationCalls == calls);
    CollisionCheck_FreeQueueRestore(plan);
    // Destroy in a different order from registration and release handles last.
    for (int i = 0; i < 3; ++i) CollisionCheck_DestroyContext(&play[(i + 1) % 3], &play[(i + 1) % 3].colChkCtx);
    for (int i = 0; i < 3; ++i) CollisionCheck_FreeQueueSnapshot(snapshot[i]);
    assert(!liveAllocations && !sCollisionCheckQueueOwners);
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
    else if (!strcmp(argv[1], "rewind_teardown")) rewind_teardown();
    else if (!strcmp(argv[1], "snapshot_rebind")) snapshot_rebind();
    else if (!strcmp(argv[1], "snapshot_oom")) snapshot_oom();
    else if (!strcmp(argv[1], "snapshot_epochs")) snapshot_epochs();
    else assert(false);
    return 0;
}
