#ifndef MM_TRADE_CONSUMPTION_FIXTURE_H
#define MM_TRADE_CONSUMPTION_FIXTURE_H

/* Source-function fixture, not the native engine ABI. Production enums, slot
 * tables, binding macros, trade state and semantic handoff bodies are extracted
 * unchanged. Graphics, animation, text advancement and item-offer scheduling
 * are controlled boundaries. */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef float f32;
#include "mm_constants.inc"

typedef struct NeiSaveData {
    uint32_t tradeAdultOwned;
    uint32_t tradeAdultRevision[23];
    uint8_t tradeAdultCursor, pendantOwned, pendantEffectOff;
    uint32_t extEquipOwnedBits;
    uint8_t extEquipBoots, extBootsLayoutVersion;
} NeiSaveData;
typedef struct ExtendedEquipmentState { u8 currentExtBoots; } ExtendedEquipmentState;
typedef struct SaveContext {
    struct {
        u8 playerForm;
        struct {
            struct { u8 items[48], dungeonItems[8], strayFairies[8]; s8 dungeonKeys[8]; u32 questItems; } inventory;
            struct { u8 buttonItems[4][4], cButtonSlots[4][4]; } equips;
            u8 weekEventReg[100];
            u32 skullTokenCount;
        } saveInfo;
        struct {
            NeiSaveData nei;
            struct { u8 dpadItems[4][4], dpadSlots[4][4]; } dpadEquips;
            struct { s8 foundDungeonKeys[8]; } rando;
        } shipSaveInfo;
    } save;
} SaveContext;
typedef struct Actor { s16 id, params; struct Actor* parent; } Actor;
typedef struct Player { PlayerItemAction exchangeItemAction; } Player;
typedef struct MessageContext { u8 msgMode, stateTimer, fixtureState; } MessageContext;
typedef struct PlayState { MessageContext msgCtx; Player player; s16 sceneId; } PlayState;
typedef struct AnimationCommon { s16 lastFrame; } AnimationCommon;
typedef struct AnimationHeader { AnimationCommon common; } AnimationHeader;
typedef struct AnimationInfo { AnimationHeader* animation; } AnimationInfo;
typedef struct SkelAnime { f32 curFrame; } SkelAnime;
typedef struct EnSellnuts {
    Actor actor; SkelAnime skelAnime;
    s16 animIndex, unk_368, unk_33A, unk_340;
    void (*actionFunc)(struct EnSellnuts*, PlayState*);
} EnSellnuts;
typedef struct EnAkindonuts {
    Actor actor; SkelAnime skelAnime;
    s16 animIndex, unk_356, unk_32C, unk_364;
    void (*actionFunc)(struct EnAkindonuts*, PlayState*);
    void (*unk_2DC)(struct EnAkindonuts*, PlayState*);
} EnAkindonuts;
typedef struct MsgScriptCmdDeleteItem { u8 cmd, itemH, itemL; } MsgScriptCmdDeleteItem;
typedef s32 (*MsgScriptCallback)(Actor*, PlayState*);
typedef int GIVanillaBehavior;

enum { VB_GIVE_ITEM_FROM_ROMANI = 1, VB_DOOR_HEALTH_CHECK_FAIL,
       VB_GIVE_PENDANT_OF_MEMORIES_FROM_KAFEI, VB_MSG_SCRIPT_DEL_ITEM,
       VB_GIVE_NEW_WAVE_BOSSA_NOVA, VB_TOILET_HAND_TAKE_ITEM };
enum { TEXT_STATE_EVENT = 1, TEXT_STATE_CHOICE, TEXT_STATE_PAUSE_MENU,
       TEXT_STATE_DONE, MSGMODE_TEXT_CLOSING = 9, PLAYER_CSACTION_19 = 19 };
enum { ENSELLNUTS_ANIM_5 = 5, ENSELLNUTS_ANIM_6 = 6,
       ENAKINDONUTS_ANIM_5 = 5, ENAKINDONUTS_ANIM_6 = 6 };
enum { ACTOR_EN_PM = 1, ACTOR_EN_AL, ACTOR_EN_SELLNUTS, ACTOR_EN_AKINDONUTS,
       SCENE_MILK_BAR = 10, SCENE_POSTHOUSE, SCENE_STOCK_POT_INN };
enum { RO_SHUFFLE_GOLD_SKULLTULAS, RO_SHUFFLE_FROGS };
typedef int RandoInf;
enum { RANDO_INF_MAX = 0, RANDO_INF_PURCHASED_BEANS_FROM_SOUTHERN_SWAMP_SCRUB,
       RANDO_INF_PURCHASED_BOMB_BAG_FROM_GORON_VILLAGE_SCRUB,
       RANDO_INF_PURCHASED_POTION_FROM_ZORA_HALL_SCRUB,
       RANDO_INF_PURCHASED_POTION_FROM_IKANA_CANYON_SCRUB };

#define ENAKINDONUTS_GET_3(actor) ((actor)->params & 3)
#define GET_PLAYER(play) (&(play)->player)
#define SCRIPT_PACK_16(hi, lo) (((hi) << 8) | (lo))
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define RANDO_SAVE_OPTIONS fixtureRandoOptions
#define CHECK_QUEST_ITEM(item) (gSaveContext.save.saveInfo.inventory.questItems & (1u << (item)))

#ifdef __cplusplus
extern "C" {
#endif
extern SaveContext gSaveContext, saveContextCopy;
extern ExtendedEquipmentState gExtEquipState;
extern PlayState* gPlayState;
extern u8 gItemSlots[];
extern bool fixtureRando, fixtureKeepExpress, fixtureVeto, fixtureAdvance, fixtureApproveGive;
extern bool fixtureRandoOptions[2];
extern int gFcCombo_SuppressRecord;
extern unsigned fixtureOffers, fixtureCsActions, fixtureCallbacks;
extern GetItemId fixtureLastOffer;
NeiSaveData* Nei_Save(void);
u8 TradeAdult_IsOwnedIndex(s32);
u8 TradeAdult_IsOwnedItem(u8);
s32 TradeAdult_IndexOfItem(u8);
void TradeAdult_GiveIndex(s32);
void TradeAdult_GiveItem(u8);
void TradeAdult_FoldCurrent(u8);
void TradeAdult_ConsumeItem(u8);
void TradeAdult_ConsumeIndex(s32);
void TradeAdult_ReconcileConsumed(void);
u8 TradeAdult_IsConsumedIndex(s32);
u8 TradeAdult_CellItem(void);
s32 TradeAdult_CursorIndex(void);
s32 TradeAdult_OwnedCount(void);
void ExtEquip_GivePendant(void);
void ExtEquip_RemoveItem(s16, u8);
void Inventory_DeleteItem(s16, s16);
s32 Inventory_ReplaceItem(PlayState*, u8, u8);
u8 Item_Give(PlayState*, u8);
int GameInteractor_Should(int, int, ...);
int GameInteractor_ShouldItemGive(u8);
void GameInteractor_ExecuteOnItemGive(u8);
void ItemGrantAudit_Begin(const char*, int, int, int);
void ItemGrantAudit_End(void);
void Interface_LoadItemIconImpl(PlayState*, u8);
void Interface_Dpad_LoadItemIconImpl(PlayState*, u8);
void Flags_SetWeekEventReg(u16);
bool Flags_GetRandoInf(RandoInf);
void Flags_SetRandoInf(RandoInf);
void Rupees_ChangeBy(s16);
u8 Message_GetState(MessageContext*);
int Message_ShouldAdvance(PlayState*);
s16 Animation_GetLastFrame(void*);
void SubS_ChangeAnimationByInfoS(SkelAnime*, AnimationInfo*, s16);
void Player_SetCsActionWithHaltedActors(PlayState*, Actor*, s32);
int Actor_HasParent(Actor*, PlayState*);
void Actor_OfferGetItem(Actor*, PlayState*, GetItemId, f32, f32);
void Message_StartTextbox(PlayState*, u16, Actor*);
void func_80ADBAB8(EnSellnuts*, PlayState*);
void func_80ADBBEC(EnSellnuts*, PlayState*);
void func_80ADB0D8(EnSellnuts*, PlayState*);
void func_80ADBCE4(EnSellnuts*, PlayState*);
void func_80BEF20C(EnAkindonuts*, PlayState*);
void func_80BEF360(EnAkindonuts*, PlayState*);
void func_80BEF450(EnAkindonuts*, PlayState*);
void func_80BEEFA8(EnAkindonuts*, PlayState*);
s32 func_80BED034(EnAkindonuts*);
s32 func_80BECFBC(EnAkindonuts*);
s32 MsgEvent_DeleteItem(Actor*, PlayState*, u8**, MsgScriptCallback, s32*);
s32 EnBjt_TakeItem(s32);
void Probe_NativeTradeCycleReset(PlayState*);
void Probe_RandoTradeCycleRestore(void);
void Probe_RandoActorUpdate(Actor*);
void Probe_OldBootsMigration(void);
#ifdef __cplusplus
}
#endif
#endif
