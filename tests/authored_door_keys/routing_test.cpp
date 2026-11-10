// Real native callbacks, matrices, graph and GBI; resource readiness is
// controlled.
#define main RingFixtureMain
#include "tests/shuffle_parity/key_rings/render_test.cpp"
#undef main
#ifdef KEY_RING_TEST_MM
#include "mm/2s2h/Rando/Types.h"
#define COMBO_DOOR_KEY_HOST_MM
PlayState *gPlayState = &play;
using DoorTestItem = RandoItemId;
#else
#include "soh/soh/Enhancements/randomizer/randomizerTypes.h"
using DoorTestItem = RandomizerGet;
#endif
static bool modelBase = true, modelAlt = false, ownerAlt = false;
static const char *expectedOwner = "oot";
static unsigned queries, fallbacks;
static MtxF inherited;
extern "C" int ResourceMgr_IsGiModelAvailableForGame(const char *owner,
                                                     const char *path) {
  ++queries;
  assert(!std::strcmp(owner, expectedOwner));
  assert(std::strstr(path, !std::strcmp(owner, "mm") ? "RustyKeyDL"
                                                     : "OverworldKeyDL"));
  return modelBase || (ownerAlt && modelAlt);
}
extern "C" void GetItem_Draw(PlayState *state, s16 id) {
  assert(state == &play && id == GID_KEY_SMALL);
  assert(!std::memcmp(&inherited, sCurrentMatrix, sizeof(inherited)));
  ++fallbacks;
}
static const char *gHouseKeyDL = "__OTR__objects/object_key/HouseKeyDL";
#include "combo/ComboDoorKeyGi.h"
extern "C" {
#define FrameInterpolation_RecordMatrixRotate1Coord(...) ((void)0)
#define FrameInterpolation_RecordMatrixRotateZYX(...) ((void)0)
#define FrameInterpolation_RecordMatrixReplaceRotation(...) ((void)0)
#ifndef KEY_RING_TEST_MM
void SkinMatrix_SetRotateZYX(MtxF *, s16, s16, s16) {
  assert(false && "NEW matrix branch is outside this APPLY-only recipe");
}
f32 Math_SinS(s16 angle) {
  return std::sin(angle * (3.14159265358979323846f / 32768.f));
}
f32 Math_CosS(s16 angle) {
  return std::cos(angle * (3.14159265358979323846f / 32768.f));
}
#endif
#include "door_matrix.inc"
}
#include "door_callback.inc"
#ifndef KEY_RING_TEST_MM
#include "door_layers.inc"
#else
#include "door_dispatch.inc"
#endif
#include "door_exporter.inc"
// The actual foreign consumer functions operate on this focused descriptor
// view.
struct DoorForeignInfo {
  int count = 0, xluStart = -1, opCount = 0, drawKind = 0;
  float scale = 0;
  const char *dls[CW_DRAW_MAX_DLISTS]{};
  CwDrawOp ops[CW_DRAW_MAX_OPS]{};
  const void *setupDlOpa = nullptr;
  const void *setupDlXlu = nullptr;
  bool hasEnvColor = false;
  uint8_t envColor[4]{}, primColorOpa[4]{};
};
#ifdef KEY_RING_TEST_MM
using ComboForeignDrawInfoOOT = DoorForeignInfo;
#define MM_FOREIGN_PIN_OPA() ((void)0)
#define MM_FOREIGN_PIN_XLU() ((void)0)
Gfx *MM_DrawForeignMagicJarDList(Gfx *, const char *, const uint8_t *) {
  assert(false);
  return nullptr;
}
#else
using ComboForeignDrawInfo = DoorForeignInfo;
#define OOT_FOREIGN_PIN_OPA() ((void)0)
#define OOT_FOREIGN_PIN_XLU() ((void)0)
#define COMBO_FOREIGN_MTX(disp)                                                \
  gSPMatrix(disp,                                                              \
            Matrix_NewMtx(play->state.gfxCtx, (char *)__FILE__, __LINE__),     \
            G_MTX_MODELVIEW | G_MTX_LOAD)
#endif
#include "door_consumer.inc"
static void ResetDoor() {
  Reset();
  Vec3f axis{0, 1, 0};
#ifdef KEY_RING_TEST_MM
  Matrix_RotateAxisF(.63f, &axis, MTXMODE_APPLY);
#else
  Matrix_RotateAxis(.63f, &axis, MTXMODE_APPLY);
#endif
  inherited = *sCurrentMatrix;
}
struct Key {
  DoorTestItem item;
  ComboDoorKeyGi::Origin origin;
};
static constexpr Key keys[] = {
#ifdef KEY_RING_TEST_MM
#define RUSTY_KEY(game, key, rg, ri, flag, name)                               \
  {ri, ComboDoorKeyGi::Origin::game},
#else
#define RUSTY_KEY(game, key, rg, ri, flag, name)                               \
  {rg, ComboDoorKeyGi::Origin::game},
#endif
#include "combo/rando/RustyDoorKeyCatalog.inc"
#undef RUSTY_KEY
};
static void Draw(DoorTestItem item) {
#ifdef KEY_RING_TEST_MM
  DispatchNativeDoor(item);
#else
  GetItemEntry entry{};
  entry.drawItemId = item;
  Randomizer_DrawOverworldKey(&play, &entry);
#endif
}
static void CheckModel(const Key &key, bool authored) {
  const char *suffix = key.origin == ComboDoorKeyGi::Origin::MM
                           ? "objects/combo_door_keys/RustyKeyDL"
                           : "objects/combo_door_keys/OverworldKeyDL";
  unsigned count = 0;
  MtxF active{};
  for (auto *p = opaque; p < graphics.polyOpa.p; ++p) {
    if (p->words.w0 >> 24 == G_MTX)
      std::memcpy(&active, reinterpret_cast<const void *>(p->words.w1),
                  sizeof(active));
    if (p->words.w0 >> 24 != G_DL_OTR_FILEPATH)
      continue;
    const char *path = reinterpret_cast<const char *>(p->words.w1);
    if (!std::strstr(path, suffix))
      continue;
    ++count;
    assert(authored);
    const float c = .7071067811865475f;
    const float expected[9] = {.25f * c * (inherited.xx - inherited.xy),
                               .25f * c * (inherited.xx + inherited.xy),
                               .25f * inherited.xz,
                               .25f * c * (inherited.yx - inherited.yy),
                               .25f * c * (inherited.yx + inherited.yy),
                               .25f * inherited.yz,
                               .25f * c * (inherited.zx - inherited.zy),
                               .25f * c * (inherited.zx + inherited.zy),
                               .25f * inherited.zz};
    const float actual[9] = {active.xx, active.xy, active.xz,
                             active.yx, active.yy, active.yz,
                             active.zx, active.zy, active.zz};
    for (int i = 0; i < 9; ++i)
      assert(std::abs(actual[i] - expected[i]) < 1e-6);
    assert(active.xw == 7 && active.yw == 11 && active.zw == -13);
#ifdef COMBO_BUILD
    assert(!std::strncmp(path,
                         key.origin == ComboDoorKeyGi::Origin::MM
                             ? "__OTR__@mm:"
                             : "__OTR__@oot:",
                         key.origin == ComboDoorKeyGi::Origin::MM ? 11 : 12));
#else
    assert(!std::strncmp(path, "__OTR__objects/", 15));
#endif
  }
  assert(count == unsigned(authored));
}
int main() {
  static_assert(std::size(keys) == 66);
  int mm = 0, oot = 0;
  for (const auto &key : keys) {
    const bool isMm = key.origin == ComboDoorKeyGi::Origin::MM;
    isMm ? ++mm : ++oot;
    expectedOwner = isMm ? "mm" : "oot";
    assert(ComboDoorKeyGi::ItemOrigin(key.item) == key.origin);
    for (int mode = 0; mode < 4; ++mode) {
      modelBase = mode == 0;
      modelAlt = mode >= 2;
      ownerAlt = mode == 3;
      const bool ready = modelBase || (modelAlt && ownerAlt);
      ResetDoor();
      const auto before = *sCurrentMatrix;
      const auto fallbackBefore = fallbacks;
      Draw(key.item);
      CheckModel(key, ready);
      assert(sCurrentMatrix == sMatrixStack &&
             !std::memcmp(&before, sCurrentMatrix, sizeof(before)) &&
             !openDepth);
#ifndef KEY_RING_TEST_MM
      assert(fallbacks == fallbackBefore + unsigned(!ready && isMm));
      if (!ready && !isMm) {
        bool house = false;
        MtxF fallbackMatrix{};
        for (auto *p = opaque; p < graphics.polyOpa.p; ++p) {
          if (p->words.w0 >> 24 == G_MTX)
            std::memcpy(&fallbackMatrix,
                        reinterpret_cast<const void *>(p->words.w1),
                        sizeof(fallbackMatrix));
          if (p->words.w0 >> 24 == G_DL_OTR_FILEPATH)
            house |= reinterpret_cast<const char *>(p->words.w1) == gHouseKeyDL;
        }
        assert(!std::memcmp(&fallbackMatrix, &inherited, sizeof(inherited)));
        assert(house);
      }
#endif
      CwItemDrawInfo recipe{};
      const bool exported = DescribeExportedDoor(key.item, &recipe);
#ifdef KEY_RING_TEST_MM
      assert(exported == ready);
#else
      assert(exported == (ready || !isMm));
      if (!ready && !isMm)
        assert(recipe.dlists[0] == gHouseKeyDL && recipe.scale == 0);
#endif
      assert(recipe.stateDependent == 2);
      if (ready) {
        assert(recipe.dlistCount == 1 && recipe.xluStartIndex == -1);
#ifdef KEY_RING_TEST_MM
        assert(recipe.drawKind == CW_DRAW_KIND_OPS && recipe.scale == 0 &&
               recipe.opCount == 9);
#else
        assert(recipe.drawKind == CW_DRAW_KIND_SIMPLE && recipe.scale == .25f &&
               recipe.opCount == 1);
#endif
        // Ask the same production selector for the receiving host's supported
        // recipe and execute that host's real consumer, with inherited Y spin.
        CwItemDrawInfo replay{};
#ifdef KEY_RING_TEST_MM
        assert(ComboDoorKeyGi::Describe(key.origin, &replay, false));
#else
        assert(ComboDoorKeyGi::Describe(key.origin, &replay, true));
#endif
        DoorForeignInfo info{};
        info.count = replay.dlistCount;
        info.xluStart = replay.xluStartIndex;
        info.opCount = replay.opCount;
        info.drawKind = replay.drawKind;
        info.scale = replay.scale;
        std::memcpy(info.ops, replay.ops, sizeof(info.ops));
        std::memcpy(info.dls, replay.dlists, sizeof(info.dls));
        ResetDoor();
        const auto replayBefore = *sCurrentMatrix;
#ifdef KEY_RING_TEST_MM
        MM_DrawForeignSimple(&info);
#else
        OOT_DrawForeignOps(&play, &info);
#endif
        CheckModel(key, true);
        assert(
            sCurrentMatrix == sMatrixStack &&
            !std::memcmp(&replayBefore, sCurrentMatrix, sizeof(replayBefore)) &&
            !openDepth);
      }
    }
    modelBase = true;
    ResetDoor();
    available = false;
    Draw(key.item);
    assert(!allocations && graphics.polyOpa.p == opaque);
  }
  assert(mm == 39 && oot == 27);
#ifndef KEY_RING_TEST_MM
  const DoorTestItem registered[] = {
#include "door_registered.inc"
  };
  assert(std::size(registered) == 66);
  for (const auto &key : keys) {
    int occurrences = 0;
    for (auto item : registered)
      occurrences += item == key.item;
    assert(occurrences == 1);
  }
#endif
  const auto before = queries;
#ifdef KEY_RING_TEST_MM
  for (auto id : {RI_WOODFALL_SMALL_KEY, RI_WOODFALL_BOSS_KEY,
                  RI_WOODFALL_KEY_RING, RI_SKELETON_KEY}) {
#else
  for (auto id : {RG_FOREST_TEMPLE_SMALL_KEY, RG_FOREST_TEMPLE_BOSS_KEY,
                  RG_FOREST_TEMPLE_KEY_RING, RG_SKELETON_KEY}) {
#endif
    assert(ComboDoorKeyGi::ItemOrigin(id) == ComboDoorKeyGi::Origin::None);
    CwItemDrawInfo exported{};
    assert(!DescribeExportedDoor(id, &exported));
    CwItemDrawInfo recipe{};
    assert(!ComboDoorKeyGi::Describe(ComboDoorKeyGi::ItemOrigin(id), &recipe));
  }
  assert(queries == before);
  std::puts("PASS real host door callbacks: 39 MM + 27 OoT identities/aliases, "
            "owner/.25 scale/-45deg local roll, real OPS/SIMPLE replay, "
            "fallback, Alt retry, arena denial");
}
