#include "global.h"
#include "ComboGameMode.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "2s2h/Enhancements/Gameplay/BlackMarket.h"
#include "2s2h/Enhancements/Gameplay/BlackMarketDoorNative.h"
#include "entrances/BlackMarketEntrance.h"
#include "BlackMarketExitDoor.h"
#include "entrances/SoloForest.h"
#include "rando/BlackMarket.h"
#include "BlackMarketText.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/Rando/StaticData/StaticData.h"
#include "overlays/actors/ovl_En_Door/z_en_door.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include "entry_diagnostics_fixture.h"
extern "C" u8 ResourceMgr_FileExists(const char*) { return destination; }
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
SaveContext gSaveContext{};
// Native title policy is exercised by lost_woods_entrance/run_mm_title_card_tests.py.
extern "C" void Combo_MM_RequestCrossGameTitleCard(void) {}
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
extern "C" void Player_Action_36(Player*, PlayState*) {}
static int vanillaDoors, stoppedDoors, doorErrors, doorCutscenes;
extern "C" s16 CutsceneManager_Start(s16, Actor*) { ++doorCutscenes; return 1; }
static void NativeIdle(Player*, PlayState*) {}
extern "C" void Player_StopCutscene(Player*) { ++stoppedDoors; }
void func_80839E74(Player* current, PlayState*) { current->actionFunc = NativeIdle; }
extern "C" void Audio_PlaySfx(u16) { ++doorErrors; }
CollisionPoly flatFloor;
bool floorExists = true;
extern "C" f32 BgCheck_EntityRaycastFloor5_3(PlayState*, CollisionContext*, CollisionPoly** poly,
    s32* bg, Actor*, Vec3f*) {
    *poly = floorExists ? &flatFloor : nullptr; *bg = BGCHECK_SCENE;
    return floorExists ? 0 : BGCHECK_Y_MIN;
}
extern "C" void Camera_InitFocalActorSettings(Camera*, Actor*) {}
namespace ComboBlackMarketItems {
RandoItemId Id(const Item&) { return RI_UNKNOWN; }
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
Entry last;
void LoadCustomMessageIntoFont(Entry entry) { last = std::move(entry); }
}
namespace Rando::StaticData { std::map<RandoItemId, RandoStaticItem> Items; }
float sNESFontWidths[160]{}; // Actual native widths/decoder are exercised by run_text_codec_tests.py.
// Room rendering is exercised separately by run_granny_native_tests.py.
void GrannySceneBegin() {}
void ApplyGrannyInterior() {}
void EndBrowsing(PlayState*,bool = false) { sTalking = false; }
struct ShelfFixtureStub { void Reset() {} } sShelfFixture;
/* NATIVE_HALF_DAY_TABLE */
/* PRODUCTION */

void NativeOffer(EnDoor* current, PlayState* play) {
    Player* player = GET_PLAYER(play);
    player->doorType = PLAYER_DOORTYPE_HANDLE;
    /* NATIVE_DOOR_OFFER */
}

s32 NativeOpen(Player* current, PlayState* play, KnobDoorActor* knobDoor) {
    knobDoor->requestOpen = true;
    /* NATIVE_DOOR_OPEN */
    ++vanillaDoors;
    return true;
}
s32 NativeCaller(Player* current, PlayState* play, KnobDoorActor* knobDoor) {
    /* NATIVE_DOOR_CALLER */
    return true;
}

void Reset() {
    destination = true;
    diagnostics.clear();
    gComboGameMode = COMBO_GAME_BOTH;
    gSaveContext = {}; play = {}; player = {};
    gPlayState = &play; gGameState = &play.state;
    play.state.running = true;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    player.actor.category = ACTORCAT_PLAYER;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.fileNum = 0;
    gSaveContext.save.shipSaveInfo.rando.finalSeed = 77;
    gSaveContext.save.saveInfo.playerData.health = 0x50;
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
    vanillaDoors = stoppedDoors = doorErrors = doorCutscenes = 0;
    sSelected = 0;
}

void TestPlacedDoor() {
    TransitionActorEntry transitions[2]{};
    KnobDoorActor door{};
    auto prepare = [&] {
        Reset();
        play.sceneId = SCENE_LOST_WOODS;
        play.roomCtx.curRoom.num = 0;
        gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 0);
        transitions[0] = {}; transitions[0].id = -ACTOR_EN_HOLL;
        transitions[0].sides[0].room = 1; transitions[0].sides[1].room = 0;
        auto& entry = transitions[1]; entry = {};
        entry.id = -ACTOR_EN_DOOR; entry.pos = {-680, 0, 72};
        entry.rotY = (106 << 7) | 127;
        entry.sides[0].room = entry.sides[1].room = 0;
        play.transitionActors.count = 2; play.transitionActors.list = transitions;
        door = {}; door.dyna.actor.id = ACTOR_EN_DOOR;
        door.dyna.actor.params = TRANSITION_ACTOR_PARAMS(1, 0);
        door.dyna.actor.world.pos = door.dyna.actor.home.pos = {-680, 0, 72};
        player.doorActor = &door.dyna.actor; player.actionFunc = Player_Action_36;
    };
    const s16 halfDays[] = {HALFDAYBIT_DAY0_DAWN, HALFDAYBIT_DAY0_NIGHT,
        HALFDAYBIT_DAY1_DAWN, HALFDAYBIT_DAY1_NIGHT, HALFDAYBIT_DAY2_DAWN, HALFDAYBIT_DAY2_NIGHT,
        HALFDAYBIT_DAY3_DAWN, HALFDAYBIT_DAY3_NIGHT, HALFDAYBIT_DAY4_DAWN, HALFDAYBIT_DAY4_NIGHT};
    // Execute the native offer branch, not just a manually preselected Open action.
    for (s16 halfDay : halfDays) {
        prepare();
        EnDoor offered{}; offered.knobDoor = door;
        offered.doorType = ENDOOR_TYPE_WHOLE_DAY; offered.typeVar.halfDayCheck = 0;
        play.actorCtx.halfDaysBit = halfDay;
        NativeOffer(&offered, &play);
        assert(player.doorType == PLAYER_DOORTYPE_HANDLE);
        // A different scene, transition index, or room pair retains its schedule.
        for (int other = 0; other < 3; ++other) {
            prepare(); offered.knobDoor = door;
            play.actorCtx.halfDaysBit = halfDay;
            if (other == 0) play.sceneId = SCENE_TOWN;
            if (other == 1) offered.knobDoor.dyna.actor.params = TRANSITION_ACTOR_PARAMS(0, 0);
            if (other == 2) transitions[1].sides[1].room = 1;
            NativeOffer(&offered, &play);
            const s16 nightBit = ((halfDay & HALFDAYBIT_DAWNS) >> 1) | (halfDay & HALFDAYBIT_NIGHTS);
            assert(player.doorType == ((nightBit & D_801AED48[0]) ?
                PLAYER_DOORTYPE_HANDLE : PLAYER_DOORTYPE_TALKING));
        }
    }
    prepare();
    const auto original = transitions[1];
    const auto respawn = gSaveContext;
    NativeCaller(&player, &play, &door);
    assert(sEntering && sTravelSlot == 0 && sTravelSeed == 77);
    assert(play.transitionTrigger == TRANS_TRIGGER_START && play.nextEntrance == COMBO_BLACK_MARKET_ENTRANCE);
    assert(play.transitionType == TRANS_TYPE_FADE_BLACK && door.requestOpen && vanillaDoors == 0);
    assert(doorCutscenes == 1 && (player.stateFlags1 & PLAYER_STATE1_20000000));
    assert(std::memcmp(&transitions[1], &original, sizeof(original)) == 0);
    assert(std::memcmp(&gSaveContext.respawn, &respawn.respawn, sizeof(respawn.respawn)) == 0);
    // Returning uses the authored door's outward direction, not the old shell.
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    sReturnPending = true;
    SceneInit(SCENE_LOST_WOODS, 0);
    const float angle = 106 * (3.14159265359f / 180);
    assert(std::abs(player.actor.world.pos.x - (-680 + std::sin(angle) * 226)) < 0.1f);
    assert(std::abs(player.actor.world.pos.z - (72 + std::cos(angle) * 226)) < 0.1f);
    assert(player.actor.shape.rot.y == DEG_TO_BINANG(106));
    // A future Prelude translation/rotation drives the same native callback.
    prepare();
    transitions[1].id = ACTOR_EN_DOOR; // also valid before native spawning
    transitions[1].pos = {350, 10, -250}; transitions[1].rotY = (270 << 7) | 127;
    door.dyna.actor.home.pos = door.dyna.actor.world.pos = {350, 10, -250};
    BlackMarketDoorNative::Pose moved;
    assert(BlackMarketDoorNative::Find(&play, moved) && moved.yaw == DEG_TO_BINANG(270));
    NativeCaller(&player, &play, &door);
    assert(sEntering && vanillaDoors == 0);
    for (int reason = 0; reason < 3; ++reason) {
        prepare();
        if (reason == 0) destination = false;
        if (reason == 1) play.transitionTrigger = TRANS_TRIGGER_START;
        if (reason == 2) play.transitionMode = TRANS_MODE_SETUP;
        const auto previousTrigger = play.transitionTrigger;
        NativeCaller(&player, &play, &door);
        assert(!sEntering && play.transitionTrigger == previousTrigger && !door.requestOpen);
        assert(vanillaDoors == 0 && stoppedDoors == 1 && doorErrors == 1 && player.actionFunc == NativeIdle);
        assert(doorCutscenes == 0); // Rejected entry must not restart the infinite global door cutscene.
    }
    // Ordinary doors, other scenes and prologue visits retain the native path.
    for (int reason = 0; reason < 4; ++reason) {
        prepare();
        if (reason == 0) play.sceneId = SCENE_TOWN;
        if (reason == 1) transitions[1].sides[1].room = 1;
        if (reason == 2) gSaveContext.save.cutsceneIndex = 0xFFF0;
        if (reason == 3) door.dyna.actor.params = TRANSITION_ACTOR_PARAMS(0, 0);
        NativeCaller(&player, &play, &door);
        assert(!sEntering && vanillaDoors == 1 && door.requestOpen && stoppedDoors == 0);
    }
    puts("PASS authored native Open action, moved door/return, blocked-shop cleanup, ordinary-door and respawn preservation");
}
#include "interior_exit_cases.inc"
int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "door-exit") { TestInteriorExitDoor(); return 0; }
    TestPlacedDoor();
    Reset();
    u16 textId = kBrowseText; bool fromTable = true;
    MarketText(&textId, &fromTable);
    assert(!fromTable && CustomMessage::last.msg.find("\xB0 Select   \xB1 Back") != std::string::npos);
    textId = kConfirmText; fromTable = true; MarketText(&textId,&fromTable);
    assert(!fromTable && CustomMessage::last.msg.find("\xC2I'll buy it\x11No thanks") != std::string::npos);
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
        if (i == 0) destination = false;
        if (i == 1) sTravelSlot = 1;
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
    // The final authored scene owns the new door. The retired placeholder
    // trigger on the far side of the clearing must no longer enter the shop.
    Reset();
    play.sceneId = SCENE_LOST_WOODS;
    play.roomCtx.curRoom.num = 0;
    gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 0);
    TransitionActorEntry authored{};
    authored.id = -ACTOR_EN_DOOR; // native spawn marks transition entries negative
    authored.pos = {-680, 0, 72};
    authored.rotY = (106 << 7) | 127;
    authored.sides[0].room = authored.sides[1].room = 0;
    play.transitionActors.count = 1;
    play.transitionActors.list = &authored;
    sDoorArmed = true;
    player.actor.world.pos = {410, 0, -300};
    PlayerUpdate(&player.actor);
    assert(!sEntering && play.transitionTrigger == TRANS_TRIGGER_OFF);
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
        if (gate == 0) destination = false;
        if (gate == 1) play.transitionMode = TRANS_MODE_SETUP;
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
