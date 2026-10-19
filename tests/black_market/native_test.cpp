#include "global.h"
#include "ComboGameMode.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "entrances/BlackMarketEntrance.h"
#include "entrances/SoloForest.h"
#include "rando/BlackMarket.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
SaveContext gSaveContext{};
PlayState* gPlayState;
GameState* gGameState;
using namespace ComboRando::BlackMarket;
Session sSession;
uint16_t sClears;
bool sEntering, sReturnPending, sDoorArmed, sTitlePending, sTalking;
bool sResumeFromMarket;
int sSelected;
float sDoorFloor;
int sTravelSlot = -1;
u32 sTravelSeed;
Vec3f sSoloReturn = { 0, 0, 0 };
s16 sSoloReturnYaw = -8192;
u16 sSoloReturnEntrance = ENTRANCE(WOODS_OF_MYSTERY, 0);
RespawnData sPreservedReturn;
bool sReturnStaged = false;
std::array<Actor, 8> sDisplays;
std::string sStatus;
bool pack = true, owned = false, usable = true, grantWorks = true, inCs = false;
int grants, writes;
nlohmann::json persisted;
nlohmann::json nativeSavedDisk;
int CVarGetInteger(const char*, int fallback) { return fallback; }
std::string SaveManager_GetFileName(int slot) { return "file" + std::to_string(slot) + ".json"; }
int SaveManager_ReadSaveFile(const std::filesystem::path&, nlohmann::json& saved) { saved = nativeSavedDisk; return 0; }
void SaveManager_WriteSaveFile(const std::filesystem::path&, nlohmann::json saved) { nativeSavedDisk = std::move(saved); }
static PlayState play;
static Player player;
bool PackAvailable() { return pack; }
bool Interior();
bool Active() { return Interior() && sSession.Active(gSaveContext.fileNum); }
extern "C" bool Player_InCsMode(PlayState*) { return inCs; }
extern "C" void Player_Action_35(Player*, PlayState*) {}
CollisionPoly flatFloor;
bool floorExists = true;
extern "C" f32 BgCheck_EntityRaycastFloor5_3(PlayState*, CollisionContext*, CollisionPoly** poly,
    s32* bg, Actor*, Vec3f*) {
    *poly = floorExists ? &flatFloor : nullptr; *bg = BGCHECK_SCENE;
    return floorExists ? 0 : BGCHECK_Y_MIN;
}
extern "C" void Camera_InitFocalActorSettings(Camera*, Actor*) {}
namespace ComboBlackMarketItems {
bool Owned(const Item&) { return owned; }
bool Available(const Item&) { return usable; }
bool Grant(const Item&) { if (!grantWorks) return false; owned = true; ++grants; return true; }
}
void SaveManager_SaveCurrentForCombo(bool) {
    ++writes;
    persisted = { { "market", sSession.Save(gSaveContext.fileNum) },
                  { "rupees", gSaveContext.save.saveInfo.playerData.rupees } };
}
constexpr u16 kBrowseText = 0x8F10, kResultText = 0x8F11, kWelcomeText = 0x8F12, kConfirmText = 0x8F13;
namespace CustomMessage {
struct Entry { std::string msg; int firstItemCost; };
Entry last;
void LoadCustomMessageIntoFont(const Entry& entry) { last = entry; }
}
// Room rendering is exercised separately by run_granny_native_tests.py.
void GrannySceneBegin() {}
void ApplyGrannyInterior() {}
void EndBrowsing(PlayState*,bool = false) { sTalking = false; }
struct ShelfFixtureStub { void Reset() {} } sShelfFixture;
/* PRODUCTION */

void Reset() {
    gComboGameMode = COMBO_GAME_BOTH;
    gSaveContext = {}; play = {}; player = {};
    gPlayState = &play; gGameState = &play.state;
    play.state.running = true;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.fileNum = 0;
    gSaveContext.save.shipSaveInfo.rando.finalSeed = 77;
    play.sceneId = COMBO_BLACK_MARKET_SCENE;
    sSession.Bind(Generate(77), 0); sSession.state = {};
    sClears = 3; sEntering = sReturnPending = false;
    sTravelSlot = -1; sTravelSeed = 0;
    sResumeFromMarket = false;
    sSoloReturn = {}; sSoloReturnYaw = -8192;
    sSoloReturnEntrance = ENTRANCE(WOODS_OF_MYSTERY, 0); sReturnStaged = false;
    owned = false; usable = grantWorks = pack = true;
    inCs = false; floorExists = true; flatFloor = {}; flatFloor.normal.y = 32767;
    player.actor.floorPoly = &flatFloor; player.actor.floorHeight = 0;
    grants = writes = 0; gSaveContext.save.saveInfo.playerData.rupees = 500;
    sSelected = 0;
}
int main() {
    Reset();
    u16 textId = kBrowseText; bool fromTable = true;
    MarketText(&textId, &fromTable);
    assert(!fromTable && CustomMessage::last.msg.find("A: Select   B: Back") != std::string::npos);
    textId = kConfirmText; fromTable = true; MarketText(&textId,&fromTable);
    assert(!fromTable && CustomMessage::last.msg.find(std::string("\xC2") + "Buy\nCancel") != std::string::npos);
    textId = kWelcomeText; fromTable = true; MarketText(&textId,&fromTable);
    assert(!fromTable && CustomMessage::last.msg.find("Left / Right: browse items") != std::string::npos);
    const int price = sSession.market["stock"][0]["price"];
    Purchase();
    assert(grants == 1 && writes == 1 && persisted["rupees"] == 500 - price);
    assert(persisted["market"]["state"]["purchased"] == 1);
    Purchase(); assert(grants == 1 && writes == 1);
    Reset(); grantWorks = false; Purchase();
    assert(grants == 0 && writes == 0 && gSaveContext.save.saveInfo.playerData.rupees == 500);
    assert(sSession.state.purchased == 0);
    Reset(); gSaveContext.rupeeAccumulator = -450; Purchase();
    if (price > 50) assert(grants == 0 && writes == 0);
    Reset(); sClears = 0;
    for (int i = 0; i < 8; ++i) if (sSession.market["stock"][i]["clears"] != 0) {
        sSelected = i; Purchase(); assert(grants == 0 && writes == 0);
    }
    Reset();
    gSaveContext.save.entrance = COMBO_BLACK_MARKET_ENTRANCE;
    const auto protectedSave = gSaveContext.save.saveInfo;
    const auto protectedTop = gSaveContext.respawn[RESPAWN_MODE_TOP];
    ComboBlackMarket_NormalizeEntry();
    assert(gSaveContext.save.entrance == ENTRANCE(LOST_WOODS, 0) && sReturnPending);
    assert(!std::memcmp(&protectedSave, &gSaveContext.save.saveInfo, sizeof(protectedSave)));
    assert(!std::memcmp(&protectedTop, &gSaveContext.respawn[RESPAWN_MODE_TOP], sizeof(protectedTop)));
    Reset(); sEntering = true; sTravelSlot = 0; sTravelSeed = 77;
    gSaveContext.save.entrance = COMBO_BLACK_MARKET_ENTRANCE;
    ComboBlackMarket_NormalizeEntry(); assert(gSaveContext.save.entrance == COMBO_BLACK_MARKET_ENTRANCE);
    gSaveContext.respawnFlag = 1;
    ComboBlackMarket_NormalizeEntry(); assert(gSaveContext.save.entrance == ENTRANCE(LOST_WOODS, 0));
    Reset(); sEntering = true; sTravelSlot = 1; sTravelSeed = 77;
    gSaveContext.save.entrance = COMBO_BLACK_MARKET_ENTRANCE;
    ComboBlackMarket_NormalizeEntry();
    assert(gSaveContext.save.entrance == ENTRANCE(LOST_WOODS, 0));
    Reset(); sEntering = true; sTravelSlot = 0; sTravelSeed = 78;
    gSaveContext.save.entrance = COMBO_BLACK_MARKET_ENTRANCE;
    ComboBlackMarket_NormalizeEntry();
    assert(gSaveContext.save.entrance == ENTRANCE(LOST_WOODS, 0));
    for (int i = 0; i < 4; ++i) {
        Reset(); sEntering = true; sTravelSlot = 0; sTravelSeed = 77;
        gSaveContext.save.entrance = COMBO_BLACK_MARKET_ENTRANCE;
        if (i == 0) pack = false;
        if (i == 1) sSession.Bind(nullptr, -1);
        if (i == 2) gSaveContext.respawnFlag = -1;
        if (i == 3) gSaveContext.fileNum = 2;
        ComboBlackMarket_NormalizeEntry();
        assert(gSaveContext.save.entrance == ENTRANCE(LOST_WOODS, 0));
    }
    assert(ComboBlackMarket_InDoor(COMBO_BLACK_MARKET_DOOR_X, 0, COMBO_BLACK_MARKET_DOOR_Z, 0));
    assert(!ComboBlackMarket_InDoor(COMBO_BLACK_MARKET_DOOR_X - 226, 0, COMBO_BLACK_MARKET_DOOR_Z, 0));
    assert(ComboBlackMarket_AtExit(-11, 0, 410)); // native customer doorway, Z420
    assert(!ComboBlackMarket_AtExit(-11, 0, 381)); // safe native entry
    assert(!ComboBlackMarket_AtExit(0, 0, -260)); // staff area is not an exit
    assert(!ComboBlackMarket_AtExit(60, 0, 410)); // beside the door
    Reset();
    gSaveContext.save.entrance = COMBO_BLACK_MARKET_ENTRANCE;
    sSession.state.purchased = 0xA5;
    nlohmann::json saved;
    ComboBlackMarket_SaveState(saved,0);
    assert(saved["blackMarket"]["savedInside"] == true);
    sSession.state = {};
    ComboBlackMarket_LoadState(saved,0);
    assert(sSession.state.purchased == 0xA5);
    gSaveContext.save.entrance = ENTRANCE(SOUTH_CLOCK_TOWN,0);
    ComboBlackMarket_ApplyResume(1);
    assert(gSaveContext.save.entrance == ENTRANCE(LOST_WOODS,0) && sReturnPending);
    assert(sTravelSlot == 0 && sTravelSeed == 77);
    Reset();
    ComboBlackMarket_LoadState(saved,0);
    gSaveContext.save.entrance = ENTRANCE(SOUTH_CLOCK_TOWN,0);
    ComboBlackMarket_ApplyResume(0);
    assert(gSaveContext.save.entrance == ENTRANCE(SOUTH_CLOCK_TOWN,0) && !sReturnPending);
    Reset();
    nlohmann::json copied = saved;
    ComboBlackMarket_SaveState(copied,1);
    assert(copied == saved); // another slot's copy/migration must not erase purchases
    ComboBlackMarket_LoadState(saved,1);
    ComboBlackMarket_ApplyResume(1);
    assert(!sReturnPending && sSession.state.purchased == 0);
    Reset();
    gSaveContext.save.entrance = ENTRANCE(CLOCK_TOWER_INTERIOR,0);
    nlohmann::json cycle;
    ComboBlackMarket_SaveState(cycle,0);
    assert(cycle["blackMarket"]["savedInside"] == false);
    Reset();
    gComboGameMode = COMBO_GAME_MM;
    const auto soloMarket = Generate(77, OptionsFromJson({ { "games", "mm" } }));
    nlohmann::json soloSave = { { "blackMarket", { { "market", soloMarket },
        { "state", { { "purchased", 0x5A } } }, { "clears", 0x500 } } } };
    sSession.Bind(nullptr, -1);
    ComboBlackMarket_LoadState(soloSave, 0);
    assert(sSession.Active(0) && sSession.market == soloMarket);
    assert(sSession.state.purchased == 0x5A && sClears == 0x500);
    nlohmann::json soloRoundTrip;
    ComboBlackMarket_SaveState(soloRoundTrip, 0);
    assert(soloRoundTrip["blackMarket"]["clears"] == 0x500);
    nativeSavedDisk = soloSave;
    nativeSavedDisk["blackMarket"]["savedInside"] = true;
    gSaveContext.save.entrance = ENTRANCE(CLOCK_TOWER_INTERIOR, 0);
    LoadSolo(0);
    assert(sResumeFromMarket && !sReturnStaged);
    // Remember Save Location runs later and loads all three native respawns.
    gSaveContext.respawn[RESPAWN_MODE_RETURN].pos = { 44, 55, 66 };
    const auto loadedReturn = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    ComboBlackMarket_NormalizeEntry();
    assert(gSaveContext.save.entrance == COMBO_SOLO_FOREST_MM_ENTRANCE);
    assert(sReturnStaged && gSaveContext.respawn[RESPAWN_MODE_RETURN].roomIndex == 0);
    assert(!std::memcmp(&sPreservedReturn, &loadedReturn, sizeof(loadedReturn)));
    play.sceneId = COMBO_SOLO_FOREST_MM_SCENE; play.roomCtx.curRoom.num = 0;
    SceneInit(COMBO_SOLO_FOREST_MM_SCENE, 0);
    assert(!std::memcmp(&gSaveContext.respawn[RESPAWN_MODE_RETURN], &loadedReturn, sizeof(loadedReturn)));
    play.sceneId = COMBO_BLACK_MARKET_SCENE;
    nlohmann::json nativeCopy = soloSave;
    gGameState = nullptr;
    ComboBlackMarket_SaveState(nativeCopy, 0);
    assert(nativeCopy == soloSave);
    gGameState = &play.state;
    assert(Interior());
    play.sceneId = COMBO_SOLO_FOREST_MM_SCENE;
    play.roomCtx.curRoom.num = 0;
    assert(SoloExterior(&play) && Exterior(&play));
    play.sceneId = SCENE_26SARUNOMORI; assert(!SoloExterior(&play));
    play.sceneId = COMBO_SOLO_FOREST_MM_SCENE;
    assert(!ComboOpeningForest_IsExploration(&play));
    assert(Door().x == 410 && Door().z == -300);
    const auto soloTop = gSaveContext.respawn[RESPAWN_MODE_TOP];
    gSaveContext.respawn[RESPAWN_MODE_RETURN].pos = { 44, 55, 66 };
    const auto soloReturn = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    gSaveContext.save.entrance = COMBO_BLACK_MARKET_ENTRANCE;
    ComboBlackMarket_NormalizeEntry();
    assert(gSaveContext.save.entrance == COMBO_SOLO_FOREST_MM_ENTRANCE);
    assert(gSaveContext.respawnFlag == 2 && sReturnStaged);
    assert(gSaveContext.respawn[RESPAWN_MODE_RETURN].roomIndex == 0);
    assert(!std::memcmp(&soloTop, &gSaveContext.respawn[RESPAWN_MODE_TOP], sizeof(soloTop)));
    assert(!std::memcmp(&soloReturn, &sPreservedReturn, sizeof(soloReturn)));
    gComboGameMode = COMBO_GAME_OOT;
    play.sceneId = COMBO_BLACK_MARKET_SCENE;
    assert(!Interior() && !SoloExterior(&play));
    gComboGameMode = COMBO_GAME_MM;
    // Binding a native solo save cannot accept another game's frozen stock.
    soloSave["blackMarket"]["market"] = Generate(77, OptionsFromJson({ { "games", "oot" } }));
    ComboBlackMarket_LoadState(soloSave, 0);
    assert(!sSession.Active(0));
    Reset(); gComboGameMode = COMBO_GAME_MM;
    play.sceneId = COMBO_SOLO_FOREST_MM_SCENE; play.roomCtx.curRoom.num = 0;
    player.actor.world.pos = { 0, 500, 0 }; PlayerUpdate(&player.actor);
    assert(sSoloReturn.y == 0);
    Reset();
    gComboGameMode = COMBO_GAME_MM;
    play.sceneId = COMBO_SOLO_FOREST_MM_SCENE; play.roomCtx.curRoom.num = 0;
    gSaveContext.save.entrance = COMBO_SOLO_FOREST_MM_ENTRANCE;
    player.actor.world.pos = { 0, 0, 0 };
    PlayerUpdate(&player.actor);
    assert(sDoorArmed && play.transitionTrigger == TRANS_TRIGGER_OFF);
    const auto captured = player.actor.world.pos;
    player.actor.world.pos = { 410, 0, -300 };
    PlayerUpdate(&player.actor);
    assert(sEntering && play.nextEntrance == COMBO_BLACK_MARKET_ENTRANCE);
    assert(play.transitionTrigger == TRANS_TRIGGER_START && gComboGameMode == COMBO_GAME_MM);
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    play.sceneId = COMBO_BLACK_MARKET_SCENE;
    player.actor.world.pos = { -11, 0, 410 };
    PlayerUpdate(&player.actor);
    assert(sReturnPending && play.nextEntrance == COMBO_SOLO_FOREST_MM_ENTRANCE);
    gSaveContext.save.entrance = play.nextEntrance;
    ComboBlackMarket_NormalizeEntry();
    play.sceneId = COMBO_SOLO_FOREST_MM_SCENE;
    SceneInit(COMBO_SOLO_FOREST_MM_SCENE, 0);
    assert(!sReturnPending && !sReturnStaged);
    assert(player.actor.world.pos.x == captured.x && player.actor.world.pos.z == captured.z);
    for (int gate = 0; gate < 8; ++gate) {
        Reset(); gComboGameMode = COMBO_GAME_MM;
        play.sceneId = COMBO_SOLO_FOREST_MM_SCENE; play.roomCtx.curRoom.num = 0;
        player.actor.world.pos = { 410, 0, -300 }; sDoorArmed = true;
        if (gate == 0) pack = false;
        if (gate == 1) sSession.Bind(nullptr, -1);
        if (gate == 2) play.pauseCtx.state = PAUSE_STATE_MAIN;
        if (gate == 3) inCs = true;
        if (gate == 4) play.roomCtx.curRoom.num = 1;
        if (gate == 5) gComboGameMode = COMBO_GAME_OOT;
        if (gate == 6) floorExists = false;
        if (gate == 7) gSaveContext.fileNum = 0xFF;
        PlayerUpdate(&player.actor);
        assert(!sEntering && play.transitionTrigger == TRANS_TRIGGER_OFF);
    }
    std::puts("PASS market native transactions and local entry/reload/void recovery; slot/seed and TOP isolation");
}
