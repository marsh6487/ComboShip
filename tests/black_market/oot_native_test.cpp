#include "global.h"
#include "ComboGameMode.h"
#include "entrances/LostWoodsEntrance.h"
#include "entrances/BlackMarketEntrance.h"
#include "entrances/SoloForest.h"
#include "rando/BlackMarket.h"
#include "BlackMarketKeeper.h"
#include "soh/ActorDB.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <memory>
extern "C" { int gComboGameMode = COMBO_GAME_OOT; }
SaveContext gSaveContext{};
PlayState* gPlayState;
using namespace ComboRando::BlackMarket;
Session sSession;
nlohmann::json sPendingSave;
bool sFileLoadPending = true;
u16 sClears;
bool sEntering, sReturnPending, sReturnStaged, sResumeFromMarket;
bool sDoorArmed, sTalking, sTitlePending, sKeeperSpawned;
int sKeeperActorId = 1000;
alignas(2) constexpr char kCard[] = "__OTR__custom/skiejer_market/title";
int sTravelSlot = -1, sSelected;
float sDoorFloor;
u32 sTravelSeed;
Vec3f sReturnPos = { 184, 0, -300 };
s16 sReturnYaw = -16384;
u16 sReturnEntrance = ENTR_SKIEJER_FOREST;
Vec3f sGroundedReturnPos = { 184, 0, -300 };
s16 sGroundedReturnYaw = 0;
u16 sGroundedReturnEntrance = ENTR_SKIEJER_FOREST;
RespawnData sPreservedReturn;
std::string sStatus;
bool pack = true, owned = false, usable = true, grantWorks = true;
int grants = 0, writes = 0;
u16 clearsAtWait;
Vec3f returnPosAtWait;
nlohmann::json persisted;
static PlayState play;
static Player player;
EntranceInfo gEntranceTable[ENTR_MAX];
int keeperSpawns, nativeInits, customInits;
ActorDBInit lastProfile;
int profileRegistrations;
ActorDB::ActorDB() {}
ActorDB::Entry::Entry() {}
ActorDB actorDatabase;
ActorDB* ActorDB::Instance = &actorDatabase;
ActorDB::Entry& ActorDB::AddEntry(const ActorDBInit& profile) {
    ++profileRegistrations;
    lastProfile = profile;
    static Entry entry;
    entry.entry.id = 1000;
    return entry;
}
bool inCs = false;
extern "C" s32 Player_InCsMode(PlayState*) { return inCs; }
extern "C" Actor* Actor_Spawn(ActorContext*, PlayState*, s16 actor, f32 x, f32 y, f32 z, s16, s16 yaw, s16, s16 marker) {
    assert(actor > ACTOR_ID_MAX); // dedicated profile, independent of vanilla actor controllers
    assert(actor == sKeeperActorId && marker == 0); ++keeperSpawns;
    assert(x == BlackMarketKeeper::kX && y == BlackMarketKeeper::kY && z == BlackMarketKeeper::kZ &&
           yaw == BlackMarketKeeper::kYaw);
    return &player.actor;
}
CollisionPoly flatFloor;
extern "C" f32 BgCheck_EntityRaycastFloor5(PlayState*, CollisionContext*, CollisionPoly** poly, s32* bg, Actor*, Vec3f*) {
    *poly = &flatFloor; *bg = BGCHECK_SCENE; return 0;
}
void KeeperInit(Actor*, PlayState*) { ++customInits; }
void KeeperUpdate(Actor*, PlayState*) {}
void KeeperDraw(Actor*, PlayState*) {}
void VanillaInit(Actor*, PlayState*) { ++nativeInits; }
const char* SOH_ReadBlackMarketOptions() { return "{}"; }
u32 Seed() { return 77; }
bool PackAvailable() { return pack; }
namespace ComboBlackMarketOotItems {
RandomizerGet Id(const Item&) { return RG_LANTERN; }
bool Owned(const Item&) { return owned; }
bool Available(const Item&) { return usable; }
bool Grant(const Item&) { if (!grantWorks) return false; owned = true; ++grants; return true; }
}
namespace Rando::StaticData {
struct NativeRow {
    std::shared_ptr<GetItemEntry> entry;
    std::shared_ptr<GetItemEntry> GetGIEntryUnresolved() const { return entry; }
};
NativeRow RetrieveItem(RandomizerGet id) {
    auto entry = std::make_shared<GetItemEntry>(); entry->getItemId = id; return { entry };
}
}
int shopDraws, fallbackDraws;
bool shopDrawAvailable = true;
extern "C" bool NeiGi_DrawShop(PlayState*, GetItemEntry* entry) {
    assert(entry && entry->getItemId == RG_LANTERN); ++shopDraws; return shopDrawAvailable;
}
extern "C" void GetItemEntry_Draw(PlayState*, GetItemEntry) { ++fallbackDraws; }
struct SaveManager {
    static SaveManager* Instance;
    void ThreadPoolWait() { clearsAtWait = sClears; returnPosAtWait = sReturnPos; }
    void SaveData(const std::string&, const nlohmann::json& value) { persisted = value; }
    void SaveSection(int, int, bool) {
        ++writes;
        persisted = { { "market", sSession.Save(gSaveContext.fileNum) }, { "rupees", gSaveContext.rupees } };
    }
};
SaveManager saveManager;
SaveManager* SaveManager::Instance = &saveManager;
/* PRODUCTION */

void Reset() {
    gComboGameMode = COMBO_GAME_OOT;
    gSaveContext = {};
    play = {}; player = {}; gPlayState = &play; play.state.running = true;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    gSaveContext.fileNum = 0;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    Options options; options.games = Games::Oot;
    sSession.Bind(Generate(77, options), 0); sSession.state = {};
    sClears = 3;
    sEntering = sReturnPending = sReturnStaged = sResumeFromMarket = false;
    sDoorArmed = sTalking = sTitlePending = sKeeperSpawned = false;
    sTravelSlot = -1; sTravelSeed = 0; sSelected = 0;
    sReturnEntrance = ENTR_SKIEJER_FOREST; sReturnPos = { 184, 0, -300 }; sReturnYaw = -16384;
    sGroundedReturnPos = sReturnPos; sGroundedReturnYaw = sReturnYaw; sGroundedReturnEntrance = sReturnEntrance;
    owned = false; usable = grantWorks = pack = true; grants = writes = 0;
    keeperSpawns = nativeInits = customInits = 0; inCs = false;
    sKeeperActorId = 1000;
    flatFloor = {}; flatFloor.normal.y = 32767;
    player.actor.floorPoly = &flatFloor; player.actor.floorHeight = 0;
    gEntranceTable[ENTR_SKIEJER_FOREST].scene = SCENE_SKIEJER_FOREST;
    sPendingSave = nullptr;
    sFileLoadPending = true;
    gSaveContext.rupees = 500;
    clearsAtWait = 0;
    shopDraws = fallbackDraws = 0; shopDrawAvailable = true;
}
int main() {
    Reset(); DrawOffer(&play, *Find("nei.lantern"));
    assert(shopDraws == 1 && fallbackDraws == 0);
    shopDrawAvailable = false; DrawOffer(&play, *Find("nei.lantern"));
    assert(shopDraws == 2 && fallbackDraws == 1);
    Reset();
    gSaveContext.entranceIndex = ENTR_SKIEJER_MARKET;
    const auto top = gSaveContext.respawn[RESPAWN_MODE_TOP];
    gSaveContext.respawn[RESPAWN_MODE_RETURN].pos = { 44, 55, 66 };
    const auto local = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    ComboBlackMarketOot_NormalizeEntry();
    assert(gSaveContext.entranceIndex == ENTR_SKIEJER_FOREST);
    assert(sReturnPending && sReturnStaged && gSaveContext.respawnFlag == 2);
    assert(!std::memcmp(&top, &gSaveContext.respawn[RESPAWN_MODE_TOP], sizeof(top)));
    assert(!std::memcmp(&local, &sPreservedReturn, sizeof(local)));
    Reset(); sEntering = true; RememberTravel();
    gSaveContext.entranceIndex = ENTR_SKIEJER_MARKET;
    ComboBlackMarketOot_NormalizeEntry();
    assert(gSaveContext.entranceIndex == ENTR_SKIEJER_MARKET && !sReturnPending);
    gSaveContext.respawnFlag = 1;
    ComboBlackMarketOot_NormalizeEntry();
    assert(gSaveContext.entranceIndex == ENTR_SKIEJER_FOREST);
    Reset(); gComboGameMode = COMBO_GAME_BOTH;
    gSaveContext.entranceIndex = ENTR_SKIEJER_MARKET;
    ComboBlackMarketOot_NormalizeEntry();
    assert(gSaveContext.entranceIndex == ENTR_SKIEJER_MARKET && !sReturnPending);
    Reset(); play.sceneNum = SCENE_LOST_WOODS;
    assert(!Exterior(&play)); // native LW now approaches the forest, never the shop directly
    Reset();
    const int price = sSession.market["stock"][0]["price"];
    Purchase();
    assert(grants == 1 && writes == 1 && persisted["rupees"] == 500 - price);
    assert(persisted["market"]["state"]["purchased"] == 1);
    Purchase(); assert(grants == 1 && writes == 1);
    Reset(); grantWorks = false; Purchase();
    assert(grants == 0 && writes == 0 && gSaveContext.rupees == 500 && !sSession.state.purchased);
    Reset(); gSaveContext.rupeeAccumulator = -450; Purchase();
    if (price > 50) assert(grants == 0 && writes == 0);
    Reset(); play.sceneNum = SCENE_SKIEJER_FOREST;
    player.actor.world.pos = { 184, 500, -300 }; PlayerUpdate();
    assert(sReturnPos.y == 0 && sReturnPos.x == 184);
    Reset(); play.sceneNum = SCENE_SKIEJER_FOREST; play.roomCtx.curRoom.num = 0;
    gSaveContext.entranceIndex = ENTR_SKIEJER_FOREST;
    player.actor.world.pos = { 184, 0, -300 }; PlayerUpdate();
    assert(sDoorArmed);
    assert(sReturnPos.x == 184); // save-worker-facing return point stays frozen while walking
    player.actor.world.pos = { 410, 80, -300 }; PlayerUpdate(); // airborne entry keeps last grounded candidate
    assert(sEntering && play.nextEntranceIndex == ENTR_SKIEJER_MARKET);
    assert(returnPosAtWait.x == 184 && sReturnPos.x == 184 && sReturnPos.y == 0);
    assert(gComboGameMode == COMBO_GAME_OOT && play.transitionTrigger == TRANS_TRIGGER_START);
    play.transitionTrigger = TRANS_TRIGGER_OFF; play.sceneNum = SCENE_SKIEJER_MARKET;
    player.actor.world.pos = { -11, 0, 381 }; PlayerUpdate(); PlayerUpdate();
    assert(keeperSpawns == 1);
    sKeeperActorId = -1; profileRegistrations = 0;
    RegisterKeeperActor(); RegisterKeeperActor();
    assert(profileRegistrations == 1 && sKeeperActorId == 1000);
    assert(lastProfile.category == ACTORCAT_NPC && lastProfile.objectId == OBJECT_GAMEPLAY_KEEP);
    assert(lastProfile.instanceSize == sizeof(Actor) && lastProfile.destroy == nullptr);
    assert(lastProfile.init == KeeperInit && lastProfile.update == KeeperUpdate && lastProfile.draw == KeeperDraw);
    lastProfile.init(&player.actor, &play); assert(customInits == 1 && nativeInits == 0);
    PresentTitle();
    const auto& title = play.actorCtx.titleCtx;
    assert(title.texture == kCard && title.width == 192 && title.height == 24);
    assert(!title.isBossCard && !title.hasTranslation && title.durationTimer == 80 && title.delayTimer == 20);
    player.actor.world.pos = { -11, 0, 410 }; PlayerUpdate();
    assert(sReturnPending && play.nextEntranceIndex == ENTR_SKIEJER_FOREST);
    gSaveContext.entranceIndex = play.nextEntranceIndex; ComboBlackMarketOot_NormalizeEntry();
    play.sceneNum = SCENE_SKIEJER_FOREST; play.transitionTrigger = TRANS_TRIGGER_OFF; PlayerUpdate();
    assert(!sReturnStaged && !sReturnPending && player.actor.world.pos.x == 184);
    Reset();
    sPendingSave = { { "market", sSession.market }, { "state", { { "purchased", 0x5A } } },
        { "clears", 5 }, { "savedInside", true } };
    BindSolo(0);
    assert(sSession.Active(0) && sSession.state.purchased == 0x5A && sClears == 5);
    assert(sResumeFromMarket && !sReturnStaged);
    // A later unordered native randomizer hook changes the savewarp entrance.
    gSaveContext.entranceIndex = ENTR_TEMPLE_OF_TIME_WARP_PAD;
    gSaveContext.respawn[RESPAWN_MODE_RETURN].pos = { 44, 55, 66 };
    const auto loadedReturn = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    ComboBlackMarketOot_NormalizeEntry();
    assert(sReturnPending && sReturnStaged && gSaveContext.entranceIndex == ENTR_SKIEJER_FOREST);
    assert(!std::memcmp(&sPreservedReturn, &loadedReturn, sizeof(loadedReturn)));
    SaveMarket(gSaveContext, 0, true);
    assert(persisted["state"]["purchased"] == 0x5A && persisted["clears"] == 5);
    gSaveContext.entranceIndex = ENTR_SKIEJER_MARKET_3; gPlayState = nullptr;
    SaveMarket(gSaveContext, 0, true);
    assert(persisted["savedInside"] == true); // native snapshot only, no live scene access on worker
    gPlayState = &play;
    sPendingSave["market"] = Generate(77, OptionsFromJson({ { "games", "mm" } }));
    sFileLoadPending = true;
    BindSolo(0); assert(!sSession.Active(0));
    Reset(); BindSolo(0); assert(sSession.Active(0) && sSession.market["games"] == "oot");
    assert(writes == 1);
    CommitPurchase(sSession.state, 4);
    BindSolo(0); // native age/warp notifications are not another file load
    assert(sSession.state.purchased == 16 && writes == 1);
    for (int gate = 0; gate < 6; ++gate) {
        Reset(); play.sceneNum = SCENE_SKIEJER_FOREST; play.roomCtx.curRoom.num = 0;
        player.actor.world.pos = { 410, 0, -300 }; sDoorArmed = true;
        if (gate == 0) pack = false;
        if (gate == 1) sSession.Bind(nullptr, -1);
        if (gate == 2) play.pauseCtx.state = 1;
        if (gate == 3) inCs = true;
        if (gate == 4) gComboGameMode = COMBO_GAME_BOTH;
        if (gate == 5) play.roomCtx.curRoom.num = 1;
        PlayerUpdate(); assert(!sEntering && play.transitionTrigger == TRANS_TRIGGER_OFF);
    }
    Reset(); ComboBlackMarketOot_ReportClear(3, 0);
    assert(clearsAtWait == 3 && sClears == 11 && writes == 1);
    ComboBlackMarketOot_ReportClear(3, 0); assert(writes == 1);
    ComboBlackMarketOot_ReportClear(4, 1); assert(writes == 1 && sClears == 11);
    gComboGameMode = COMBO_GAME_BOTH;
    ComboBlackMarketOot_ReportClear(4, 0); assert(writes == 1 && sClears == 11);
    std::puts("PASS OoT native transactions, dedicated entry, solo isolation and local RETURN/TOP recovery");
}
