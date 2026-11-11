// Production adapter + native player/message functions. The runner extracts
// them unchanged; this fixture controls only external services/frame inputs.
#include "global.h"
#include "message_data_static.h"
#include "message_data_fmt.h"
#include "combo/vanilla/ComboVanillaRewards.h"
#include "combo/vanilla/ComboVanillaMode.h"
#include "combo/menu/ItemGrantAuditBridge.h"
#include "combo/audio/DinsFireSfx.h"
#include "mods/nei_save.h"
#include "mods/extended_inventory.h"
#include "mods/extended_equipment.h"
#include "mods/items/logic/weapon_upgrades.h"
#include "mods/items/logic/twilight_upgrade.h"
#include "mods/transformation_masks/custom_forms.h"
extern "C" {
#include "soh/Enhancements/randomizer/RocsFeatherCycle.h"
}
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/Gameplay/ComboStartupReceipt.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Enhancements/randomizer/item.h"
#include "soh/Enhancements/custom-message/CustomMessageTypes.h"
#include "sword_element_message.h"
#include "soh/util.h"
#include "soh/z_message_OTR.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <string_view>

using namespace ComboVanilla;
using Rando::Item;
// The production table storage/accessor and Item implementation below are real;
// this declaration excludes unrelated randomizer UI/location dependencies.
namespace Rando {
class StaticData {
    static std::array<Item, RG_MAX> itemTable;
  public:
    static void InitItemTable();
    static Item& RetrieveItem(RandomizerGet);
};
}
extern "C" {
void Message_OpenText(PlayState*, u16);
void Message_Decode(PlayState*);
void Player_StartTalking(PlayState*, Actor*);
void Player_Action_WaitForPutAway(Player*, PlayState*);
void Player_SetupTalk(PlayState*, Player*);
void Player_Action_Talk(Player*, PlayState*);
void Player_Action_8084E6D4(Player*, PlayState*);
void Player_Action_Idle(Player*, PlayState*);
s32 Player_SetupAction(PlayState*, Player*, PlayerActionFunc, s32);
s32 Player_ActionHandler_Talk(Player*, PlayState*);
s32 Player_ActionHandler_2(Player*, PlayState*);
void Fixture_BindInput(PlayState*);
u8 Return_Item_Entry(GetItemEntry, u8);
u16 Randomizer_Item_Give(PlayState*, GetItemEntry);
u8 Cane_GiveSkill(u8);
SaveContext gSaveContext{};
int gComboGameMode = COMBO_GAME_BOTH, gComboVanillaMode = 1;
u32 gBitFlags[32]{};
u8 gEquipShifts[4] = {0, 4, 8, 12};
u8 gUpgradeShifts[8] = {0, 3, 6, 9, 12, 14, 17, 20};
u32 gUpgradeMasks[8] = {7, 56, 448, 3584, 12288, 114688, 917504, 7340032};
u8 gItemSlots[56]{};
u8 gItemAgeReqs[256]{};
static GameInfo registers{};
GameInfo* gGameInfo = &registers;
static PlayState play;
static Player player;
PlayState* gPlayState = &play;
}
static GraphicsContext graphics;
static Gfx commands[512];
static char textbox[4096];
static NeiSaveData gNeiSave;
static unsigned upperBodyDelay;
static unsigned begins, commits, aborts;
static unsigned receives;
static bool rejectBegin, rejectCommit;
static Receipt transaction;
static nlohmann::json document, disk;
static std::array<unsigned char, sizeof(gSaveContext.inventory)> inventoryBefore;
static std::array<unsigned char, sizeof(gSaveContext.eventChkInf)> eventsBefore;
static std::array<unsigned char, sizeof(gSaveContext.itemGetInf)> itemFlagsBefore;
static std::array<unsigned char, sizeof(gSaveContext.infTable)> infoBefore;
static const char* caseName = "setup";

static void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL [" << caseName << "]: " << message << '\n';
        std::exit(1);
    }
}

/* NATIVE_SAVE */
/* NATIVE_COPY */
/* NATIVE_STARTUP_CLASSIFIER */

/* NATIVE_REGISTRY */
/* NATIVE_ITEM_TABLE */

extern "C" {
void* gItemIcons[ITEM_ROCS_FEATHER + 1]{};
static const char pocket[] = "Pocket Egg tutorial.\x13\x2D\x02";
static const char vanilla[] = "Unrelated native text.\x02";
static MessageTableEntry nativeTable[] = {
    {1, 2, pocket, sizeof(pocket) - 1},
    {0x1234, 2, vanilla, sizeof(vanilla) - 1},
    {0xFFFF, 0, nullptr, 0},
};
MessageTableEntry* sNesMessageEntryTablePtr = nativeTable;
MessageTableEntry* sGerMessageEntryTablePtr = nativeTable;
MessageTableEntry* sFraMessageEntryTablePtr = nativeTable;
MessageTableEntry* sJpnMessageEntryTablePtr = nullptr;
MessageTableEntry* sStaffMessageEntryTablePtr = nativeTable;
MessageTableEntry* OTRMessage_Find(MessageTableEntry*, u16) { return nullptr; }
int32_t CVarGetInteger(const char*, int32_t fallback) { return fallback; }
bool GameInteractor_Should(GIVanillaBehavior, uint32_t result, ...) { return result; }
void Interface_ChangeHudVisibilityMode(u16) {}
void Interface_SetDoAction(PlayState*, u16) {}
s32 Flags_GetEventChkInf(s32) { return 0; }
int Flags_GetItemGetInf(int flag) {
    return (gSaveContext.itemGetInf[flag >> 4] >> (flag & 15)) & 1;
}
size_t GetEquipNowMessage(char*, char*, size_t) { std::abort(); }
void* ExtInv_GetItemIcon(uint16_t) { return (void*)"icon"; }
u8 Picto_IsOwned() { return 0; }
u8 PowerKeg_IsOwned() { return 0; }
u8 RitoMask_IsOwned() { return gNeiSave.ritoMaskFlags & RITO_FLAG_MASK_OWNED; }
void ItemGrantAudit_Begin(const char*, int, int, int) {}
void ItemGrantAudit_End() {}
s32 Player_InCsMode(PlayState*) {
    return player.stateFlags1 & (PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_DEAD);
}
s32 Player_GetExplosiveHeld(Player*) { return -1; }
void Actor_Kill(Actor* actor) { actor->update = nullptr; actor->draw = nullptr; }
void AudioOcarina_SetInstrument(u8) {}
void func_8008EC70(Player*) {}
void func_80832340(PlayState*, Player*) {}
void func_808326F0(Player*) {}
void func_80834644(PlayState*, Player*) {}
void func_80832224(Player*) {}
s32 func_808332B8(Player*) { return 0; }
void Player_FinishAnimMovement(Player*) {}
void Player_Action_8084E3C4(Player*, PlayState*) {}
void Player_Action_808507F4(Player*, PlayState*) {}
s32 Player_PutAwayHeldItem(PlayState*, Player*) { return upperBodyDelay != 0; }
s32 Player_UpdateUpperBody(Player*, PlayState*) {
    if (upperBodyDelay) { --upperBodyDelay; return 1; }
    return 0;
}
s32 LinkAnimation_Update(PlayState*, SkelAnime*) { return 1; }
void Player_AnimPlayOnceAdjusted(PlayState*, Player*, LinkAnimationHeader*) {}
void Player_AnimPlayLoop(PlayState*, Player*, LinkAnimationHeader*) {}
LinkAnimationHeader* Player_GetIdleAnim(Player*) { return nullptr; }
void Player_StartAnimMovement(PlayState*, Player*, s32) {}
void Player_AnimChangeLoopSlowMorph(PlayState*, Player*, LinkAnimationHeader*) {}
void Player_SetTurnAroundCamera(PlayState*, s16) {}
s32 Player_CheckHostileLockOn(Player*) { return 0; }
s32 Player_SetCsActionWithHaltedActors(PlayState*, Actor*, u8) { return 0; }
void Player_Action_Idle(Player*, PlayState*) {}
void Player_Action_8084CC98(Player*, PlayState*) { std::abort(); }
void Player_Action_8084D610(Player*, PlayState*) { std::abort(); }
void Player_AnimPlayLoopAdjusted(PlayState*, Player*, LinkAnimationHeader*) {}
void Player_AnimChangeOnceMorph(PlayState*, Player*, LinkAnimationHeader*) {}
void Player_AnimPlayOnce(PlayState*, Player*, LinkAnimationHeader*) {}
void Player_DetachHeldActor(PlayState*, Player* who) { Check(!who->heldActor, "unexpected held actor"); }
void func_8083AE40(Player*, s16 object) { Check(object == OBJECT_GI_GLASSES, "Rod GI object was replaced"); }
void func_8083A0F4(PlayState*, Player*) { std::abort(); }
void Player_InitItemAction(PlayState*, Player*, s8) { std::abort(); }
void Player_UseItem(PlayState*, Player*, s32) { std::abort(); }
void func_8083E4C4(PlayState*, Player*, GetItemEntry*) { Check(false, "native fast-grant path was taken"); }
void ComboStartupOot_TextStarted(PlayState*, const GetItemEntry*) {
    Check(false, "fixed Vanilla reward entered startup-only acknowledgement");
}
void DinsFireSfx_CancelCast(uint32_t) { Check(false, "Rod dialogue unexpectedly interrupted a Din cast"); }
void Magic_Reset(PlayState*) { Check(false, "Rod dialogue unexpectedly reset spell magic"); }
void func_808328EC(Player*, u16) { std::abort(); }
s32 func_8084B4D4(PlayState*, Player*) { return 0; }
s32 func_8084B3CC(PlayState*, Player*) { return 0; }
s32 Player_StartCsAction(PlayState*, Player*) { return 0; }
void func_8083A360(PlayState*, Player*) { std::abort(); }
void func_80838F18(PlayState*, Player*) { std::abort(); }
s16 func_8083DB98(Player*, s32) { std::abort(); }
void Player_ApplyYawFromAnim(Player*) {}
void func_80837C0C(PlayState*, Player*, s32, f32, f32, s16, s32) { std::abort(); }
void func_808332F4(Player*, PlayState*) {}
s32 TitleCard_Clear(PlayState*, TitleCardContext*) { return 1; }
GetItemEntry ItemTable_Retrieve(s16) { Check(false, "fixed GI identity fell back to vanilla item table"); return {}; }
u8 Item_Give(PlayState*, u8) { Check(false, "unexpected native Item_Give / trade grant"); return ITEM_NONE; }
u8 Item_CheckObtainability(u8) { Check(false, "fixed GI was treated as a vanilla inventory item"); return ITEM_NONE; }
s32 Player_HoldsHookshot(Player*) { return 0; }
s32 Player_GetStrength() { return 0; }
s32 Player_ActionToModelGroup(Player*, s32) { std::abort(); }
void Player_SetEquipmentData(PlayState*, Player*) { std::abort(); }
void Inventory_ChangeEquipment(s16, u16) { std::abort(); }
void Audio_PlayFanfare(u16) {}
void Audio_PlayFanfare_Rando(GetItemEntry) { Check(false, "Vanilla flow became randomized"); }
u8 SwordElement_MessageElementForPickup(u8) { Check(false, "unexpected vanilla sword grant"); return 0; }
static Camera camera;
Camera* Play_GetCamera(PlayState*, s16) { return &camera; }
s32 Camera_RequestSetting(Camera*, s16) { return 0; }
s16 Camera_GetCamDirYaw(Camera*) { return 0; }
s16 func_8005B1A4(Camera*) { return 0; }
f32 Math_SinS(s16) { std::abort(); }
f32 Math_CosS(s16) { std::abort(); }
s32 Math_ScaledStepToS(s16*, s16, s16) { return 0; }
s32 LinkAnimation_OnFrame(SkelAnime*, f32) { return 0; }
Actor* Actor_Spawn(ActorContext*, PlayState*, s16, f32, f32, f32, s16, s16, s16, s16) { std::abort(); }
void Flags_SetSwitch(PlayState*, s32) { Check(false, "unexpected native pending switch flag"); }
void Flags_SetTreasure(PlayState*, s32) { Check(false, "unexpected native pending treasure flag"); }
void Flags_SetClear(PlayState*, s32) { Check(false, "unexpected native pending clear flag"); }
void Flags_SetCollectible(PlayState*, s32) { Check(false, "unexpected native pending collectible flag"); }
void Flags_SetEventChkInf(s32) { Check(false, "unexpected native event/trade flag"); }
void Flags_SetItemGetInf(s32) { Check(false, "unexpected native item/trade flag"); }
void Flags_SetInfTable(s32) { Check(false, "unexpected native info/trade flag"); }
void Flags_SetEventInf(s32) { Check(false, "unexpected native event info flag"); }
void Flags_SetRandomizerInf(RandomizerInf) { Check(false, "unexpected randomizer flag"); }

// Other native grants are outside the Rod regression. The established exact
// payload suite separately exercises every manifest grant/ownership branch.
u8 Cane_GiveSkill(u8 skill) { gNeiSave.caneSkills |= 1u << skill; return 1; }
void Wand_GrantMode(u8 mode) { gNeiSave.wandRodsOwned |= 1u << mode; }
void Slate_GrantRune(u8 rune) { gNeiSave.slateRunesOwned |= 1u << rune; }
void Seasons_GrantSeason(u8 season) { gNeiSave.seasonsOwned |= 1u << season; }
u8 Seasons_RandoMode() { return NEI_SEASONS_INDIVIDUAL; }
u8 ExtEquip_HasItem(s16 type, u8 index) { return (gNeiSave.extEquipOwnedBits >> (16 + type * 3 + index - 1)) & 1; }
void ExtEquip_GiveItem(s16 type, u8 index) { gNeiSave.extEquipOwnedBits |= 1u << (16 + type * 3 + index - 1); }
void TwilightUpgrade_SetClawshot(u8 value) { gNeiSave.twilightUpgrade |= value; }
void WeaponUpgrade_SetHammerAxe(u8 value) { gNeiSave.weaponUpgrades |= value; }
void WeaponUpgrade_SetTrueMaster(u8 value) { gNeiSave.weaponUpgrades |= value << 3; }
void VanillaRocsFeather_Grant() { gNeiSave.separateRocsFlags |= 1; }
u8 ExtInv_GetItemSlot(u16) { return 24; }
u8 ExtInv_GetItemAgeReq(u16) { Check(false, "unexpected vanilla equip check"); return AGE_REQ_NONE; }
s32 Player_TryCsAction(PlayState*, Actor*, s32) { Check(false, "unexpected native reward cutscene"); return 0; }
u8 TravelTools_IsOwned(u16) { return gNeiSave.timeGateOwned; }
void TravelTools_Grant(u16) { gNeiSave.timeGateOwned = 1; }
u8 GraceHourglass_IsOwned(u16) { return gNeiSave.hyliasGraceOwned; }
void GraceHourglass_Grant(u16) { gNeiSave.hyliasGraceOwned = 1; }
void RitoMask_Grant() { gNeiSave.ritoMaskFlags |= RITO_FLAG_MASK_OWNED; }
/* NATIVE_ROD */
}

/* ADAPTER_HELPERS */

static nlohmann::json Snapshot() {
    return {{"fileType", 0}, {"version", 1},
            {"sections", {{"base", {{"data", {{"rupees", gSaveContext.rupees}}}}}}},
            {"rodOwned", gNeiSave.seasonsRodOwned},
            {"rodSlot", Nei_GetOwnedItem(SLOT_ROD_OF_SEASONS)}};
}
class SaveManager {
  public:
    static SaveManager* Instance;
    void ThreadPoolWait() {}
    nlohmann::json SaveToJsonObject() { return Snapshot(); }
};
static SaveManager manager;
SaveManager* SaveManager::Instance = &manager;
static int HostStorage(int action, int game, int slot, const char* id, const char* json) {
    assert(game == 0 && slot == 0);
    const auto* reward = FindReward(id);
    assert(reward);
    auto writer = [action](const nlohmann::json& next) {
        if ((static_cast<RewardAction>(action) == RewardAction::Begin && rejectBegin) ||
            (static_cast<RewardAction>(action) == RewardAction::Commit && rejectCommit))
            return false;
        disk = next;
        return true;
    };
    switch (static_cast<RewardAction>(action)) {
        case RewardAction::Query: return transaction.Active() ? 2 : RewardStatus(document, *reward);
        case RewardAction::Begin:
            ++begins;
            return transaction.Begin(document, *reward, false, writer);
        case RewardAction::BeginOwned: return transaction.Begin(document, *reward, true, writer);
        case RewardAction::Commit:
            ++commits;
            assert(json && transaction.Stage(game, nlohmann::json::parse(json)));
            return transaction.Commit(document, writer);
        case RewardAction::Abort: ++aborts; transaction.Abort(); return 1;
        case RewardAction::PrepareCapture: transaction.PrepareCapture(); return 1;
    }
    return -1;
}

/* ADAPTER_STATE */
#define SPDLOG_ERROR(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
/* ADAPTER_FLOW */

extern "C" bool ComboVanillaOot_TryReceiptGrant(PlayState* state, GetItemEntry entry) {
    return FixedReceiptGrant(state, entry);
}
/* NATIVE_RANDOMIZER_DISPATCH */

extern "C" void GameInteractor_ExecuteOnItemReceiveHooks(GetItemEntry entry) {
    Check(entry.comboMarketReceipt & kReceiptBit, "receive hook lost fixed receipt identity");
    if (sReceiptReward && entry.comboMarketReceipt == sReceiptTicket)
        Check(sDelivered, "receive hook ran before fixed grant/rollback");
    ++receives;
}

extern "C" void GameInteractor_ExecuteOnOpenText(uint16_t* text, bool* fromTable) {
    RewardText(text, fromTable);
}

static void RememberTrade() {
    std::memcpy(inventoryBefore.data(), &gSaveContext.inventory, inventoryBefore.size());
    std::memcpy(eventsBefore.data(), &gSaveContext.eventChkInf, eventsBefore.size());
    std::memcpy(itemFlagsBefore.data(), &gSaveContext.itemGetInf, itemFlagsBefore.size());
    std::memcpy(infoBefore.data(), &gSaveContext.infTable, infoBefore.size());
}
static void CheckTrade() {
    Check(std::memcmp(inventoryBefore.data(), &gSaveContext.inventory, inventoryBefore.size()) == 0,
          "native inventory including both trade slots changed");
    Check(std::memcmp(eventsBefore.data(), &gSaveContext.eventChkInf, eventsBefore.size()) == 0,
          "native event/trade flags changed");
    Check(std::memcmp(itemFlagsBefore.data(), &gSaveContext.itemGetInf, itemFlagsBefore.size()) == 0,
          "native item-get/trade flags changed");
    Check(std::memcmp(infoBefore.data(), &gSaveContext.infTable, infoBefore.size()) == 0,
          "native info/trade flags changed");
}
static void Reset() {
    CancelReceipt(true);
    transaction = {};
    play = {};
    player = {};
    gSaveContext = {};
    gNeiSave = {};
    registers = {};
    std::fill(std::begin(gNeiSave.ownedItems), std::end(gNeiSave.ownedItems), ITEM_NONE);
    std::fill(std::begin(gSaveContext.inventory.items), std::end(gSaveContext.inventory.items), ITEM_NONE);
    std::fill(std::begin(gItemIcons), std::end(gItemIcons), (void*)"icon");
    for (unsigned i = 0; i < 32; ++i) gBitFlags[i] = 1u << i;
    gSaveContext.inventory.items[SLOT_TRADE_CHILD] = ITEM_LETTER_ZELDA;
    gSaveContext.inventory.items[SLOT_TRADE_ADULT] = ITEM_COJIRO;
    gSaveContext.eventChkInf[0] = 0x1234;
    gSaveContext.itemGetInf[0] = 0x5678;
    gSaveContext.infTable[0] = 0x9ABC;
    gSaveContext.fileNum = 0;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.rupees = 999;
    gComboGameMode = COMBO_GAME_BOTH;
    gComboVanillaMode = 1;
    play.state.running = true;
    play.state.gfxCtx = &graphics;
    graphics.polyOpa.p = commands;
    play.msgCtx.textboxSegment = textbox;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    player.actor.category = ACTORCAT_PLAYER;
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    player.getItemEntry = GET_ITEM_NONE;
    player.pendingFlag.flagType = FLAG_NONE;
    player.talkActorDistance = 1000;
    sHost = HostStorage;
    sActorId = 0x7000;
    sStatus.clear();
    upperBodyDelay = begins = commits = aborts = receives = 0;
    rejectBegin = rejectCommit = false;
    document = {{"combo", {{"mode", "vanilla-nei"}, {"version", 1},
                          {"vanilla", {{"version", 1}, {"collected", nlohmann::json::object()}}}}},
                {"oot", Snapshot()}};
    disk = document;
    RememberTrade();
}
static RewardActor MakeReward(const char* id = "vanilla.nei.rod_of_seasons") {
    RewardActor reward{};
    reward.actor.id = sActorId;
    reward.actor.params = FindReward(id) - kRewards;
    reward.actor.category = ACTORCAT_NPC;
    reward.actor.flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY;
    reward.actor.update = RewardUpdate;
    RewardInit(&reward.actor, &play);
    return reward;
}
static std::string Decode() {
    graphics.polyOpa.p = commands;
    Message_Decode(&play);
    auto text = std::string(reinterpret_cast<char*>(play.msgCtx.msgBufDecoded), play.msgCtx.decodedTextLen);
    std::replace(text.begin(), text.end(), '\x01', ' ');
    return text;
}
static void Accept(RewardActor& reward, bool immediate = false) {
    play.actorCtx.actorLists[ACTORCAT_NPC].head = &reward.actor;
    RewardUpdate(&reward.actor, &play);
    Check(player.talkActor == &reward.actor, "native actor did not offer talk");
    Fixture_BindInput(&play);
    play.state.input[0].press.button = BTN_A;
    Check(Player_ActionHandler_Talk(&player, &play), "native player did not accept A-button offer");
    play.state.input[0].press.button = 0;
    if (immediate) Player_Action_WaitForPutAway(&player, &play);
    RewardUpdate(&reward.actor, &play);
}
static void OpenAfterPutAway(RewardActor& reward) {
    for (unsigned i = 0; i < 6; ++i) {
        if (player.actionFunc == Player_Action_WaitForPutAway)
            Player_Action_WaitForPutAway(&player, &play);
        RewardUpdate(&reward.actor, &play);
    }
    Check(play.msgCtx.msgMode == MSGMODE_TEXT_START, "native player did not open reward textbox");
}

static void ReceiptFamily() {
    GetItemEntry entry{};
    Check(!ComboStartupOot_IsReceipt(nullptr), "null entry was classified as a startup receipt");
    Check(!ComboStartupOot_IsReceipt(&entry), "ordinary entry was classified as a startup receipt");
    entry.comboMarketReceipt = kReceiptBit | 1;
    Check(!ComboStartupOot_IsReceipt(&entry), "fixed Vanilla receipt was classified as startup presentation");
    entry.comboMarketReceipt = CW_STARTUP_RECEIPT_TOKEN_BIT | 1;
    Check(ComboStartupOot_IsReceipt(&entry), "real startup receipt classifier did not recognize its token");
}

static void FirstOpen() {
    for (int previous = 0; previous < 3; ++previous) {
        Reset();
        auto rod = MakeReward();
        auto shovel = MakeReward("vanilla.nei.shovel");
        Actor unrelated{};
        play.msgCtx.talkActor = previous == 0 ? nullptr : previous == 1 ? &unrelated : &shovel.actor;
        if (previous == 2) rod.actor.next = &shovel.actor;
        Accept(rod);
        OpenAfterPutAway(rod);
        const auto text = Decode();
        CheckTrade();
        Check(text.find("Rod of Seasons") != std::string::npos,
              "first native open used stale owner / Pocket Egg fallback instead of Rod of Seasons");
        Check(text.find("Shovel") == std::string::npos && text.find("Pocket Egg") == std::string::npos,
              "previous message owner contaminated reward label");
        CheckTrade();
    }
}
static void UnknownText() {
    for (const auto textId : {kOfferText, kReceiptText, kResultText}) {
        for (int owner = 0; owner < 5; ++owner) {
            Reset();
            Actor unrelated{};
            auto previous = MakeReward("vanilla.nei.shovel");
            play.msgCtx.talkActor = &previous.actor;
            player.actor.flags |= ACTOR_FLAG_TALK;
            if (owner == 1) player.talkActor = &unrelated;
            if (owner == 2) player.talkActor = reinterpret_cast<Actor*>(uintptr_t{1}); // retired owner, never dereference
            if (owner == 3) {
                player.talkActor = &previous.actor;
                play.actorCtx.actorLists[ACTORCAT_NPC].head = &previous.actor;
                Actor_Kill(&previous.actor);
            }
            if (owner == 4) {
                player.talkActor = &previous.actor;
                play.actorCtx.actorLists[ACTORCAT_NPC].head = &previous.actor;
                gComboVanillaMode = 0;
            }
            Message_StartTextbox(&play, textId, &unrelated);
            const auto text = Decode();
            CheckTrade();
            Check(text.find("Pocket Egg") == std::string::npos && text.find("Shovel") == std::string::npos,
                  "unowned reserved reward text fell through to stock / previous reward text");
            Check(text.find("unavailable") != std::string::npos && play.msgCtx.choiceNum == 0,
                  "stale custom text did not produce a safe non-choice message");
        }
    }
    Reset();
    Message_StartTextbox(&play, 0x1234, nullptr);
    Check(Decode().find("Unrelated native text") != std::string::npos, "unrelated native text was intercepted");
}
static void Choose(RewardActor& reward, unsigned choice) {
    Decode();
    Check(play.msgCtx.choiceNum == 2, "production message decoder did not expose Yes/No");
    // Text drawing finishes the decoded choice. Rendering is outside this harness.
    play.msgCtx.msgMode = MSGMODE_TEXT_DONE;
    play.msgCtx.textboxEndType = TEXTBOX_ENDTYPE_2_CHOICE;
    play.msgCtx.choiceIndex = choice;
    play.state.input[0].press.button = BTN_A;
    Check(Message_GetState(&play.msgCtx) == TEXT_STATE_CHOICE, "choice was not native-visible");
    RewardUpdate(&reward.actor, &play);
    play.state.input[0].press.button = 0;
}
static void YesStartsReceipt() {
    Reset();
    auto rod = MakeReward();
    // Isolate the lost accepted-talk state from the independent first-open bug.
    play.msgCtx.talkActor = &rod.actor;
    upperBodyDelay = 3;
    Accept(rod);
    OpenAfterPutAway(rod);
    Choose(rod, 0);
    Check(begins == 1 && sReceiptReward == FindReward("vanilla.nei.rod_of_seasons"),
          "Yes closed without beginning the Rod receipt");
    CheckTrade();
}
static void DelayedTalk() {
    Reset();
    auto rod = MakeReward();
    upperBodyDelay = 3;
    Accept(rod);
    Check(rod.talking, "accepted NPC talk discarded while native textbox was still NONE");
    OpenAfterPutAway(rod);
    Check(rod.talking, "accepted NPC talk lost during delayed put-away");
    CheckTrade();
}

static void CloseTalk(RewardActor& reward) {
    Check(play.msgCtx.msgMode == MSGMODE_TEXT_CLOSING, "dialogue did not begin closing");
    // Drive the renderer's close-timer boundary; native Player consumes CLOSING
    // and restores ordinary control before the monitor may offer a get-item.
    play.msgCtx.stateTimer = 1;
    Check(Message_GetState(&play.msgCtx) == TEXT_STATE_CLOSING, "native close state not reached");
    Check(player.actionFunc == Player_Action_Talk, "unexpected native talk action");
    player.actionFunc(&player, &play);
    RewardUpdate(&reward.actor, &play);
    play.msgCtx.msgLength = 0;
    play.msgCtx.msgMode = MSGMODE_NONE;
    Check(!(player.actor.flags & ACTOR_FLAG_TALK) && !(player.stateFlags1 & PLAYER_STATE1_TALKING),
          "native player remained in talk after closing");
    CheckTrade();
}

static void OfferReceipt() {
    Check(sReceiptReward && !sOffered, "receipt not waiting for a safe native offer");
    MonitorReceipt();
    Check(sOffered && player.interactRangeActor == &player.actor,
          "native GiveItemEntryWithoutActor did not offer the receipt");
    Check(player.getItemId == RG_ROD_OF_SEASONS && player.getItemEntry.getItemId == RG_ROD_OF_SEASONS &&
              player.getItemEntry.itemId == ITEM_NONE && player.getItemEntry.modIndex == MOD_RANDOMIZER,
          "native Rod presentation identity changed before pickup");
    Check(player.getItemEntry.comboMarketReceipt == sReceiptTicket && player.getItemEntry.textId == kReceiptText,
          "native offer lost receipt ticket or text ID");
    Check(!ComboStartupOot_IsReceipt(&player.getItemEntry), "offered Rod receipt became startup presentation only");
    Check(!(player.stateFlags1 & PLAYER_STATE1_GETTING_ITEM), "fixture prepared getting-item flag itself");
}

static void ConsumeReceipt() {
    Check(Player_ActionHandler_2(&player, &play), "native player rejected fixed get-item offer");
    Check(player.stateFlags1 & PLAYER_STATE1_GETTING_ITEM, "native player did not enter get-item state");
    Check(player.actionFunc == Player_Action_WaitForPutAway, "native GI put-away was skipped");
    player.actionFunc(&player, &play);
    Check(player.actionFunc == Player_Action_8084E6D4, "native get-item action was not installed");
    player.actionFunc(&player, &play); // func_8084DFF4 -> Randomizer_Item_Give -> fixed receipt
    Check(sDelivered && receives == 1, "native receipt consumer/hook did not deliver exactly once");
    Check(play.msgCtx.textId == kReceiptText, "native consumer did not open fixed receipt message");
    CheckTrade();
}

static void FinishReceipt() {
    Message_CloseTextbox(&play);
    play.msgCtx.stateTimer = 1;
    player.actionFunc(&player, &play); // native consumer clears the GI entry
    Check(player.getItemId == GI_NONE && !player.getItemEntry.comboMarketReceipt,
          "native receipt close kept the old get-item entry");
    player.actionFunc(&player, &play); // native get-item action returns to idle
    play.msgCtx.msgLength = 0;
    play.msgCtx.msgMode = MSGMODE_NONE;
    MonitorReceipt();
    Check(!sReceiptReward && !sOffered && Safe(&play), "finished receipt wedged subsequent pickup");
    CheckTrade();
}

static void RodGrant() {
    for (int schedule = 0; schedule < 3; ++schedule) {
        Reset();
        auto rod = MakeReward();
        NeiSaveData expected = gNeiSave;
        expected.seasonsRodOwned = 1;
        expected.ownedItems[SLOT_ROD_OF_SEASONS - 24] = EXT_ITEM_ROD_OF_SEASONS;
        expected.season = SEASON_OFF;
        upperBodyDelay = schedule == 2 ? 3 : 0;
        Accept(rod, schedule == 0);
        OpenAfterPutAway(rod);
        Choose(rod, 0);
        Check(begins == 1 && !gNeiSave.seasonsRodOwned, "Yes did not stage an ungranted receipt");
        MonitorReceipt();
        Check(!sOffered, "receipt offered while native talk was still active");
        CloseTalk(rod);
        OfferReceipt();
        ConsumeReceipt();
        Check(gNeiSave.seasonsRodOwned == 1 && Nei_GetOwnedItem(SLOT_ROD_OF_SEASONS) == EXT_ITEM_ROD_OF_SEASONS,
              "real Seasons_GrantRod / Nei_SetOwnedItem did not grant exact Rod ownership");
        Check(gNeiSave.seasonsOwned == 0 && gNeiSave.season == SEASON_OFF,
              "Rod pickup also granted an unearned season");
        Check(std::memcmp(&expected, &gNeiSave, sizeof(expected)) == 0,
              "native Rod grant changed unrelated NEI ownership");
        Check(commits == 1 && RewardStatus(document, *FindReward("vanilla.nei.rod_of_seasons")) == 1 &&
                  RewardStatus(disk, *FindReward("vanilla.nei.rod_of_seasons")) == 1 && disk["oot"]["rodOwned"] == 1 &&
                  disk["oot"]["rodSlot"] == EXT_ITEM_ROD_OF_SEASONS,
              "native Rod/check ownership was not committed together");
        Check(Decode().find("Rod of Seasons") != std::string::npos, "native receipt text lost the Rod label");
        const auto deliveredEntry = player.getItemEntry;
        player.actionFunc(&player, &play);
        Check(commits == 1 && receives == 1, "repeat native presentation regranted receipt");
        Randomizer_Item_Give(&play, deliveredEntry);
        Check(commits == 1 && receives == 2, "duplicate callback recommitted receipt");
        FinishReceipt();
        RewardUpdate(&rod.actor, &play);
        Check(!rod.actor.update, "collected Rod actor remained available");
        Check(gSaveContext.rupees == 999, "free Rod changed rupees");
    }
}

static void NoAndReentry() {
    Reset();
    auto rod = MakeReward();
    Accept(rod);
    OpenAfterPutAway(rod);
    Choose(rod, 1);
    CloseTalk(rod);
    Check(!sReceiptReward && begins == 0 && !gNeiSave.seasonsRodOwned && rod.actor.update,
          "No consumed or granted the optional reward");
    player.talkActor = nullptr; // next native player update clears prior frame's offer
    player.talkActorDistance = 1000;
    Accept(rod);
    OpenAfterPutAway(rod);
    Choose(rod, 0);
    CloseTalk(rod);
    OfferReceipt();
    ConsumeReceipt();
    Check(gNeiSave.seasonsRodOwned && commits == 1, "reentry after No did not grant exactly once");
    FinishReceipt();
}

static void CancelledTalk() {
    for (int afterOpen = 0; afterOpen < 2; ++afterOpen) {
        Reset();
        auto rod = MakeReward();
        upperBodyDelay = 3;
        Accept(rod);
        if (afterOpen) {
            OpenAfterPutAway(rod);
            Decode();
            Message_CloseTextbox(&play);
            CloseTalk(rod);
        } else {
            // Scene/cutscene interruption removes native talk ownership while
            // the put-away state still has no textbox. Actor must drop it.
            player.actor.flags &= ~ACTOR_FLAG_TALK;
            Player_SetupAction(&play, &player, Player_Action_Idle, 1);
            RewardUpdate(&rod.actor, &play);
            Check(!rod.talking, "cancelled pre-text talk remained armed");
        }
        Check(begins == 0 && !sReceiptReward && !gNeiSave.seasonsRodOwned,
              "cancelled talk granted a reward");
        upperBodyDelay = 0;
        player.talkActor = nullptr;
        player.talkActorDistance = 1000;
        Accept(rod);
        OpenAfterPutAway(rod);
        Choose(rod, 1);
        CloseTalk(rod);
        Check(!rod.talking && rod.actor.update, "cancel/reentry wedged the optional reward");
    }
}

static void StaleChoice() {
    for (int replaced = 0; replaced < 3; ++replaced) {
        Reset();
        auto rod = MakeReward();
        auto other = MakeReward("vanilla.nei.shovel");
        rod.actor.next = &other.actor;
        Accept(rod);
        OpenAfterPutAway(rod);
        Check(rod.talking, "native accepted talk was not armed before stale-choice test");
        if (replaced == 0) player.talkActor = &other.actor;
        if (replaced == 1) play.msgCtx.talkActor = &other.actor;
        if (replaced == 2) play.msgCtx.textId = kResultText;
        Choose(rod, 0);
        Check(!begins && !sReceiptReward && !transaction.Active() && !gNeiSave.seasonsRodOwned,
              "stale player owner/message owner/text ID accepted another textbox's Yes");
        CheckTrade();
    }
}

static void SaveFailures() {
    for (bool failCommit : {false, true}) {
        Reset();
        auto rod = MakeReward();
        rejectBegin = !failCommit;
        rejectCommit = failCommit;
        Accept(rod);
        OpenAfterPutAway(rod);
        Choose(rod, 0);
        if (!failCommit) {
            Check(!sReceiptReward && play.msgCtx.textId == kResultText && Decode().find("Unable to save") != std::string::npos,
                  "begin failure did not show the current reward's safe error");
            Message_CloseTextbox(&play);
            CloseTalk(rod);
        } else {
            CloseTalk(rod);
            OfferReceipt();
            ConsumeReceipt();
            Check(Decode().find("could not be saved") != std::string::npos, "commit failure did not show rollback");
            FinishReceipt();
        }
        Check(!gNeiSave.seasonsRodOwned && Nei_GetOwnedItem(SLOT_ROD_OF_SEASONS) == ITEM_NONE &&
                  RewardStatus(document, *FindReward("vanilla.nei.rod_of_seasons")) == 0 &&
                  RewardStatus(disk, *FindReward("vanilla.nei.rod_of_seasons")) == 0 && gSaveContext.rupees == 999,
              "save failure lost availability / leaked native ownership or rupees");
        CheckTrade();
        rejectBegin = rejectCommit = false;
        receives = 0;
        player.talkActor = nullptr;
        player.talkActorDistance = 1000;
        Accept(rod);
        OpenAfterPutAway(rod);
        Choose(rod, 0);
        CloseTalk(rod);
        OfferReceipt();
        ConsumeReceipt();
        Check(gNeiSave.seasonsRodOwned && RewardStatus(disk, *FindReward("vanilla.nei.rod_of_seasons")) == 1,
              "save failure prevented a later successful retry");
        FinishReceipt();
    }
}

static void CancelledReceipt() {
    for (bool afterOffer : {false, true}) {
        Reset();
        auto rod = MakeReward();
        Accept(rod);
        OpenAfterPutAway(rod);
        Choose(rod, 0);
        CloseTalk(rod);
        if (afterOffer) OfferReceipt();
        const auto stale = sReceiptEntry;
        play.transitionTrigger = TRANS_TRIGGER_START;
        MonitorReceipt();
        Check(!sReceiptReward && !transaction.Active(), "transition did not abort staged reward");
        Randomizer_Item_Give(&play, stale);
        Check(!gNeiSave.seasonsRodOwned && !commits && !player.getItemEntry.comboMarketReceipt,
              "stale native callback granted after cancellation");
        CheckTrade();
    }
}

static void ManifestOffers() {
    unsigned checked = 0;
    for (const auto& reward : kRewards) {
        if (reward.game != Game::Oot || reward.kind == RewardKind::Retained) continue;
        caseName = reward.checkId;
        Reset();
        gSaveContext.linkAge = std::string_view(reward.prerequisite) == "child" ? LINK_AGE_CHILD : LINK_AGE_ADULT;
        gSaveContext.inventory.questItems = 0xFFFFFFFF;
        gSaveContext.inventory.equipment = 0xFFFF;
        gSaveContext.inventory.upgrades = 0xFFFFFFFF;
        gSaveContext.inventory.gsTokens = 100;
        gSaveContext.inventory.items[SLOT_HOOKSHOT] = ITEM_LONGSHOT;
        gSaveContext.inventory.items[SLOT_HAMMER] = ITEM_HAMMER;
        gSaveContext.inventory.items[SLOT_BOW] = ITEM_BOW;
        gSaveContext.itemGetInf[ITEMGETINF_TALON_BOTTLE >> 4] |= 1u << (ITEMGETINF_TALON_BOTTLE & 15);
        gNeiSave.vanillaRewardEvents = 1;
        RememberTrade();
        Check(NativePrerequisite(reward.prerequisite), "manifest fixture lacks native prerequisite");
        auto placed = MakeReward(reward.checkId);
        Check(placed.actor.update, "manifest reward lacks real presentation table metadata");
        upperBodyDelay = 2;
        Accept(placed);
        OpenAfterPutAway(placed);
        Check(Decode().find(reward.label) != std::string::npos, "manifest first-open label was wrong");
        Choose(placed, 1);
        CloseTalk(placed);
        Check(!sReceiptReward && !NativeOwned(reward), "manifest No granted/consumed reward");
        player.talkActor = nullptr;
        player.talkActorDistance = 1000;
        Accept(placed);
        OpenAfterPutAway(placed);
        Choose(placed, 0);
        Check(begins == 1 && sReceiptReward == &reward && !NativeOwned(reward),
              "manifest Yes did not stage the exact ungranted reward");
        CloseTalk(placed);
        CancelReceipt();
        CheckTrade();
        ++checked;
    }
    Check(checked == 38, "OoT optional reward manifest coverage changed");
    std::cout << "PASS native offer/No/Yes staging: " << checked << " OoT manifest rewards\n";
}

int main(int argc, char** argv) {
    Rando::StaticData::InitItemTable();
    const std::string_view which = argc > 1 ? argv[1] : "all";
    const std::pair<const char*, void(*)()> cases[] = {
        {"receipt-family", ReceiptFamily},
        {"first", FirstOpen}, {"delayed", DelayedTalk}, {"unknown", UnknownText},
        {"yes-start", YesStartsReceipt}, {"rod", RodGrant}, {"no", NoAndReentry},
        {"cancel-talk", CancelledTalk}, {"stale-choice", StaleChoice}, {"save-failures", SaveFailures},
        {"cancel-receipt", CancelledReceipt}, {"manifest", ManifestOffers},
    };
    unsigned ran = 0;
    for (const auto& [name, test] : cases) {
        if (which != "all" && which != name) continue;
        caseName = name;
        test();
        ++ran;
        std::cout << "PASS native OoT pickup case: " << name << '\n';
    }
    Check(ran != 0, "unknown test case");
    std::cout << "PASS native OoT reward pickup flow: " << which << '\n';
}
