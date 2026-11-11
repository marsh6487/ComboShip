#include "global.h"
#include "combo/rando/StartupCheckReceipt.h"
#include "combo/menu/ComboItemDrawABI.h"
#include "2s2h/CustomItem/CustomItem.h"
#include <cassert>
#include <cstring>
#include <string>
#include "2s2h/Rando/Types.h"
SaveContext gSaveContext{};
PlayState* gPlayState;
static int lookups, fills, sentinels, pickup;
static const CwStartupReceipt* observedReceipt;
static bool missing=false, animated=false;
static std::string name,shown;
struct ComboForeignDrawInfoOOT { std::string resolvedName;int nativeMmItem=-1;bool ok=false;CwItemAnimDrawInfo anim{}; };
enum class ComboForeignResolveOOT {Ok,Unknown,NotReady};
namespace ComboRando { enum { GAME_OOT=0 };struct ForeignItem {int itemGame=0;std::string itemName,fakeItemName;bool HasDisguise()const{return false;}}; }
namespace Rando::MiscBehavior { static const ComboRando::ForeignItem* MM_LookupForeign(RandoCheckId) { assert(false);return nullptr; } }
static int standardCalls, frozenCalls,imports,normalAnimCalls,frozenAnimCalls;
static int32_t ComboNativeMmImport(const char*) { ++imports;return -1; }
static int32_t NormalExporter(const char*,CwItemDrawInfo*) { ++standardCalls;return 1; }
static int32_t FrozenExporter(const CwStartupReceipt* receipt,CwItemDrawInfo* out) {
    out->dlistCount=1;out->drawKind=CW_DRAW_KIND_SIMPLE;
    ++frozenCalls;++fills;observedReceipt=receipt;name=receipt->itemName;
    assert(receipt->getItemId==17&&receipt->drawItemId==23&&receipt->gi==31&&receipt->field==37);
    return missing?CW_DRAW_NOT_READY:animated?0:1;
}
static int32_t FrozenAnimExporter(const CwStartupReceipt* receipt,CwItemAnimDrawInfo*) {
    ++frozenAnimCalls;assert(receipt==observedReceipt);assert(receipt->getItemId==17&&receipt->drawItemId==23);return 1;
}
static int32_t NormalAnimExporter(const char*,CwItemAnimDrawInfo*) { ++normalAnimCalls;return 1; }
static bool ResourceMgr_IsAltAssetsEnabled() { return false; }
static int32_t ComboSwordGi_SelectBaseAssets(CwItemDrawInfo&,bool) { return 0; }
static void* Combo_ResolveSym(const char*,const char* symbol) {
    if(!strcmp(symbol,"OOT_GetFrozenItemDrawInfo"))return reinterpret_cast<void*>(FrozenExporter);
    if(!strcmp(symbol,"OOT_GetItemDrawInfo"))return reinterpret_cast<void*>(NormalExporter);
    if(!strcmp(symbol,"OOT_GetFrozenItemAnimDrawInfo"))return reinterpret_cast<void*>(FrozenAnimExporter);
    if(!strcmp(symbol,"OOT_GetItemAnimDrawInfo"))return reinterpret_cast<void*>(NormalAnimExporter);
    if(!strcmp(symbol,"OOT_SetGiCosmeticFrame"))return nullptr;
    assert(false);return nullptr;
}
static const ComboForeignDrawInfoOOT* ComboResolveForeignDrawInfoOOT(RandoCheckId) { ++lookups;return nullptr; }
extern "C" void GetItem_Draw(PlayState*,s16) { ++sentinels; }
/* PRODUCTION */
int main() {
    PlayState play{};gPlayState=&play;EnItem00 item{};item.actor.id=ACTOR_EN_ITEM00;
    item.actor.home.rot.x=CustomItem::CALLED_ACTION|CustomItem::GIVE_ITEM_CUTSCENE;
    CwStartupReceipt receipt{};strcpy(receipt.itemName,"Exact awarded sword");
    receipt.getItemId=17;receipt.drawItemId=23;receipt.gi=31;receipt.field=37;
    for(int form=0;form<PLAYER_FORM_MAX;++form) {
        gSaveContext.save.playerForm=form;
        MM_DrawFrozenForeign(receipt,&item.actor);
        assert(name=="Exact awarded sword"&&shown==name&&lookups==0);
        assert(observedReceipt==&receipt&&standardCalls==0&&imports==0);
        assert(pickup==(form==PLAYER_FORM_GORON?2:1));
    }
    animated=true;MM_DrawFrozenForeign(receipt,&item.actor);assert(frozenAnimCalls==1&&normalAnimCalls==0);animated=false;
    const int before=fills;auto empty=receipt;empty.itemName[0]=0;MM_DrawFrozenForeign(empty,&item.actor);assert(sentinels==1&&fills==before);
    missing=true;MM_DrawFrozenForeign(receipt,&item.actor);assert(sentinels==2&&lookups==0);
    MM_DrawComboForeign(RC_CLOCK_TOWER_ROOF_SONG_OF_TIME);assert(lookups==1&&sentinels==3);
    puts("PASS actual MM named foreign recipe dispatch: exact frozen owner name, no check lookup, native per-form model fit, transient fallback and ordinary lookup");
}
