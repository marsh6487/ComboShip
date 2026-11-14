#include "global.h"
#include "soh/OTRGlobals.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "overlays/misc/ovl_kaleido_scope/z_kaleido_scope.h"
#define SWORD_ELEMENT_NONE 0
#include "rando/StartupCheckReceipt.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
SaveContext gSaveContext;
u8 gItemAgeReqs[256];
u8 ExtInv_GetItemAgeReq(u16 item){(void)item;return 0;}
Vec3f gSfxDefaultPos;
f32 gSfxDefaultFreqAndVolScale;
s8 gSfxDefaultReverb;
static GetItemEntry entry;
static int grants,pending,started,ack,fanfare,equip,tutorial,state;
int32_t CVarGetInteger(const char* k,int32_t d){(void)k;(void)d;return 1;}
GetItemEntry ItemTable_Retrieve(s16 id){(void)id;return entry;}
u8 Item_Give(PlayState* p,u8 i){(void)p;(void)i;grants++;return 0;}
u16 Randomizer_Item_Give(PlayState* p,GetItemEntry i){(void)p;(void)i;grants++;return 0;}
void Player_SetPendingFlag(Player* p,PlayState* s){(void)p;(void)s;pending++;}
void Message_StartTextbox(PlayState* p,u16 id,Actor* a){(void)p;(void)id;(void)a;started++;}
u8 Message_GetState(MessageContext* m){(void)m;return state;}
u8 Message_ShouldAdvanceSilent(PlayState* p){(void)p;return 1;}
void Message_CloseTextbox(PlayState* p){(void)p;}
void Audio_PlayFanfare_Rando(GetItemEntry e){(void)e;fanfare++;}
void Audio_PlayFanfare(u16 id){(void)id;fanfare++;}
void Audio_PlaySfxGeneral(u16 a,Vec3f*b,u8 c,f32*d,f32*e,s8*f){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;fanfare++;}
void Inventory_ChangeEquipment(s16 a,u16 b){(void)a;(void)b;equip++;}
void func_808328EC(Player* p,u16 s){(void)p;(void)s;equip++;}
void Player_SetEquipmentData(PlayState* p,Player* s){(void)p;(void)s;equip++;}
bool GameInteractor_Should(GIVanillaBehavior f,uint32_t v,...){(void)f;return v;}
s32 Player_TryCsAction(PlayState* p,Actor* a,s32 s){(void)p;(void)a;(void)s;equip++;return 1;}
u8 SwordElement_MessageElementForPickup(u8 i){(void)i;tutorial++;return 0;}
void SwordElement_MessageAppend(PlayState* p,u8 e){(void)p;(void)e;tutorial++;}
bool ComboStartupOot_IsReceipt(const GetItemEntry* e){
#ifdef COMBO_BUILD
return (e->comboMarketReceipt & CW_STARTUP_RECEIPT_TOKEN_BIT)!=0;
#else
(void)e;return false;
#endif
}
void ComboStartupOot_TextStarted(PlayState* p,const GetItemEntry* e){(void)p;(void)e;assert(started);ack++;}
void Sfx_PlaySfxCentered(u16 id){(void)id;}
EnItem00* Item_DropCollectible(PlayState* p,Vec3f* v,s16 item){(void)p;(void)v;(void)item;grants++;return NULL;}
#include "oot_receive.inc"
int main(void){
 PlayState play={0};Player player={0};gSaveContext.ship.quest.id=QUEST_RANDOMIZER;
 const int ids[]={GI_SWORD_BGS,GI_GAUNTLETS_SILVER,RG_ICE_TRAP};
 for(int i=0;i<3;i++){
  memset(&player,0,sizeof(player));grants=pending=started=ack=fanfare=equip=tutorial=0;state=TEXT_STATE_NONE;
  entry=(GetItemEntry)GET_ITEM(ITEM_SWORD_BGS,OBJECT_GI_SWORD_1,0,0xF9,0x80,CHEST_ANIM_LONG,ITEM_CATEGORY_MAJOR,MOD_NONE,ids[i]);
  if(i==2){entry.modIndex=MOD_RANDOMIZER;entry.itemId=RG_ICE_TRAP;}
#ifdef COMBO_BUILD
  entry.comboMarketReceipt=CW_STARTUP_RECEIPT_TOKEN_BIT|3;
#endif
  player.getItemEntry=entry;player.getItemId=entry.getItemId;
  NativeReceive(&play,&player);
#ifdef COMBO_BUILD
  assert(grants==0 && pending==0 && ack==1 && tutorial==0);
  assert(!gSaveContext.bgsFlag && !gSaveContext.swordHealth);
#else
  assert(ack==0);
#endif
#ifdef COMBO_BUILD
  NativeFast(&play,&player,&entry);assert(grants==0);
#endif
  assert(started==1 && fanfare==1);
  NativeReceive(&play,&player);assert(started==1);
  state=TEXT_STATE_CLOSING;NativeReceive(&play,&player);
#ifdef COMBO_BUILD
  assert(equip==0 && !gSaveContext.ship.pendingIceTrapCount && play.transitionTrigger==0);
#endif
 }
 puts("PASS native player receipt: one text/fanfare, no duplicate grant/equip/trap/story effects; original guard preserved");
}
