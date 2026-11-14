// Production placement/spawn scheduling; native layouts, controlled world services.
#include "combo/vanilla/ComboVanillaRewards.h"
extern "C" {
#include "global.h"
SaveContext gSaveContext{};
PlayState* gPlayState = nullptr;
}
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>
using namespace ComboVanilla;

static PlayState play;
static Player player;
static ActorEntry entrance;
static Actor spawned;
static CollisionPoly floorPoly;
static CollisionPoly wallPoly;
static Vec3i wallVertices[3];
static bool nativeWall;
static int degenerateRays;
static bool active, blockingCs, prerequisite, collected, noFloor, wet, wall, exitFloor, centerBlocked;
static int spawns, queries, monitors;
static s16 sActorId = 0x700;
static Vec3f sEntryPosition{};
static bool sEntryReady = false;
static int sLastSpawnRoom = -1;
static const Reward* sReceiptReward = nullptr;
static const Reward& rod = *FindReward("vanilla.nei.rod_of_seasons");
static bool Active() { return active; }
static bool NativePrerequisite(std::string_view) { return prerequisite; }
static bool NativeOwned(const Reward&) { return false; }
static bool AcknowledgeOwned(const Reward&) { std::abort(); }
static void MonitorReceipt() { ++monitors; }
static int Host(RewardAction action, const Reward& reward, int) {
    if (action != RewardAction::Query) std::abort();
    ++queries;
    return &reward != &rod || collected ? 1 : 0;
}
struct SaveManager { static SaveManager* Instance; } saveManager;
SaveManager* SaveManager::Instance = &saveManager;
struct ActorDB { static ActorDB* Instance; } actorDatabase;
ActorDB* ActorDB::Instance = &actorDatabase;
static void Live(Actor*, PlayState*) {}
s32 CollisionPoly_LineVsPoly(CollisionPoly*, Vec3i*, Vec3f*, Vec3f*, Vec3f*, s32, f32);
extern "C" {
void lusprintf(const char*, int, int, const char*, ...) {}
s32 Player_InCsMode(PlayState*) { return blockingCs; }
f32 BgCheck_EntityRaycastFloor5(PlayState*, CollisionContext*, CollisionPoly** poly, s32* bg,
                              Actor*, Vec3f* probe) {
    *bg = BGCHECK_SCENE;
    if (noFloor || (centerBlocked && probe->x == 0 && probe->z == 0)) {
        *poly = nullptr;
        return BGCHECK_Y_MIN;
    }
    *poly = &floorPoly;
    return 0;
}
u32 SurfaceType_GetSceneExitIndex(CollisionContext*, CollisionPoly*, s32) { return exitFloor; }
s32 WaterBox_GetSurface1(PlayState*, CollisionContext*, f32, f32, f32* height, WaterBox**) {
    *height = 10;
    return wet;
}
s32 BgCheck_EntityLineTest1(CollisionContext*, Vec3f* from, Vec3f* to, Vec3f* hit, CollisionPoly**,
                          s32, s32, s32, s32 oneFace, s32*) {
    const bool zero = from->x == to->x && from->y == to->y && from->z == to->z;
    degenerateRays += zero;
    if (nativeWall)
        return CollisionPoly_LineVsPoly(&wallPoly, wallVertices, from, to, hit, oneFace, 1.0f);
    return wall && !zero;
}
Actor* Actor_Spawn(ActorContext* actors, PlayState*, s16 id, f32 x, f32 y, f32 z,
                   s16, s16, s16, s16 params) {
    ++spawns;
    spawned = {};
    spawned.id = id;
    spawned.params = params;
    spawned.update = Live;
    spawned.world.pos = {x, y, z};
    spawned.next = actors->actorLists[ACTORCAT_NPC].head;
    actors->actorLists[ACTORCAT_NPC].head = &spawned;
    return &spawned;
}
}
/* PRODUCTION */

static void Check(bool value, const char* why) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", why); std::exit(1); }
}
static void Reset(unsigned frame = 1) {
    play = {}; player = {}; entrance = {}; spawned = {}; gSaveContext = {}; floorPoly = {};
    active = prerequisite = true;
    blockingCs = collected = noFloor = wet = wall = exitFloor = centerBlocked = false;
    spawns = queries = monitors = 0;
    nativeWall = false; degenerateRays = 0;
    sReceiptReward = nullptr;
    sEntryReady = false; sLastSpawnRoom = -1; sEntryPosition = {};
    SaveManager::Instance = &saveManager; ActorDB::Instance = &actorDatabase;
    gPlayState = &play;
    play.sceneNum = SCENE_LOST_WOODS; play.roomCtx.curRoom.num = 0;
    play.gameplayFrames = frame; play.state.running = true;
    play.linkActorEntry = &entrance;
    entrance.pos = {0, 0, 300};
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    player.actor.floorPoly = &floorPoly; player.actor.floorBgId = BGCHECK_SCENE;
    floorPoly.normal.y = 32767;
    gSaveContext.entranceIndex = ENTR_LOST_WOODS_SOUTH_EXIT;
}
static void Center() {
    Reset(16); SpawnRewards();
    Check(spawns == 1, "Rod must spawn in room zero");
    Check(spawned.world.pos.x == 0 && spawned.world.pos.z == 0 && spawned.world.pos.y == 22,
          "Rod must prefer the verified room-zero center over entrance offsets");
    Reset(16); play.roomCtx.curRoom.num = 1; SpawnRewards();
    Check(spawns == 0, "Rod must not spawn in another Lost Woods room");
    Reset(16); gSaveContext.entranceIndex = ENTR_SACRED_FOREST_MEADOW_SOUTH_EXIT;
    SpawnRewards();
    Check(spawns == 1 && spawned.world.pos.x == 0 && spawned.world.pos.z == 0,
          "fixed room-zero Rod must be independent of the scene-entry route");
    Reset(16); centerBlocked = true; SpawnRewards();
    Check(spawns == 1 && std::abs(spawned.world.pos.x) <= 95 && std::abs(spawned.world.pos.z) <= 95 &&
          (spawned.world.pos.x != 0 || spawned.world.pos.z != 0),
          "unsafe center must use a validated nearby fallback");
    std::puts("PASS Rod center, room restriction, route independence and safe fallback");
}
static void Timing() {
    for (unsigned phase = 0; phase < 16; ++phase) {
        Reset(phase); SpawnRewards();
        Check(spawns == 1, "first eligible spawn must not wait for a modulo-sixteen frame");
        for (unsigned frame = phase + 1; frame < phase + 33; ++frame) {
            play.gameplayFrames = frame; SpawnRewards();
        }
        Check(spawns == 1, "repeated scans must not duplicate the actor");
    }
    Reset(1); player.actor.floorPoly = nullptr; SpawnRewards();
    Check(spawns == 0 && !sEntryReady, "entry capture must wait for collision readiness");
    player.actor.floorPoly = &floorPoly; play.gameplayFrames = 2; SpawnRewards();
    Check(spawns == 1, "newly ready floor must trigger the first attempt immediately");
    Reset(1); play.roomCtx.curRoom.num = 1; SpawnRewards();
    Check(spawns == 0, "non-source room arrival must not spawn the Rod");
    play.roomCtx.curRoom.num = 0; play.gameplayFrames = 2; SpawnRewards();
    Check(spawns == 1, "room-zero arrival must not wait for the periodic scan");
    std::puts("PASS all16 frame phases, late floor readiness, room arrival and duplicate suppression");
}
static void Safety() {
    for (int condition = 0; condition < 16; ++condition) {
        Reset(1);
        switch (condition) {
            case 0: active = false; break;
            case 1: play.transitionTrigger = TRANS_TRIGGER_START; break;
            case 2: play.transitionMode = TRANS_MODE_INSTANCE_RUNNING; break;
            case 3: play.pauseCtx.state = 1; break;
            case 4: play.csCtx.state = CS_STATE_SKIPPABLE_EXEC; break;
            case 5: blockingCs = true; break;
            case 6: player.stateFlags1 = PLAYER_STATE1_TALKING; break;
            case 7: sReceiptReward = &rod; break;
            case 8: SaveManager::Instance = nullptr; break;
            case 9: ActorDB::Instance = nullptr; break;
            case 10: collected = true; break;
            case 11: prerequisite = false; break;
            case 12: noFloor = true; break;
            case 13: wet = true; break;
            case 14: wall = true; break;
            case 15: exitFloor = true; break;
        }
        SpawnRewards();
        Check(spawns == 0, "unsafe, collected or ineligible reward must remain unspawned");
        Check(monitors == 1, "receipt monitoring must run even when spawning is blocked");
    }
    Reset(16); sEntryReady = true; sEntryPosition = {120, 0, 400};
    const auto& another = *FindReward("vanilla.nei.deku_leaf");
    play.sceneNum = SCENE_SACRED_FOREST_MEADOW;
    gSaveContext.entranceIndex = ENTR_SACRED_FOREST_MEADOW_SOUTH_EXIT;
    Vec3f point{};
    Check(Anchor(another, NativeBinding(another.location), point) && point.x == 120 && point.z == 400,
          "other reward anchors must retain native entry-relative placement");
    std::puts("PASS native transition/interaction/receipt gates and loaded floor/water/wall/exit controls");
}
static void WallClearance() {
    for (int direction = 0; direction < 4; ++direction) {
        const int axis = direction / 2;
        const s16 normal = direction % 2 ? -32767 : 32767;
        Reset(1); nativeWall = true; wallPoly = {};
        wallPoly.flags_vIA = 0; wallPoly.flags_vIB = 1; wallPoly.vIC = 2;
        if (axis == 0) {
            wallPoly.normal.x = normal;
            wallVertices[0] = {0, -100, -20}; wallVertices[1] = {0, 100, -20}; wallVertices[2] = {0, 0, 20};
        } else {
            wallPoly.normal.z = normal;
            wallVertices[0] = {-20, -100, 0}; wallVertices[1] = {-20, 100, 0}; wallVertices[2] = {20, 0, 0};
        }
        SpawnRewards();
        Check(spawns == 1 && (spawned.world.pos.x != 0 || spawned.world.pos.z != 0),
              "actual native wall intersection at center must select a clear nearby fallback");
        Check(degenerateRays == 0, "preferred center must use meaningful nonzero wall probes");
    }
    std::puts("PASS actual native wall triangles at center in both axes and nonzero clearance rays");
}
int main(int argc, char** argv) {
    const std::string_view selected = argc > 1 ? argv[1] : "all";
    if (selected == "all" || selected == "center") Center();
    if (selected == "all" || selected == "timing") Timing();
    if (selected == "all" || selected == "safety") Safety();
    if (selected == "all" || selected == "wall") WallClearance();
}
