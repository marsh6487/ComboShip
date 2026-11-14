#include "global.h"
#include "combo/ComboExport.h"
#include "combo/rando/StartupCheckReceipt.h"
#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include <nlohmann/json.hpp>
#include "2s2h/Rando/Rando.h"
#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
SaveContext gSaveContext{};
PlayState* gPlayState;
GameState* gGameState;
GameInteractor* GameInteractor::Instance;
int gComboGameMode=COMBO_GAME_BOTH;
static int level,ack,draws,foreignDraws,killed,destroyed,boxes,toasts;
static bool available=true, blocked=false, vanilla=false, failAck=false;
static CwStartupReceipt pending{};
static std::string shown, drawnName;
static int drawnId;
int32_t CVarGetInteger(const char*, int32_t);
namespace Rando::StaticData {
std::map<RandoItemId,RandoStaticItem> Items;
/* NATIVE_POLICY */
}
int32_t CVarGetInteger(const char*, int32_t) { return level; }
static bool Combo_IsVanilla() { return vanilla; }
extern "C" u8 ItemInput_IsBlocked(Player*,PlayState*) { return blocked; }
void CustomMessage::StartTextbox(std::string text, Entry) { shown=text;++boxes; }
void CustomMessage::SetActiveCustomMessage(std::string text, Entry) { shown=text;++boxes; }
namespace Notification {
struct Options { std::string message,suffix; };
void Emit(Options o) { ++toasts;shown=o.message+" "+o.suffix; }
}
extern "C" void Actor_Kill(Actor*) { ++killed; }
void CustomItem00_Destroy(Actor*,PlayState*) { ++destroyed; }
extern "C" void Matrix_Scale(f32,f32,f32,MatrixMode) {}
void Rando::DrawResolvedItem(RandoItemId id,RandoCheckId,Actor*) { ++draws;drawnId=id; }
void Rando::DrawFrozenForeignItem(const CwStartupReceipt& receipt,Actor*) { ++foreignDraws;drawnName=receipt.itemName; }
static std::vector<std::unique_ptr<EnItem00>> spawned;
extern "C" Actor* Actor_Spawn(ActorContext*,PlayState*,s16 id,f32 x,f32 y,f32 z,s16 rx,s16 ry,s16 rz,s32 params) {
    auto item=std::make_unique<EnItem00>();auto* actor=&item->actor;
    actor->id=id;actor->world.pos={x,y,z};actor->home.rot={rx,ry,rz};actor->params=params;
    spawned.push_back(std::move(item));return actor;
}
static int32_t Host(int32_t action,int32_t game,int32_t slot,CwStartupReceipt* receipt) {
    assert(game==1);assert(slot==gSaveContext.fileNum);
    if(action==CW_STARTUP_PEEK) { if(!available)return 0;*receipt=pending;return 1; }
    assert(action==CW_STARTUP_ACKNOWLEDGE);
    assert(!shown.empty());if(receipt->generation!=pending.generation)return -1;if(failAck){available=false;return 0;}++ack;available=false;return 1;
}
/* PRODUCTION */
static void OtherReceipt(Actor*,PlayState*) {}
int main() {
    GameInteractor interactor;GameInteractor::Instance=&interactor;
    PlayState play{};Player player{};gPlayState=&play;gGameState=&play.state;
    play.state.running=1;play.actorCtx.actorLists[ACTORCAT_PLAYER].first=&player.actor;
    player.actor.bgCheckFlags=BGCHECKFLAG_GROUND;gSaveContext.fileNum=0;
    gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;
    sHost=Host;
    const RandoItemId ids[]={RI_RUPEE_RED,RI_HEART_PIECE,RI_BOW};
    const RandoItemType types[]={RITYPE_JUNK,RITYPE_HEALTH,RITYPE_MAJOR};
    for(int c=0;c<3;++c) Rando::StaticData::Items[ids[c]].randoItemType=types[c];
    for(int owner=0;owner<2;++owner)for(int category=0;category<3;++category)for(level=0;level<4;++level) {
        available=true;shown.clear();boxes=toasts=0;pending={};pending.version=1;pending.generation=42;
        pending.state=CW_STARTUP_READY;pending.itemGame=owner;pending.importance=category;
        pending.getItemId=ids[category];strcpy(pending.itemName,"Concrete Progressive Tier");strcpy(pending.displayName,"Actual frozen tier");
        Update();assert(sReceipt.actor);auto* actor=sReceipt.actor;
        bool full=(level==0 || level<category+1);
        assert(bool(actor->home.rot.x&CustomItem::GIVE_ITEM_CUTSCENE)==full);
        assert(bool(actor->home.rot.x&CustomItem::GIVE_OVERHEAD)!=full);
        const int before=ack;GiveReceipt(actor,&play);assert(ack==before+1);
        assert(shown.find("Actual frozen tier")!=shown.npos&&shown.find(CW_STARTUP_SOURCE)!=shown.npos);
        assert(full?(boxes==1&&toasts==0):(boxes==0&&toasts==1));
        assert(shown.find("Actual frozen tier")<shown.find(CW_STARTUP_SOURCE));
        DrawReceipt(actor,&play);
        if(owner==1)assert(drawnId==ids[category]);else assert(drawnName=="Concrete Progressive Tier");
        GiveReceipt(actor,&play);assert(ack==before+1);
        ReceiptDestroyed(actor,&play);assert(!sReceipt.actor);
        Update();assert(!sReceipt.actor); // acknowledged records cannot replay
    }
    available=true;pending.itemGame=1;pending.generation=56;level=0;Update();auto* old=sReceipt.actor;
    const int before=ack;++old->home.rot.z;GiveReceipt(old,&play);assert(ack==before);--old->home.rot.z;
    ++pending.generation;GiveReceipt(old,&play);assert(ack==before);Update();assert(sReceipt.actor!=old);
    old=sReceipt.actor;Reset();GiveReceipt(old,&play);assert(ack==before&&available);
    Update();assert(sReceipt.actor);old=sReceipt.actor;
    gSaveContext.fileNum=1;GiveReceipt(old,&play);assert(ack==before);Reset();gSaveContext.fileNum=0;
    interactor.currentEvent=GIEventGiveItem{true,13,OtherReceipt,OtherReceipt};Reset();
    assert(std::get<GIEventGiveItem>(interactor.currentEvent).giveItem==OtherReceipt);
    interactor.currentEvent=GIEventNone{};interactor.events.emplace_back(GIEventGiveItem{true,13,OtherReceipt,OtherReceipt});
    Update();assert(!sReceipt.actor&&interactor.events.size()==1);interactor.events.clear();
    pending.state=CW_STARTUP_WAIT_MM;Update();assert(!sReceipt.actor);pending.state=CW_STARTUP_READY;
    blocked=true;Update();assert(!sReceipt.actor);blocked=false;
    vanilla=true;Update();assert(!sReceipt.actor);vanilla=false;
    gComboGameMode=COMBO_GAME_MM;Update();assert(!sReceipt.actor);gComboGameMode=COMBO_GAME_BOTH;
    pending.displayName[0]=0;Update();assert(!sReceipt.actor);strcpy(pending.displayName,"Restored");
    Update();assert(sReceipt.actor);old=sReceipt.actor;
    ++play.sceneId;GiveReceipt(old,&play);assert(ack==before);Update();assert(sReceipt.actor!=old);
    Reset();assert(available);
    Update();old=sReceipt.actor;++gSaveContext.save.shipSaveInfo.rando.finalSeed;
    GiveReceipt(old,&play);assert(ack==before);Update();assert(sReceipt.actor!=old);Reset();
    available=false;Update();assert(!sReceipt.actor);available=true;
    failAck=true;Update();assert(sReceipt.actor);old=sReceipt.actor;GiveReceipt(old,&play);
    assert(ack==before&&!shown.empty());const int priorDraw=draws;DrawReceipt(old,&play);assert(draws==priorDraw+1);shown="Already displayed";GiveReceipt(old,&play);assert(shown=="Already displayed");
    const auto count=spawned.size();ReceiptDestroyed(old,&play);assert(!sReceipt.actor);
    Update();assert(!sReceipt.actor&&spawned.size()==count&&ack==before);
    failAck=false;Update();assert(ack==before+1);Update();assert(!sReceipt.actor&&spawned.size()==count);
    available=true;failAck=true;++pending.generation;Update();old=sReceipt.actor;assert(old);GiveReceipt(old,&play);
    ReceiptDestroyed(old,&play);assert(!sReceipt.actor);
    // Recreate the same slot and seed; only its durable receipt generation changes.
    ++pending.generation;available=true;failAck=false;const int beforeRecreate=ack;
    Update();Update();assert(sReceipt.actor&&sReceipt.value.generation==pending.generation);
    GiveReceipt(sReceipt.actor,&play);assert(ack==beforeRecreate+1);ReceiptDestroyed(sReceipt.actor,&play);
    available=true;
    memset(pending.itemName,'x',sizeof(pending.itemName));Update();assert(!sReceipt.actor);
    puts("PASS MM native/foreign 4 skip modes x 3 classes, frozen draw, text/source, no-grant callback, cancellation, slot/generation and scheduler ownership");
}
