#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <cstdio>
#include <stdexcept>

#include "functions.h"
#include "variables.h"
#include "macros.h"
#include "soh/cvar_prefixes.h"
#include "soh/OTRGlobals.h"
#include "rando/StartupCheckReceipt.h"
#include "soh/Enhancements/enhancementTypes.h"
#include "ComboGameMode.h"
SaveContext gSaveContext;
PlayState* gPlayState;
int gComboGameMode=COMBO_GAME_BOTH;
static PlayState play;
static Player player;
static int loaded=1,policy=1,offers=0,toasts=0,ackCalls=0,captures=0,ackResult=1;
static CwStartupReceipt durable{};
static int saveCalls=0,waitCalls=0,failSave=0,prepared=0,prepareResult=1,contextResult=1;
struct SaveManager {static SaveManager* Instance;void ThreadPoolWait(){waitCalls++;}};
static SaveManager manager;SaveManager* SaveManager::Instance=&manager;
void Save_SaveFile(){assert(prepared);saveCalls++;if(failSave)throw std::runtime_error("save capture failed before enqueue");}
struct GameInteractor{static bool IsSaveLoaded(){return loaded;}};
namespace Notification {struct Options {std::string message,suffix;};void Emit(Options o){assert(o.message=="You received");assert(o.suffix.find("Concrete Bow")<o.suffix.find(CW_STARTUP_SOURCE));toasts++;}}
namespace ComboRando {enum {GAME_MM=1};struct ForeignItem {int itemGame=1;std::string itemName="Progressive Bow",displayName="Bow (MM)",category="major";bool advancement=true,trap=false;};}
static ComboRando::ForeignItem foreign;
static bool foreignPresent=false;
const ComboRando::ForeignItem* OOT_LookupForeignByCheck(RandomizerCheck){return foreignPresent?&foreign:nullptr;}
GetItemCategory OOT_GetForeignCategory(RandomizerCheck){return ITEM_CATEGORY_MAJOR;}
namespace ComboDungeonPickup {bool IsMapOrCompass(const std::string& s){return s.find("Map")!=std::string::npos || s.find("Compass")!=std::string::npos;}}
namespace Rando::StaticData {
struct Name{std::string english="Concrete Bow";};
struct Item{std::shared_ptr<GetItemEntry> entry;Name name;std::shared_ptr<GetItemEntry> GetGIEntryUnresolved()const{return entry;}const Name& GetName()const{return name;}};
static Item item,disguise;
const Item& RetrieveItem(RandomizerGet id){static Item empty;return id==RG_MASTER_SWORD?disguise:id==RG_FAIRY_BOW||id==RG_COMBO_FOREIGN?item:empty;}
}
int32_t CVarGetInteger(const char*,int32_t){return policy;}
s32 Player_InCsMode(PlayState*){return 0;}
u8 Message_GetState(MessageContext*){return TEXT_STATE_NONE;}
s32 GiveItemEntryWithoutActor(PlayState*,GetItemEntry e){offers++;player.getItemEntry=e;player.getItemId=e.getItemId;return 1;}
static int32_t Host(int32_t action,int32_t game,int32_t slot,CwStartupReceipt* r){assert(game==0 && slot>=0 && slot<=2);if(slot!=0)return 0;switch(action){case CW_STARTUP_PREPARE_CONTEXT_CHANGE:return contextResult;case CW_STARTUP_PREPARE_PEER_SNAPSHOT:assert(waitCalls);prepared++;return prepareResult;case CW_STARTUP_BEGIN_FILE:durable={};return 1;case CW_STARTUP_CAPTURE:captures++;durable=*r;durable.generation=42;return 1;case CW_STARTUP_PEEK:if(!durable.version)return 0;*r=durable;return 1;case CW_STARTUP_ACKNOWLEDGE:ackCalls++;assert(r->generation==durable.generation);if(ackResult==1)durable={};return ackResult;}return -1;}
#include "oot_core.inc"
static void DisguiseDraw(PlayState*,GetItemEntry*){}
int main(){
 gPlayState=&play;play.actorCtx.actorLists[ACTORCAT_PLAYER].head=&player.actor;gSaveContext.fileNum=0;gSaveContext.ship.quest.id=QUEST_RANDOMIZER;
 GetItemEntry entry=GET_ITEM(ITEM_BOW,OBJECT_GI_BOW,0,0x70,0x80,CHEST_ANIM_LONG,ITEM_CATEGORY_MAJOR,MOD_NONE,GI_BOW);
 Rando::StaticData::item.entry=std::make_shared<GetItemEntry>(entry);
 sHost=nullptr;assert(PrepareContextChange());sHost=Host;contextResult=0;assert(!PrepareContextChange());contextResult=1;assert(PrepareContextChange());sHost=nullptr;assert(CaptureSave(0)==0);sHost=Host;assert(CaptureSave(1)==0);assert(saveCalls==0);assert(CaptureSave(0)==1);assert(saveCalls==1&&waitCalls==2);failSave=1;assert(CaptureSave(0)==0);assert(saveCalls==2);failSave=0;prepareResult=0;assert(CaptureSave(0)==0);assert(saveCalls==2);prepareResult=1;
 sHost=nullptr;assert(!Capture(&entry,RC_SONG_FROM_IMPA));sHost=Host;
 CwStartupReceipt malformed{};GetItemEntry rejected{};assert(!BuildEntry(malformed,rejected));
 assert(!Capture(&entry,RC_LINKS_POCKET) && captures==0);
 gComboGameMode=COMBO_GAME_OOT;BeginFile();assert(!Capture(&entry,RC_SONG_FROM_IMPA) && captures==0);gComboGameMode=COMBO_GAME_BOTH;
 BeginFile();assert(!Capture(&entry,RC_SONG_FROM_IMPA) && captures==1);assert(!strcmp(durable.itemName,"Concrete Bow") && durable.getItemId==GI_BOW);
 foreignPresent=true;entry.getItemId=RG_COMBO_FOREIGN;entry.modIndex=MOD_RANDOMIZER;assert(Capture(&entry,RC_SONG_FROM_IMPA));assert(durable.state==CW_STARTUP_WAIT_MM && !strcmp(durable.itemName,"Progressive Bow"));
 foreignPresent=false;
 for(int cat=0;cat<=6;cat++)for(int setting=0;setting<=2;setting++){
  CwStartupReceipt r{};r.category=cat;
  bool junk=cat==0||cat==1||cat==2||cat==5;
  assert(SkipAnimation(r,setting)==(setting==SGIA_ALL||(setting==SGIA_JUNK&&junk)));
  r.dungeonInfo=1;assert(SkipAnimation(r,SGIA_JUNK)==false);r.trap=1;assert(!SkipAnimation(r,SGIA_ALL));r.itemGame=1;assert(SkipAnimation(r,SGIA_ALL));
 }
 BeginFile();entry=*Rando::StaticData::item.entry;Capture(&entry,RC_SONG_FROM_IMPA);policy=SGIA_ALL;Frame();Frame();assert(toasts==1 && ackCalls==1 && offers==0);
 BeginFile();Capture(&entry,RC_SONG_FROM_IMPA);policy=SGIA_DISABLED;ackResult=0;Frame();assert(offers==1 && ackCalls==1);Cancel();assert(durable.version);Frame();assert(offers==2);player.stateFlags1|=PLAYER_STATE1_GETTING_ITEM;TextStarted(&play,&player.getItemEntry);TextStarted(&play,&player.getItemEntry);assert(toasts==2 && ackCalls==2);Frame();assert(offers==2 && toasts==2 && ackCalls==3);ackResult=1;Frame();assert(!durable.version && offers==2 && toasts==2);
 Cancel();player={};gSaveContext.fileNum=0;BeginFile();Capture(&entry,RC_SONG_FROM_IMPA);policy=SGIA_DISABLED;Frame();assert(sOffered);gSaveContext.fileNum=1;player.getItemId=GI_BOW;Frame();assert(sSlot==-1 && durable.version);gSaveContext.fileNum=0;player={};
 CwStartupReceipt disguised=durable;disguised.drawModIndex=MOD_RANDOMIZER;disguised.drawItemId=RG_MASTER_SWORD;disguised.gid=0;GetItemEntry disguise=entry;disguise.itemId=ITEM_SWORD_MASTER;disguise.getItemId=GI_SWORD_KOKIRI;disguise.drawFunc=DisguiseDraw;Rando::StaticData::disguise.entry=std::make_shared<GetItemEntry>(disguise);assert(BuildEntry(disguised,rejected));assert(rejected.drawFunc==DisguiseDraw&&rejected.gid==0);
 CwStartupReceipt foreignReady=durable;foreignReady.itemGame=1;strcpy(foreignReady.itemName,"Large Quiver");strcpy(foreignReady.displayName,"Large Quiver (MM)");assert(BuildEntry(foreignReady,rejected));assert(rejected.textId==kStartupText && rejected.comboForeignCheck==0);foreignReady.state=CW_STARTUP_WAIT_MM;assert(!BuildEntry(foreignReady,rejected));
 puts("PASS actual startup adapter: source eligibility, frozen pregrant identity, WAIT_MM, GI policy matrix, cancellation, repeated callbacks and failed ACK retry");
}
