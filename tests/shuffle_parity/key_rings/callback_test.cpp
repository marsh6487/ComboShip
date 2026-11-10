// Actual native callbacks, using the renderer fixture's controlled services.
#define main KeyRingCoreMain
#include "render_test.cpp"
#undef main
#include "combo/menu/ComboItemDrawABI.h"
#include <string>
#include <unordered_set>
static unsigned fallbacks,resolutions;
static bool ready=true,ringBase=true,ringAlt=false,alt=false,module=false,badPath=false;
static int expectedOwner;
static const char* const metalPath="__OTR__objects/metal";
static const char* const emblemPath="__OTR__objects/emblem";
#ifdef KEY_RING_TEST_MM
#include "mm/2s2h/Rando/DungeonItemVisuals.h"
PlayState* gPlayState=&play;
extern "C" {
u8 ResourceMgr_FileExists(const char* path) {
    assert(!std::strcmp(path,"__OTR__objects/object_keyring/gKeyringRingDL"));return ringBase;
}
u8 ResourceMgr_FileAltExists(const char* path) {
    assert(!std::strcmp(path,"__OTR__objects/object_keyring/gKeyringRingDL"));return ringAlt;
}
bool ResourceMgr_IsAltAssetsEnabled(){return alt;}
s32 GetItem_GetDungeonKeyModel(s16 gid,s32 owner,const char** metal,const char** emblem,Color_RGBA8* steel,Color_RGBA8* badge) {
    assert(gid==GID_KEY_SMALL&&owner==expectedOwner);*metal=metalPath;*emblem=emblemPath;
    *steel={213,224,236,255};*badge={201,38,41,255};return ready;
}
void GetItem_Draw(PlayState* state,s16 id) {
    assert(state==&play&&id==GID_KEY_SMALL&&requestedMatrices==1&&requestedOpa==32);++fallbacks;
}
}
#else
#include "soh/soh/Enhancements/randomizer/randomizerTypes.h"
static int32_t Describe(const char* name,CwItemDrawInfo* recipe) {
    static const char* names[]={"RI_WOODFALL_KEY_RING","RI_SNOWHEAD_KEY_RING","RI_GREAT_BAY_KEY_RING","RI_STONE_TOWER_KEY_RING"};
    assert(!std::strcmp(name,names[expectedOwner]));if(!ready)return CW_DRAW_NOT_READY;
    *recipe={};recipe->drawKind=CW_DRAW_KIND_MM_KEY_RING;recipe->dlistCount=3;
    recipe->dlists[0]=metalPath;recipe->dlists[1]=emblemPath;
    recipe->dlists[2]=badPath?"invalid":"__OTR__objects/object_keyring/gKeyringRingDL";
    recipe->opCount=1;recipe->ops[0].op=CW_OP_END;
    const int counts[]={1,3,1,4};recipe->ops[0].a=counts[expectedOwner];recipe->ops[0].b=expectedOwner;
    return 1;
}
static void* Combo_ResolveSym(const char* owner,const char* name) {
    assert(!std::strcmp(owner,"2ship")&&!std::strcmp(name,"MM_GetItemDrawInfo"));++resolutions;
    return module?reinterpret_cast<void*>(Describe):nullptr;
}
static const char* ComboInternRoutedPath(const std::string& path) {
    static std::unordered_set<std::string> pool;return pool.insert(path).first->c_str();
}
void Randomizer_DrawMmSmallKey(PlayState* state,GetItemEntry*) {
    assert(state==&play&&requestedMatrices==1&&requestedOpa==32);++fallbacks;
}
#endif
#include "key_ring_callback.inc"
int main() {
    const int counts[]={1,3,1,4};
#ifdef KEY_RING_TEST_MM
    const RandoItemId ids[]={RI_WOODFALL_KEY_RING,RI_SNOWHEAD_KEY_RING,RI_GREAT_BAY_KEY_RING,RI_STONE_TOWER_KEY_RING};
    auto draw=[&](int owner){expectedOwner=owner;DrawKeyRing(ids[owner]);};
#else
    const RandomizerGet ids[]={RG_MM_KEY_RING_WOODFALL,RG_MM_KEY_RING_SNOWHEAD,RG_MM_KEY_RING_GREAT_BAY,RG_MM_KEY_RING_STONE_TOWER};
    auto draw=[&](int owner){expectedOwner=owner;GetItemEntry entry{};entry.drawItemId=ids[owner];Randomizer_DrawMmKeyRing(&play,&entry);};
    Reset();draw(0);assert(fallbacks==1&&resolutions==1&&!models);
    module=true;Reset();draw(0);assert(resolutions==2&&models==2);
#endif
    for(int owner=0;owner<4;++owner) {
        Reset();const auto before=fallbacks;draw(owner);
        assert(fallbacks==before&&models==size_t(1+counts[owner])&&vertexReports==size_t(counts[owner]));
        Reset();available=false;draw(owner);assert(fallbacks==before&&!models&&!allocations&&graphics.polyOpa.p==opaque);
        ready=false;Reset();draw(owner);assert(fallbacks==before+1&&!models);
        Reset();available=false;draw(owner);assert(fallbacks==before+1&&!allocations);ready=true;
    }
#ifdef KEY_RING_TEST_MM
    ringBase=false;ringAlt=true;alt=false;Reset();const auto before=fallbacks;draw(3);assert(fallbacks==before+1&&!models);
    alt=true;Reset();draw(3);assert(fallbacks==before+1&&models==5);
#else
    badPath=true;Reset();const auto before=fallbacks;draw(3);assert(fallbacks==before+1&&!models);
#endif
    std::puts("PASS native ring callbacks: all owners, safe fallback, arena denial, module/Alt retry");return 0;
}
