#define main KeyRingFoundationMain
#include "../key_rings/native_test.cpp"
#undef main
#include "2s2h/CustomItem/CustomItem.h"
#include "Rando/MMEnvironment.h"
#include "Rando/VanillaCompatibility.h"
#include "rando/SongEvents.h"
#include <cstdarg>
#include <functional>
#include <map>
#include <set>
#include "src/overlays/actors/ovl_Obj_Mure2/z_obj_mure2.h"
#include "src/overlays/actors/ovl_Obj_Bean/z_obj_bean.h"
#include "src/overlays/actors/ovl_Obj_Swprize/z_obj_swprize.h"
extern "C" { void ObjSwprize_DoNothing(ObjSwprize*,PlayState*); void func_80C25640(ObjSwprize*); void func_80C25654(ObjSwprize*,PlayState*); }
#include "rando/CrossEntranceBoss.h"
#include "2s2h/FleetShipCombo/FleetComboIds.h"
namespace Rando::ActorBehavior {
void InitBgIcicleBehavior();
}
namespace Hooks {
std::map<int, std::function<void(Actor *, bool *)>> ShouldActorInit;
std::map<int, std::function<void(Actor *)>> OnActorUpdate, OnActorDestroy;
std::function<void(s8, s8)> scene;
std::map<int, std::function<void(bool *, va_list)>> behaviors;
} // namespace Hooks
#undef COND_ID_HOOK
#undef COND_HOOK
#undef COND_VB_SHOULD
#define COND_ID_HOOK(kind, id, cond, ...) Hooks::kind[id] = __VA_ARGS__
#define COND_HOOK(kind, cond, ...) Hooks::scene = __VA_ARGS__
#define COND_VB_SHOULD(id, cond, ...)                                          \
  Hooks::behaviors[id] = [](bool *should, va_list args) __VA_ARGS__
static std::map<const Actor *, s16> indices;
static std::map<const void *, RandoCheckId> checkIds;
extern "C" {
s16 currentActorListIndex = -1;
int16_t GetActorListIndex(const Actor *actor) {
  return indices.contains(actor) ? indices[actor] : -1;
}
void SetActorListIndex(const Actor *actor, int16_t index) {
  indices[actor] = index;
}
}
namespace Rando::ActorBehavior {
RandoCheckId GetObjectRandoCheckId(void *actor) {
  return checkIds.contains(actor) ? checkIds[actor] : RC_UNKNOWN;
}
void SetObjectRandoCheckId(const void *actor, RandoCheckId id) {
  checkIds[actor] = id;
}
} // namespace Rando::ActorBehavior
static bool failAllocation;
static int allocations;
static EnItem00 itemStorage[1024];
extern "C" Actor *Actor_Spawn(ActorContext *, PlayState *, s16 id, f32 x, f32 y,
                              f32 z, s16 rx, s16 ry, s16 rz, s32 params) {
  ++allocations;
  if (failAllocation)
    return nullptr;
  REQUIRE(allocations < 1024);
  auto &item = itemStorage[allocations];
  item = {};
  item.actor.id = id;
  item.actor.world.pos = {x, y, z};
  item.actor.home.rot = {rx, ry, rz};
  item.actor.params = params;
  return &item.actor;
}
namespace Rando {
void DrawItem(RandoItemId, RandoCheckId, Actor *) {}
RandoItemId ConvertItem(RandoItemId id, RandoCheckId) { return id; }
} // namespace Rando
extern "C" void Matrix_Scale(f32, f32, f32, MatrixMode) {}
#include "2s2h/GameInteractor/GameInteractor_VanillaBehavior.h"
#include "environment_callbacks.inc"
#include "environment_json.inc"
#include "environment_spawn.inc"
#include "src/overlays/actors/ovl_Bg_Icicle/z_bg_icicle.h"
static int vanillaDrops, kills;
extern "C" {
bool GameInteractor_Should(GIVanillaBehavior id, uint32_t result, ...) {
  REQUIRE(Hooks::behaviors.contains(id));
  bool should = result;
  va_list args;
  va_start(args, result);
  Hooks::behaviors[id](&should, args);
  va_end(args);
  return should;
}
void SoundSource_PlaySfxAtFixedWorldPos(PlayState *, Vec3f *, u32, u16) {}
f32 Rand_CenteredFloat(f32) { return 0; }
f32 Rand_ZeroOne() { return 0; }
void EffectSsEnIce_Spawn(PlayState *, Vec3f *, f32, Vec3f *, Vec3f *,
                         Color_RGBA8 *, Color_RGBA8 *, s32) {}
void Item_DropCollectibleRandom(PlayState *, Actor *, Vec3f *, s16) {
  ++vanillaDrops;
}
Actor *Item_DropCollectible(PlayState *, Vec3f *, u32) {
  ++vanillaDrops;
  return nullptr;
}
s32 func_800A8150(s32) { return 0; }
void Actor_Kill(Actor *a) {
  ++kills;
  a->update = nullptr;
}
void DynaPoly_EnableCollision(PlayState *, DynaCollisionContext *, s32) {}
void Actor_MoveWithGravity(Actor *) {}
void Actor_UpdateBgCheckInfo(PlayState *, Actor *, f32, f32, f32, u32) {}
s32 CollisionCheck_SetAT(PlayState *, CollisionCheckContext *, Collider *) {
  return 0;
}
}
void BgIcicle_Regrow(BgIcicle *, PlayState *) {}
#include "environment_native_break.inc"

#include "native_services.inc"

static bool Break(Actor *actor, ...) {
  bool should = true;
  va_list args;
  va_start(args, actor);
  Hooks::behaviors[VB_ICICLE_DROP_COLLECTIBLE](&should, args);
  va_end(args);
  return should;
}
static bool EmitBreak(Actor *actor) { return Break(nullptr, actor); }
static void NativeAssignIndex(Actor *actor) {
#include "environment_actor_index.inc"
}
#include "environment_slots.inc"
namespace Rando::Logic {
uint64_t gCurrentRegionTime;
}
bool HaveEnemySoul(ActorId) { std::abort(); }
extern "C" unsigned char ExtEquip_HasItem(short, unsigned char) { return 0; }
static void TestActualEnvironmentalRules() {
  using namespace Rando;
  using namespace Rando::Logic;
  const std::map<RandoCheckId, std::pair<std::function<bool()>, std::string>>
      rules = {
#include "environment_rules.inc"
      };
  REQUIRE(rules.size() == 7);
  auto reset = []() {
    gSaveContext = {};
    gSaveContext.save.playerForm = PLAYER_FORM_HUMAN;
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    memset(gSaveContext.save.saveInfo.inventory.items, ITEM_NONE,
           sizeof(gSaveContext.save.saveInfo.inventory.items));
  };
  reset();
  for (auto &[id, rule] : rules)
    REQUIRE(!rule.first());
  for (int item : {ITEM_BOW, ITEM_HOOKSHOT, ITEM_DEKU_STICK, ITEM_DEKU_NUT, ITEM_MASK_DEKU, ITEM_MASK_GORON,
                   ITEM_MASK_ZORA}) {
    reset();
    INV_CONTENT(item) = item;
    for (auto &[id, rule] : rules)
      REQUIRE(rule.first());
    INV_CONTENT(item) = ITEM_NONE;
    for (auto &[id, rule] : rules)
      REQUIRE(!rule.first());
  }
  // Native nut-bag ownership still gates the actual HAS_ITEM predicate.
  reset();
  INV_CONTENT(ITEM_DEKU_NUT) = ITEM_DEKU_NUT;
  RANDO_SAVE_OPTIONS[RO_SHUFFLE_DEKU_NUT_BAG] = 1;
  for (auto &[id, rule] : rules)
    REQUIRE(!rule.first());
}
#include "all_families.inc"
#include "native_transition_tests.inc"
#include "all_rules_tests.inc"
#include "source_eligibility_tests.inc"

int main() {
  if (std::getenv("MM_ENV_RED_SPAWN")) {
    static PlayState allocationPlay{};
    gPlayState = &allocationPlay;
    failAllocation = true;
    REQUIRE(CustomItem::Spawn(
                0, 0, 0, 0, 0, 0, [](Actor *, PlayState *) {}, nullptr) ==
            nullptr);
    return 0;
  }
  using namespace Rando;
  TestAllNativeEnvironmentRules();
  TestNativeEnvironmentTransitions();
  TestAllEnvironmentFamilies();
  TestActualSourceAvailability();
  TestActualEnvironmentalRules();
  const RandoCheckType iconIndices[] = {
#include "environment_category_indices.inc"
  };
  REQUIRE(std::size(iconIndices) == RCTYPE_MAX);
  for (int index = 0; index < RCTYPE_MAX; ++index)
    REQUIRE(iconIndices[index] == index);
  REQUIRE(StaticData::Options.at(RO_SHUFFLE_ICICLE_DROPS).defaultValue == 0);
  auto &region = Logic::Regions[static_cast<RandoRegionId>(0)];
  for (const auto &[id, row] : StaticData::Checks)
    if (id != RC_UNKNOWN)
      region.checks[id] = {[] { return true; },
                           "catalogue membership boundary"};
  size_t baselineItems = 0;
  for (bool on : {false, true})
    for (bool excludeAll : {false, true}) {
      excluded.clear();
      if (excludeAll)
        for (auto &p : MMEnvironment::Sources)
          excluded.push_back(p.check);
      std::sort(excluded.begin(), excluded.end());
      excluded.erase(std::unique(excluded.begin(),excluded.end()),excluded.end());
      auto save = std::make_unique<RandoSaveInfo>();
      for (auto &[id, row] : StaticData::Options)
        save->randoSaveOptions[id] = row.defaultValue;
      save->randoSaveOptions[RO_SHUFFLE_ICICLE_DROPS] = on;
      std::vector<RandoCheckId> checks;
      std::vector<RandoItemId> items;
      Logic::GeneratePools(*save, checks, items);
      if (!on && !excludeAll)
        baselineItems = items.size();
      REQUIRE(items.size() == baselineItems + ((on && !excludeAll) ? 25 : 0));
      for (auto &p : MMEnvironment::Icicles) {
        REQUIRE(std::count(checks.begin(), checks.end(), p.check) ==
                int(on && !excludeAll));
        if (!on || excludeAll)
          REQUIRE(!save->randoSaveChecks[p.check].shuffled);
        auto &row = StaticData::Checks.at(p.check);
        REQUIRE(row.sceneId == SCENE_HAKUGIN);
        REQUIRE(row.randoCheckType == RCTYPE_ICICLE);
      }
    }
  static PlayState play{};
  gPlayState = &play;
  play.sceneId = SCENE_HAKUGIN;
  ActorBehavior::InitBgIcicleBehavior();
  for (auto &p : MMEnvironment::Icicles) {
    gSaveContext = {};
    Hooks::scene(0, 0);
    checkIds.clear();
    indices.clear();
    auto &save = RANDO_SAVE_CHECKS[p.check];
    Actor owner{};
    owner.id = ACTOR_BG_ICICLE;
    owner.room = p.room;
    currentActorListIndex = p.index;
    NativeAssignIndex(&owner);
    REQUIRE(GetActorListIndex(&owner) == p.index);
    REQUIRE(currentActorListIndex == -1);
    Actor dynamic{};
    NativeAssignIndex(&dynamic);
    REQUIRE(GetActorListIndex(&dynamic) == -1);
    bool should = true;
    Hooks::ShouldActorInit[ACTOR_BG_ICICLE](&owner, &should);
    REQUIRE(EmitBreak(&owner));
    REQUIRE(allocations == 0); // default-off native control
    save.shuffled = true;
    Hooks::ShouldActorInit[ACTOR_BG_ICICLE](&owner, &should);
    REQUIRE(ActorBehavior::GetObjectRandoCheckId(&owner) == p.check);
    failAllocation = true;
    REQUIRE(!EmitBreak(&owner));
    REQUIRE(!save.eligible);
    REQUIRE(activeDrops.empty());
    REQUIRE(pendingDrops.size() == 1);
    // A failed allocation must never retry at stale room-zero coordinates after
    // crossing to room one.
    failAllocation = false;
    play.roomCtx.curRoom.num = p.room + 1;
    int beforeRoomChange = allocations;
    Hooks::OnActorUpdate[ACTOR_PLAYER](nullptr);
    REQUIRE(allocations == beforeRoomChange);
    REQUIRE(activeDrops.empty());
    REQUIRE(pendingDrops.empty());
    REQUIRE(!save.eligible);
    // Returning does not materialize an old pending reward. The renewable
    // physical source must break again.
    play.roomCtx.curRoom.num = p.room;
    Hooks::OnActorUpdate[ACTOR_PLAYER](nullptr);
    REQUIRE(allocations == beforeRoomChange);
    failAllocation = true;
    REQUIRE(!EmitBreak(&owner));
    REQUIRE(pendingDrops.size() == 1);
    REQUIRE(!save.eligible);
    failAllocation = false;
    Hooks::OnActorUpdate[ACTOR_PLAYER](nullptr);
    REQUIRE(activeDrops.size() == 1);
    REQUIRE(pendingDrops.empty());
    int count = allocations;
    REQUIRE(!EmitBreak(&owner));
    REQUIRE(allocations == count); // regrow/duplicate event
    Actor *drop = activeDrops.at(p.check);
    auto *item = reinterpret_cast<EnItem00 *>(drop);
    item->actionFunc(item, &play);
    REQUIRE(save.eligible);
    // Receipt queue owns permanent receipt. Execute native serialized saved
    // state + native cycle-reset hooks.
    save.obtained = true;
    save.cycleObtained = true;
    save.eligible = false;
    nlohmann::json encoded = save;
    save = encoded.get<RandoSaveCheck>();
    REQUIRE(save.obtained);
    MiscBehavior::BeforeEndOfCycleSave();
    MiscBehavior::AfterEndOfCycleSave();
    REQUIRE(save.obtained);
    REQUIRE(!save.cycleObtained);
    Hooks::OnActorDestroy[ACTOR_EN_ITEM00](drop);
    Hooks::scene(0, 0);
    REQUIRE(EmitBreak(&owner));
    REQUIRE(allocations == count);
    // Unload does not emit a break or award. Scene restores an uncollected
    // physical check.
    save.obtained = false;
    save.cycleObtained = false;
    save.eligible = false;
    Hooks::scene(0, 0);
    count = allocations;
    Hooks::OnActorDestroy[ACTOR_EN_ITEM00](drop);
    REQUIRE(allocations == count);
    REQUIRE(!save.eligible);
    allocations = 0;
  }
  for (int scene : {SCENE_HAKUGIN, SCENE_INISIE_N})
    for (int room : {-1, 1, 20})
      for (int index : {-1, 0, 6, 100})
        REQUIRE(MMEnvironment::IdentifyIcicle(scene, room, index) ==
                RC_UNKNOWN);

  // Actual native attacked/fall transition bodies: no destruction/unload
  // inference.
  for (int mode : {ICICLE_STALAGMITE_RANDOM_DROP, ICICLE_STALAGMITE_FIXED_DROP,
                   ICICLE_STALACTITE_REGROW}) {
    for (bool shuffled : {false, true}) {
      gSaveContext = {};
      Hooks::scene(0, 0);
      checkIds.clear();
      BgIcicle ice{};
      ice.dyna.actor.id = ACTOR_BG_ICICLE;
      ice.dyna.actor.room = 0;
      ice.dyna.actor.params = mode;
      auto check = MMEnvironment::Icicles[0].check;
      auto &saved = RANDO_SAVE_CHECKS[check];
      saved.shuffled = shuffled;
      currentActorListIndex = 0;
      NativeAssignIndex(&ice.dyna.actor);
      bool should = true;
      Hooks::ShouldActorInit[ACTOR_BG_ICICLE](&ice.dyna.actor, &should);
      int before = vanillaDrops;
      int spawns = allocations;
      ice.collider.base.acFlags = AC_HIT;
      BgIcicle_UpdateAttacked(&ice, &play);
      REQUIRE(vanillaDrops ==
              before + int(!shuffled && mode != ICICLE_STALACTITE_REGROW));
      REQUIRE(allocations == spawns + int(shuffled && mode != ICICLE_STALACTITE_REGROW));
      REQUIRE(!saved.eligible);
      if (mode == ICICLE_STALACTITE_REGROW) {
        int count = allocations;
        ice.dyna.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
        BgIcicle_Fall(&ice, &play);
        REQUIRE(allocations == count);
      }
    }
  }
  puts(
      "PASS native MM icicle identities/pools, actual spawn allocation-failure "
      "recovery, real callbacks and permanent JSON/cycle receipt ownership");
}
