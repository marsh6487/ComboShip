#include "combo/vanilla/ComboVanillaSharing.h"
#ifdef TEST_MM
#include "mm/mods/nei_save.h"
#include "mm/2s2h/FleetShipCombo/FleetComboItems.h"
#include "combo/rando/RpgStatsJson.h"
#include "mm/2s2h/Rando/Types.h"
#include "mm/2s2h/Rando/SoulShuffleShared.h"
#else
#include "soh/mods/nei_save.h"
#include "soh/soh/Enhancements/randomizer/randomizerEnums.h"
#include "soh/soh/Enhancements/randomizer/SoulShuffleShared.h"
#endif
#include "combo/rando/SharedSoulRuntime.h"
#include <cassert>
#include <iostream>
#include <cstring>
#include <algorithm>
#include "z64item.h"
#include "combo/vanilla/ComboVanillaMode.h"
#include "combo/vanilla/ComboVanillaPolicy.h"

using nlohmann::json;
using namespace ComboVanilla;

// The real adapters compile against the real native item IDs and NeiSaveData.
// Serialization/renderer and native save storage are fixture boundaries.
struct InventoryBoundary { uint16_t equipment{}; uint8_t items[48]{}; uint32_t nativeStory[16]{}; };
#ifdef TEST_MM
struct SaveBoundary {
    struct { struct { InventoryBoundary inventory; struct { uint16_t equipment{}; uint8_t buttonItems[4][4]{}; } equips;
        struct { unsigned threeDayResetCount{}; } playerData; } saveInfo; } save;
    int32_t fileNum{}; uint32_t nativeVitalsAndTime[16]{};
} gSaveContext;
#define MM_INV gSaveContext.save.saveInfo.inventory
#define MM_EQ gSaveContext.save.saveInfo.equips
#else
struct ItemEquips { uint8_t buttonItems[8]{}; uint8_t cButtonSlots[7]{}; };
struct SaveBoundary { InventoryBoundary inventory; ItemEquips equips, childEquips, adultEquips; uint8_t bgsFlag{}; int32_t fileNum{}; uint32_t nativeVitalsAndTime[16]{}; } gSaveContext;
#endif
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
int gComboVanillaMode = 1;
}
static NeiSaveData live{};
static unsigned legacyCalls{}, repairs{}, refreshes{}, pulls{}, peerSignals{}, legacyGrants{}, legacyFanouts{};
static int sReceiveDepth{};
static bool sPullPending{};
static void* gPlayState = reinterpret_cast<void*>(1);
static unsigned peerExtracts{}, peerApplies{};
static unsigned peerSoulExtracts{}, peerSoulGrants{}, nativeSoulGrants{};
static uint32_t soulMask{};
static std::set<std::string> nativeSoulFlags;
static json peerSoulState;
static json sentPacket;
static json sNetFlat;
static int sNetMismatchStreak{}, sNetFutileResyncs{}, sNetResyncCooldownLeft{};
static constexpr int kNetResyncCooldown = 10;
static void FleetShipCombo_PushPacket(const char* wire) { sentPacket = json::parse(wire); }
static const char* ExtractPeerState() { ++peerExtracts; return "{}"; }
static const char* ExtractPeerSoulState() {
    ++peerSoulExtracts;
    static std::string wire;
    wire = peerSoulState.dump();
    return wire.c_str();
}
static void* Combo_ResolveSym(const char* module, const char* name) {
#ifdef TEST_MM
    assert(std::string(module) == "soh");
    const std::string prefix = "SOH";
#else
    assert(std::string(module) == "2ship");
    const std::string prefix = "MM";
#endif
    if (name == prefix + "_ExtractSharedState") return reinterpret_cast<void*>(ExtractPeerState);
    assert(name == prefix + "_ExtractSharedSoulState");
    return reinterpret_cast<void*>(ExtractPeerSoulState);
}
static void FleetSync_ApplySharedState(const char*) { ++peerApplies; }
struct FleetSharedReceiveGuard {
    FleetSharedReceiveGuard() { ++sReceiveDepth; }
    ~FleetSharedReceiveGuard() { --sReceiveDepth; }
};
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
#define SPDLOG_ERROR(...) ((void)0)
extern "C" NeiSaveData* Nei_Save() { return &live; }
extern "C" void FleetShared_RequestPullFromPeer() { ++pulls; sPullPending = true; }
static void PeerGrant(const char* name) { assert(name == nullptr); ++peerSignals; }
static auto ResolvePeerGrant() { return PeerGrant; }
static void RepairFlagOwnedCells(NeiSaveData*) { ++repairs; }
static void ExtEquip_ResyncFromSave() { ++refreshes; }
static unsigned char Cane_GiveSkill(unsigned char skill) { live.caneSkills |= 1u << skill; return 1; }
static void Wand_GrantMode(unsigned char mode) { live.wandRodsOwned |= 1u << mode; }
static void Slate_GrantRune(unsigned char rune) { live.slateRunesOwned |= 1u << rune; }
static void Seasons_GrantSeason(unsigned char season) { live.seasonsOwned |= 1u << season; }
static void Seasons_GrantRod() { live.seasonsRodOwned = 1; }
#ifdef TEST_MM
static void GrantMmItemByName(const char* name, void*) { assert(name && *name); ++legacyGrants; }
#define GrantSharedItem MM_GrantSharedItem
#else
static void GrantOotItemByName(const char* name, void*) { assert(name && *name); ++legacyGrants; }
#define GrantSharedItem SOH_GrantSharedItem
#define INV_CONTENT(item) gSaveContext.inventory.items[SLOT_NAYRUS_LOVE]
#endif
#ifndef COMBO_EXPORT
#define COMBO_EXPORT
#endif
#define ARRAY_COUNT(a) (static_cast<int>(sizeof(a) / sizeof((a)[0])))
#ifndef TEST_MM
using s32 = int32_t;
using u8 = uint8_t;
// Engine mode/flag services remain mutable; the actual trade and mask
// ownership/migration bodies are linked by the extraction runner.
static bool fixtureRandomizer{};
#define IS_RANDO fixtureRandomizer
static bool fixtureRandomizerFlags[RAND_INF_MAX]{};
static s32 Flags_GetRandomizerInf(RandomizerInf flag) {
    return fixtureRandomizerFlags[flag];
}
struct Player { uint16_t heldItemId; };
static u8 TwilightUpgrade_IsClawshotActive() { return 0; }
#endif
// This fixture owns Vanilla NEI storage and FleetShared routing. Real native
// soul flags/grants remain controlled services; the production public wrappers
// and complete SharedSoulRuntime perform category, slot and ownership routing.
struct SoulServiceAdapter {
    static ComboRando::GameId Game() {
#ifdef TEST_MM
        return ComboRando::GAME_MM;
#else
        return ComboRando::GAME_OOT;
#endif
    }
    static int Slot() { return gSaveContext.fileNum; }
    static uint32_t Mask() {
        return Combo_IsCombinedGame() && !Combo_IsVanilla() && Slot() >= 0 && Slot() <= 2 ? soulMask : 0;
    }
    static int SourceItem() {
#ifdef TEST_MM
        return RI_SOUL_ENEMY_OCTOROK;
#else
        return RG_SOUL_ENEMY_OCTOROK;
#endif
    }
    static const char* LocalName() {
        return Game() == ComboRando::GAME_OOT ? ComboRando::SharedSoulPairs[0].ootName
                                             : ComboRando::SharedSoulPairs[0].mmName;
    }
    static const char* PeerName() {
        return Game() == ComboRando::GAME_OOT ? ComboRando::SharedSoulPairs[0].mmName
                                             : ComboRando::SharedSoulPairs[0].ootName;
    }
    static std::string ItemName(int item) { return item == SourceItem() ? LocalName() : ""; }
    static std::string LegacyPeerName(int) { return {}; }
    static bool Owned(const std::string& name) { return nativeSoulFlags.contains(name); }
    static void GrantNative(const char* name) {
        assert(sReceiveDepth == 1 && name == std::string(LocalName()));
        ++nativeSoulGrants;
        nativeSoulFlags.insert(name);
    }
    static void GrantPeer(const char* name, int slot, uint32_t mask) {
        assert(name == std::string(PeerName()) && slot == Slot() && mask == ComboRando::SS_ENEMY);
        ++peerSoulGrants;
    }
};
using Runtime = ComboRando::SharedSoulRuntime<SoulServiceAdapter>;
#include "native_adapter.inc"

static void TestHashPolicy() {
    sNetFlat = {{"/vanillaNei/weaponUpgrades", 0}};
    const json mismatch = {{"h", std::to_string(NetHash(sNetFlat) + 1)}};
    sentPacket = {{"untouched", true}};
    for (int i = 0; i < 10; ++i) HandleHash(mismatch);
    assert(sNetMismatchStreak == 0 && sNetFutileResyncs == 0 && sNetResyncCooldownLeft == 0);
    assert(sentPacket == json({{"untouched", true}}));
    for (int mode : {COMBO_GAME_BOTH, COMBO_GAME_OOT, COMBO_GAME_MM}) {
        gComboGameMode = mode;
        gComboVanillaMode = mode == COMBO_GAME_BOTH ? 0 : 1;
        sNetMismatchStreak = sNetFutileResyncs = sNetResyncCooldownLeft = 0;
        HandleHash(mismatch);
        assert(sNetMismatchStreak == 1 && sNetFutileResyncs == 0);
        HandleHash(mismatch);
        assert(sNetMismatchStreak == 0 && sNetFutileResyncs == 1 && sNetResyncCooldownLeft == kNetResyncCooldown);
        assert(sentPacket == json({{"op", "resync"}}));
        HandleHash({{"h", std::to_string(NetHash(sNetFlat))}});
        assert(sNetMismatchStreak == 0 && sNetFutileResyncs == 0);
    }
    gComboGameMode = COMBO_GAME_BOTH;
    gComboVanillaMode = 1;
}

static void TestNativeAdapters(const json& incoming) {
    std::fill(std::begin(live.ownedItems), std::end(live.ownedItems), 0xFF);
    live.bottomlessContent = 77;
    live.bottomlessCount = 9;
    live.comboObtainedFc[0] = 8;
    live.capeHidden = 1;
    live.wandMode = 2;
    live.slateMode = 3;
    live.season = 4;
#ifdef TEST_MM
    live.activeCustomForm = 3;
#endif
#ifdef TEST_MM
    auto before = live;
    ApplyVanillaShared(incoming);
    assert(!std::memcmp(&before, &live, sizeof(live)) && repairs == 0 && refreshes == 0);
    gSaveContext.save.saveInfo.playerData.threeDayResetCount = 1;
#else
    gSaveContext.inventory.equipment = 7;
    gSaveContext.inventory.items[SLOT_HAMMER] = ITEM_HAMMER;
    // The Giant's Knife bit alone is not native Biggoron ownership.
    ApplyVanillaShared(incoming);
    assert((live.weaponUpgrades & 16) == 0);
    gSaveContext.bgsFlag = 1;
#endif
    const auto nativeBefore = gSaveContext;
    const auto neiBeforeWrongPair = live;
    auto wrongSlot = incoming; wrongSlot["vanillaNei"]["slot"] = 1;
    ApplyVanillaShared(wrongSlot);
    assert(!std::memcmp(&neiBeforeWrongPair, &live, sizeof(live)));
    ApplyVanillaShared(incoming);
    assert(!std::memcmp(&nativeBefore, &gSaveContext, sizeof(gSaveContext)));
    assert(live.caneSkills == 63 && live.wandRodsOwned == 63 && live.slateRunesOwned == 31);
    assert(live.bottomlessContent == 77 && live.bottomlessCount == 9 && live.comboObtainedFc[0] == 8);
    assert(live.capeHidden == 1 && live.wandMode == 2 && live.slateMode == 3 && live.season == 4);
#ifdef TEST_MM
    assert(live.clawshotOwned && live.weaponUpgrades == 0 && live.pendantOwned == 0);
    assert(live.ultrashotOwned == 1 && live.ritoMaskOwned == 1 && live.ootWeaponUpgradesOwned == 9);
    assert(live.ootHookshotLevel == 0 && live.ootHammerOwned == 0 && live.ootUpgrades == 0);
    assert(live.activeCustomForm == 3);
    assert(Nei_HookshotLevel() == 0 && Nei_HookshotVariant() != NEI_HOOK_VARIANT_ULTRASHOT);
    json persisted; to_json(persisted, live);
    assert(persisted["ultrashotOwned"] == 1 && persisted["ritoMaskOwned"] == 1 && persisted["ootWeaponUpgradesOwned"] == 9);
    NeiSaveData restored{};
    from_json(json::parse(persisted.dump()), restored);
    assert(restored.ultrashotOwned == 1 && restored.ritoMaskOwned == 1 && restored.ootWeaponUpgradesOwned == 9);
    assert(restored.ootHookshotLevel == 0 && restored.ootHammerOwned == 0 && restored.ootUpgrades == 0);
    persisted.erase("ultrashotOwned"); persisted.erase("ritoMaskOwned"); persisted.erase("ootWeaponUpgradesOwned");
    from_json(persisted, restored);
    assert(restored.ultrashotOwned == 0 && restored.ritoMaskOwned == 0 && restored.ootWeaponUpgradesOwned == 0);
    for (const auto& malformed : {json(-1), json(256), json("1"), json(true), json(1.0), json(nullptr)}) {
        auto invalid = persisted;
        invalid["ultrashotOwned"] = invalid["ritoMaskOwned"] = invalid["ootWeaponUpgradesOwned"] = malformed;
        from_json(invalid, restored);
        assert(restored.ultrashotOwned == 0 && restored.ritoMaskOwned == 0 && restored.ootWeaponUpgradesOwned == 0);
    }
    assert(live.ootSpellsOwned == 8);
    assert(live.shieldOwned == 0x18 && (live.extEquipOwnedBits & (1u << 21)) == 0);
    assert(live.ownedItems[7] == 0x224 && live.ownedItems[15] == 0x220 && live.ownedItems[23] == 0x223);
    assert(live.ownedItems[24] == 0xFF);
    MM_INV.items[FC_MM_MASK_SLOT_BASE] = kFcMmMaskItemBySlot[0];
    MM_INV.items[FC_MM_MASK_SLOT_BASE + 1] = kFcMmMaskItemBySlot[2]; // Wrong cell is not a second mask.
    MM_INV.items[SLOT_SWORD_GREAT_FAIRY] = ITEM_SWORD_GREAT_FAIRY;
    MM_INV.items[SLOT_TRADE_COUPLE] = ITEM_PENDANT_OF_MEMORIES;
    MM_EQ.equipment = 0x23;
    MM_EQ.buttonItems[0][0] = ITEM_SWORD_GILDED;
#else
    assert(live.weaponUpgrades == 31 && live.pendantOwned == 1);
    assert(live.ultrashotOwned == 1 && live.ritoMaskFlags == RITO_FLAG_MASK_OWNED);
    assert(Nei_ArmsHookVariant(nullptr) == 0);
    Player hookshot{ITEM_HOOKSHOT}, longshot{ITEM_LONGSHOT};
    assert(Nei_ArmsHookVariant(&hookshot) == 0 && Nei_ArmsHookVariant(&longshot) == 2);
    assert(live.separateRocsFlags == 1);
    assert(live.shieldOwned == 0x38 && (live.extEquipOwnedBits & (1u << 21)) != 0);
    assert(live.ownedItems[24] == 0xB8 && live.ownedItems[25] == 0xB9);
#endif
    const auto neiBeforeExtract = live;
    json snapshot = {{"vitals", {{"rupees", 999}}}, {"comboObtainedFc", {255}}};
    ExtractVanillaShared(snapshot);
    assert(snapshot.size() == 1 && snapshot.contains("vanillaNei"));
    assert(snapshot["vanillaNei"]["separateRocsFeather"] == true);
    assert(SharingFlag(snapshot["vanillaNei"]["ultrashotOwned"]) && snapshot["vanillaNei"]["ritoMaskOwned"] == true);
    assert(!std::memcmp(&neiBeforeExtract, &live, sizeof(live)));
    assert(snapshot["vanillaNei"]["ownedItems"][7] == 0x224);
#ifdef TEST_MM
    assert(snapshot["vanillaNei"]["mmMasks"][0] == 0xB8 && snapshot["vanillaNei"]["mmMasks"][1] == 0xFF);
    assert(snapshot["vanillaNei"]["mmSwordTier"] == 3 && snapshot["vanillaNei"]["mmGreatFairySword"] == true);
    assert(snapshot["vanillaNei"]["mmMirrorShield"] == true && snapshot["vanillaNei"]["pendantOwned"] == true);
    assert(snapshot["vanillaNei"]["weaponUpgrades"] == 9);
    // A real native receipt survives replacing its visible sword/shield with NEI.
    live.comboObtainedFc[FCI_KOKIRI_SWORD] = live.comboAppliedFc[FCI_KOKIRI_SWORD] = 3;
    live.extEquipSword = live.extEquipShield = 1;
    live.shieldOwned |= FC_SHIELD_IKANA;
    MM_EQ.equipment = 0;
    MM_EQ.buttonItems[0][0] = 0xD1; // NEI custom B item cannot prove native ownership.
    const auto nativeBeforeAlias = gSaveContext;
    ExtractVanillaShared(snapshot);
    assert(snapshot["vanillaNei"]["mmSwordTier"] == 3 && snapshot["vanillaNei"]["mmMirrorShield"] == true);
    assert(!std::memcmp(&nativeBeforeAlias, &gSaveContext, sizeof(gSaveContext)));
    for (uint8_t invalid : {uint8_t(4), uint8_t(255)}) {
        live.comboObtainedFc[FCI_KOKIRI_SWORD] = live.comboAppliedFc[FCI_KOKIRI_SWORD] = invalid;
        ExtractVanillaShared(snapshot);
        assert(snapshot["vanillaNei"]["mmSwordTier"] == 0);
    }
    live.comboObtainedFc[FCI_KOKIRI_SWORD] = 3; live.comboAppliedFc[FCI_KOKIRI_SWORD] = 2;
    ExtractVanillaShared(snapshot);
    assert(snapshot["vanillaNei"]["mmSwordTier"] == 0); // Unmaterialized generic deficits are no native receipt.
    live.ultrashotOwned = 0; live.ootHookshotLevel = 3;
    ExtractVanillaShared(snapshot);
    assert(snapshot["vanillaNei"]["ultrashotOwned"] == true && live.ultrashotOwned == 0 && live.ootHookshotLevel == 3);
    live.ultrashotOwned = 1; live.ootHookshotLevel = 0;
#else
    // Imported Rito ownership fills an empty corner mask wheel. An earned Farore
    // spell stays in its own cell and is remembered only for legacy migration.
    const json ritoOnly = {{"vanillaNei", {{"version", 1}, {"slot", 0}, {"ritoMaskOwned", true}}}};
    live.ritoMaskFlags = 0; gSaveContext.inventory.items[SLOT_FARORES_WIND] = ITEM_NONE;
    gSaveContext.inventory.items[SLOT_TRADE_CHILD] = ITEM_NONE;
    ApplyVanillaShared(ritoOnly);
    assert(live.ritoMaskFlags == RITO_FLAG_MASK_OWNED && gSaveContext.inventory.items[SLOT_TRADE_CHILD] == ITEM_RITO_MASK);
    assert(gSaveContext.inventory.items[SLOT_FARORES_WIND] == ITEM_NONE);
    live.ritoMaskFlags = 0; gSaveContext.inventory.items[SLOT_FARORES_WIND] = ITEM_FARORES_WIND;
    ApplyVanillaShared(ritoOnly);
    assert(live.ritoMaskFlags == (RITO_FLAG_MASK_OWNED | RITO_FLAG_FARORES_OWNED));
    assert(gSaveContext.inventory.items[SLOT_FARORES_WIND] == ITEM_FARORES_WIND);
    live.ritoMaskFlags = RITO_FLAG_FARORES_OWNED;
    ExtractVanillaShared(snapshot);
    assert(snapshot["vanillaNei"]["ritoMaskOwned"] == false);
#endif
    auto lower = incoming;
    lower["vanillaNei"]["ownedItems"][0] = 0x9E;
    ApplyVanillaShared(lower);
#ifdef TEST_MM
    assert(FcEquip_MmToOot16(live.ownedItems[0]) == 0x9F);
#else
    assert(live.ownedItems[0] == 0x9F);
#endif
    const auto beforeGuards = legacyCalls;
    ExtractShared(snapshot); ApplyShared(incoming); ApplyRegistryToNatives();
    ApplyFcRegistryToNatives(); ExtractEquips(snapshot); ApplyEquips(incoming);
    assert(legacyCalls == beforeGuards);
    GrantSharedItem(nullptr);
    FleetShared_OnNativeObtained(7);
    assert(legacyGrants == 0 && legacyFanouts == 0 && pulls == 2 && peerSignals == 1);
    sReceiveDepth = 1;
    FleetShared_OnNativeObtained(7);
    assert(pulls == 2 && peerSignals == 1);
    sReceiveDepth = 0;
    for (int mode : {COMBO_GAME_BOTH, COMBO_GAME_OOT, COMBO_GAME_MM}) {
        gComboGameMode = mode;
        gComboVanillaMode = mode == COMBO_GAME_BOTH ? 0 : 1;
        ExtractShared(snapshot); ApplyShared(incoming); ApplyRegistryToNatives();
        ApplyFcRegistryToNatives(); ExtractEquips(snapshot); ApplyEquips(incoming);
        GrantSharedItem("native-seeded-item");
        FleetShared_OnNativeObtained(7);
    }
    assert(legacyCalls == beforeGuards + 18 && legacyGrants == 3 && legacyFanouts == 1);
    gComboGameMode = COMBO_GAME_BOTH;
    gComboVanillaMode = 1;
#ifdef TEST_MM
    gSaveContext.save.saveInfo.playerData.threeDayResetCount = 0;
    PullFromPeerWhenReady();
    assert(sPullPending && peerExtracts == 0);
    gSaveContext.save.saveInfo.playerData.threeDayResetCount = 1;
#endif
    PullFromPeerWhenReady();
    assert(!sPullPending && peerExtracts == 1 && peerApplies == 1 && sReceiveDepth == 0);
    PullFromPeerWhenReady();
    assert(peerExtracts == 1);
    const json scalar = {{"op", "delta"}, {"d", {{"/vanillaNei/caneSkills", 4}}}};
    NetSend(scalar);
    assert(sentPacket["vanillaNei"]["slot"] == 0 && sentPacket["vanillaNei"]["version"] == 1);
    auto partial = sentPacket["d"].unflatten();
    assert(ComboVanilla::PrepareSharingDelta(partial, sentPacket, 0));
    assert(ComboVanilla::FilterSharing(partial, ComboVanilla::Receiver{.world=ComboVanilla::World::Oot, .slot=0})["caneSkills"] == 4);
    const json array = {{"op", "delta"}, {"a", {{"/vanillaNei/ownedItems", incoming["vanillaNei"]["ownedItems"]}}}};
    NetSend(array);
    assert(sentPacket["vanillaNei"]["slot"] == 0 && sentPacket.contains("a"));
    gSaveContext.fileNum = 1;
    NetSend(scalar);
    assert(!ComboVanilla::PrepareSharingDelta(partial, sentPacket, 0));
    gSaveContext.fileNum = 0;
    gComboVanillaMode = 0;
    NetSend(scalar);
    assert(sentPacket == scalar); // seeded transport retains its exact payload
    gComboVanillaMode = 1;
#ifndef TEST_MM
    // Native Biggoron completion happens outside the randomizer give hook.
    gSaveContext.bgsFlag = 0;
    PullFromPeerWhenReady();
    assert(peerExtracts == 2 && peerApplies == 2);
    gSaveContext.nativeVitalsAndTime[0] = 999;
    PullFromPeerWhenReady();
    assert(peerExtracts == 2);
#endif
}

static void TestSharedSoulRouting() {
    gComboGameMode = COMBO_GAME_BOTH;
    gComboVanillaMode = 0;
    gSaveContext.fileNum = 0;
    soulMask = ComboRando::SS_ENEMY;
    const auto ordinary = legacyFanouts;
    const auto peerBefore = peerSoulGrants;
    FleetShared_OnNativeObtained(7); // Ordinary receipt retains the legacy path.
    assert(legacyFanouts == ordinary + 1 && peerSoulGrants == peerBefore);
    FleetShared_OnNativeObtained(SoulServiceAdapter::SourceItem());
    assert(legacyFanouts == ordinary + 1 && peerSoulGrants == peerBefore + 1);
    assert(nativeSoulGrants == 0); // Native source ownership is not granted recursively.
    soulMask = 0;
    FleetShared_OnNativeObtained(SoulServiceAdapter::SourceItem());
    assert(legacyFanouts == ordinary + 2 && peerSoulGrants == peerBefore + 1);
    soulMask = ComboRando::SS_ENEMY;
    sReceiveDepth = 1;
    FleetShared_OnNativeObtained(SoulServiceAdapter::SourceItem());
    sReceiveDepth = 0;
    for (int mode : {COMBO_GAME_OOT, COMBO_GAME_MM}) {
        gComboGameMode = mode;
        FleetShared_OnNativeObtained(SoulServiceAdapter::SourceItem());
    }
    gComboGameMode = COMBO_GAME_BOTH;
    gComboVanillaMode = 1;
    FleetShared_OnNativeObtained(SoulServiceAdapter::SourceItem());
    assert(legacyFanouts == ordinary + 2 && peerSoulGrants == peerBefore + 1);

    gComboVanillaMode = 0;
    const auto nativeBefore = gSaveContext;
    const auto neiBefore = live;
    peerSoulState = {{"slot", 0}, {"mask", uint32_t(ComboRando::SS_ENEMY)},
                     {"owned", json::array({SoulServiceAdapter::PeerName()})}};
    const auto extracts = peerSoulExtracts;
    sPullPending = true;
    PullFromPeerWhenReady();
    assert(peerSoulExtracts == extracts + 1 && nativeSoulGrants == 1);
    assert(nativeSoulFlags == std::set<std::string>{SoulServiceAdapter::LocalName()});
    assert(!sPullPending && sReceiveDepth == 0);
    sPullPending = true;
    PullFromPeerWhenReady();
    assert(nativeSoulGrants == 1); // Exact native ownership makes reconciliation idempotent.
    nativeSoulFlags.clear();
    peerSoulState["slot"] = 1;
    sPullPending = true;
    PullFromPeerWhenReady();
    assert(nativeSoulFlags.empty() && nativeSoulGrants == 1);
    peerSoulState["slot"] = 0;
    peerSoulState["mask"] = uint32_t(ComboRando::SS_NPC);
    sPullPending = true;
    PullFromPeerWhenReady();
    assert(nativeSoulFlags.empty() && nativeSoulGrants == 1);
    peerSoulState["mask"] = uint32_t(ComboRando::SS_ENEMY);
    soulMask = 0;
    sPullPending = true;
    PullFromPeerWhenReady();
    assert(nativeSoulFlags.empty() && nativeSoulGrants == 1);
    soulMask = ComboRando::SS_ENEMY;
    gComboVanillaMode = 1;
    sPullPending = true;
    PullFromPeerWhenReady();
    assert(nativeSoulFlags.empty() && nativeSoulGrants == 1 && sReceiveDepth == 0);
    assert(!std::memcmp(&nativeBefore, &gSaveContext, sizeof(gSaveContext)));
    assert(!std::memcmp(&neiBefore, &live, sizeof(live)));
    std::cout << "PASS real shared-soul wrappers/runtime: ordinary fallthrough, handled receipt, slot/category, Vanilla/solo/receive gates and idempotent native ownership\n";
}

int main() {
    Receiver oot;
    oot.world = World::Oot;
    oot.slot = 0;
    Receiver mm;
    mm.world = World::Mm;
    mm.slot = 0;
    mm.firstCycleComplete = true;
    json incoming = {{"vitals", {{"healthCapacity", 320}, {"rupees", 999}}},
                     {"upgrades", {{"wallet", 3}}}, {"form", 0}, {"adult", true},
                     {"comboObtainedFc", json::array({255,255,255})},
                     {"vanillaNei", {{"version", 1}, {"slot", 0}, {"shovelOwned", 1}, {"dominionOwned", 1},
                         {"pokeballOwned", 1}, {"clawshotOwned", 1}, {"capeOwned", 1},
                         {"bombArrowsOwned", 1}, {"timeGateOwned", 1}, {"magicMirrorOwned", 1},
                         {"hyliasGraceOwned", 1}, {"phantomHourglassOwned", 1}, {"netEquipped", 1},
                         {"bottomlessBottleMode", 1}, {"twilightUpgrade", 7}, {"quartzOwned", 1},
                         {"cojiroPartnerOwned", 1}, {"ultrashotOwned", 1}, {"ritoMaskOwned", true}, {"separateRocsFeather", 1}, {"caneSkills", 0x3F}, {"wandRodsOwned", 0x3F},
                         {"slateRunesOwned", 0x1F}, {"seasonsOwned", 15}, {"seasonsRodOwned", 1},
                         {"extEquipOwnedBits", 0x0FFF0000}, {"weaponUpgrades", 31},
                         {"shieldOwned", 0xFFFF}, {"pendantOwned", true},
                         {"vitals", {{"magic", 96}}}, {"rpgStats", {{"magic", 15}}},
                         {"nativeSongs", 0xFFFFFFFF}, {"ootHookshotLevel", 3}, {"ootHammerOwned", 1},
                         {"ootUpgrades", 65535}, {"ritoMaskFlags", 3}, {"bottleSlots", json::array({42,42})},
                         {"form", 0}, {"adult", true}, {"bottomlessContent", 42},
                         {"ownedItems", json::array({0x9F,0xA9,0xAA,0xD0,0xAF,0xA3,0xA4,0x224,
                             0xB2,0xA5,0xB0,0xA2,0xA6,0xA7,0xA8,0x220,0xB1,0x221,0xB5,0xB4,0x222,0xAB,0xB3,0x223})},
                         {"mmMasks", json::array({0xB8,0xB9,0xBA})},
                         {"mmSwordTier", 3}, {"mmGreatFairySword", true}, {"mmMirrorShield", true}}}};
    auto wrongPair = incoming; wrongPair["vanillaNei"]["slot"] = 1;
    assert(FilterSharing(wrongPair, oot).empty());
    auto filtered = FilterSharing(incoming, oot);
    assert(!filtered.contains("vitals") && !filtered.contains("upgrades") && !filtered.contains("rpgStats"));
    assert(!filtered.contains("nativeSongs") && !filtered.contains("form") && !filtered.contains("adult"));
    assert(!filtered.contains("bottleSlots") && !filtered.contains("bottomlessContent"));
    assert(!filtered.contains("comboObtainedFc") && filtered["weaponUpgrades"] == 0);
    assert(!filtered.contains("ootHookshotLevel") && !filtered.contains("ootHammerOwned") &&
           !filtered.contains("ootUpgrades") && !filtered.contains("ritoMaskFlags"));
    assert(filtered["shieldOwned"] == 0x38 && filtered["pendantOwned"] == 1);
    assert(filtered["ultrashotOwned"] == 1 && filtered["ritoMaskOwned"] == 1);
    assert(filtered["ownedItems"][7] == 0x224 && filtered["ownedItems"][15] == 0x220);
    assert(filtered["ownedItems"][17] == 0x221 && filtered["ownedItems"][20] == 0x222);
    assert(filtered["ownedItems"][23] == 0x223);
    oot.kokiriSword = true;
    assert(FilterSharing(incoming, oot)["weaponUpgrades"] == 6);
    oot.masterSword = oot.hammer = oot.biggoronSword = true;
    assert(FilterSharing(incoming, oot)["weaponUpgrades"] == 31);
    auto forMm = FilterSharing(incoming, mm);
    assert(!forMm.contains("pendantOwned") && !forMm.contains("mmMasks"));
    assert(forMm["shieldOwned"] == 0x18 && forMm["extEquipOwnedBits"] == (0x0FFF0000 & ~(1 << 21)));
    assert(forMm["weaponUpgrades"] == 0 && forMm["ootWeaponUpgradesOwned"] == 9);
    mm.firstCycleComplete = false;
    assert(FilterSharing(incoming, mm).empty());
    assert(FilterSharing(json{{"vitals", {{"health", 320}}}}, oot).empty());
    auto malformed = incoming;
    malformed["vanillaNei"]["version"] = 2;
    assert(FilterSharing(malformed, oot).empty());
    malformed = incoming;
    malformed["vanillaNei"]["caneSkills"] = -1;
    malformed["vanillaNei"]["wandRodsOwned"] = "all";
    malformed["vanillaNei"]["ownedItems"][7] = 0x10224;
    malformed["vanillaNei"]["mmMasks"][0] = 0xCF; // Wrong mask cannot move to another cell.
    auto safe = FilterSharing(malformed, oot);
    assert(!safe.contains("caneSkills") && !safe.contains("wandRodsOwned"));
    assert(safe["ownedItems"][7] == 0xFF && safe["mmMasks"][0] == 0xFF);

    // This is the real per-game NeiSaveData, not a hand-built substitute. Sharing
    // changes ownership only; runtime settings, active selection and registries stay local.
    NeiSaveData native{};
    native.bottomlessContent = 42;
    native.bottomlessCount = 3;
    native.season = 3;
    native.caneSkillSel[0] = 2;
    native.comboObtainedFc[0] = 7;
    std::array<unsigned, 5> grants{};
    auto grant = [&](SharedGrant kind, unsigned index) {
        assert(index < 6);
        ++grants[static_cast<unsigned>(kind)];
    };
    ApplySharingOwnership(native, filtered, grant);
    assert(native.shovelOwned && native.netEquipped && native.bottomlessBottleMode);
    assert(native.twilightUpgrade == 7 && native.ultrashotOwned == 1);
    assert(native.caneSkills == 0x3F && native.wandRodsOwned == 0x3F && native.slateRunesOwned == 0x1F);
    assert(native.seasonsOwned == 15 && native.seasonsRodOwned == 1);
    assert(native.bottomlessContent == 42 && native.bottomlessCount == 3 && native.season == 3);
    assert(native.caneSkillSel[0] == 2 && native.comboObtainedFc[0] == 7);
    assert((grants == std::array<unsigned,5>{6,6,5,4,1}));
    ApplySharingOwnership(native, filtered, grant);
    assert((grants == std::array<unsigned,5>{6,6,5,4,1})); // Reconciliation is idempotent.
    // Four native cane wheel entries share only two historical selection bytes.
    // An incoming ownership union may never index that array by type 2 or 3,
    // and existing Block-only / Stone-only choices remain valid in Vanilla.
    for (unsigned type = 0; type < 4; ++type) {
        NeiSaveData choice{};
        const uint8_t onlySkill[] = {1, 2, 4, 5};
        choice.caneSkills = 1u << onlySkill[type];
        choice.caneType = type;
        choice.caneSkillSel[0] = 2;
        choice.caneSkillSel[1] = type == 2 ? 1 : 231; // Stone selection for Pacci; unused local byte otherwise
        auto changingGrant = [&](SharedGrant, unsigned) {
            choice.caneType = 0;
            choice.caneSkillSel[0] = choice.caneSkillSel[1] = 0;
        };
        ApplySharingOwnership(choice, filtered, changingGrant);
        assert(choice.caneType == type && choice.caneSkillSel[0] == 2 &&
               choice.caneSkillSel[1] == (type == 2 ? 1 : 231));
    }
    {
        NeiSaveData invalidChoice{};
        invalidChoice.caneSkills = 1;
        invalidChoice.caneType = 3; // not owned before the incoming union
        ApplySharingOwnership(invalidChoice, filtered, [&](SharedGrant, unsigned) { invalidChoice.caneType = 0; });
        assert(invalidChoice.caneType == 0); // union cannot make an invalid old preference authoritative
    }
    const auto snapshot = SharingSnapshot(native, 0);
    assert(snapshot["vanillaNei"]["caneSkills"] == 0x3F);
    assert(!snapshot["vanillaNei"].contains("comboObtainedFc"));
    for (const auto& badVersion : {json(1.0), json("1"), json(nullptr), json(true)}) {
        auto invalid = incoming;
        invalid["vanillaNei"]["version"] = badVersion;
        assert(FilterSharing(invalid, oot).empty());
    }
    auto delta = json{{"vanillaNei", {{"caneSkills", 4}}}};
    auto packet = json{{"op", "delta"}};
    StampSharingDelta(packet, 0);
    assert(PrepareSharingDelta(delta, packet, 0));
    assert(FilterSharing(delta, oot)["caneSkills"] == 4);
    assert(!PrepareSharingDelta(delta, packet, 1));
    auto conflict = delta; conflict["vanillaNei"]["slot"] = 1;
    assert(!PrepareSharingDelta(conflict, packet, 0));
    assert(!PrepareSharingDelta(delta, json{{"op", "delta"}}, 0));
    delta = {{"vitals", {{"rupees", 999}}}};
    assert(!PrepareSharingDelta(delta, packet, 0));
    assert(FilterSharing(delta, oot).empty());
    TestHashPolicy();
    TestNativeAdapters(incoming);
    TestSharedSoulRouting();
#ifndef TEST_MM
    // A seeded migration after the Vanilla cases keeps the mode service live
    // and verifies that shared Rito ownership cannot retain a child trade cell.
    live = {};
    live.ritoMaskFlags = RITO_FLAG_MASK_OWNED;
    std::fill(std::begin(gSaveContext.inventory.items), std::end(gSaveContext.inventory.items), ITEM_NONE);
    gSaveContext.inventory.items[SLOT_TRADE_CHILD] = ITEM_WEIRD_EGG;
    fixtureRandomizerFlags[RAND_INF_CHILD_TRADES_HAS_CHICKEN] = true;
    fixtureRandomizer = true;
    RitoMask_MigrateLegacy();
    assert(gSaveContext.inventory.items[SLOT_TRADE_ADULT] == ITEM_WEIRD_EGG);
    assert(gSaveContext.inventory.items[SLOT_TRADE_CHILD] == ITEM_RITO_MASK);
    assert(live.tradeAdultOwned == ((1u << 20) | (1u << 21)));
#endif
    std::cout << "PASS NEI-only ownership, native progression isolation, first-cycle quarantine, exact aliases, sword prerequisites, full-width cells and idempotent grants\n";
}
