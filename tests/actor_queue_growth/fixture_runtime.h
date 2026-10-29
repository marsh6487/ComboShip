typedef struct Actor {
    void (*update)(void);
    CollisionCheckInfo colChkInfo;
} Actor;
typedef struct PlayState { CollisionCheckContext colChkCtx; } PlayState;
typedef s32 (*ColChkResetFunc)(PlayState*, Collider*);
typedef void (*ColChkVsFunc)(PlayState*, CollisionCheckContext*, Collider*, Collider*);
typedef void (*ColChkApplyFunc)(PlayState*, CollisionCheckContext*, Collider*);
typedef s32 (*ColChkLineFunc)(PlayState*, CollisionCheckContext*, Collider*, Vec3f*, Vec3f*);

static int registers[32];
#define AREG(index) registers[index]
static bool secondUpdate, frameAdvance;
static int acNotes, championNotes, teardownNotes, hitEffects;
static Collider* lastAcNoted;
static bool GameInteractor_SecondCollisionUpdate(void) { return secondUpdate; }
static bool FrameAdvance_IsEnabled(PlayState* play) { return frameAdvance; }
static void TimeCtl_NoteAcCollider(Collider* col) { ++acNotes; lastAcNoted = col; }
static void Champion_NoteIncomingAttacks(PlayState* play) { ++championNotes; }
static void DinFireSword_FinishTeardown(PlayState* play) { ++teardownNotes; }
static void CollisionCheck_HitEffects(PlayState* play, Collider* at, ColliderInfo* atInfo,
                                     Collider* ac, ColliderInfo* acInfo, Vec3f* hitPos) { ++hitEffects; }
#define osSyncPrintf(...) ((void)0)
void CollisionCheck_ClearContext(PlayState*, CollisionCheckContext*);

/* Intercept only allocation boundaries. Force moves to reproduce stale savestate pointers. */
static int failAllocation, failAtAllocation, allocationCalls, liveAllocations, moveAllocations;
typedef struct QueueTestAllocation { void* pointer; size_t size; } QueueTestAllocation;
static QueueTestAllocation queueTestAllocations[512];
static int QueueTest_FindAllocation(void* ptr) {
    for (int i = 0; i < 512; ++i) if (queueTestAllocations[i].pointer == ptr) return i;
    return -1;
}
static void* QueueTest_Realloc(void* ptr, size_t size) {
    ++allocationCalls;
    if (failAllocation || allocationCalls == failAtAllocation) {
        if (failAllocation) --failAllocation;
        return NULL;
    }
    int slot = QueueTest_FindAllocation(ptr);
    assert(slot >= 0 && size > 0);
    void* result;
    if (moveAllocations && ptr) {
        result = malloc(size);
        if (result) {
            memcpy(result, ptr, queueTestAllocations[slot].size < size ? queueTestAllocations[slot].size : size);
            free(ptr);
        }
    } else {
        result = realloc(ptr, size);
    }
    if (result) {
        if (!ptr) ++liveAllocations;
        queueTestAllocations[slot] = (QueueTestAllocation){result, size};
    }
    return result;
}
static void QueueTest_Free(void* ptr) {
    if (ptr) {
        int slot = QueueTest_FindAllocation(ptr);
        assert(slot >= 0); /* A saved heap pointer must never free an obsolete allocation. */
        queueTestAllocations[slot] = (QueueTestAllocation){0};
        --liveAllocations;
    }
    free(ptr);
}
#define realloc QueueTest_Realloc
#define free QueueTest_Free
