#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <cstdio>
#include "soh/Enhancements/randomizer/randomizerTypes.h"
#include "soh/Enhancements/item-tables/ItemTableTypes.h"
#include "rando/StartupCheckReceipt.h"
#include "ComboItemDrawABI.h"

static GetItemEntry seen;
static RandomizerGet seenRg;
static int resolutions=0;
static bool advanced=false;
namespace Rando::StaticData {
struct Name{std::string english;};
struct Item{GetItemEntry raw{},next{};Name name;std::shared_ptr<GetItemEntry> GetGIEntry(RandomizerGet*)const{resolutions++;return std::make_shared<GetItemEntry>(advanced?next:raw);}std::shared_ptr<GetItemEntry> GetGIEntryUnresolved()const{return std::make_shared<GetItemEntry>(raw);}const Name& GetName()const{return name;}};
std::unordered_map<RandomizerGet,Item> items;
std::unordered_map<std::string,RandomizerGet> itemNameToEnum;
const Item& RetrieveItem(RandomizerGet id){return items.at(id);}
}
struct OTRGlobals{static OTRGlobals* Instance;void* gRandomizer=(void*)1;void* gRandoContext=(void*)1;};
static OTRGlobals globals;OTRGlobals* OTRGlobals::Instance=&globals;
namespace ComboSoulMiniOOT {bool UsesDetailed(int32_t){return false;}}
static bool OOT_BossSoulUsesSkeleton(RandomizerGet rg){return rg==RG_GOHMA_SOUL;}
static int OOT_DrawDependency(RandomizerGet,const CwItemDrawInfo&){return 1;}
struct AuditScope{template<class...A>AuditScope(A...){}};
namespace ItemGrantAudit{using Scope=AuditScope;}
static int animCalls=0;
int32_t OOT_GetItemAnimDrawInfo(const char* name,CwItemAnimDrawInfo*){assert(!strcmp(name,"Gohma Soul"));animCalls++;return 1;}
#include "oot_frozen_draw.inc"
static void Draw(PlayState*,GetItemEntry*){}
int main(){
 const RandomizerGet ids[]={RG_CANE_OF_SOMARIA,RG_PROGRESSIVE_ROCS,RG_STONE_OF_AGONY};
 const char* names[]={"Cane of Somaria","Progressive Roc","Stone of Agony"};
 for(int i=0;i<3;i++){
  auto& item=Rando::StaticData::items[ids[i]];item.name.english=names[i];Rando::StaticData::itemNameToEnum[names[i]]=ids[i];
  item.raw.itemId=100+i;item.raw.getItemId=200+i;item.raw.tableId=TABLE_RANDOMIZER;item.raw.modIndex=MOD_RANDOMIZER;item.raw.drawItemId=ids[i];item.raw.gid=300+i;item.raw.drawFunc=Draw;item.next=item.raw;item.next.gid=400+i;
  CwStartupReceipt r{};r.version=1;r.state=CW_STARTUP_READY;r.itemGame=0;r.modIndex=item.raw.modIndex;r.tableId=item.raw.tableId;r.getItemId=item.raw.getItemId;r.itemId=item.raw.itemId;r.gid=item.raw.gid;r.gi=301+i;r.drawItemId=ids[i];r.drawModIndex=MOD_RANDOMIZER;strcpy(r.itemName,names[i]);
  advanced=true;resolutions=0;CwItemDrawInfo out{};assert(OOT_GetFrozenItemDrawInfo(&r,&out)==1);assert(resolutions==0&&seen.gid==300+i&&seen.gi==301+i&&seen.drawFunc==Draw&&seenRg==ids[i]);
  OOT_FillItemDrawInfo(ids[i],&out);assert(resolutions==1&&seen.gid==400+i);
  r.itemId++;assert(OOT_GetFrozenItemDrawInfo(&r,&out)==0);
 }
 auto& soul=Rando::StaticData::items[RG_GOHMA_SOUL];soul.name.english="Gohma Soul";CwStartupReceipt trap{};trap.version=1;trap.state=CW_STARTUP_READY;trap.itemGame=0;trap.drawModIndex=MOD_RANDOMIZER;trap.drawItemId=RG_GOHMA_SOUL;strcpy(trap.itemName,"Ice Trap");Rando::StaticData::itemNameToEnum["Ice Trap"]=RG_ICE_TRAP;auto& trapRow=Rando::StaticData::items[RG_ICE_TRAP];trapRow.raw.itemId=RG_ICE_TRAP;trapRow.raw.getItemId=RG_ICE_TRAP;trapRow.raw.tableId=TABLE_RANDOMIZER;trapRow.raw.modIndex=MOD_RANDOMIZER;trap.itemId=RG_ICE_TRAP;trap.getItemId=RG_ICE_TRAP;trap.tableId=TABLE_RANDOMIZER;trap.modIndex=MOD_RANDOMIZER;CwItemDrawInfo staticSoul{};assert(OOT_GetFrozenItemDrawInfo(&trap,&staticSoul)==0);CwItemAnimDrawInfo anim{};resolutions=0;assert(OOT_GetFrozenItemAnimDrawInfo(&trap,&anim)==1);assert(animCalls==1&&resolutions==0);trap.drawModIndex=MOD_NONE;assert(OOT_GetFrozenItemAnimDrawInfo(&trap,&anim)==0);
 puts("PASS production frozen OoT selector: first Cane/Progressive Roc/Stone preserve captured GI/model/callback after inventory advances; animated trap identity and ordinary lookup preserved");
}
