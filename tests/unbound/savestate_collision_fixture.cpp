#include "global.h"
#include "soh/Enhancements/savestates.h"
#include "soh/Enhancements/BgCheckStateSnapshot.h"
#include "soh/unbound/SceneDB.h"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

static size_t allocationIndex, failAllocation;
static bool countAllocations;
extern "C" void* __real_malloc(size_t);
extern "C" void* __real_realloc(void*, size_t);
extern "C" void __real_free(void*);
extern "C" void* __real__Znwm(size_t);
static void* trackedAllocations[4096];
static size_t trackedCount;
static void Track(void* pointer) {
    if (pointer) { assert(trackedCount<4096); trackedAllocations[trackedCount++]=pointer; }
}
static void Untrack(void* pointer) {
    for(size_t i=0;i<trackedCount;++i) if(trackedAllocations[i]==pointer) {
        trackedAllocations[i]=trackedAllocations[--trackedCount]; return;
    }
}
static bool RefuseAllocation() { return countAllocations && ++allocationIndex == failAllocation; }
extern "C" void* __wrap_malloc(size_t size) {
    if(RefuseAllocation()) return nullptr;
    void* pointer=__real_malloc(size);Track(pointer);return pointer;
}
extern "C" void* __wrap_realloc(void* pointer, size_t size) {
    if(RefuseAllocation()) return nullptr;
    void* replacement=__real_realloc(pointer,size);
    if(replacement) { Untrack(pointer);Track(replacement); }
    return replacement;
}
extern "C" void __wrap_free(void* pointer) { Untrack(pointer);__real_free(pointer); }
extern "C" void* __wrap__Znwm(size_t size) { if(RefuseAllocation()) throw std::bad_alloc(); return __real__Znwm(size); }
extern "C" {
PlayState* gPlayState;
u8* gSystemHeap;
u8* gAudioHeap;
AudioContext gAudioContext;
SaveContext gSaveContext;
EffectContext sEffectContext;
ActiveSequence gActiveSeqs[4];
ActiveSound gActiveSounds[7][MAX_CHANNELS_PER_BANK];
u8 gSoundBankMuted[7], D_801333F0, gAudioSfxSwapOff, gAudioSfxSwapMode[10];
u16 gAudioSfxSwapSource[10], gAudioSfxSwapTarget[10];
void (*D_801755D0)(void);
static GameInfo gameInfo;
GameInfo* gGameInfo = &gameInfo;
int Fixture_GrowGeometry(CollisionContext*);
void Fixture_GrowTables(CollisionContext*);
int Fixture_Append(CollisionCheckContext*,int,Collider*);
StaticLookup* BgCheck_GetStaticLookup(CollisionContext*,StaticLookup*,Vec3f*);
f32 BgCheck_RaycastFloorStaticList(CollisionContext*,u16,SSList*,CollisionPoly**,Vec3f*,f32,f32,s32);
}
static unsigned resets, surfaceResets;
extern "C" void TimeCtl_Reset(PlayState*) { ++resets; }
extern "C" void Sm64Surfaces_RefreshActorColliders(PlayState* play) { assert(play==nullptr); ++surfaceResets; }
struct { std::mutex mutex; } audio;
std::unordered_map<int32_t,SavedSceneFlags> sCustomSceneFlags;
using ExtBitset = std::vector<uint32_t>;
std::unordered_map<int32_t,ExtBitset> sExtClearFlags,sExtLiveClearFlags,sExtTempClearFlags;
/* ACTUAL SCENE FLAGS */
/* ACTUAL SAVESTATE DATA */
class SaveState {
  public:
    std::shared_ptr<SaveStateInfo> info;
    SaveStateReturn Save();
    SaveStateReturn Load();
    void BackupSeqScriptState() {}
    void LoadSeqScriptState() {}
    void SaveOverlayStaticData() {}
    void LoadOverlayStaticData() {}
    void SaveTransitionActors();
    void LoadTransitionActors();
};
/* ACTUAL TRANSITION RESTORE */
/* ACTUAL TRANSITION CAPTURE */
/* ACTUAL SAVE */
/* ACTUAL LOAD */

static void CheckEmptyContext(PlayState* play) {
    auto& context=play->colCtx;
    assert(context.colHeader && context.colHeader->numPolygons==0 && context.colHeader->numVertices==0);
    Vec3f point{1,1,1};
    auto* cell=BgCheck_GetStaticLookup(&context,context.lookupTbl,&point);
    assert(cell && cell->floor.head==UINT32_MAX);
    CollisionPoly* poly=nullptr;
    assert(BgCheck_RaycastFloorStaticList(&context,0,&cell->floor,&poly,&point,BGCHECK_Y_MIN,1,0)==BGCHECK_Y_MIN);
    assert(poly==nullptr);
}

int main() {
    static_assert(sizeof(PlayState)<2*1024*1024);
    gSystemHeap=static_cast<u8*>(std::calloc(1,SYSTEM_HEAP_SIZE));
    gAudioHeap=static_cast<u8*>(std::calloc(1,AUDIO_HEAP_SIZE));
    gPlayState=reinterpret_cast<PlayState*>(gSystemHeap);
    auto* play=gPlayState;
    play->state.running=1;play->sceneNum=0;play->roomCtx.curRoom.num=0;play->roomCtx.prevRoom.num=-1;
    play->sceneSegment=reinterpret_cast<void*>(2);play->roomCtx.curRoom.segment=reinterpret_cast<void*>(3);
    // Fresh and repeated collisionless setup must not free uninitialized bytes or depend on a collision command.
    std::memset(&play->colCtx,0xA5,sizeof(play->colCtx));
    BgCheck_InitEmpty(&play->colCtx,play);CheckEmptyContext(play);
    auto firstEpoch=BgCheck_GetAllocationEpoch(&play->colCtx);
    CollisionHeader normal{};Vec3i staticVertices[3]{};CollisionPoly staticPoly{};
    normal.numVertices=3;normal.vtxList=staticVertices;normal.numPolygons=1;normal.polyList=&staticPoly;
    BgCheck_Allocate(&play->colCtx,play,&normal);
    assert(BgCheck_GetAllocationEpoch(&play->colCtx)!=firstEpoch);
    BgCheck_InitEmpty(&play->colCtx,play);CheckEmptyContext(play);
    CollisionCheck_InitContext(play,&play->colChkCtx);
    auto* actor=reinterpret_cast<Actor*>(gSystemHeap+2*1024*1024);
    auto* collider=reinterpret_cast<ColliderCylinder*>(actor+1);
    collider->base.actor=actor;
    actor->floorPoly=&play->colCtx.dyna.polyList[0];actor->floorPoly->dist=123;
    actor->params=7;
    assert(Fixture_Append(&play->colChkCtx,0,&collider->base)==0);
    assert(Fixture_Append(&play->colChkCtx,1,&collider->base)==0);
    sCustomSceneFlags[128].chest=7;sExtClearFlags[128]={1,2};sExtLiveClearFlags[128]={3,4};sExtTempClearFlags[128]={5,6};
    SaveState state;
    assert(state.Save()==SaveStateReturn::SUCCESS);
    auto& saved=*state.info;
    assert(saved.collision && saved.queues);
    auto* originalSnapshot=state.info.get();
    countAllocations=true;allocationIndex=0;failAllocation=1;
    assert(state.Save()==SaveStateReturn::FAIL_NO_MEMORY);
    countAllocations=false;assert(state.info.get()==originalSnapshot);
    CollisionContext other;
    std::memset(&other,0xA5,sizeof(other));BgCheck_InitEmpty(&other,play);
    auto otherEpoch=BgCheck_GetAllocationEpoch(&other);
    auto* originalPoly=actor->floorPoly;
    CollisionHeader large{};large.numVertices=16385;large.numPolygons=16385;
    auto grow=[&] {
        play->colCtx.dyna.bgActorFlags[1]=1;play->colCtx.dyna.bgActors[1].colHeader=&large;
        assert(Fixture_GrowGeometry(&play->colCtx));
        Fixture_GrowTables(&play->colCtx);
        for(int i=0;i<100;++i) assert(Fixture_Append(&play->colChkCtx,0,&collider->base)>=0);
        originalPoly->dist=999;actor->params=99;sCustomSceneFlags[128].chest=99;
        sExtClearFlags[128]={99};sExtLiveClearFlags[128]={99};sExtTempClearFlags[128]={99};
    };
    play->colCtx.dyna.bgActorFlags[1]=1;play->colCtx.dyna.bgActors[1].colHeader=&large;
    size_t ownedBefore=BgCheck_GetGeometryBuffers(&play->colCtx,nullptr,0);
    auto* originalVertices=play->colCtx.dyna.vtxList;
    for(size_t failure=1;failure<=4;++failure) {
        countAllocations=true;allocationIndex=0;failAllocation=failure;
        assert(!Fixture_GrowGeometry(&play->colCtx));countAllocations=false;
        assert(play->colCtx.dyna.polyList==originalPoly && play->colCtx.dyna.vtxList==originalVertices);
        assert(BgCheck_GetGeometryBuffers(&play->colCtx,nullptr,0)==ownedBefore);
    }
    grow();
    auto* newerPolys=play->colCtx.dyna.polyList;
    originalPoly->dist=456;newerPolys[0].dist=789;
    SaveState second;assert(second.Save()==SaveStateReturn::SUCCESS);
    originalPoly->dist=999;newerPolys[0].dist=888;
    // Measure staging only; all allocation sites then fail in turn before a single world byte changes.
    countAllocations=true;allocationIndex=0;failAllocation=SIZE_MAX;
    auto plan=saved.collision->Prepare(&play->colCtx);
    QueueRestore queues(CollisionCheck_PrepareQueueRestore(&play->colChkCtx,saved.queues.get()));
    auto flags=saved.sceneFlags;
    size_t allocations=allocationIndex;countAllocations=false;plan.reset();queues.reset();
    assert(allocations>10);
    std::vector<u8> before(gSystemHeap,gSystemHeap+SYSTEM_HEAP_SIZE);
    for(size_t failure=1;failure<=allocations;++failure) {
        countAllocations=true;allocationIndex=0;failAllocation=failure;
        auto result=state.Load();countAllocations=false;
        assert(result==SaveStateReturn::FAIL_NO_MEMORY);
        assert(std::memcmp(before.data(),gSystemHeap,SYSTEM_HEAP_SIZE)==0 && originalPoly->dist==999);
        assert(sCustomSceneFlags[128].chest==99 && sExtLiveClearFlags[128][0]==99);
    }
    for(int retry=0;retry<18;++retry) {
        assert(state.Load()==SaveStateReturn::SUCCESS);
        assert(actor->params==7 && actor->floorPoly==originalPoly && originalPoly->dist==123);
        assert(BgCheck_GetAllocationEpoch(&other)==otherEpoch);
        assert(play->colChkCtx.colATCount==1 && play->colChkCtx.colACCount==1);
        assert(play->colChkCtx.colAT[0]->actor->floorPoly->dist==123);
        assert(sCustomSceneFlags[128].chest==7 && sExtClearFlags[128][1]==2 && sExtLiveClearFlags[128][1]==4 && sExtTempClearFlags[128][1]==6);
        CollisionCheck_ClearContext(play,&play->colChkCtx);
        assert(Fixture_Append(&play->colChkCtx,2,&collider->base)==0);
        if(retry<17) grow();
    }
    assert(BgCheck_GetGeometryBuffers(&play->colCtx,nullptr,0)>32 && resets==18 && surfaceResets==18);
    // Another slot may target a newer current buffer and an older retired buffer; neither may be freed by load.
    assert(second.Load()==SaveStateReturn::SUCCESS);
    assert(play->colCtx.dyna.polyList==newerPolys && newerPolys[0].dist==789);
    assert(actor->floorPoly==originalPoly && originalPoly->dist==456);
    before.assign(gSystemHeap,gSystemHeap+SYSTEM_HEAP_SIZE);
    auto stale=saved.collision->Prepare(&play->colCtx);assert(stale && stale->Validate());
    BgCheck_InvalidateRoomSnapshots(&play->colCtx);
    assert(!stale->Validate() && !stale->Commit());stale.reset();
    assert(state.Load()==SaveStateReturn::FAIL_STATE_MISMATCH && std::memcmp(before.data(),gSystemHeap,SYSTEM_HEAP_SIZE)==0);
    CollisionCheck_DestroyContext(play,&play->colChkCtx);
    BgCheck_Free(&play->colCtx);BgCheck_Free(&play->colCtx);
    assert(state.Load()==SaveStateReturn::FAIL_STATE_MISMATCH);
    BgCheck_InitEmpty(&play->colCtx,play);CheckEmptyContext(play);
    assert(state.Load()==SaveStateReturn::FAIL_STATE_MISMATCH);
    BgCheck_Free(&play->colCtx);
    assert(BgCheck_GetAllocationEpoch(&other)==otherEpoch);BgCheck_Free(&other);
    state.info.reset();second.info.reset();gPlayState=nullptr;
    std::free(gSystemHeap);std::free(gAudioHeap);
    assert(trackedCount==0);
}
