#include "2s2h/CustomItem/CustomItem.h"
#include "global.h"
#include "mods/forms/custom_forms.h"
#include "mods/items/logic/adult_link_render.h"
#include "variables.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string>
/* CAMERA DATA */
#define RELOAD_PARAMS(camera)                                                  \
  ((camera->animState == 0) || (camera->animState == 10) ||                    \
   (camera->animState == 20))
#define GET_NEXT_RO_DATA(values) ((values++)->val)
#define GET_NEXT_SCALED_RO_DATA(values) (GET_NEXT_RO_DATA(values) * .01f)
#define ADULT_LINK_SCALE_DEFAULT .01f
static PlayState play;
static Player player, peer;
SaveContext gSaveContext{};
PlayState *gPlayState = &play;
static FlexSkeletonHeader skeleton{}, *sSkel = &skeleton;
static u8 sReady = 1, sIsChildRig = 0;
static s32 mode = 1, form = CUSTOM_FORM_NONE;
static f32 modelScale = .01f, modelYOffset = 0;
static bool mario = false, wolf = false;
static int rewards, kills, offers, nativeDraws, adultDraws, collisions;
static f32 callbackScale;
Vec3f D_801EDDD0{}, D_801EDDE0{}, D_801EDDF0{};
extern "C" {
s32 CVarGetInteger(const char *, s32) { return 0; }
s32 AdultLink_IsActive(void) { return mode; }
s32 CustomForms_ActiveForm(void) { return form; }
f32 CVarGetFloat(const char *name, f32 fallback) {
  if (!strcmp(name, "gAdultLink.Scale"))
    return modelScale;
  if (!strcmp(name, "gAdultLink.YOffset"))
    return modelYOffset;
  return fallback;
}
u8 Sm64Mario_ShouldHideLink(void) { return mario; }
u8 WolfLinkForm_IsSelected(void) { return wolf; }
s32 AdultLink_ShouldHide(void) { return mode && sReady; }
void AdultLink_Draw(PlayState *, Player *) { ++adultDraws; }
void Player_DrawGetItem(PlayState *, Player *) { ++nativeDraws; }
void DinFireSword_BeginPlayerDraw(PlayState *, Player *) {}
void DinFireSword_DrawAfterPlayer(PlayState *, Player *) {}
void Actor_SetScale(Actor *a, f32 scale) { a->scale = {scale, scale, scale}; }
void Actor_Kill(Actor *) { ++kills; }
s32 Actor_HasParent(Actor *a, PlayState *) { return a->parent != nullptr; }
s32 Actor_OfferGetItem(Actor *, PlayState *, GetItemId item, f32, f32) {
  assert(item == GI_SHIP);
  ++offers;
  return 1;
}
void Audio_PlaySfx(u16) {}
void Actor_MoveWithGravity(Actor *) { assert(false); }
void Actor_UpdateBgCheckInfo(PlayState *, Actor *, f32, f32, f32, u32) {
  assert(false);
}
void Collider_UpdateCylinder(Actor *, ColliderCylinder *) { assert(false); }
s32 CollisionCheck_SetAC(PlayState *, CollisionCheckContext *, Collider *) {
  assert(false);
  return 0;
}
void GetItem_Draw(PlayState *, s16) {
  Vec3f origin{}, unit{1, 0, 0}, a{}, b{};
  Matrix_MultVec3f(&origin, &a);
  Matrix_MultVec3f(&unit, &b);
  callbackScale = sqrtf(SQ(b.x - a.x) + SQ(b.y - a.y) + SQ(b.z - a.z));
}
void *THA_AllocTailAlign16(TwoHeadArena *, size_t) {
  static MtxF matrices[40];
  return matrices;
}
void FrameInterpolation_RecordMatrixMultVec3f(Vec3f *, Vec3f *) {}
void FrameInterpolation_RecordMatrixTranslate(f32, f32, f32, u8) {}
void FrameInterpolation_RecordMatrixScale(f32, f32, f32, u8) {}
void FrameInterpolation_RecordMatrixRotate1Coord(u32, f32, u8) {}
void FrameInterpolation_RecordMatrixSetTranslateRotateYXZ(f32, f32, f32,
                                                          Vec3s *) {}
f32 Math_FAtan2F(f32 y, f32 x) { return atan2f(y, x); }
f32 Math_FTanF(f32 x) { return tanf(x); }
s16 Math_Atan2S(f32 y, f32 x) {
  return static_cast<s16>(atan2f(y, x) * 32768.f / M_PI);
}
void func_800CBFA4(Camera *, Vec3f *, Vec3f *, s32) { ++collisions; }
s16 func_800CC260(Camera *, Vec3f *, Vec3f *, VecGeo *, Actor **, s16) {
  ++collisions;
  return 0;
}
void func_800CC938(Camera *) {}
s32 Camera_BgCheck(Camera *, Vec3f *, Vec3f *) {
  ++collisions;
  return 0;
}
f32 BgCheck_CameraRaycastFloor2(CollisionContext *, CollisionPoly **, s32 *,
                                Vec3f *) {
  return BGCHECK_Y_MIN;
}
}
static void Reward(EnItem00 *, PlayState *) { ++rewards; }
/* PRODUCTION */
static bool Near(float a, float b) { return fabsf(a - b) < .003f; }
static void Expect(float actual, float expected, const char *what) {
  if (!Near(actual, expected)) {
    fprintf(stderr, "FAIL %s: actual=%f expected=%f\n", what, actual, expected);
    abort();
  }
}
static void Reset() {
  play = {};
  player = {};
  peer = {};
  gSaveContext = {};
  gPlayState = &play;
  InitCameraData();
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
  player.actor.id = ACTOR_PLAYER;
  player.transformation = PLAYER_FORM_HUMAN;
  gSaveContext.save.playerForm = PLAYER_FORM_HUMAN;
  player.actor.world.pos = {10, 100, 20};
  player.actor.scale = {.01f, .01f, .01f};
  player.leftHandWorld.pos = {7, 171, 23};
  player.bodyPartsPos[PLAYER_BODYPART_RIGHT_HAND] = {13, 175, 27};
  player.stateFlags1 = PLAYER_STATE1_400;
  sReady = 1;
  sSkel = &skeleton;
  sIsChildRig = 0;
  mode = 1;
  form = CUSTOM_FORM_NONE;
  modelScale = .01f;
  modelYOffset = 0;
  mario = wolf = false;
  rewards = kills = offers = nativeDraws = adultDraws = collisions = 0;
}
static EnItem00 Receipt(int flags) {
  EnItem00 item{};
  item.actor.world.pos = player.actor.world.pos;
  item.actor.home.rot.x = flags;
  item.actor.parent = &player.actor;
  item.actor.shape.yOffset = 1250;
  item.unk152 = -1;
  item.actionFunc = Reward;
  return item;
}
static Vec3f FinalOrigin(Actor *actor) {
  Matrix_Init(&play.state); // Native matrix stack allocation supplied below.
  DrawReceiptTransform(actor, &play);
  CustomItem00_Draw(actor, &play);
  Vec3f origin{}, world{};
  Matrix_MultVec3f(&origin, &world);
  return world;
}
static void Held(bool adult) {
  if (!adult)
    mode = 0;
  auto item =
      Receipt(CustomItem::GIVE_ITEM_CUTSCENE | CustomItem::HIDE_TILL_OVERHEAD);
  for (int frame = 0; frame < 20; ++frame) {
    CustomItem00_Update(&item.actor, &play);
    assert(item.unk152 == frame && Near(item.actor.scale.y, 0) && rewards == 0);
  }
  CustomItem00_Update(&item.actor, &play);
  assert(rewards == 1 && item.unk152 == 20 && Near(item.actor.scale.y, .007f));
  const auto origin = FinalOrigin(&item.actor);
  Expect(origin.x, 10, "held X");
  Expect(origin.y, adult ? 187 : 153.3f, "held final Actor_Draw origin Y");
  Expect(origin.z, adult ? 28.3f : 20, "held forward Z");
  Expect(callbackScale, .21f, "held composed draw scale");
  CustomItem00_Update(&item.actor, &play);
  assert(rewards == 1);
  player.stateFlags1 = 0;
  CustomItem00_Update(&item.actor, &play);
  assert(kills == 1);
}
static Camera CameraFixture(s16 setting) {
  Camera c{};
  c.play = &play;
  c.focalActor = &player.actor;
  c.setting = setting;
  c.mode = CAM_MODE_NORMAL;
  c.focalActorPosRot = player.actor.world;
  c.at = {10, 144, 20};
  c.eye = c.eyeNext = {10, 144, 120};
  c.dist = 100;
  c.fov = 45;
  c.fovUpdateRate = 1;
  play.view.unk164 = 1;
  c.focalActorFloorHeight = 100;
  return c;
}
static void Item0(bool adult) {
  if (!adult)
    mode = 0;
  auto c = CameraFixture(CAM_SET_ITEM0);
  Camera_KeepOn4(&c);
  auto &ro = c.paramData.keep4.roData;
  auto &rw = c.paramData.keep4.rwData;
  Expect(ro.unk_00, adult ? 6.8f : 2.8f, "ITEM0 target relative offset");
  Expect(ro.unk_04, adult ? 34.f : 39.2f, "ITEM0 radius");
  Expect(ro.unk_08, adult ? -20.f : 25.f, "ITEM0 pitch");
  Expect(ro.unk_18, adult ? 60.f : 45.f, "ITEM0 FOV");
  Expect(D_801EDDD0.y, adult ? 174.8f : 146.8f, "ITEM0 actual target Y");
  assert(ro.timer == 6 && rw.timer == 5 && Near(ro.unk_14, .25f));
  if (adult)
    assert((ro.interfaceFlags & 0xE) == 0xE && rw.unk_10 == 0);
  assert(collisions == 2);
  for (int i = 0; i < 6; ++i)
    Camera_KeepOn4(&c);
  assert(rw.timer == 0 && (c.stateFlags & CAM_STATE_4));
}
static void Chest(bool adult) {
  if (!adult)
    mode = 0;
  auto c = CameraFixture(CAM_SET_LONG_CHEST_OPENING);
  c.animState = 3;
  c.paramData.demo2.rwData.animFrame = 167;
  c.paramData.demo2.rwData.initialAt = player.actor.world.pos;
  c.paramData.demo2.rwData.yawDir = 1;
  Camera_Demo2(&c);
  Expect(c.at.y, adult ? 171.f : 147.f, "large chest final target Y");
  assert(c.animState == 30 && c.paramData.demo2.rwData.animFrame == 168);
}
static void Scaled() {
#ifdef ADULT_ITEM_QUERY
  for (float scale : {.005f, .01f, .02f})
    for (float yOffset : {-8.f, 0.f, 11.f}) {
      Reset();
      modelScale = scale;
      modelYOffset = yOffset;
      auto c = CameraFixture(CAM_SET_ITEM0);
      c.eyeNext.x += 40;
      c.eye = c.eyeNext;
      const auto initial = OLib_Vec3fDiffToVecGeo(&c.at, &c.eyeNext);
      Camera_KeepOn4(&c);
      // Independent numerical donor expectations for each size; YOffset
      // translates only.
      float target = scale == .005f ? 139.1f : scale == .01f ? 174.8f : 246.2f;
      float radius = scale == .005f ? 25.5f : scale == .01f ? 34.f : 51.f;
      Expect(D_801EDDD0.y, target + yOffset, "scaled ITEM0 target");
      Expect(c.paramData.keep4.roData.unk_04, radius,
             "scaled ITEM0 radius unaffected by translation");
      assert(c.paramData.keep4.rwData.unk_10 == initial.yaw);
      assert((c.paramData.keep4.roData.interfaceFlags & ~0xE) ==
             (D_801B5338[6].val & ~0xE));
      c = CameraFixture(CAM_SET_LONG_CHEST_OPENING);
      c.animState = 3;
      c.paramData.demo2.rwData.animFrame = 167;
      c.paramData.demo2.rwData.initialAt = player.actor.world.pos;
      c.paramData.demo2.rwData.yawDir = 1;
      Camera_Demo2(&c);
      Expect(c.at.y,
             (scale == .005f  ? 137.f
              : scale == .01f ? 171.f
                              : 239.f) +
                 yOffset,
             "scaled chest final target");
      // Rendered hands already contain all model placement, scale, and
      // animation offsets.
      player.leftHandWorld.pos = {-20, 42, 70};
      player.bodyPartsPos[PLAYER_BODYPART_RIGHT_HAND] = {4, 88, -10};
      player.actor.shape.rot.y = 0x4000;
      auto item =
          Receipt(CustomItem::GIVE_ITEM_CUTSCENE | CustomItem::KEEP_ON_PLAYER |
                  CustomItem::STOP_BOBBING);
      item.unk152 = 19;
      CustomItem00_Update(&item.actor, &play);
      auto at = FinalOrigin(&item.actor);
      Expect(at.x, -4.7f, "actual rendered hand anchor X");
      Expect(at.y, 79, "actual rendered hand anchor Y");
      Expect(at.z, 30, "actual rendered hand anchor Z");
      Expect(callbackScale, .21f, "scaled adult preserves custom item scale");
      item = Receipt(CustomItem::GIVE_OVERHEAD);
      CustomItem00_Update(&item.actor, &play);
      float bob = Math_SinS(static_cast<s16>(14 * 15000)) * 4.2f;
      Expect(item.actor.world.pos.y,
             (scale == .005f  ? 132.5f
              : scale == .01f ? 165.f
                              : 230.f) +
                 yOffset + bob,
             "scaled overhead world Y");
    }
#endif
}
static void NativeControls() {
#ifdef ADULT_ITEM_QUERY
  for (int variant = 0; variant < 14; ++variant) {
    Reset();
    switch (variant) {
    case 0:
      sReady = 0;
      break;
    case 1:
      sSkel = nullptr;
      break;
    case 2:
      sIsChildRig = 1;
      break;
    case 3:
      mario = true;
      break;
    case 4:
      wolf = true;
      break;
    case 5:
      player.transformation = PLAYER_FORM_DEKU;
      break;
    case 6:
      player.transformation = PLAYER_FORM_GORON;
      break;
    case 7:
      player.transformation = PLAYER_FORM_ZORA;
      break;
    case 8:
      player.transformation = PLAYER_FORM_FIERCE_DEITY;
      break;
    default:
      form = variant - 9;
      break;
    }
    gSaveContext.save.playerForm = player.transformation;
    assert(!AdultLink_UsesAdultItemPresentation(&player));
    auto receipt = [&] {
      auto item =
          Receipt(CustomItem::GIVE_ITEM_CUTSCENE | CustomItem::KEEP_ON_PLAYER |
                  CustomItem::STOP_BOBBING);
      item.unk152 = 19;
      CustomItem00_Update(&item.actor, &play);
      return FinalOrigin(&item.actor);
    };
    auto a = receipt();
    auto camera = CameraFixture(CAM_SET_ITEM0);
    Camera_KeepOn4(&camera);
    auto chest = CameraFixture(CAM_SET_LONG_CHEST_OPENING);
    chest.animState = 3;
    chest.paramData.demo2.rwData.animFrame = 167;
    chest.paramData.demo2.rwData.initialAt = player.actor.world.pos;
    chest.paramData.demo2.rwData.yawDir = 1;
    Camera_Demo2(&chest);
    mode = 0;
    auto b = receipt();
    Expect(a.x, b.x, "control held X");
    Expect(a.y, b.y, "control held Y");
    Expect(a.z, b.z, "control held Z");
    auto baseline = CameraFixture(CAM_SET_ITEM0);
    Camera_KeepOn4(&baseline);
    assert(!memcmp(&camera.paramData, &baseline.paramData,
                   sizeof(camera.paramData)));
    Expect(camera.at.y, baseline.at.y, "control item camera at");
    auto nativeChest = CameraFixture(CAM_SET_LONG_CHEST_OPENING);
    nativeChest.animState = 3;
    nativeChest.paramData.demo2.rwData.animFrame = 167;
    nativeChest.paramData.demo2.rwData.initialAt = player.actor.world.pos;
    nativeChest.paramData.demo2.rwData.yawDir = 1;
    Camera_Demo2(&nativeChest);
    Expect(chest.at.y, nativeChest.at.y, "control chest camera at");
  }
  // Even an eligible adult keeps all other camera settings and peer framing
  // native.
  for (bool peerActor : {false, true})
    for (s16 setting : {CAM_SET_ITEM0, CAM_SET_ITEM1}) {
      Reset();
      auto c = CameraFixture(setting);
      if (peerActor) {
        peer = player;
        c.focalActor = &peer.actor;
      }
      if (!peerActor && setting == CAM_SET_ITEM0)
        continue;
      Camera_KeepOn4(&c);
      mode = 0;
      auto native = CameraFixture(setting);
      if (peerActor)
        native.focalActor = &peer.actor;
      Camera_KeepOn4(&native);
      assert(!memcmp(&c.paramData, &native.paramData, sizeof(c.paramData)));
    }
  for (bool peerActor : {false, true}) {
    Reset();
    auto c =
        CameraFixture(peerActor ? CAM_SET_LONG_CHEST_OPENING : CAM_SET_ITEM0);
    if (peerActor) {
      peer = player;
      c.focalActor = &peer.actor;
    }
    c.animState = 3;
    c.paramData.demo2.rwData.animFrame = 167;
    c.paramData.demo2.rwData.initialAt = player.actor.world.pos;
    c.paramData.demo2.rwData.yawDir = 1;
    Camera_Demo2(&c);
    Expect(c.at.y, 147.f, "Demo2 other setting/peer remains native");
  }
#endif
}
int main(int argc, char **argv) {
  assert(argc == 2);
  Reset();
  std::string test = argv[1];
  if (test == "scale")
    Scaled();
  if (test == "scope")
    NativeControls();
  if (test == "controls") {
    Held(false);
    Reset();
    Item0(false);
    Reset();
    Chest(false);
  }
  if (test == "held")
    Held(true);
  if (test == "item0")
    Item0(true);
  if (test == "chest")
    Chest(true);
  if (test == "native") {
    for (int selectedForm : {CUSTOM_FORM_NONE, CUSTOM_FORM_RITO}) {
      form = selectedForm;
      for (int drawId : {0, 1, 2, 7}) {
        player.getItemDrawIdPlusOne = drawId;
        nativeDraws = 0;
        DrawAdultBranch(&play, &player);
        assert(nativeDraws ==
               ((selectedForm == CUSTOM_FORM_NONE) && (drawId > GID_NONE + 1)));
      }
    }
  }
  if (test == "eligibility") {
#ifdef ADULT_ITEM_QUERY
    assert(AdultLink_UsesAdultPresentation(&player));
    assert(AdultLink_UsesAdultItemPresentation(&player));
    assert(!AdultLink_UsesAdultItemPresentation(nullptr));
    assert(!AdultLink_UsesAdultItemPresentation(&peer));
    for (int f = 0; f < PLAYER_FORM_MAX; ++f) {
      player.transformation = f;
      assert(bool(AdultLink_UsesAdultItemPresentation(&player)) ==
             (f == PLAYER_FORM_HUMAN));
    }
    player.transformation = PLAYER_FORM_HUMAN;
    sReady = 0;
    assert(!AdultLink_UsesAdultItemPresentation(&player));
    sReady = 1;
    sSkel = nullptr;
    assert(!AdultLink_UsesAdultItemPresentation(&player));
    sSkel = &skeleton;
    sIsChildRig = 1;
    assert(!AdultLink_UsesAdultItemPresentation(&player));
    sIsChildRig = 0;
    mode = 0;
    assert(!AdultLink_UsesAdultItemPresentation(&player));
    mode = 1;
    form = CUSTOM_FORM_RITO;
    assert(AdultLink_UsesAdultPresentation(&player));
    assert(!AdultLink_UsesAdultItemPresentation(&player));
    form = CUSTOM_FORM_NONE;
    mario = true;
    assert(!AdultLink_UsesAdultItemPresentation(&player));
    mario = false;
    wolf = true;
    assert(!AdultLink_UsesAdultItemPresentation(&player));
    wolf = false;
#else
    puts("baseline has no narrow ordinary-adult query");
#endif
  }
  if (test == "overhead") {
    auto item = Receipt(CustomItem::GIVE_OVERHEAD);
    CustomItem00_Update(&item.actor, &play);
    Expect(item.actor.world.pos.y,
           165.f + Math_SinS(static_cast<s16>(14 * 15000)) * (14 * .3f),
           "adult overhead lift");
    assert(rewards == 1 && item.unk152 == 14 && Near(item.actor.scale.y, .01f));
    for (int i = 0; i < 14; ++i)
      CustomItem00_Update(&item.actor, &play);
    assert(kills == 1 && rewards == 1);
  }
  puts(("PASS " + test).c_str());
}
