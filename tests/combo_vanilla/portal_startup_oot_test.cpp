#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
using s16 = int16_t;
using s32 = int;
using u8 = uint8_t;
enum { ENTRANCE_TABLE_SIZE = 0x800, ENTR_OUTSIDE_GANONS_CASTLE_1_2 = 0x70,
       ENTR_CASTLE_GROUNDS_RAINBOW_BRIDGE_EXIT = 0x80, SCENE_CASTLE_COURTYARD_GUARDS_DAY = 0x45,
       SCENE_CASTLE_COURTYARD_GUARDS_NIGHT = 0x46, ENTR_MARKET_DAY_OUTSIDE_HAPPY_MASK_SHOP = 0x1D1,
       GAMEMODE_NORMAL = 0, Play_Init = 1, Title_Init = 2 };
struct Vec3 { float x = 0, y = 0, z = 0; };
struct RespawnData { int entranceIndex = 0, roomIndex = 0, playerParams = 0; Vec3 pos; };
struct SaveContext {
    int fileNum = 0, gameMode = 0, magicFillTarget = 0, magic = 20, magicLevel = 1;
    int entranceIndex = 0, respawnFlag = 0, cutsceneIndex = 0xFFF1, health = 48;
    bool showTitleCard = false;
    RespawnData respawn[4];
} gSaveContext;
struct GameState { bool running = true; int next = 0; };
struct PlayState { int sceneNum = 0; };
static PlayState* gPlayState = nullptr;
static s16 entranceOverrideTable[ENTRANCE_TABLE_SIZE];
static int gComboReturnFileNum = 0, crossArrival = -1, bossRecovery = -1, fwArrival = -1;
static bool sComboCrossGameTitleCardPending = false, sComboCustomTitleCardPresentation = false;
static bool isRando = false;
static int lookups = 0, forestArrival = -1;
#define IS_RANDO isRando
#define ARRAY_COUNT(array) (int(sizeof(array) / sizeof((array)[0])))
template <class... Args> void discardLog(Args&&...) {}
#define LUSLOG_INFO(...) discardLog(__VA_ARGS__)
#define osSyncPrintf(...) ((void)0)
#define SET_NEXT_GAMESTATE(state, fn, type) ((state)->next = (fn))
void SaveContext_Init() { gSaveContext = {}; }
void Sram_OpenSave() { gSaveContext.respawn[2].pos = {111, 222, 333}; }
void GameInteractor_ExecuteOnExitGame(int) {}
void GameInteractor_ExecuteOnLoadGame(int) { gSaveContext.entranceIndex = 0xBB; }
int Combo_IsCombinedGame() { return 1; }
int32_t Combo_SOH_CrossDungeonRoute(int, int) { return -1; }
void Entrance_SetEntranceDiscovered(int, bool) { ++lookups; }
void EntranceTracker_SetLastEntranceOverride(int) {}
s16 Grotto_OverrideSpecialEntrance(s16 entrance) { return entrance; }
int32_t Combo_SOH_ConsumeCrossDungeonArrival() { const int r = crossArrival; crossArrival = -1; return r; }
int32_t Combo_SOH_ConsumeCrossBossRecoveryResume() { const int r = bossRecovery; bossRecovery = -1; return r; }
void SOH_ApplyComboPortalArrival(int allowed) {
    if (allowed && forestArrival >= 0) gSaveContext.entranceIndex = forestArrival;
    forestArrival = -1;
}
void ComboFw_ApplyArrival() { if (fwArrival >= 0) gSaveContext.entranceIndex = fwArrival; fwArrival = -1; }
static void require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

/* PRODUCTION_BODIES */

static void boot(int expected, const char* message) {
    gComboReturnFileNum = 0; lookups = 0;
    GameState state;
    TitleSetup_InitImpl(&state);
    require(gSaveContext.entranceIndex == expected, message);
    require(!state.running && state.next == Play_Init && gComboReturnFileNum == -1 &&
                gSaveContext.cutsceneIndex == 0 && gSaveContext.showTitleCard,
            "cross-game return must retain normal Play/title-card/cutscene initialization");
    require(gSaveContext.respawn[2].pos.y == 222 && gSaveContext.health == 48,
            "Market return must preserve native health and Farore's Wind");
}
int main() {
    // Native zero-initialization exactly reproduces the supplied 0x1D1 -> 0 trace.
    isRando = false;
    boot(0x1D1, "vanilla Clock Tower return must reach Market even with an uninitialized randomizer table");
    require(lookups == 0, "vanilla return must not mutate randomizer discovery or grotto state");
    entranceOverrideTable[0x1D1] = 0x234;
    boot(0x1D1, "vanilla return must ignore stale shuffled entrances from a previous seed");
    isRando = true;
    boot(0x234, "seeded Market returns must retain their actual native shuffle mapping");
    require(lookups == 1, "seeded native entrance routing must still run once");
    entranceOverrideTable[0x1D1] = 0;
    boot(0, "a seeded mapping to entrance zero is legitimate and must remain usable");
    for (bool random : {false, true}) {
        isRando = random;
        crossArrival = 0x345; boot(0x345, "composed cross-dungeon arrival must win over Market");
        require(lookups == 0, "composed arrival must avoid a second native permutation");
        bossRecovery = 0x456; boot(0x456, "boss recovery must retain its composed destination");
        forestArrival = 0x11E; boot(0x11E, "the forest return must retain its explicit arrival");
        fwArrival = 0x567; boot(0x567, "Farore recall must retain final arrival priority");
    }
    std::puts("PASS OoT vanilla Market return, stale/seeded/zero mappings and composed/forest/Farore priorities");
}
