#include <nlohmann/json_fwd.hpp>
#include "global.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/Rando/MiscBehavior/MiscBehavior.h"
#include "2s2h/Rando/ItemReceiptText.h"
#include "mods/items/logic/adult_link_render.h"
#include "sword_element_message.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

SaveContext gSaveContext{};
PlayState* gPlayState;
GameInteractor* GameInteractor::Instance;
static PlayState play;
static Player player;
static int skipLevel=0;
static int frame, offers, grantCount, grantFrame, receiptMessages, firstVisible=-1;
static std::vector<std::unique_ptr<EnItem00>> actors;
static std::vector<RandoCheckId> granted;
static constexpr auto firstCheck=RC_STARTING_ITEM_DEKU_MASK;
static constexpr auto secondCheck=RC_STARTING_ITEM_SONG_OF_HEALING;
void CustomItem00_Init(Actor*,PlayState*);
void CustomItem00_Update(Actor*,PlayState*);

// Notification is an external UI boundary; native queue/events/save/actor types
// remain the real production types. This minimal Options mirrors used fields.
namespace Notification {
struct Options {const char* itemIcon=nullptr; std::string message,suffix;};
void Emit(Options) {}
}
extern "C" {
u16 gEquipNegMasks[]={0xFFF0,0xFF0F,0xF0FF,0x0FFF};
u8 gEquipShifts[]={0,4,8,12};
void Flags_SetWeekEventReg(s32 flag) {gSaveContext.save.saveInfo.weekEventReg[flag >> 8] |= flag & 0xFF;}
int32_t CVarGetInteger(const char* name,int32_t fallback) {return std::string(name)=="gEnhancements.Cutscenes.SkipGetItemCutscenes" ? skipLevel : fallback;}
void Actor_SetScale(Actor* a,f32 value) {a->scale={value,value,value};}
void Actor_Kill(Actor* a) {a->update=nullptr;}
void Math_Vec3f_Copy(Vec3f* to,Vec3f* from) {*to=*from;}
f32 Math_SinS(s16 x) {return std::sin(x*3.141592653589793/32768);}
f32 Math_CosS(s16 x) {return std::cos(x*3.141592653589793/32768);}
f32 Rand_ZeroOne() {return 0.5f;}
void Audio_PlaySfx(u16) {}
void Actor_ProcessInitChain(Actor*,InitChainEntry*) {}
s32 Collider_InitAndSetCylinder(PlayState*,ColliderCylinder*,Actor*,ColliderCylinderInit*) {return 1;}
void Collider_UpdateCylinder(Actor*,ColliderCylinder*) {}
s32 CollisionCheck_SetAC(PlayState*,CollisionCheckContext*,Collider*) {return 1;}
void Actor_MoveWithGravity(Actor*) {}
void Actor_UpdateBgCheckInfo(PlayState*,Actor*,f32,f32,f32,u32) {}
s32 Actor_HasParent(Actor* a,PlayState*) {return a->parent!=nullptr;}
s32 Actor_OfferGetItem(Actor*,PlayState*,GetItemId,f32,f32) {++offers;return 1;}
PlayerExplosive Player_GetExplosiveHeld(Player*) {return PLAYER_EXPLOSIVE_NONE;}
void Matrix_Scale(f32,f32,f32,MatrixMode) {}
Actor* Actor_Spawn(ActorContext*,PlayState* p,s16 id,f32 x,f32 y,f32 z,s16 rx,s16 ry,s16 rz,s32 params) {
    auto item=std::make_unique<EnItem00>(); auto* actor=&item->actor;
    actor->id=id; actor->world.pos={x,y,z}; actor->home.rot={rx,ry,rz}; actor->params=params;
    actor->update=CustomItem00_Update;
    CustomItem00_Init(actor,p);
    actors.push_back(std::move(item));
    return actor;
}
s32 AdultLink_UsesAdultItemPresentation(const Player*) {return 0;}
f32 AdultLink_GetItemPresentationHeight() {return 68;}
f32 AdultLink_GetItemPresentationYOffset() {return 0;}
}
namespace CustomMessage {
void SetActiveCustomMessage(std::string,Entry) {++receiptMessages;}
void StartTextbox(std::string,Entry) {++receiptMessages;}
std::string RemoveColorCodes(const std::string& text) {return text;}
}
namespace Rando {
RandoItemId ConvertItem(RandoItemId id,RandoCheckId) {return id;}
RandoItemId CurrentJunkItem(RandoCheckId) {return RI_RUPEE_GREEN;}
void DrawResolvedItem(RandoItemId,RandoCheckId,Actor*) {}
bool ApplyItemReceiptText(RandoItemId,CustomMessage::Entry&) {return false;}
void AppendReceiptSource(CustomMessage::Entry&,const std::string&) {}
void GiveItem(RandoItemId,RandoCheckId check) {++grantCount;grantFrame=frame;granted.push_back(check);}
namespace StaticData {
std::map<RandoCheckId,RandoStaticCheck> Checks;
std::map<RandoItemId,RandoStaticItem> Items;
u8 GetIconForZMessage(RandoItemId) {return 0;}
const char* GetIconTexturePath(RandoItemId) {return "fixture";}
#ifdef COMBO_BUILD
std::string GetItemName(RandoItemId,bool,RandoCheckId,bool) {return "fixture item";}
#else
std::string GetItemName(RandoItemId,bool,RandoCheckId) {return "fixture item";}
#endif
}
}
std::string GetTrapMessage() {return "fixture trap";}

#ifdef COMBO_BUILD
extern "C" {
int gComboGameMode=COMBO_GAME_BOTH;
int gMMComboGoalRequired=0;
int (*gMMComboOtherTriforceCount)(void)=nullptr;
bool MagicMirror_CommitStumpRewardToken() {return true;}
void MMAnchor_BroadcastCrossItem(int,const char*,const char*) {abort();}
}
void SaveManager_SaveCurrentForCombo() {abort();}
namespace Rando {
void LatchComboForeign(RandoCheckId) {abort();}
const char* ComboForeignLatchedName(RandoCheckId) {return nullptr;}
uint8_t ComboForeignMessageIcon(RandoCheckId) {return 0;}
bool ApplyForeignItemReceiptText(const char*,CustomMessage::Entry&,RandoCheckId) {return false;}
namespace StaticData {
const std::string& GetCheckDisplayName(RandoCheckId) {static std::string name="fixture check";return name;}
}
namespace MiscBehavior {
void OfferTrapItem() {abort();}
const ComboRando::ForeignItem* MM_LookupForeign(RandoCheckId) {return nullptr;}
void SendForeignCheck(RandoCheckId) {abort();}
bool ShouldShowForeignCutscene(RandoCheckId) {return true;}
void BroadcastCheckObtainedIfFirst(RandoCheckId,RandoItemId,bool) {}
}
}
#endif

// Arrival pose/movement setup is an engine boundary here. The real start-mode
// helper still chooses counter and PLAYER_STATE1_20000000 readiness state.
static void func_80835324(PlayState*,Player*,f32,s16) {}
static void Player_Anim_PlayLoopSlowMorph(PlayState*,Player*,const void*) {}
extern "C" void func_80123140(PlayState*,Player*) {}
static void Player_Action_55(Player*,PlayState*) {}
static s32 Player_SetAction(PlayState*,Player*,PlayerActionFunc,s32) {return 1;}
extern "C" s32 WaterBox_GetSurface1(PlayState*,CollisionContext*,f32,f32,f32*,WaterBox**) {return false;}
#include "objects/gameplay_keep/gameplay_keep.h"
#include "production.inc"

static void require(bool condition,const char* label) {
    if(!condition) {std::fprintf(stderr,"FAIL frame%d: %s\n",frame,label); abort();}
}
static void reset() {
    play={};player={};gSaveContext={};actors.clear();granted.clear();
    frame=offers=grantCount=receiptMessages=0;grantFrame=firstVisible=-1;skipLevel=0;
    gPlayState=&play;play.actorCtx.actorLists[ACTORCAT_PLAYER].first=&player.actor;
    gSaveContext.save.playerForm=PLAYER_FORM_HUMAN;
    Rando::MiscBehavior::CheckQueueReset();
    Rando::StaticData::Checks={{firstCheck,{}},{secondCheck,{}}};
    Rando::StaticData::Items[RI_RUPEE_GREEN].randoItemType=RITYPE_JUNK;
    RANDO_SAVE_CHECKS[firstCheck].randoItemId=RI_RUPEE_GREEN;
    RANDO_SAVE_CHECKS[secondCheck].randoItemId=RI_RUPEE_GREEN;
    RANDO_SAVE_CHECKS[firstCheck].eligible=true;
}
static void queueAndProcess() {Rando::MiscBehavior::CheckQueue();ProcessEvents(&player.actor);}
static void tickItem(EnItem00* item) {
    ++frame; item->actor.xzDistToPlayer=0;item->actor.playerHeightRel=0;
    if(item->actor.update) item->actor.update(&item->actor,&play);
    if(firstVisible<0 && item->actor.scale.x>0) firstVisible=frame;
}
int main() {
    GameInteractor interactor;GameInteractor::Instance=&interactor;
    reset();
    Rando::PrepareNewRandomizedSave(); NativeStartingChecksEligible();
    require(gSaveContext.save.entrance==ENTRANCE(SOUTH_CLOCK_TOWN,0) &&
            gSaveContext.save.playerForm==PLAYER_FORM_HUMAN && gSaveContext.save.cutsceneIndex==0 &&
            gSaveContext.save.saveInfo.playerData.threeDayResetCount==1 &&
            CHECK_WEEKEVENTREG(WEEKEVENTREG_59_04) && CHECK_WEEKEVENTREG(WEEKEVENTREG_31_04),
            "actual randomizer initialization skips first-cycle arrival dialogue and starts human in South Clock Town");
    require(RANDO_SAVE_CHECKS[firstCheck].eligible && RANDO_SAVE_CHECKS[secondCheck].eligible,
            "actual producer makes both starting checks immediately eligible");
    RANDO_SAVE_CHECKS[firstCheck].randoItemId=RI_RUPEE_GREEN;
    RANDO_SAVE_CHECKS[secondCheck].randoItemId=RI_RUPEE_GREEN;
    PlayerAgeProperties properties{};properties.unk_24=10;player.ageProperties=&properties;
    Player_StartMode_D(&play,&player);
    require(player.av2.actionVar2==-20 && (player.stateFlags1 & PLAYER_STATE1_20000000),
            "real stationary arrival mode sets negative20 counter and blocking cutscene flag");
    play.transitionTrigger=TRANS_TRIGGER_END;play.transitionMode=TRANS_MODE_SETUP;
    queueAndProcess();
    require(actors.empty() && interactor.events.size()==1 && queuedCheck==firstCheck,
            "authentic incoming-transition/arrival state blocks already-eligible starting check");
    play.transitionTrigger=TRANS_TRIGGER_OFF;play.transitionMode=TRANS_MODE_OFF;queueAndProcess();
    require(actors.empty(),"arrival player flag independently blocks after fade ends");
    player.stateFlags1 &= ~PLAYER_STATE1_20000000;queueAndProcess();
    require(actors.size()==1,"starting-check event spawns on first call after arrival and transition clear");
    std::puts("PASS: authentic starting checks/initialization; native start-D counter-20 and cutscene bit; arrival gates independently block then release immediately");

    reset();play.msgCtx.msgMode=MSGMODE_TEXT_DISPLAYING;
    for(frame=1;frame<=9;++frame) queueAndProcess();
    require(actors.empty() && interactor.events.size()==1 && grantCount==0,"message blocks processing, not eligibility/one-event enqueue");
    play.msgCtx.msgMode=MSGMODE_NONE;player.csAction=PLAYER_CSACTION_1;
    for(;frame<=14;++frame) queueAndProcess();
    require(actors.empty() && interactor.events.size()==1,"blocking native cutscene retains queued event");
    player.csAction=PLAYER_CSACTION_NONE;play.transitionMode=TRANS_MODE_SETUP;
    queueAndProcess();require(actors.empty(),"native transition retains event");
    play.transitionMode=TRANS_MODE_OFF;queueAndProcess();
    require(actors.size()==1 && interactor.events.empty() && grantCount==0,"first unblocked processing spawns immediately");
    auto* item=actors.back().get();int parentStart=frame;
    for(int i=0;i<7;++i) tickItem(item);
    require(offers==7 && item->unk152==-1 && grantCount==0 && firstVisible==-1,"no parent: no countdown advance or visible/granted item");
    item->actor.parent=&player.actor;player.stateFlags1|=PLAYER_STATE1_400;
    tickItem(item);require(item->unk152==0 && grantCount==0,"first parent update initializes counter zero");
    int zeroFrame=frame;
    for(int i=0;i<19;++i) tickItem(item);
    require(item->unk152==19 && grantCount==0 && firstVisible==-1,"counter19 remains hidden and ungranted");
    tickItem(item);
    require(item->unk152==20 && grantCount==1 && firstVisible==frame && grantFrame==frame,"counter20 grants exactly once and shows item together");
    require(RANDO_SAVE_CHECKS[firstCheck].obtained && !RANDO_SAVE_CHECKS[firstCheck].eligible,"actual queue callback consumes eligible check");
    require(frame-zeroFrame==20,"post-zero counter delay is twenty item updates");
    for(int i=0;i<5;++i) tickItem(item);
    require(grantCount==1,"continued receipt animation cannot repeat grant");
    std::printf("TIMING: blocked message9 + cutscene5; first eligible event spawned immediately on unblock; parent wait7 offer updates; parent counter0 at%d; grant+visible at%d (+20); before-parent span%d\n",zeroFrame,grantFrame,zeroFrame-parentStart);

    reset();RANDO_SAVE_CHECKS[secondCheck].eligible=true;queueAndProcess();
    item=actors.back().get();
    for(int i=0;i<6;++i) {queueAndProcess();tickItem(item);}
    require(actors.size()==1 && interactor.events.empty() && queuedCheck==firstCheck,"second eligible check waits behind first ungranted check");
    item->actor.parent=&player.actor;player.stateFlags1|=PLAYER_STATE1_400;
    for(int i=0;i<21;++i) tickItem(item);
    require(grantCount==1 && granted[0]==firstCheck,"first grant consumes first check only");
    play.msgCtx.msgMode=MSGMODE_TEXT_DISPLAYING;queueAndProcess();
    require(interactor.events.size()==1 && actors.size()==1,"second check enqueues but active receipt message blocks spawn");
    play.msgCtx.msgMode=MSGMODE_NONE;queueAndProcess();
    require(actors.size()==2,"second check spawns when receipt/message gate clears");
    auto* second=actors.back().get();second->actor.parent=&player.actor;
    for(int i=0;i<21;++i) tickItem(second);
    require(grantCount==2 && granted[1]==secondCheck,"two serialized checks grant once each in order");
    queueAndProcess();require(actors.size()==2 && interactor.events.empty(),"obtained checks do not requeue");
    std::puts("PASS: two checks serialized; first check/receipt gate delays second; exactly one grant per check");

    reset();queueAndProcess();item=actors.back().get();
    item->actor.destroy(&item->actor,&play);
    require(interactor.events.size()==1 && std::holds_alternative<GIEventNone>(interactor.currentEvent),"ungranted destroyed actor requeues its event");
    queueAndProcess();require(actors.size()==2,"requeued event spawns replacement");
    second=actors.back().get();second->actor.parent=&player.actor;player.stateFlags1|=PLAYER_STATE1_400;
    for(int i=0;i<21;++i) tickItem(second);
    second->actor.destroy(&second->actor,&play);queueAndProcess();
    require(grantCount==1 && interactor.events.empty() && actors.size()==2,"granted destroyed actor cannot requeue duplicate");
    std::puts("PASS: interrupted pre-grant actor requeues; granted actor destruction does not duplicate");

    reset();skipLevel=1;queueAndProcess();item=actors.back().get();tickItem(item);
    require(grantCount==1 && firstVisible==1 && item->unk152==14,"overhead path grants+shows on first in-range update, then14 counter remains");
    std::puts("PASS: overhead control has no cutscene20 hold; production pacing unchanged");
}
