#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include "combo/entrances/LostWoodsEntrance.h"
#include "combo/vanilla/ComboVanillaPolicy.h"

using s32 = int;
#define COMBO_EXPORT
enum { SOUTH_CLOCK_TOWN = 0x6C, CUTSCENE = 0x0E, LOST_WOODS = 0x62, WOODS_OF_MYSTERY = 0x43,
       WOODFALL = 0x14, PLAYER_FORM_HUMAN = 4, PLAYER_FORM_DEKU = 3, RESPAWN_MODE_RETURN = 1,
       RESPAWN_MODE_TOP = 2, PLAYER_START_MODE_E = 14, TRANS_TYPE_FADE_BLACK = 2,
       ITEM_OCARINA_OF_TIME = 0, ITEM_MASK_DEKU = 0x32, ITEM_MASK_KEATON = 0x3A,
       ITEM_DEKU_NUT = 9, ITEM_DEKU_NUTS_10 = 0x8E, QUEST_SONG_TIME = 12, QUEST_SONG_HEALING = 13,
       SCENE_INSIDETOWER = 1, SCENE_OPENINGDAN = 2, SCENE_YOUSEI_IZUMI = 3,
       WEEKEVENTREG_59_04 = 0, WEEKEVENTREG_31_04 = 1, WEEKEVENTREG_ENTERED_EAST_CLOCK_TOWN = 2,
       WEEKEVENTREG_ENTERED_WEST_CLOCK_TOWN = 3, WEEKEVENTREG_ENTERED_NORTH_CLOCK_TOWN = 4,
       WEEKEVENTREG_55_02 = 5, WEEKEVENTREG_90_01 = 6, WEEKEVENTREG_89_40 = 7,
       WEEKEVENTREG_89_08 = 8, WEEKEVENTREG_85_80 = 9, WEEKEVENTREG_73_10 = 10, WEEKEVENTREG_85_02 = 11,
       VB_PLAY_TRANSITION_CS = 1 };
#define ENTRANCE(scene, spawn) ((scene) * 512 + (spawn) * 16)
#define CLOCK_TIME(h, m) ((h) * 60 + (m))
#define PLAYER_PARAMS(cam, mode) (((cam) << 8) | (mode))
struct Vec3 { float x = 0, y = 0, z = 0; };
struct RespawnData { int entrance = 0, roomIndex = 0, yaw = 0, playerParams = 0, data = 0; Vec3 pos; };
struct SceneFlags { unsigned switch0 = 0, chest = 0; };
struct SaveContext {
    struct {
        int entrance = ENTRANCE(CUTSCENE, 0), cutsceneIndex = 0, isFirstCycle = 0, isOwlSave = 0;
        int playerForm = PLAYER_FORM_HUMAN, hasTatl = 0, day = 0, time = 77, eventDayCount = 0;
        struct {
            struct { unsigned threeDayResetCount = 0; bool isMagicAcquired = false; } playerData;
            struct { unsigned questItems = 0; } inventory;
            int bombersCaughtNum = 0, bombersCaughtOrder[5]{};
        } saveInfo;
    } save;
    int nextCutsceneIndex = 0xFFEF, cutsceneTrigger = 0, respawnFlag = 0, nextTransitionType = -1;
    bool showTitleCard = false;
    int skyboxTime = 0;
    std::array<int, 256> inventory;
    std::array<int, 256> ammo{};
    std::array<bool, 12> events{};
    SceneFlags cycleSceneFlags[5];
    RespawnData respawn[4];
} gSaveContext;
struct PlayState {};
static PlayState play;
static PlayState* gPlayState = &play;
static bool vanilla = true, skipIntro = true, skipCycle = true;
static int sComboPortalRoute = COMBO_PORTAL_MARKET;
static bool sComboPortalArrivalPending = false;
static int gComboEntryIsResume = 0;
static int cameras = 0, lastCamera = -1, mirrors = 0, saves = 0, grants = 0, regionalCalls = 0;
static int savedEntrance = -1;
static bool titlePending = false;
static std::function<void()> introHook;
static void require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
int Combo_IsVanilla() { return vanilla; }
int CVarGetInteger(const char* key, int fallback) {
    if (!std::strcmp(key, "gEnhancements.Cutscenes.SkipIntroSequence")) return skipIntro;
    if (!std::strcmp(key, "gEnhancements.Cutscenes.SkipFirstCycle")) return skipCycle;
    return fallback;
}
void Combo_ApplyOwlSaveOpen(int) { gSaveContext.save.entrance = 0x1230; }
void Combo_ClearReturnRequests() {}
void ComboRegionalPassagesMm_ApplyArrival(int) { ++regionalCalls; }
void ComboForestArrivalCamera_Queue(int route) { ++cameras; lastCamera = route; }
void MagicMirror_QueueArrival() { ++mirrors; }
void Combo_MM_RequestCrossGameTitleCard() { titlePending = gSaveContext.showTitleCard = true; }
#define AMMO(item) gSaveContext.ammo[item]
#define SET_WEEKEVENTREG(event) (gSaveContext.events[event] = true)
#define CLEAR_WEEKEVENTREG(event) (gSaveContext.events[event] = false)
#define CURRENT_DAY gSaveContext.save.day
#define COND_VB_SHOULD(id, condition, ...) do { if (condition) introHook = []() __VA_ARGS__; } while (0)
void Item_Give(PlayState*, int item) {
    ++grants;
    if (item == ITEM_DEKU_NUTS_10) { gSaveContext.inventory[ITEM_DEKU_NUT] = ITEM_DEKU_NUT; AMMO(ITEM_DEKU_NUT) = 10; }
    else gSaveContext.inventory[item] = item;
}
void Sram_SaveSpecialEnterClockTown(PlayState*) {
    ++saves;
    savedEntrance = gSaveContext.save.entrance;
    require(gSaveContext.save.isFirstCycle, "native skip must mark first arrival before persistence");
}

/* PRODUCTION_BODIES */

static void reset(int route) {
    gSaveContext = {};
    gSaveContext.inventory.fill(0xFF);
    gSaveContext.respawn[RESPAWN_MODE_TOP].pos = {111, 222, 333};
    gSaveContext.cycleSceneFlags[4].chest = 0x55;
    sComboPortalRoute = route;
    sComboPortalArrivalPending = route != COMBO_PORTAL_MARKET;
    vanilla = skipIntro = skipCycle = true;
    cameras = mirrors = saves = grants = regionalCalls = 0;
    lastCamera = savedEntrance = -1;
    titlePending = false;
    introHook = {};
}

int main() {
    // Restoring the old early consume must fail before any skip/item service runs.
    for (int route : {COMBO_PORTAL_LOST_WOODS, COMBO_PORTAL_OPENING_FOREST, COMBO_PORTAL_MAGIC_MIRROR,
                      COMBO_PORTAL_WOODFALL}) {
        reset(route);
        require(ComboVanilla_PrepareMMOpening(0), "fresh vanilla must prepare its native opening");
        MM_ApplyComboPortalArrival();
        require(sComboPortalArrivalPending, "fresh skipped opening must retain the requested portal until native skip finishes");
        require(grants == 0 && saves == 0, "portal preparation must not grant or persist starting items");
        RegisterSkipIntroSequence();
        require(bool(introHook), "native intro-skip hook must remain registered");
        gSaveContext.save.cutsceneIndex = gSaveContext.nextCutsceneIndex;
        gSaveContext.nextCutsceneIndex = 0xFFEF;
        introHook();
        const int expected = route == COMBO_PORTAL_LOST_WOODS ? 0x8600
                             : route == COMBO_PORTAL_WOODFALL ? 0x2800 : 0xC400;
        require(gSaveContext.save.entrance == expected, "first crossing must arrive at the selected forest");
        require(!sComboPortalArrivalPending && titlePending, "deferred arrival must consume once and request its title card");
        require(grants == 3 && saves == 1 && savedEntrance == 0xD800, "native skip must grant once and persist before portal placement");
        require(gSaveContext.save.saveInfo.playerData.threeDayResetCount == 1 &&
                    gSaveContext.save.playerForm == PLAYER_FORM_HUMAN &&
                    gSaveContext.save.saveInfo.playerData.isMagicAcquired,
                "native first-cycle completion, form and magic must survive");
        require(gSaveContext.save.day == 1 && gSaveContext.save.eventDayCount == 1 &&
                    gSaveContext.save.time == CLOCK_TIME(6, 0),
                "fresh passage must finish native dawn before any destination actor or day screen consumes its spawn");
        require(gSaveContext.inventory[ITEM_MASK_DEKU] == 0x32 &&
                    gSaveContext.inventory[ITEM_MASK_KEATON] == 0xFF &&
                    gSaveContext.save.saveInfo.inventory.questItems == 0x3000,
                "keep the native Deku Mask and songs without unrelated rewards");
        require(gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y == 222 &&
                    gSaveContext.cycleSceneFlags[4].chest == 0x55,
                "portal must preserve Farore's Wind and unrelated progression");
        require(route != COMBO_PORTAL_LOST_WOODS ||
                    (gSaveContext.respawnFlag == 2 && gSaveContext.respawn[RESPAWN_MODE_RETURN].roomIndex == 1),
                "Woods of Mystery must retain its positioned clearing arrival");
        require(route != COMBO_PORTAL_OPENING_FOREST || (cameras == 1 && lastCamera == route),
                "opening forest must queue the correct camera once");
        require(route != COMBO_PORTAL_MAGIC_MIRROR || (mirrors == 1 && cameras == 0),
                "Mirror must retain its own arrival presentation");
        const int oldCameras = cameras, oldMirrors = mirrors;
        introHook(); MM_ApplyComboPortalArrival();
        require(grants == 3 && saves == 1 && cameras == oldCameras && mirrors == oldMirrors,
                "re-entry must never repeat grants, persistence or arrival presentation");
    }
    for (bool intro : {false, true}) for (bool cycle : {false, true}) {
        if (intro && cycle) continue;
        reset(COMBO_PORTAL_LOST_WOODS); skipIntro = intro; skipCycle = cycle;
        ComboVanilla_PrepareMMOpening(0); MM_ApplyComboPortalArrival();
        require(!sComboPortalArrivalPending && gSaveContext.save.entrance == 0x1C00 && grants == 0,
                "unskipped native prologue must discard a crossing without grants or teleportation");
    }
    reset(COMBO_PORTAL_LOST_WOODS); gSaveContext.save.isFirstCycle = 1;
    gSaveContext.save.playerForm = PLAYER_FORM_DEKU;
    ComboVanilla_PrepareMMOpening(0); MM_ApplyComboPortalArrival();
    require(!sComboPortalArrivalPending && gSaveContext.save.playerForm == PLAYER_FORM_DEKU &&
                gSaveContext.save.entrance == 0xD800, "an existing first-cycle Deku save must retain its native opening");
    reset(COMBO_PORTAL_LOST_WOODS);
    gSaveContext.save.playerForm = PLAYER_FORM_DEKU;
    ComboVanilla_PrepareMMOpening(1); MM_ApplyComboPortalArrival();
    require(!sComboPortalArrivalPending && gSaveContext.save.entrance == 0x1230 &&
                gSaveContext.save.playerForm == PLAYER_FORM_DEKU,
            "first-cycle owl resumes must retain native save location and form");
    reset(COMBO_PORTAL_LOST_WOODS);
    ComboVanilla_PrepareMMOpening(0); MM_ApplyComboPortalArrival();
    require(sComboPortalArrivalPending, "fresh passage must be pending before entry cancellation");
    MM_SetComboEntryIsResume(1);
    require(gComboEntryIsResume == 1 && !sComboPortalArrivalPending && sComboPortalRoute == COMBO_PORTAL_MARKET,
            "a new resume must cancel a deferred passage from the previous entry");
    RegisterSkipIntroSequence(); introHook();
    require(gSaveContext.save.entrance == 0xD800 && !titlePending,
            "a canceled deferred passage must never replay after a later skip");
    reset(COMBO_PORTAL_MARKET);
    ComboVanilla_PrepareMMOpening(0); RegisterSkipIntroSequence(); introHook();
    require(gSaveContext.save.entrance == 0xD800 && !titlePending && grants == 3,
            "the Market portal and ordinary intro skip must retain the native Clock Town landing");
    for (bool mode : {false, true}) {
        reset(COMBO_PORTAL_LOST_WOODS); vanilla = mode;
        gSaveContext.save.saveInfo.playerData.threeDayResetCount = mode ? 1 : 0;
        gSaveContext.save.day = 2; gSaveContext.save.time = 0x8000;
        MM_ApplyComboPortalArrival();
        require(gSaveContext.save.entrance == 0x8600 && !sComboPortalArrivalPending && grants == 0 &&
                    gSaveContext.save.day == 2 && gSaveContext.save.time == 0x8000,
                "seeded and initialized vanilla crossings must retain immediate arrival and cycle time");
    }
    reset(COMBO_PORTAL_LOST_WOODS);
    gSaveContext.save.saveInfo.playerData.threeDayResetCount = 1;
    gSaveContext.events.fill(true);
    gSaveContext.save.saveInfo.bombersCaughtNum = 5;
    for (int& value : gSaveContext.save.saveInfo.bombersCaughtOrder) value = 2;
    MM_ApplyComboPortalAfterIntroSkip();
    require(gSaveContext.save.day == 1 && gSaveContext.save.eventDayCount == 1 &&
                gSaveContext.save.saveInfo.bombersCaughtNum == 0 &&
                !gSaveContext.events[WEEKEVENTREG_55_02] && !gSaveContext.events[WEEKEVENTREG_73_10] &&
                gSaveContext.events[WEEKEVENTREG_59_04],
            "deferred day-zero passage must run native dawn resets without clearing unrelated story flags");
    require(gSaveContext.save.entrance == 0x8600 && gSaveContext.respawnFlag == 2 &&
                gSaveContext.respawn[RESPAWN_MODE_RETURN].roomIndex == 1,
            "native dawn must finish before the positioned forest arrival is installed");
    std::puts("PASS MM fresh forest/Mirror/Woodfall arrivals, native skip grants, exactly-once routing and opening controls");
}
