#include "global.h"
#include "soh/ShipUtils.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/Enhancements/enhancementTypes.h"
#include "src/overlays/actors/ovl_En_Tp/z_en_tp.h"
#include "src/overlays/actors/ovl_En_Skb/z_en_skb.h"
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <limits>

SaveContext gSaveContext{};
static bool masterQuest;
static int randomizerMode = ENEMY_RANDOMIZER_RANDOM_SEEDED;
static uint64_t seedValue;
static int spawnCount, raycastCount;
static f32 floorHeight = BGCHECK_Y_MIN;
static Vec3f raycastPosition;
static Actor spawned;
static Vec3s spawnedRot;

extern "C" int32_t CVarGetInteger(const char* name, int32_t fallback) {
    return std::strstr(name, "RandomizedEnemies") ? randomizerMode : fallback;
}
extern "C" uint32_t ResourceMgr_IsSceneMasterQuest(s16) { return masterQuest; }
extern "C" f32 BgCheck_AnyRaycastFloor1(CollisionContext*, CollisionPoly*, Vec3f* position) {
    ++raycastCount;
    raycastPosition = *position;
    return floorHeight;
}
extern "C" Actor* Actor_Spawn(ActorContext*, PlayState*, s16 id, f32 x, f32 y, f32 z,
                               s16 rx, s16 ry, s16 rz, s16 params) {
    ++spawnCount;
    spawned.id = id;
    spawned.params = params;
    spawned.world.pos = { x, y, z };
    spawnedRot = { rx, ry, rz };
    return &spawned;
}
namespace ShipUtils {
void RandInit(uint64_t seed, uint64_t*) { seedValue = seed; }
uint32_t Random(uint32_t minimum, uint32_t, uint64_t*) { return minimum; }
}
namespace Rando {
class Context {
  public:
    static Context* GetInstance() { static Context value; return &value; }
    u32 GetSeed() const { return 77; }
};
}
#define CVAR_ENEMY_RANDOMIZER_NAME CVAR_ENHANCEMENT("RandomizedEnemies")
#define CVAR_ENEMY_RANDOMIZER_DEFAULT ENEMY_RANDOMIZER_OFF
#define CVAR_ENEMY_RANDOMIZER_VALUE CVarGetInteger(CVAR_ENEMY_RANDOMIZER_NAME, CVAR_ENEMY_RANDOMIZER_DEFAULT)
#include "enemy_randomizer_production.inc"

static Actor* SpawnEntry(PlayState* play, ActorEntry& entry) {
    bool should = true;
    Actor* result = nullptr;
    ProductionSpawnEntry(&should, &play->actorCtx, &entry, play, &result);
    assert(!should);
    return result;
}

int main() {
    static PlayState play{};
    play.sceneNum = SCENE_HYRULE_FIELD;
    play.roomCtx.curRoom.num = 0;
    gSaveContext.linkAge = LINK_AGE_ADULT;
    gSaveContext.ship.stats.fileCreatedAt = 1234;
    selectedEnemyList = { { "fixture", "Keese", ACTOR_EN_FIREFLY, 2 } };

    ActorEntry entry{};
    entry.id = ACTOR_OBJ_TSUBO;
    entry.params = 24597;
    entry.pos = { 180000.25f, 90000.5f, -170000.75f };
    entry.rot = { 100, -200, 300 };
    const auto original = entry;
    assert(SpawnEntry(&play, entry) == &spawned);
    assert(spawned.world.pos.x == 180000.25f && spawned.world.pos.y == 90000.5f &&
           spawned.world.pos.z == -170000.75f);
    assert(spawned.params == 24067 && raycastCount == 0);
    assert(spawnedRot.x == 100 && spawnedRot.y == -200 && spawnedRot.z == 300);
    assert(std::memcmp(&entry, &original, sizeof(entry)) == 0);

    entry.id = ACTOR_EN_AM;
    entry.params = -1;
    floorHeight = 91234.625f;
    assert(SpawnEntry(&play, entry) == &spawned);
    assert(spawned.id == ACTOR_EN_FIREFLY && spawned.params == 2);
    assert(spawned.world.pos.x == 180000.25f && spawned.world.pos.y == 91309.625f &&
           spawned.world.pos.z == -170000.75f);
    assert(raycastPosition.x == 180000.25f && raycastPosition.y == 90050.5f &&
           raycastPosition.z == -170000.75f);
    assert(spawnedRot.x == 0 && spawnedRot.y == -200 && spawnedRot.z == 300);
    const u32 expectedSeed = play.sceneNum + ACTOR_EN_AM + 180000 + 91234 - 170000 + 100 - 200 + 300 - 1;
    assert(seedValue == uint64_t(expectedSeed) + 1234);

    // Vanilla integer entries retain their seeded selection input even though
    // fractional floor height is now preserved through the native spawn.
    entry.pos = { -123, 500, 789 };
    floorHeight = 123.875f;
    assert(SpawnEntry(&play, entry) == &spawned && spawned.world.pos.y == 198.875f);
    const u32 legacySeed = play.sceneNum + ACTOR_EN_AM - 123 + 123 + 789 + 100 - 200 + 300 - 1;
    assert(seedValue == uint64_t(legacySeed) + 1234);
    floorHeight = BGCHECK_Y_MIN;
    entry.pos = { 85000.75f, 100000.5f, -123456.25f };
    assert(SpawnEntry(&play, entry) == &spawned && spawned.world.pos.y == 100075.5f);

    // Existing progression/object restrictions and timer fixes remain intact.
    const int previousSpawns = spawnCount;
    play.sceneNum = SCENE_GRAVEYARD;
    entry.id = ACTOR_EN_FIREFLY;
    assert(SpawnEntry(&play, entry) == nullptr && spawnCount == previousSpawns);
    play.sceneNum = SCENE_JABU_JABU;
    play.roomCtx.curRoom.num = 12;
    entry.id = ACTOR_OBJ_ROOMTIMER;
    entry.params = 30760;
    assert(SpawnEntry(&play, entry) == &spawned && spawned.params == ((30760 & ~0x3FF) | 120));
    masterQuest = true;
    const EnemyEntry bari{ "fixture", "Bari", ACTOR_EN_VALI, -1 };
    assert(!IsEnemyAllowedToSpawn(SCENE_FIRE_TEMPLE, 15, bari, 64, false));
    assert(!IsEnemyAllowedToSpawn(SCENE_FIRE_TEMPLE, 15, bari, 64.625f, false));
    assert(IsEnemyAllowedToSpawn(SCENE_FIRE_TEMPLE, 15, bari, 65600.25f, false));
    assert(!IsEnemyAllowedToSpawn(SCENE_HYRULE_FIELD, 0, bari, 64, true));
    masterQuest = false;
    assert(!IsEnemyFoundToRandomize(SCENE_GERUDO_TRAINING_GROUND, 2, ACTOR_EN_WALLMAS, 1, -2345.75f));
    assert(IsEnemyFoundToRandomize(SCENE_GERUDO_TRAINING_GROUND, 2, ACTOR_EN_WALLMAS, 1, -67881.75f));
    play.sceneNum = SCENE_JABU_JABU;
    play.roomCtx.curRoom.num = 11;
    entry.id = ACTOR_EN_RR;
    entry.params = 0;
    entry.pos = { 1003.5f, 1000.75f, -99999.25f };
    assert(SpawnEntry(&play, entry) == &spawned);
    assert(spawned.world.pos.x == 928.5f && spawned.world.pos.y == 875.75f &&
           spawned.world.pos.z == -99999.25f);
    play.sceneNum = SCENE_GERUDO_TRAINING_GROUND;
    play.roomCtx.curRoom.num = 5;
    entry.id = ACTOR_EN_AM;
    entry.params = -1;
    entry.pos.y = 117.5f;
    assert(SpawnEntry(&play, entry) == &spawned && spawned.world.pos.y == 193.5f);
    assert(EnemyRando_SeedCoordinate(-1.75f) == UINT32_MAX);
    assert(EnemyRando_SeedCoordinate(2147483648.0f) == 0x80000000u);
    assert(EnemyRando_SeedCoordinate(4294967296.0f) == 0);
    assert(EnemyRando_SeedCoordinate(std::numeric_limits<f32>::max()) == 0);

    EnTp tail{};
    tail.actor.world.pos = { 150000.25f, 85000.5f, -180000.75f };
    tail.collider.elements = tail.colliderItems;
    tail.colliderItems[0].dim.modelSphere.radius = 123.5f;
    tail.colliderItems[0].dim.scale = 2.5f;
    EnemyRando_SetColliderPosTp(&tail);
    assert(tail.colliderItems[0].dim.worldSphere.center.x == 150000.25f &&
           tail.colliderItems[0].dim.worldSphere.center.y == 85000.5f &&
           tail.colliderItems[0].dim.worldSphere.center.z == -180000.75f &&
           tail.colliderItems[0].dim.worldSphere.radius == 308.75f);
    EnSkb child{};
    child.actor.world.pos = tail.actor.world.pos;
    child.collider.elements = child.colliderItem;
    for (auto& element : child.colliderItem) {
        element.dim.modelSphere.radius = 60000.5f;
        element.dim.scale = 1.5f;
    }
    EnemyRando_SetColliderPosSkb(&child);
    for (const auto& element : child.colliderItem)
        assert(element.dim.worldSphere.center.x == 150000.25f &&
               element.dim.worldSphere.center.y == 85000.5f &&
               element.dim.worldSphere.center.z == -180000.75f && element.dim.worldSphere.radius == 90000.75f);
    std::puts("PASS Enemy Randomizer: wide fractional enemy/nonenemy spawn, floor offsets, seed inputs, restrictions and colliders");
}
