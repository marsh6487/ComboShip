#include "combo/vanilla/ComboVanillaRewards.h"
#include "combo/vanilla/ComboVanillaMode.h"
#include "soh/expansions/sw97/sw97_config.h"
#include "soh/mods/nei_save.h"
extern "C" {
#include "z64.h"
}
extern "C" {
#include "macros.h"
#include "variables.h"
}
#include <cassert>
#include <cmath>
#include <cstring>
#include <functional>
#include <iostream>
#include <string_view>
#include <vector>

using namespace ComboVanilla;
extern "C" {
SaveContext gSaveContext{};
int gComboGameMode = COMBO_GAME_BOTH;
int gComboVanillaMode = 1;
u8 gItemSlots[56]{};
Vec3f gSfxDefaultPos{};
f32 gSfxDefaultFreqAndVolScale = 1;
s8 gSfxDefaultReverb = 0;
u32 gBitFlags[32]{};
u8 gEquipShifts[4] = { 0, 4, 8, 12 };
u8 gUpgradeShifts[8] = { 0, 3, 6, 9, 12, 14, 17, 20 };
u32 gUpgradeMasks[8] = { 7, 56, 448, 3584, 12288, 114688, 917504, 7340032 };
static NeiSaveData nei{};
NeiSaveData* Nei_Save() {
    return &nei;
}
static unsigned grantKind, grantPayload;
void QueueGrantSave();
uint8_t Cane_GiveSkill(uint8_t index) {
    grantKind = 1;
    grantPayload = index;
    nei.caneSkills |= 1u << index;
    return 1;
}
void Wand_GrantMode(uint8_t index) {
    grantKind = 2;
    grantPayload = index;
    nei.wandRodsOwned |= 1u << index;
}
void Slate_GrantRune(uint8_t index) {
    grantKind = 3;
    grantPayload = index;
    nei.slateRunesOwned |= 1u << index;
}
void Seasons_GrantSeason(uint8_t index) {
    grantKind = 4;
    grantPayload = index;
    nei.seasonsOwned |= 1u << index;
}
void Seasons_GrantRod() {
    nei.seasonsRodOwned = 1;
}
uint8_t ExtEquip_HasItem(short type, uint8_t index) {
    return (nei.extEquipOwnedBits >> (16 + type * 3 + index - 1)) & 1;
}
void ExtEquip_GiveItem(short type, uint8_t index) {
    nei.extEquipOwnedBits |= 1u << (16 + type * 3 + index - 1);
}
void TwilightUpgrade_SetClawshot(uint8_t value) {
    nei.twilightUpgrade |= value;
}
void WeaponUpgrade_SetHammerAxe(uint8_t value) {
    nei.weaponUpgrades |= value;
}
void WeaponUpgrade_SetTrueMaster(uint8_t value) {
    nei.weaponUpgrades |= value << 3;
}
void VanillaRocsFeather_Grant() {
    nei.separateRocsFlags |= 1;
}
static uint16_t& cellItem = nei.ownedItems[0];
uint8_t ExtInv_GetItemSlot(uint16_t) {
    return 24;
}
uint16_t ExtInv_GetSlotItem(int) {
    return cellItem;
}
void ExtInv_SetItemById(uint16_t item) {
    cellItem = item;
    grantPayload = item;
    QueueGrantSave();
}
uint8_t TravelTools_IsOwned(uint16_t) {
    return nei.timeGateOwned;
}
void TravelTools_Grant(uint16_t item) {
    nei.timeGateOwned = 1;
    grantPayload = item;
}
uint8_t GraceHourglass_IsOwned(uint16_t) {
    return nei.hyliasGraceOwned;
}
void GraceHourglass_Grant(uint16_t item) {
    nei.hyliasGraceOwned = 1;
    grantPayload = item;
}
void RitoItem_NoteCellItem(int item) {
    if (item == ITEM_FARORES_WIND)
        nei.ritoMaskFlags |= RITO_FLAG_FARORES_OWNED;
}
static bool nativePrize = false;
int Flags_GetItemGetInf(int) {
    return nativePrize;
}
PlayState* gPlayState = nullptr;
}
constexpr uint16_t EXT_ITEM_SHADOW_CRYSTAL = 0x222;
constexpr uint8_t SLOT_ROCS = 24;
#include "oot_rewards_helpers.inc"

static nlohmann::json document, disk;
static Receipt transaction;
static std::vector<std::function<void()>> workers;
static bool queueOnGrant = false, rejectCommit = false, active = true;
static unsigned drains = 0, nativeGrants = 0;
static nlohmann::json Snapshot() {
    return { { "fileType", 0 },
             { "version", 1 },
             { "sections", { { "base", { { "data", { { "rupees", gSaveContext.rupees } } } } } } },
             { "neiItem", cellItem },
             { "caneSkills", nei.caneSkills } };
}
static void OrdinaryWrite(nlohmann::json native) {
    if (transaction.Active())
        assert(transaction.Stage(0, native));
    else
        document["oot"] = disk["oot"] = std::move(native);
}
extern "C" void QueueGrantSave() {
    ++nativeGrants;
    if (queueOnGrant) {
        const auto save = Snapshot();
        workers.emplace_back([save] { OrdinaryWrite(save); });
    }
}
class SaveManager {
  public:
    static SaveManager* Instance;
    void ThreadPoolWait() {
        ++drains;
        while (!workers.empty()) {
            auto pending = std::move(workers);
            workers.clear();
            for (auto& work : pending)
                work();
        }
    }
    nlohmann::json SaveToJsonObject() {
        return Snapshot();
    }
};
SaveManager manager;
SaveManager* SaveManager::Instance = &manager;
static int FakeHost(int action, int game, int slot, const char* checkId, const char* native) {
    assert(game == 0 && slot == 0);
    const auto* reward = FindReward(checkId);
    assert(reward);
    auto writer = [](const nlohmann::json& next) {
        disk = next;
        return true;
    };
    switch (static_cast<RewardAction>(action)) {
        case RewardAction::Query:
            return transaction.Active() ? 2 : RewardStatus(document, *reward);
        case RewardAction::Begin:
            return transaction.Begin(document, *reward, false, writer);
        case RewardAction::BeginOwned:
            return transaction.Begin(document, *reward, true, writer);
        case RewardAction::Commit:
            assert(native && transaction.Stage(game, nlohmann::json::parse(native)));
            if (rejectCommit) {
                const auto granted = Snapshot();
                workers.emplace_back([granted] {
                    assert(transaction.Active());
                    OrdinaryWrite(granted);
                });
                return 0;
            }
            return transaction.Commit(document, writer);
        case RewardAction::Abort:
            transaction.Abort();
            return 1;
        case RewardAction::PrepareCapture:
            transaction.PrepareCapture();
            return 1;
    }
    return -1;
}
constexpr int kReceiptBit = 0x40000000;
static RewardHost sHost = FakeHost;
static const Reward* sReceiptReward = nullptr;
static int sReceiptSlot = -1, sReceiptTicket = 0, sSequence = 0;
static bool sOffered = false, sDelivered = false;
static GetItemEntry sReceiptEntry{};
static std::string sStatus;
static bool Active() {
    return active;
}
static bool BuildEntry(const Reward&, GetItemEntry& entry) {
    entry = {};
    entry.getItemId = 42;
    return true;
}
class CustomMessage {
  public:
    explicit CustomMessage(const std::string&) {
    }
    void AutoFormat() {
    }
    void LoadIntoFont() {
    }
};
#define SPDLOG_ERROR(...) (void)0
#include "oot_rewards_receipt.inc"

static bool sEntryReady = true;
static Vec3f sEntryPosition = { 0, 0, 0 };
static CollisionPoly safeFloor{};
static unsigned badFloor = 0;
static float BgCheck_EntityRaycastFloor5(PlayState*, CollisionContext*, CollisionPoly** out, s32* bg, Actor*, Vec3f*) {
    *out = badFloor == 1 ? nullptr : &safeFloor;
    *bg = badFloor == 2 ? 1 : BGCHECK_SCENE;
    safeFloor.normal.y = badFloor == 3 ? 16000 : 32767;
    return badFloor == 4 ? 100 : 0;
}
static u32 SurfaceType_GetSceneExitIndex(CollisionContext*, CollisionPoly*, s32) {
    return badFloor == 5;
}
static int WaterBox_GetSurface1(PlayState*, CollisionContext*, float, float, float* y, WaterBox**) {
    *y = 10;
    return badFloor == 6;
}
static int BgCheck_EntityLineTest1(CollisionContext*, Vec3f*, Vec3f*, Vec3f*, CollisionPoly**, int, int, int, int,
                                   s32*) {
    return badFloor == 7;
}
#include "oot_rewards_geometry.inc"
static int medallionOption = 0, bombOption = 0;
extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    if (std::strcmp(key, SW97_MEDALLIONS_CVAR) == 0)
        return medallionOption;
    if (std::strcmp(key, "gMods.BombArrows.Mode") == 0)
        return bombOption;
    return fallback;
}
extern "C" uint8_t TwilightUpgrade_HasBombArrows() {
    return (nei.twilightUpgrade & 2) != 0;
}
static unsigned wheelRuns = 0, iconsRefreshed = 0;
template <class... Args> static void Audio_PlaySoundGeneral(Args...) {
}
static void Sw97_RefreshButtonIcons(PlayState*) {
    ++iconsRefreshed;
}
static void Interface_LoadItemIcon1(PlayState*, int) {
    ++iconsRefreshed;
}
static void KaleidoWheel_Run(PlayState* play, int, int, u8*, void (*cycle)(PlayState*, s32)) {
    ++wheelRuns;
    cycle(play, 1);
}
#define ARROW_WHEEL_MAX_ENTRIES SW97_ELEM_COUNT
static u8 sArrowWheelEntries[ARROW_WHEEL_MAX_ENTRIES]{}, sArrowWheelAvailCount = 0, sArrowWheelCursor = 0;
static u8 sArrowWheelIsSling = 0, sArrowWheelPressAActive = 0;
static s8 sArrowWheelCycleCurIdx = 0, gCurrentItemCyclingSlot = -1;
static s32 sArrowWheelCycleCBtn = -1;
#include "oot_rewards_elements.inc"

static void ResetReceiptFixture() {
    transaction.Abort();
    workers.clear();
    queueOnGrant = rejectCommit = false;
    active = true;
    sReceiptReward = nullptr;
    sReceiptSlot = -1;
    sOffered = sDelivered = false;
    gSaveContext = {};
    nei = {};
    cellItem = ITEM_NONE;
    gSaveContext.fileNum = 0;
    gSaveContext.linkAge = LINK_AGE_CHILD;
    gSaveContext.rupees = 99;
    document = { { "combo",
                   { { "mode", "vanilla-nei" },
                     { "vanilla", { { "version", 1 }, { "collected", nlohmann::json::object() } } } } },
                 { "oot", Snapshot() } };
    disk = document;
}
static void AcceptReceipt(Player& player) {
    sOffered = true;
    player.getItemEntry = sReceiptEntry;
    player.stateFlags1 = PLAYER_STATE1_GETTING_ITEM;
}

int main() {
    for (unsigned i = 0; i < 32; ++i)
        gBitFlags[i] = 1u << i;
    unsigned rows = 0;
    for (const auto& reward : kRewards) {
        if (reward.game != Game::Oot)
            continue;
        ++rows;
        const auto binding = NativeBinding(reward.location);
        assert(binding.scene >= 0 && (binding.entrance >= 0 || binding.actor >= 0));
        assert(NativePrerequisiteKnown(reward.prerequisite));
        gSaveContext = {};
        nei = {};
        cellItem = ITEM_NONE;
        nativePrize = false;
        std::memset(gSaveContext.inventory.items, ITEM_NONE, sizeof(gSaveContext.inventory.items));
        gSaveContext.linkAge = LINK_AGE_CHILD;
        const std::string_view gate = reward.prerequisite;
        assert(NativePrerequisite(gate) == (gate == "native_access" || gate == "existing_source" || gate == "child"));
        assert(!NativeOwned(reward));
        if (reward.kind == RewardKind::Retained)
            continue;
        // Grant helpers must preserve every native progression family.
        gSaveContext.inventory.equipment = 0x7777;
        gSaveContext.inventory.questItems = 0xFFFFFFFF;
        gSaveContext.inventory.items[SLOT_HOOKSHOT] = ITEM_LONGSHOT;
        gSaveContext.inventory.items[SLOT_HAMMER] = ITEM_HAMMER;
        gSaveContext.inventory.items[SLOT_BOW] = ITEM_BOW;
        gSaveContext.inventory.upgrades = 0xFFFFFFFF;
        gSaveContext.inventory.gsTokens = 30;
        gSaveContext.linkAge = gate == "child" ? LINK_AGE_CHILD : LINK_AGE_ADULT;
        nativePrize = true;
        assert(NativePrerequisite(gate));
        const auto before = gSaveContext;
        assert(GrantNative(reward));
        assert(NativeOwned(reward));
        assert(gSaveContext.inventory.equipment == before.inventory.equipment);
        assert(gSaveContext.inventory.questItems == before.inventory.questItems);
        assert(gSaveContext.healthCapacity == before.healthCapacity && gSaveContext.rupees == before.rupees);
        if (reward.kind == RewardKind::Item)
            assert(grantPayload == reward.payload);
        if (reward.kind == RewardKind::Cane)
            assert(grantKind == 1 && grantPayload == reward.payload);
        if (reward.kind == RewardKind::Wand)
            assert(grantKind == 2 && grantPayload == reward.payload);
        if (reward.kind == RewardKind::Rune)
            assert(grantKind == 3 && grantPayload == reward.payload);
        if (reward.kind == RewardKind::Season)
            assert(grantKind == 4 && grantPayload == reward.payload);
    }
    assert(rows == 39);
    assert(!NativePrerequisiteKnown("future_unimplemented_gate"));
    assert(!NativePrerequisite("future_unimplemented_gate"));
    gSaveContext = {};
    nativePrize = false;
    nei.vanillaRewardEvents = 1;
    assert(NativePrerequisite("bowling_prize"));
    nei.vanillaRewardEvents = 0;
    assert(!NativePrerequisite("bridge_or_longshot"));
    gSaveContext.eventChkInf[EVENTCHKINF_CARPENTERS_FREE_INDEX] = EVENTCHKINF_CARPENTERS_FREE_MASK_ALL;
    assert(NativePrerequisite("bridge_or_longshot"));
    const auto* lantern = FindReward("vanilla.nei.lantern");
    assert(lantern && lantern->price == 40);
    PlayState play{};
    Player player{};
    gPlayState = &play;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    Actor bowlingPit{};
    bowlingPit.id = ACTOR_EN_BOM_BOWL_PIT;
    player.interactRangeActor = &bowlingPit;
    play.sceneNum = SCENE_BOMBCHU_BOWLING_ALLEY;
    for (const auto prize :
         { GI_BOMB_BAG_30, GI_BOMB_BAG_40, GI_HEART_PIECE, GI_BOMBCHUS_10, GI_BOMBS_1, GI_RUPEE_PURPLE }) {
        GetItemEntry native{};
        native.modIndex = MOD_NONE;
        native.getItemId = prize;
        assert(NativeBowlingPrize(&play, native));
        native.modIndex = MOD_RANDOMIZER;
        assert(!NativeBowlingPrize(&play, native));
    }
    GetItemEntry native{};
    native.modIndex = MOD_NONE;
    native.getItemId = GI_RUPEE_PURPLE;
    bowlingPit.id = ACTOR_EN_ITEM00;
    assert(!NativeBowlingPrize(&play, native));
    bowlingPit.id = ACTOR_EN_BOM_BOWL_PIT;
    play.sceneNum = SCENE_LINKS_HOUSE;
    assert(!NativeBowlingPrize(&play, native));
    player.interactRangeActor = nullptr;
    ResetReceiptFixture();
    queueOnGrant = true;
    const auto stale = Snapshot();
    workers.emplace_back([stale] { OrdinaryWrite(stale); });
    assert(BeginReceipt(*lantern) && workers.empty() && transaction.Active());
    AcceptReceipt(player);
    const unsigned beforeGrants = nativeGrants;
    assert(FixedReceiptGrant(&play, sReceiptEntry));
    assert(nativeGrants == beforeGrants + 1 && workers.empty() && !transaction.Active());
    assert(gSaveContext.rupees == 59 && disk["oot"]["sections"]["base"]["data"]["rupees"] == 59);
    assert(RewardStatus(disk, *lantern) == 1 && disk["oot"]["neiItem"] == lantern->payload);
    assert(FixedReceiptGrant(&play, sReceiptEntry));
    assert(gSaveContext.rupees == 59 && nativeGrants == beforeGrants + 1);
    ResetReceiptFixture();
    queueOnGrant = true;
    rejectCommit = true;
    const auto oldNative = disk["oot"];
    assert(BeginReceipt(*lantern));
    AcceptReceipt(player);
    assert(FixedReceiptGrant(&play, sReceiptEntry));
    assert(workers.empty() && !transaction.Active() && gSaveContext.rupees == 99 && cellItem == ITEM_NONE);
    assert(disk["oot"] == oldNative && RewardStatus(disk, *lantern) == 0);
    assert(disk["combo"]["vanilla"]["pending"].contains(lantern->checkId));
    ResetReceiptFixture();
    assert(BeginReceipt(*lantern));
    AcceptReceipt(player);
    cellItem = lantern->payload; // An independent safe reconciliation wins before receipt.
    assert(FixedReceiptGrant(&play, sReceiptEntry));
    assert(gSaveContext.rupees == 99 && disk["combo"]["vanilla"]["collected"][lantern->checkId]["price"] == 0);
    ResetReceiptFixture();
    gSaveContext.rupees = 39;
    assert(!BeginReceipt(*lantern) && !transaction.Active());
    gSaveContext.rupees = 99;
    active = false;
    assert(!BeginReceipt(*lantern));
    GetItemEntry ordinary{};
    assert(!FixedReceiptGrant(&play, ordinary));
    ActorEntry entry{};
    play.linkActorEntry = &entry;
    Vec3f point{};
    gSaveContext.entranceIndex = ENTR_GRAVEYARD_ROYAL_TOMB_EXIT;
    play.sceneNum = SCENE_GRAVEYARD;
    const auto* crystal = FindReward("vanilla.nei.shadow_crystal");
    assert(crystal);
    assert(Anchor(*crystal, NativeBinding(crystal->location), point));
    gSaveContext.entranceIndex = ENTR_GRAVEYARD_ENTRANCE;
    assert(!Anchor(*crystal, NativeBinding(crystal->location), point));
    const auto* clawshot = FindReward("vanilla.nei.clawshot");
    assert(clawshot);
    play.sceneNum = SCENE_GERUDO_TRAINING_GROUND;
    gSaveContext.entranceIndex = ENTR_GERUDO_TRAINING_GROUND_ENTRANCE;
    play.roomCtx.curRoom.num = 2;
    assert(!Anchor(*clawshot, NativeBinding(clawshot->location), point));
    play.roomCtx.curRoom.num = 0;
    assert(Anchor(*clawshot, NativeBinding(clawshot->location), point));
    assert(SafeRewardPosition(&play, { 0, 0, 0 }, 0, point) && point.y == 22);
    for (badFloor = 1; badFloor <= 7; ++badFloor)
        assert(!SafeRewardPosition(&play, { 0, 0, 0 }, 0, point));
    badFloor = 0;
    play.sceneNum = SCENE_SACRED_FOREST_MEADOW;
    const auto* leaf = FindReward("vanilla.nei.deku_leaf");
    const auto* fourSword = FindReward("vanilla.nei.four_sword");
    assert(leaf && fourSword && play.actorCtx.actorLists[ACTORCAT_NPC].head == nullptr);
    gSaveContext.entranceIndex = ENTR_SACRED_FOREST_MEADOW_SOUTH_EXIT;
    assert(Anchor(*leaf, NativeBinding(leaf->location), point));
    gSaveContext.entranceIndex = ENTR_SACRED_FOREST_MEADOW_OUTSIDE_TEMPLE;
    assert(Anchor(*fourSword, NativeBinding(fourSword->location), point));
    gSaveContext.entranceIndex = ENTR_SACRED_FOREST_MEADOW_FOREST_TEMPLE_BLUE_WARP;
    assert(Anchor(*fourSword, NativeBinding(fourSword->location), point));
    sEntryReady = false;
    assert(!Anchor(*clawshot, NativeBinding(clawshot->location), point));
    nei = {};
    gSaveContext = {};
    gSaveContext.inventory.questItems = 0xFFFFFFFF;
    assert(!Sw97_ElementWheelEnabled());
    nei.bombArrowsOwned = 1;
    assert(Sw97_ElementWheelEnabled() && Sw97_ElementCount(0) == 2 && Sw97_ElementCount(1) == 1);
    Sw97_SetElement(0, SW97_ELEM_BOMB);
    assert(Sw97_EffectiveElement(0) == SW97_ELEM_BOMB);
    Sw97_SetElement(0, SW97_ELEM_FIRE);
    assert(Sw97_EffectiveElement(0) == SW97_ELEM_BOMB);
    nei.sw97BowElement = SW97_ELEM_NONE;
    play.pauseCtx.cursorItem[PAUSE_ITEM] = ITEM_BOW;
    gSaveContext.equips.buttonItems[1] = ITEM_BOW;
    ArrowWheel_HandlePressA(&play);
    assert(wheelRuns == 1 && Sw97_EffectiveElement(0) == SW97_ELEM_BOMB && iconsRefreshed);
    assert(medallionOption == 0 && bombOption == 0);
    nei.bombArrowsOwned = 0;
    nei.twilightUpgrade = 2;
    nei.sw97BowElement = SW97_ELEM_NONE;
    ArrowWheel_HandlePressA(&play);
    assert(wheelRuns == 2 && Sw97_EffectiveElement(0) == SW97_ELEM_BOMB);
    play.pauseCtx.cursorItem[PAUSE_ITEM] = ITEM_SLINGSHOT;
    ArrowWheel_HandlePressA(&play);
    assert(wheelRuns == 2 && Sw97_EffectiveElement(1) == SW97_ELEM_NONE);
    medallionOption = 1;
    Sw97_SetElement(0, SW97_ELEM_FIRE);
    assert(Sw97_EffectiveElement(0) == SW97_ELEM_FIRE);
    medallionOption = 0;
    gComboVanillaMode = 0;
    assert(!Sw97_ElementWheelEnabled() && Sw97_EffectiveElement(0) == SW97_ELEM_NONE);
    gComboVanillaMode = 1;
    gComboGameMode = COMBO_GAME_OOT;
    assert(!Sw97_ElementWheelEnabled());
    std::cout << "PASS every OoT manifest binding, exact NEI payload, native "
                 "progression isolation and unknown-gate rejection\n";
    std::cout << "PASS actual OoT receipt: worker drains, atomic debit, "
                 "failed-write rollback, pending recovery, pre-owned alias and "
                 "stale-token isolation\n";
    std::cout << "PASS native return/room anchors and missing, dynamic, steep, "
                 "low, exit, submerged or obstructed floor rejection\n";
    std::cout << "PASS earned Bomb/Twilight actual element and native pause selector with module off, independent "
                 "medallions and seeded/solo controls\n";
}
