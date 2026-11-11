#include "global.h"
#include "combo/ComboExport.h"
#include "combo/rando/StartupCheckReceipt.h"
#include <nlohmann/json.hpp>
#include "2s2h/Rando/Rando.h"
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <unordered_map>
int gComboGameMode=COMBO_GAME_BOTH;
SaveContext gSaveContext{};
static int grants,shares,saves,conversions;
static bool throwSave=false,throwGive=false,throwShare=false,vanilla=false;
static int flushed,receiveDepth;
static CwStartupReceipt* observed;
#define SPDLOG_INFO(...) ((void)0)
namespace ItemGrantAudit { struct Scope { explicit Scope(const char*) {} }; }
static RandoItemId startingTier=RI_BOW;
namespace Rando::StaticData {
std::map<RandoItemId,RandoStaticItem> Items;
const std::string& GetItemDisplayName(RandoItemId id) {
    static const std::string bow="Bow", red="Red Rupee", quiver="Large Quiver";
    return id==RI_BOW?bow:id==RI_RUPEE_RED?red:quiver;
}
}
namespace Rando {
bool gComboDormantGive=false;
void GiveItem(RandoItemId,RandoCheckId) { assert(gComboDormantGive);++grants;if(throwGive)throw std::runtime_error("give");startingTier=RI_QUIVER_50; }
RandoItemId ConvertItem(RandoItemId id,RandoCheckId) { ++conversions;return id==RI_BOW?startingTier:id; }
}
static bool Combo_IsVanilla() { return vanilla; }
static const std::unordered_map<std::string,RandoItemId>& Combo_MM_SpoilerNameToItemId() {
    static const std::unordered_map<std::string,RandoItemId> map={{"Bow",RI_BOW},{"Junk",RI_JUNK},
        {"Minor",RI_BOMBERS_NOTEBOOK},{"Health",RI_HEART_PIECE},{"Map",RI_WOODFALL_MAP},
        {"BossKey",RI_WOODFALL_BOSS_KEY},{"SmallKey",RI_WOODFALL_SMALL_KEY},
        {"Token",RI_GS_TOKEN_SWAMP},{"Fairy",RI_WOODFALL_STRAY_FAIRY},{"Trap",RI_TRAP}};return map;
}
struct FleetSharedReceiveGuard { FleetSharedReceiveGuard() { ++receiveDepth; } ~FleetSharedReceiveGuard() { --receiveDepth; } };
extern "C" s32 Inventory_HasEmptyBottle() { return true; }
static void Combo_MM_FlushAccumulators() { ++flushed; }
static void FleetShared_OnNativeObtained(int) { assert(observed&&observed->state==CW_STARTUP_READY);assert(receiveDepth==0);++shares;if(throwShare)throw std::runtime_error("share"); }
void SaveManager_SaveCurrentForCombo() { ++saves;if(throwSave)throw std::runtime_error("save"); }
/* OOT_POLICY */
/* PRODUCTION */
int main() {
    gSaveContext.fileNum=0;gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;
    for(auto id:{RI_BOW,RI_QUIVER_40,RI_RUPEE_RED}) {
        auto& item=Rando::StaticData::Items[id];item.itemId=ITEM_BOW;item.getItemId=GI_QUIVER_30;item.drawId=GID_BOW;
        item.randoItemType=id==RI_RUPEE_RED?RITYPE_JUNK:RITYPE_MAJOR;
    }
    CwStartupReceipt receipt{};receipt.version=1;receipt.generation=9;receipt.state=CW_STARTUP_WAIT_MM;receipt.itemGame=1;
    strcpy(receipt.itemName,"Bow");observed=&receipt;
    startingTier=RI_QUIVER_40; // matching new save starting inventory is already resident
    assert(MM_FinishStartupReceipt(&receipt)==1&&grants==1&&shares==1&&saves==0&&flushed==1&&!Rando::gComboDormantGive);
    assert(receipt.state==CW_STARTUP_READY&&receipt.getItemId==RI_QUIVER_40&&receipt.importance==CW_STARTUP_MAJOR&&receipt.category==6);
    assert(std::string(receipt.itemName)=="Large Quiver");
    assert(MM_FinishStartupReceipt(&receipt)==1&&grants==1&&shares==1&&saves==1&&conversions==1);
    receipt.state=CW_STARTUP_WAIT_MM;strcpy(receipt.itemName,"Junk");throwSave=true;
    assert(MM_FinishStartupReceipt(&receipt)==1&&saves==1);
    try { MM_FinishStartupReceipt(&receipt);assert(false); } catch(const std::runtime_error&) {}
    assert(receipt.state==CW_STARTUP_READY&&receipt.getItemId==RI_RUPEE_RED&&grants==2&&shares==2);
    throwSave=false;assert(MM_FinishStartupReceipt(&receipt)==1&&grants==2&&shares==2);
    auto invalid=receipt;invalid.state=CW_STARTUP_WAIT_MM;strcpy(invalid.itemName,"missing");
    assert(MM_FinishStartupReceipt(&invalid)==-1&&grants==2);invalid.version=99;
    assert(MM_FinishStartupReceipt(&invalid)==-1&&grants==2);
    invalid=receipt;invalid.itemGame=0;assert(MM_FinishStartupReceipt(&invalid)==-1);
    invalid=receipt;memset(invalid.itemName,'x',sizeof(invalid.itemName));assert(MM_FinishStartupReceipt(&invalid)==-1);
    gSaveContext.fileNum=0xFF;assert(MM_FinishStartupReceipt(&receipt)==-1&&grants==2);
    gSaveContext.fileNum=0;
    struct CategoryCase { const char* name;RandoItemId id;RandoItemType type;int category;int importance; };
    const CategoryCase categories[]={
        {"Junk",RI_RUPEE_RED,RITYPE_JUNK,0,CW_STARTUP_JUNK},
        {"Minor",RI_BOMBERS_NOTEBOOK,RITYPE_LESSER,1,CW_STARTUP_MINOR},
        {"Health",RI_HEART_PIECE,RITYPE_HEALTH,2,CW_STARTUP_MINOR},
        {"Map",RI_WOODFALL_MAP,RITYPE_LESSER,1,CW_STARTUP_MINOR},
        {"BossKey",RI_WOODFALL_BOSS_KEY,RITYPE_BOSS_KEY,3,CW_STARTUP_MAJOR},
        {"SmallKey",RI_WOODFALL_SMALL_KEY,RITYPE_SMALL_KEY,4,CW_STARTUP_MINOR},
        {"Token",RI_GS_TOKEN_SWAMP,RITYPE_SKULLTULA_TOKEN,5,CW_STARTUP_MINOR},
        {"Fairy",RI_WOODFALL_STRAY_FAIRY,RITYPE_STRAY_FAIRY,6,CW_STARTUP_MINOR},
        {"Trap",RI_TRAP,RITYPE_LESSER,6,CW_STARTUP_MINOR},
    };
    for(const auto& c:categories) {
        Rando::StaticData::Items[c.id].randoItemType=c.type;
        if(c.id==RI_TRAP) {
            Rando::StaticData::Items[c.id].itemId=ITEM_NONE;
            Rando::StaticData::Items[c.id].getItemId=GI_NONE;
            Rando::StaticData::Items[c.id].drawId=GID_NONE;
        }
        receipt.state=CW_STARTUP_WAIT_MM;strcpy(receipt.itemName,c.name);
        assert(MM_FinishStartupReceipt(&receipt)==1);
        assert(receipt.category==c.category&&receipt.importance==c.importance);
        if(c.id==RI_TRAP)assert(receipt.state==CW_STARTUP_READY&&receipt.drawItemId==GID_NONE&&receipt.itemId==ITEM_NONE);
        assert(receipt.trap==(c.id==RI_TRAP));assert(receipt.dungeonInfo==(c.id==RI_WOODFALL_MAP));
        const bool junkClass=c.category==0||c.category==1||c.category==2||c.category==5;
        assert(!SkipAnimation(receipt,SGIA_DISABLED));
        assert(SkipAnimation(receipt,SGIA_JUNK)==(junkClass&&!receipt.dungeonInfo));
        assert(SkipAnimation(receipt,SGIA_ALL));
    }
    const int sharedBefore=shares,grantedBefore=grants,savedBefore=saves;
    throwShare=true;receipt.state=CW_STARTUP_WAIT_MM;strcpy(receipt.itemName,"Junk");
    try { MM_FinishStartupReceipt(&receipt);assert(false); } catch(const std::runtime_error&) {}
    assert(receipt.state==CW_STARTUP_READY&&grants==grantedBefore+1&&shares==sharedBefore+1&&saves==savedBefore);
    throwShare=false;assert(MM_FinishStartupReceipt(&receipt)==1);
    assert(grants==grantedBefore+1&&shares==sharedBefore+1&&saves==savedBefore+1);
    const int priorSaves=saves;
    Combo_MM_GiveDormantResolved(RI_RUPEE_RED);assert(saves==priorSaves+1&&!Rando::gComboDormantGive);
    throwGive=true;
    try { Combo_MM_GiveDormantResolved(RI_RUPEE_RED);assert(false); } catch(const std::runtime_error&) {}
    assert(!Rando::gComboDormantGive&&saves==priorSaves+1);
    puts("PASS MM deferred native startup grant, initialized progressive tier, exactly once award/share, READY-before-save exception, save-only retry and rejection-before-mutation");
}
