/* Engine-boundary fixture; no native ABI or configured-game claim. */
#include "soh/include/z64item.h"
#include "soh/soh/Enhancements/randomizer/randomizerEnums.h"
#include "soh/soh/Enhancements/game-interactor/vanilla-behavior/GIVanillaBehavior.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef int32_t s32;
typedef uint32_t u32;
typedef float f32;
typedef struct Actor { struct Actor* parent; int textId, params, flags; float xzDistToPlayer, yDistToPlayer; } Actor;
typedef struct Input { struct { u16 button; } cur, press; } Input;
typedef struct PlayState { struct { int state, choiceIndex, msgMode; } msgCtx; struct { Input input[1]; } state; } PlayState;
typedef struct Npc Npc;
typedef void (*NpcAction)(Npc*, PlayState*);
struct Npc { Actor actor; NpcAction actionFunc; struct { int talkState; } interactInfo;
             int getItemId, unk_262, unk_26E, unk_277, unk_27A, unk_2A8, brewTimer,
                 messageState, exchangeItemId, isTrading, skelAnime, flags, timer; };
typedef Npc EnNiwLady;
typedef Npc EnHs;
typedef Npc EnDs;
typedef Npc EnKo;
typedef Npc EnToryo;
typedef Npc EnKz;
typedef Npc EnMk;
typedef Npc EnGo2;
typedef Npc EnTa;
typedef Npc EnHeishi2;
typedef NpcAction EnDsActionFunc;
typedef struct Player { Actor actor; int exchangeItemId; } Player;
typedef struct ItemEquips { u8 buttonItems[8], cButtonSlots[7]; } ItemEquips;
static struct { struct { u8 items[24]; } inventory; ItemEquips equips; int bgsFlag, subTimerState, eventInf[2], beans; } gSaveContext;
static PlayState play, *gPlayState = &play;
static Player player;
static bool randomizer, earlyFrog, advance, talking, childSlotOccupied, shuffleAdultTrade;
static bool itemFlags[512], infFlags[512], eventFlags[512], randoFlags[RAND_INF_MAX];
static int consumed[256], granted[256], failures, offered, liveIcons, timerValue;
static int rewardFlagType = -1, rewardFlag = -1, rewardItem = ITEM_NONE;
static bool available[256];
#define EXPECT(condition) do { if (!(condition)) { fprintf(stderr, "line %d: %s\n", __LINE__, #condition); failures++; } } while (0)
#define IS_RANDO randomizer
static bool adult;
#define LINK_IS_ADULT adult
#define LINK_IS_CHILD (!adult)
static Player* Probe_Player(void) { return &player; }
#define GET_PLAYER(p) Probe_Player()
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define osSyncPrintf(...) ((void)0)
#define VT_FGCOL(x) ""
#define VT_RST ""
#define CVAR_ENHANCEMENT(x) x
#define CHECK_BTN_ANY(v,m) (((v) & (m)) != 0)
#define CUR_EQUIP_VALUE(x) (shield)
#define BEANS_BOUGHT gSaveContext.beans
#define INV_CONTENT(item) (gSaveContext.inventory.items[Probe_Slot(item)])
static int shield;
enum { TEXT_STATE_NONE, TEXT_STATE_DONE, TEXT_STATE_EVENT, TEXT_STATE_CHOICE,
       NPC_TALK_STATE_IDLE, NPC_TALK_STATE_TALKING, SUBTIMER_STATE_OFF,
       ACTOR_FLAG_TALK = 1, ACTOR_FLAG_TALK_OFFER_AUTO_ACCEPTED = 2,
       MSGMODE_PAUSED = 3, ANIMMODE_LOOP = 4, BTN_R = 0x10,
       GORON_DMT_BIGGORON = 1, FLAG_EVENT_CHECK_INF = 0, FLAG_ITEM_GET_INF = 1,
       FLAG_INF_TABLE = 2, FLAG_RANDOMIZER_INF = 3 };
enum { ITEMGETINF_0C = 0x0c, ITEMGETINF_2C = 0x2c, ITEMGETINF_2E = 0x2e,
       ITEMGETINF_30 = 0x30, ITEMGETINF_31 = 0x31,
       EVENTCHKINF_TALON_WOKEN_IN_CASTLE = 0x13, EVENTCHKINF_TALON_WOKEN_IN_KAKARIKO = 0x6a,
       INFTABLE_SHOWED_ZELDAS_LETTER_TO_GATE_GUARD = 0x76,
       INFTABLE_GORON_CITY_DOORS_UNLOCKED = 0x109,
       ITEMGETINF_OBTAINED_NUT_UPGRADE_FROM_STAGE = 0x19 };
typedef int GetItemEntry;
#define GET_ITEM_NONE 0
#define MOD_NONE 0
static int sTradeItemTextIds[9];
static int object_hs_Anim_0005C0;

static int Probe_Slot(int item) { return item >= ITEM_POCKET_EGG && item <= ITEM_CLAIM_CHECK ? SLOT_TRADE_ADULT : SLOT_TRADE_CHILD; }
static int TradeAdult_IndexOfItem(u8 item) {
    if (item >= ITEM_POCKET_EGG && item <= ITEM_CLAIM_CHECK) return item - ITEM_POCKET_EGG;
    if (item >= ITEM_WEIRD_EGG && item <= ITEM_LETTER_ZELDA) return item - ITEM_WEIRD_EGG + 20;
    return -1;
}
static void TradeAdult_ConsumeItem(u8 item) { consumed[item]++; available[item] = false; }
static void TradeAdult_GiveItem(u8 item) { granted[item]++; available[item] = true; }
static void Probe_RandoBehavior(int id, bool* should, va_list args);
void RandomizerOnFlagSetHandler(int16_t flagType, int16_t flag);
static void Probe_Flag(int type, int flag) {
    if (randomizer) RandomizerOnFlagSetHandler(type, flag);
    if (type == rewardFlagType && flag == rewardFlag) TradeAdult_GiveItem(rewardItem);
}
static bool Flags_GetItemGetInf(int flag) { return itemFlags[flag]; }
static void Flags_SetItemGetInf(int flag) { itemFlags[flag] = true; Probe_Flag(FLAG_ITEM_GET_INF, flag); }
static bool Flags_GetInfTable(int flag) { return infFlags[flag]; }
static void Flags_SetInfTable(int flag) { infFlags[flag] = true; Probe_Flag(FLAG_INF_TABLE, flag); }
static void Flags_SetEventChkInf(int flag) { eventFlags[flag] = true; Probe_Flag(FLAG_EVENT_CHECK_INF, flag); }
static void Flags_SetRandomizerInf(int flag) { randoFlags[flag] = true; Probe_Flag(FLAG_RANDOMIZER_INF, flag); }
static void Flags_UnsetRandomizerInf(int flag) { randoFlags[flag] = false; }
static void Flags_SetSwitch(PlayState* p, int flag) {}
static u8 Randomizer_GetNextAdultTradeItem(void) { return ITEM_CLAIM_CHECK; }
static int Inventory_ReplaceItem(PlayState* p, u16 old, u16 next) {
    for (int i=0; i<24; ++i) if (gSaveContext.inventory.items[i] == old) { gSaveContext.inventory.items[i] = next; return 1; }
    return 0;
}
static bool Probe_Shuffle(void) { return shuffleAdultTrade; }
static const struct { bool (*Get)(void); } probeOption = { Probe_Shuffle };
#define RAND_GET_OPTION(x) probeOption
static bool GameInteractor_Should(int id, bool result, ...) {
    if (id == VB_GIVE_EYEBALL_FROG_EARLY) return earlyFrog;
    if (id == VB_GIVE_ITEM_FROM_GORON && randomizer) return false;
    if (randomizer) { va_list args; va_start(args, result); Probe_RandoBehavior(id, &result, args); va_end(args); }
    return result;
}
static int CVarGetInteger(const char* name, int fallback) { return earlyFrog; }
static int Message_GetState(void* msg) { return play.msgCtx.state; }
static bool Message_ShouldAdvance(PlayState* p) { return advance; }
static void Message_CloseTextbox(PlayState* p) {}
static void Message_ContinueTextbox(PlayState* p, int text) {}
static bool Actor_HasParent(Actor* actor, PlayState* p) { return actor->parent != NULL; }
static void Actor_OfferGetItem(Actor* actor, PlayState* p, int gi, float x, float y) { offered = gi; }
static bool Actor_ProcessTalkRequest(Actor* actor, PlayState* p) { return talking; }
static int Actor_GetPlayerExchangeItemId(PlayState* p) { return player.exchangeItemId; }
static void Actor_GetScreenPos(PlayState* p, Actor* actor, s16* x, s16* y) { *x = *y = -1; }
static void Actor_OfferTalkExchangeEquiCylinder(Actor* actor, PlayState* p, float d, int exchange) {}
static void Player_SetCsActionWithHaltedActors(PlayState* p, void* actor, int action) {}
static void Interface_LoadItemIcon1(PlayState* p, int button) { liveIcons++; }
static void Interface_SetSubTimer(int seconds) { timerValue = seconds; }
static void Environment_ClearBgsDayCount(void) {}
static void SkelAnime_Update(int* anime) {}
static void Animation_Change(int* anime, int* anim, float speed, float start, float end, int mode, float morph) {}
static float Animation_GetLastFrame(int* anim) { return 1; }
static u8 Return_Item(u8 item, int mod, u8 result) { return result; }
static bool ChildTradeSlotOccupied(void) { return childSlotOccupied; }
static void func_80ABA778(Npc* n, PlayState* p) {}
static void func_80ABA244(Npc* n, PlayState* p) {}
static void func_80ABAC00(Npc* n, PlayState* p) {}
static void func_80ABAA9C(Npc* n, PlayState* p) {}
static void func_80A6E630(Npc* n, PlayState* p) {}
static void func_80A6E70C(Npc* n, PlayState* p) {}
static void EnDs_DisplayOddPotionText(Npc* n, PlayState* p) {}
static void EnDs_TalkAfterGiveOddPotion(Npc* n, PlayState* p) {}
static void EnDs_BrewOddPotion1(Npc* n, PlayState* p) {}
static void EnDs_Talk(Npc* n, PlayState* p) {}
static void func_80A99560(Npc* n, PlayState* p) {}
static int EnToryo_TalkRespond(Npc* n, PlayState* p) { return n->messageState; }
static int EnToryo_DoneTalking(Npc* n, PlayState* p) { return 0; }
static int EnToryo_ReactToExchangeItem(Npc* n, PlayState* p) { return 0; }
static int EnToryo_GetTextId(Npc* n, PlayState* p) { return 0; }
static void EnKz_StartTimer(Npc* n, PlayState* p) {}
static void func_80AACA40(Npc* n, PlayState* p) {}
static void EnMk_Wait(Npc* n, PlayState* p) {}
static void EnGo2_GetItemAnimation(Npc* n, PlayState* p) {}
static void EnGo2_RollingAnimation(Npc* n, PlayState* p) {}
static void EnGo2_GoronRollingBigContinueRolling(Npc* n, PlayState* p) {}
static void func_80A46B40(Npc* n, PlayState* p) {}
static void EnTa_IdleAwakeInCastle(Npc* n, PlayState* p) {}
static void EnTa_IdleAwakeInKakariko(Npc* n, PlayState* p) {}
static void EnTa_WakeUp(Npc* n, PlayState* p) {}
static void EnTa_SleepTalkInCastle(Npc* n, PlayState* p) {}
static void EnTa_SleepTalkInKakariko(Npc* n, PlayState* p) {}
static void EnTa_AnimRepeatCurrent(void) {}
static void EnTa_AnimSleeping(void) {}
static void func_80A53908(Npc* n, PlayState* p) {}
static void func_80A6E3A0(Npc* n, NpcAction action) { n->actionFunc = action; }
static void EnTa_SetupAction(Npc* n, NpcAction action, void (*anim)(void)) { n->actionFunc = action; }

/* PRODUCTION_RANDO */
/* PRODUCTION_NATIVE */

static void Reset(void) {
    memset(&play, 0, sizeof(play)); memset(&player, 0, sizeof(player)); memset(&gSaveContext, 0, sizeof(gSaveContext));
    memset(gSaveContext.inventory.items, ITEM_NONE, sizeof(gSaveContext.inventory.items));
    memset(consumed, 0, sizeof(consumed)); memset(granted, 0, sizeof(granted)); memset(available, 1, sizeof(available));
    memset(itemFlags, 0, sizeof(itemFlags)); memset(infFlags, 0, sizeof(infFlags));
    memset(eventFlags, 0, sizeof(eventFlags)); memset(randoFlags, 0, sizeof(randoFlags));
    randomizer = earlyFrog = advance = talking = childSlotOccupied = adult = false;
    shuffleAdultTrade = true;
    offered = liveIcons = timerValue = shield = 0;
    rewardFlagType = rewardFlag = -1; rewardItem = ITEM_NONE;
}
static int ConsumedCount(void) { int n = 0; for (int i=0; i<256; ++i) n += consumed[i]; return n; }
static void TestAdultNative(void) {
    struct { NpcAction action; int item; } cases[] = {
        {func_80A6E740, ITEM_COJIRO}, {EnDs_GiveOddPotion, ITEM_ODD_MUSHROOM},
        {func_80A99504, ITEM_ODD_POTION}, {EnToryo_HandleTalking, ITEM_SAW}, {func_80AACA94, ITEM_FROG}
    };
    for (unsigned i=0; i<ARRAY_COUNT(cases); ++i) {
        Reset(); Npc n = {0}; n.messageState = 4;
        cases[i].action(&n, &play); EXPECT(ConsumedCount() == 0); // GI offered, not accepted.
        n.actor.parent = &player.actor; cases[i].action(&n, &play);
        EXPECT(consumed[cases[i].item] == 1); EXPECT(ConsumedCount() == 1);
    }
    Reset(); adult = true; itemFlags[ITEMGETINF_2C] = true; Npc lady = {0};
    func_80ABAC84(&lady, &play); EXPECT(ConsumedCount() == 0);
    play.msgCtx.state = TEXT_STATE_DONE; advance = true; func_80ABAC84(&lady, &play);
    EXPECT(consumed[ITEM_POCKET_CUCCO] == 1 && itemFlags[ITEMGETINF_2E]);
    Reset(); adult = true; play.msgCtx.state = TEXT_STATE_DONE; advance = true;
    func_80ABAC84(&lady, &play); EXPECT(ConsumedCount() == 0 && itemFlags[ITEMGETINF_2C]);
}
static void TestRefusalsAndChild(void) {
    Npc n = {0};
    for (int rando=0; rando<2; ++rando) {
        Reset(); randomizer = rando; play.msgCtx.state = TEXT_STATE_CHOICE; play.msgCtx.choiceIndex = 1; advance = true;
        n.unk_262 = TEXT_STATE_CHOICE; func_80ABAB08(&n, &play); func_80A6E7BC(&n, &play); EnDs_OfferOddPotion(&n, &play);
        EXPECT(ConsumedCount() == 0);
    }
    Reset(); talking = true; player.exchangeItemId = EXCH_ITEM_NONE; EnTa_IdleAsleepInCastle(&n, &play);
    EXPECT(ConsumedCount() == 0 && !eventFlags[EVENTCHKINF_TALON_WOKEN_IN_CASTLE]);
    player.exchangeItemId = EXCH_ITEM_CHICKEN; EnTa_IdleAsleepInCastle(&n, &play);
    EXPECT(ConsumedCount() == 0); // Presentation starts waking; the completed awake transition owns cleanup.
    EnTa_SetupAwake(&n); EXPECT(consumed[ITEM_CHICKEN] == 1 && eventFlags[EVENTCHKINF_TALON_WOKEN_IN_CASTLE]);
    Reset(); adult = true; talking = true; player.exchangeItemId = EXCH_ITEM_POCKET_CUCCO;
    EnTa_IdleAsleepInKakariko(&n, &play); EnTa_SetupAwake(&n);
    EXPECT(ConsumedCount() == 0 && eventFlags[EVENTCHKINF_TALON_WOKEN_IN_KAKARIKO]);
    for (int rando=0; rando<2; ++rando) {
        Reset(); randomizer = rando; func_80A54038(&n, &play); EXPECT(ConsumedCount() == 0);
        play.msgCtx.state = TEXT_STATE_EVENT; func_80A54038(&n, &play); EXPECT(ConsumedCount() == 0);
        advance = true; func_80A54038(&n, &play);
        EXPECT(consumed[ITEM_LETTER_ZELDA] == 1 && infFlags[INFTABLE_SHOWED_ZELDAS_LETTER_TO_GATE_GUARD]);
    }
}
static void TestKingZoraAndBiggoron(void) {
    for (int early=0; early<2; ++early) for (int trade=0; trade<2; ++trade) {
        Reset(); earlyFrog = early; Npc n = {0}; n.isTrading = !early && trade;
        player.exchangeItemId = trade ? EXCH_ITEM_PRESCRIPTION : EXCH_ITEM_NONE;
        EnKz_SetupGetItem(&n, &play); EXPECT(ConsumedCount() == 0);
        EXPECT(offered == (trade ? GI_FROG : GI_TUNIC_ZORA));
        player.exchangeItemId = EXCH_ITEM_NONE; n.actor.parent = &player.actor; // GI handshake clears presentation.
        EnKz_SetupGetItem(&n, &play); EXPECT(consumed[ITEM_PRESCRIPTION] == trade);
    }
    const int gi[] = {GI_PRESCRIPTION, GI_CLAIM_CHECK, GI_SWORD_BGS, GI_TUNIC_GORON};
    const int old[] = {ITEM_SWORD_BROKEN, ITEM_EYEDROPS, ITEM_CLAIM_CHECK, ITEM_NONE};
    for (int i=0; i<4; ++i) {
        Reset(); Npc n = {0}; n.actor.params = GORON_DMT_BIGGORON; n.getItemId = gi[i];
        EnGo2_SetGetItem(&n, &play); EXPECT(ConsumedCount() == 0);
        play.msgCtx.state = TEXT_STATE_DONE; EnGo2_SetGetItem(&n, &play); EXPECT(ConsumedCount() == 0);
        advance = true; EnGo2_SetGetItem(&n, &play);
        EXPECT(ConsumedCount() == (i != 3)); if (i != 3) EXPECT(consumed[old[i]] == 1);
        if (i == 2) EXPECT(gSaveContext.bgsFlag);
    }
    Reset(); Npc other = {0}; other.getItemId = GI_SWORD_BGS; play.msgCtx.state = TEXT_STATE_DONE; advance = true;
    EnGo2_SetGetItem(&other, &play); EXPECT(ConsumedCount() == 0);
}
static void TestRandomizerTiming(void) {
    const int hooks[] = {VB_TRADE_POCKET_CUCCO, VB_TRADE_COJIRO, VB_TRADE_ODD_MUSHROOM,
                         VB_TRADE_ODD_POTION, VB_TRADE_SAW, VB_TRADE_FROG};
    const int items[] = {ITEM_POCKET_CUCCO, ITEM_COJIRO, ITEM_ODD_MUSHROOM, ITEM_ODD_POTION, ITEM_SAW, ITEM_FROG};
    const int flags[] = {ITEMGETINF_2E, -1, ITEMGETINF_30, ITEMGETINF_31, -1, -1};
    for (int i=0; i<6; ++i) {
        Reset(); randomizer = true; Npc n = {0}; gSaveContext.inventory.items[SLOT_TRADE_ADULT] = items[i];
        rewardFlagType = FLAG_ITEM_GET_INF; rewardFlag = flags[i]; rewardItem = items[i];
        EXPECT(!GameInteractor_Should(hooks[i], true, &n)); EXPECT(consumed[items[i]] == 1);
        if (flags[i] >= 0) EXPECT(available[items[i]] && granted[items[i]] == 1);
    }
    const int completion[] = {RAND_INF_ADULT_TRADES_DMT_TRADE_BROKEN_SWORD,
                              RAND_INF_ADULT_TRADES_DMT_TRADE_EYEDROPS,
                              RAND_INF_ADULT_TRADES_DMT_TRADE_CLAIM_CHECK};
    const int inputs[] = {ITEM_SWORD_BROKEN, ITEM_EYEDROPS, ITEM_CLAIM_CHECK};
    const int rewards[] = {GI_PRESCRIPTION, GI_CLAIM_CHECK, GI_SWORD_BGS};
    for (int shuffle=0; shuffle<2; ++shuffle) for (int i=0; i<3; ++i) {
        Reset(); randomizer = true; shuffleAdultTrade = shuffle;
        rewardFlagType = FLAG_RANDOMIZER_INF; rewardFlag = completion[i]; rewardItem = inputs[i];
        Flags_SetRandomizerInf(completion[i]); EXPECT(consumed[inputs[i]] == 1 && available[inputs[i]]);
        Npc n = {0}; n.actor.params = GORON_DMT_BIGGORON; n.getItemId = rewards[i];
        EnGo2_SetGetItem(&n, &play); EXPECT(consumed[inputs[i]] == 1 && available[inputs[i]]);
    }
    for (int early=0; early<2; ++early) for (int trade=0; trade<2; ++trade) {
        Reset(); randomizer = true; earlyFrog = early; Npc n = {0}; n.isTrading = trade;
        player.exchangeItemId = trade ? EXCH_ITEM_PRESCRIPTION : EXCH_ITEM_NONE;
        rewardFlagType = FLAG_RANDOMIZER_INF; rewardFlag = RAND_INF_ADULT_TRADES_ZD_TRADE_PRESCRIPTION; rewardItem = ITEM_PRESCRIPTION;
        EXPECT(!GameInteractor_Should(VB_ADULT_KING_ZORA_ITEM_GIVE, true, &n));
        EXPECT(consumed[ITEM_PRESCRIPTION] == trade); if (trade) EXPECT(available[ITEM_PRESCRIPTION]);
    }

    // Run native callers around the real rando callback. A native cleanup
    // after the queued same-item receipt would retire the replacement copy.
    Reset(); randomizer = true; Npc fado = {0};
    rewardFlagType = FLAG_ITEM_GET_INF; rewardFlag = ITEMGETINF_31; rewardItem = ITEM_ODD_POTION;
    func_80A99504(&fado, &play);
    EXPECT(consumed[ITEM_ODD_POTION] == 1 && granted[ITEM_ODD_POTION] == 1 && available[ITEM_ODD_POTION]);

    Reset(); randomizer = true; Npc carpenter = {0}; carpenter.messageState = 4;
    rewardFlagType = FLAG_RANDOMIZER_INF; rewardFlag = RAND_INF_ADULT_TRADES_GV_TRADE_SAW; rewardItem = ITEM_SAW;
    EnToryo_HandleTalking(&carpenter, &play);
    EXPECT(consumed[ITEM_SAW] == 1 && granted[ITEM_SAW] == 1 && available[ITEM_SAW]);

    Reset(); randomizer = true; Npc lab = {0};
    rewardFlagType = FLAG_RANDOMIZER_INF; rewardFlag = RAND_INF_ADULT_TRADES_LH_TRADE_FROG; rewardItem = ITEM_FROG;
    func_80AACA94(&lab, &play);
    EXPECT(consumed[ITEM_FROG] == 1 && granted[ITEM_FROG] == 1 && available[ITEM_FROG]);

    Reset(); randomizer = true; Npc king = {0}; king.isTrading = true;
    rewardFlagType = FLAG_RANDOMIZER_INF; rewardFlag = RAND_INF_ADULT_TRADES_ZD_TRADE_PRESCRIPTION; rewardItem = ITEM_PRESCRIPTION;
    EnKz_SetupGetItem(&king, &play);
    EXPECT(consumed[ITEM_PRESCRIPTION] == 1 && granted[ITEM_PRESCRIPTION] == 1 && available[ITEM_PRESCRIPTION]);

    Reset(); randomizer = true; Npc grog = {0}; play.msgCtx.state = TEXT_STATE_CHOICE; advance = true;
    rewardFlagType = FLAG_RANDOMIZER_INF; rewardFlag = RAND_INF_ADULT_TRADES_LW_TRADE_COJIRO; rewardItem = ITEM_COJIRO;
    func_80A6E7BC(&grog, &play); EXPECT(granted[ITEM_COJIRO] == 0 && !available[ITEM_COJIRO]);
    grog.actionFunc(&grog, &play);
    EXPECT(granted[ITEM_COJIRO] == 1 && available[ITEM_COJIRO]);
}
static void TestGrantAndDelete(void) {
    const int items[] = {ITEM_POCKET_EGG, ITEM_POCKET_CUCCO, ITEM_COJIRO, ITEM_ODD_MUSHROOM,
        ITEM_ODD_POTION, ITEM_SAW, ITEM_SWORD_BROKEN, ITEM_PRESCRIPTION, ITEM_FROG, ITEM_EYEDROPS,
        ITEM_CLAIM_CHECK, ITEM_WEIRD_EGG, ITEM_CHICKEN, ITEM_LETTER_ZELDA};
    for (unsigned i=0; i<ARRAY_COUNT(items); ++i) {
        Reset(); available[items[i]] = false; Probe_NativeGrant(NULL, items[i]);
        EXPECT(granted[items[i]] == 1 && available[items[i]] && ConsumedCount() == 0);
    }
    for (int i=0; i<2; ++i) {
        Reset(); childSlotOccupied = true; int item = i ? ITEM_LETTER_ZELDA : ITEM_WEIRD_EGG; available[item] = false;
        Probe_RandoGrant(i ? RG_ZELDAS_LETTER : RG_WEIRD_EGG); EXPECT(granted[item] == 1 && available[item]);
    }
    Reset(); gSaveContext.inventory.items[SLOT_TRADE_ADULT] = ITEM_CLAIM_CHECK;
    Inventory_DeleteItem(ITEM_COJIRO, SLOT_TRADE_ADULT);
    EXPECT(consumed[ITEM_COJIRO] == 1 && gSaveContext.inventory.items[SLOT_TRADE_ADULT] == ITEM_CLAIM_CHECK);
    Reset(); gSaveContext.inventory.items[SLOT_BEAN] = ITEM_BEAN; gSaveContext.beans = 5;
    Inventory_DeleteItem(ITEM_BEAN, SLOT_BEAN);
    EXPECT(ConsumedCount() == 0 && gSaveContext.beans == 0 && gSaveContext.inventory.items[SLOT_BEAN] == ITEM_NONE);
    Reset(); Inventory_DeleteItem(0x100 | ITEM_COJIRO, SLOT_TRADE_ADULT); EXPECT(ConsumedCount() == 0);
    Reset(); Probe_NativeGrant(NULL, ITEM_MASK_KEATON); EXPECT(granted[ITEM_MASK_KEATON] == 0 && ConsumedCount() == 0);
}
int main(void) {
    TestAdultNative(); TestRefusalsAndChild(); TestKingZoraAndBiggoron(); TestRandomizerTiming(); TestGrantAndDelete();
    if (failures) { fprintf(stderr, "FAIL: %d OoT trade hand-in/acquisition checks\n", failures); return 1; }
    puts("PASS: extracted OoT NPC hand-ins, refusals, native/rando timing, same-item rewards and acquisitions");
    return 0;
}
