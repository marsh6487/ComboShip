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

/* Faults intercept only the allocator boundary. Native realloc/free own memory. */
static int failAllocation, allocationCalls, liveAllocations;
static void* QueueTest_Realloc(void* ptr, size_t size) {
    ++allocationCalls;
    if (failAllocation) {
        --failAllocation;
        return NULL;
    }
    void* result = realloc(ptr, size);
    if (result && !ptr) ++liveAllocations;
    return result;
}
static void QueueTest_Free(void* ptr) {
    if (ptr) --liveAllocations;
    free(ptr);
}
#define realloc QueueTest_Realloc
#define free QueueTest_Free
