// Native adapter boundary tests. run_native_tests.py inserts the production
// adapter bodies verbatim and links native Player_DrawGetItem/Player_GetHeight
// C bodies against real host structures. Renderer, asset queue, CVar storage,
// adult resource readiness, and model submission are controlled boundaries.
#include "combo/render/ItemGetLightShaft.h"
#include "combo/render/ItemGetLightShaftBridge.h"
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"
#ifdef GI_SHAFT_TEST_MM
#include "2s2h/CustomItem/CustomItem.h"
#include "mods/forms/custom_forms.h"
#include "mods/items/logic/adult_link_render.h"
#endif
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

static PlayState fixturePlay{}, otherPlay{};
static Player fixturePlayer{}, peer{};
static GraphicsContext fixtureGfx{};
static bool hasEnabled = false;
static int enabled = 0;
static int queueResult = -1, queueCalls = 0, modelDraws = 0;
static int submittedDrawId = -1;
static Vec3f submittedReference{};
static Player *expectedModelPlayer = &fixturePlayer;
static std::vector<ItemGetLightShaft::Frame> frames;
static const char *currentCase = "setup";
#ifdef GI_SHAFT_TEST_MM
static constexpr const char *kHost = "mm";
static constexpr u32 kReceiptFlag = PLAYER_STATE1_400;
static RegEditor registers{};
static FlexSkeletonHeader skeleton{}, *sSkel = &skeleton;
static u8 sReady = 1, sIsChildRig = 0;
static s32 adultMode = 0, customForm = CUSTOM_FORM_NONE;
static f32 modelScale = .01f, modelYOffset = 0;
static bool mario = false, wolf = false;
#define ADULT_LINK_SCALE_DEFAULT .01f
#define MARK_NATIVE MMItemGetLightShaft_MarkNative
#else
static constexpr const char *kHost = "soh";
static constexpr u32 kReceiptFlag = PLAYER_STATE1_GETTING_ITEM;
static GameInfo registers{};
static f32 transformHeight = 0;
#define MARK_NATIVE SoHItemGetLightShaft_MarkNative
#endif

static void Check(bool condition, const char *explanation) {
  if (!condition) {
    std::fprintf(stderr, "FAIL [%s][%s] %s\n", kHost, currentCase, explanation);
    std::exit(1);
  }
}
static void Near(float actual, float expected, const char *explanation) {
  if (std::fabs(actual - expected) > .002f) {
    std::fprintf(stderr, "FAIL [%s][%s] %s: actual=%f expected=%f\n", kHost,
                 currentCase, explanation, actual, expected);
    std::exit(1);
  }
}

extern "C" {
PlayState *gPlayState = &fixturePlay;
SaveContext gSaveContext{};
#ifdef GI_SHAFT_TEST_MM
RegEditor *gRegEditor = &registers;
extern Vec3f sPlayerGetItemRefPos;
s32 AdultLink_IsActive(void) { return adultMode; }
s32 CustomForms_ActiveForm(void) { return customForm; }
u8 Sm64Mario_ShouldHideLink(void) { return mario; }
u8 WolfLinkForm_IsSelected(void) { return wolf; }
f32 CVarGetFloat(const char *name, f32 fallback) {
  if (!std::strcmp(name, "gAdultLink.Scale"))
    return modelScale;
  if (!std::strcmp(name, "gAdultLink.YOffset"))
    return modelYOffset;
  return fallback;
}
#else
GameInfo *gGameInfo = &registers;
extern Vec3f sGetItemRefPos;
f32 TransformMasks_GetFormHeight(void) { return transformHeight; }
#endif
int32_t CVarGetInteger(const char *name, int32_t fallback) {
  Check(!std::strcmp(name, ItemGetLightShaft::kCVar),
        "adapter read an unrelated setting");
  return hasEnabled ? enabled : fallback;
}
s32 osRecvMesg(OSMesgQueue *queue, OSMesg *message, s32 flags) {
  Check(queue == &expectedModelPlayer->giObjectLoadQueue &&
            message == nullptr && flags == OS_MESG_NOBLOCK,
        "native wrapper changed the nonblocking GI loading boundary");
  ++queueCalls;
  return queueResult;
}
void Player_DrawGetItemImpl(PlayState *play, Player *player, Vec3f *reference,
                            s32 drawId) {
  Check(play == &fixturePlay && player == expectedModelPlayer,
        "native wrapper changed the model owner");
  Check(!player->giObjectLoading,
        "native wrapper submitted a model before clearing its ready flag");
  ++modelDraws;
  submittedReference = *reference;
  submittedDrawId = drawId;
}
}

#ifdef GI_SHAFT_TEST_MM
// This function is an identity token for MarkCustom; no update policy is faked.
void CustomItem00_Update(Actor *, PlayState *) {}
static void OtherUpdate(Actor *, PlayState *) {}
static void OrdinaryReceiptDraw(Actor *, PlayState *) {}
static void ForeignReceiptDraw(Actor *, PlayState *) {}
/* GI_SHAFT_ADULT_QUERIES */
#endif

static void ItemGetLightShaft_Draw(PlayState *play,
                                   const ItemGetLightShaft::Frame &frame) {
  Check(play == gPlayState, "adapter sent the shaft to another play state");
  frames.push_back(frame);
}

/* GI_SHAFT_NATIVE_ADAPTER */

static void SetNativeId(Player &player, int id) {
#ifdef GI_SHAFT_TEST_MM
  player.getItemDrawIdPlusOne = id;
#else
  player.unk_862 = id;
#endif
}

static void FixtureReset(bool featureEnabled = true) {
  fixturePlay = {};
  otherPlay = {};
  fixturePlayer = {};
  peer = {};
  fixtureGfx = {};
  registers = {};
  gSaveContext = {};
  gPlayState = &fixturePlay;
  fixturePlay.state.gfxCtx = &fixtureGfx;
  fixturePlay.state.running = true;
  fixturePlay.gameplayFrames = 100;
  fixturePlayer.actor.id = ACTOR_PLAYER;
  fixturePlayer.actor.world.pos = {10, 100, 20};
  fixturePlayer.stateFlags1 = kReceiptFlag;
  SetNativeId(fixturePlayer, 7);
#ifdef GI_SHAFT_TEST_MM
  fixturePlay.actorCtx.actorLists[ACTORCAT_PLAYER].first = &fixturePlayer.actor;
  fixturePlay.sceneId = 1;
  fixturePlayer.transformation = PLAYER_FORM_HUMAN;
  gSaveContext.save.playerForm = PLAYER_FORM_HUMAN;
  sPlayerGetItemRefPos = {9, 160, 22};
  sReady = 1;
  sSkel = &skeleton;
  sIsChildRig = 0;
  adultMode = 0;
  customForm = CUSTOM_FORM_NONE;
  modelScale = .01f;
  modelYOffset = 0;
  mario = wolf = false;
#else
  fixturePlay.actorCtx.actorLists[ACTORCAT_PLAYER].head = &fixturePlayer.actor;
  fixturePlay.sceneNum = 1;
  gSaveContext.linkAge = LINK_AGE_CHILD;
  sGetItemRefPos = {9, 160, 22};
  transformHeight = 0;
#endif
  R_UPDATE_RATE = 3;
  hasEnabled = featureEnabled;
  enabled = featureEnabled ? 1 : 0;
  queueResult = -1;
  queueCalls = modelDraws = 0;
  submittedDrawId = -1;
  submittedReference = {};
  expectedModelPlayer = &fixturePlayer;
  frames.clear();
  Reset();
}

static void ExpectEmpty(const char *explanation) {
  Check(!sMarker.Consume().active, explanation);
}
static ItemGetLightShaft::Presentation
TakePresentation(const char *explanation) {
  const auto presentation = sMarker.Consume();
  Check(presentation.active, explanation);
  return presentation;
}
static ItemGetLightShaft::Presentation MarkAndTake(float itemY = 174) {
  MARK_NATIVE(&fixturePlay, &fixturePlayer, itemY);
  return TakePresentation(
      "ready local native receipt did not mark its presentation");
}

// Catches an accidentally enabled fallback and a setting read cached at mark
// time.
static void TestDisabled() {
  FixtureReset(false);
  MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
  ExpectEmpty("missing CVar enabled a default-off effect");
  hasEnabled = true;
  enabled = 0;
  MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
  ExpectEmpty("explicitly disabled feature marked a receipt");
  enabled = 1;
  MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
  enabled = 0;
  Draw();
  Check(frames.size() == 1 && !frames.back().visible,
        "turning off after marking left a visible shaft");
}

// Catches marking peers, Kafei/NPCs, stale play states, or an absent draw
// context.
static void TestIdentity() {
  FixtureReset();
  peer = fixturePlayer;
  MARK_NATIVE(&fixturePlay, &peer, 174);
  ExpectEmpty("peer player marked the local shaft");
#ifdef GI_SHAFT_TEST_MM
  peer.actor.id = ACTOR_EN_TEST3;
#else
  peer.actor.id = ACTOR_EN_BOX;
#endif
  MARK_NATIVE(&fixturePlay, &peer, 174);
  ExpectEmpty("NPC receipt marked the local shaft");
  fixturePlayer.actor.id = peer.actor.id;
  MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
  ExpectEmpty("non-player actor in the player slot marked a shaft");
  fixturePlayer.actor.id = ACTOR_PLAYER;
  MARK_NATIVE(nullptr, &fixturePlayer, 174);
  MARK_NATIVE(&otherPlay, &fixturePlayer, 174);
  MARK_NATIVE(&fixturePlay, nullptr, 174);
  ExpectEmpty("null or foreign context marked a shaft");
  gPlayState = nullptr;
  MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
  ExpectEmpty("destroyed play state marked a shaft");
  gPlayState = &fixturePlay;
  fixturePlay.state.gfxCtx = nullptr;
  MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
  ExpectEmpty("missing graphics context marked a shaft");
}

// Catches replacing the true receipt bit with talking/carrying/general cutscene
// state.
static void TestReceiptFlag() {
  for (u32 flags : {0u, static_cast<u32>(PLAYER_STATE1_TALKING),
                    static_cast<u32>(PLAYER_STATE1_CARRYING_ACTOR), 1u << 29}) {
    FixtureReset();
    fixturePlayer.stateFlags1 = flags;
    MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
    ExpectEmpty("non-receipt player state marked a shaft");
  }
  FixtureReset();
  MarkAndTake();
  for (int drawId : {0, -1}) {
    SetNativeId(fixturePlayer, drawId);
    MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
    ExpectEmpty("zero or invalid native draw ID marked a shaft");
  }
}

// Catches missing native call-site wiring, early marking, and changed GI
// reference height.
static void TestNativeBoundary() {
  FixtureReset();
  Player_DrawGetItem(&fixturePlay, &fixturePlayer);
  Check(modelDraws == 1 && submittedDrawId == 7,
        "ready native item was not submitted once");
  Near(submittedReference.y, 160, "native item reference moved");
  const auto ready =
      TakePresentation("actual Player_DrawGetItem did not mark the shaft");
  Near(ready.itemHeight, 74,
       "native wrapper did not include the accepted fourteen-unit lift");

  FixtureReset();
  fixturePlayer.giObjectLoading = true;
  queueResult = -1;
  Player_DrawGetItem(&fixturePlay, &fixturePlayer);
#ifdef GI_SHAFT_TEST_MM
  Check(queueCalls == 1 && modelDraws == 0 && fixturePlayer.giObjectLoading,
        "MM submitted its item before the async GI load completed");
  ExpectEmpty("MM marked a shaft before its GI model was ready");
  queueResult = 0;
  Player_DrawGetItem(&fixturePlay, &fixturePlayer);
  Check(queueCalls == 2 && modelDraws == 1 && !fixturePlayer.giObjectLoading,
        "MM ready GI load did not hand off to its model exactly once");
  TakePresentation("MM ready callback did not mark a receipt");

  FixtureReset();
  fixturePlayer.transformation = PLAYER_FORM_DEKU;
  Player_DrawGetItem(&fixturePlay, &fixturePlayer);
  Near(submittedReference.y, 128, "MM native Deku GI reference moved");
  Near(TakePresentation("Deku native receipt was not marked").itemHeight, 42,
       "Deku native receipt lost its actual reference height");

  FixtureReset();
  peer = fixturePlayer;
  peer.actor.id = ACTOR_EN_TEST3;
  expectedModelPlayer = &peer;
  Player_DrawGetItem(&fixturePlay, &peer);
  Check(modelDraws == 1, "Kafei native GI path was not exercised");
  ExpectEmpty("Kafei's real Player_DrawGetItem path marked a local shaft");
#else
  // The current SoH port deliberately has its queue check commented out.
  Check(queueCalls == 0 && modelDraws == 1 && !fixturePlayer.giObjectLoading,
        "SoH native wrapper no longer matches its existing nonblocking port "
        "behavior");
  TakePresentation("SoH actual ready model submission did not mark a shaft");
#endif

  FixtureReset();
  SetNativeId(fixturePlayer, 0);
  Player_DrawGetItem(&fixturePlay, &fixturePlayer);
  ExpectEmpty("native wrapper marked a shaft for a zero GI draw ID");
}

// Catches pause/transition checks applied only when marking, leaving stale
// end-stage work.
static void TestPauseTransition() {
  for (int gate = 0; gate < 3; ++gate) {
    FixtureReset();
    if (gate == 0)
      fixturePlay.pauseCtx.state = 1;
    if (gate == 1)
      fixturePlay.transitionTrigger = TRANS_TRIGGER_START;
    if (gate == 2)
      fixturePlay.transitionMode = TRANS_MODE_SETUP;
    MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
    ExpectEmpty("paused or transitioning host accepted a marker");

    FixtureReset();
    MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
    if (gate == 0)
      fixturePlay.pauseCtx.state = 1;
    if (gate == 1)
      fixturePlay.transitionTrigger = TRANS_TRIGGER_START;
    if (gate == 2)
      fixturePlay.transitionMode = TRANS_MODE_SETUP;
    Draw();
    Check(frames.size() == 1 && !frames.back().visible,
          "pause or transition between mark and draw left a visible shaft");
  }
#ifndef GI_SHAFT_TEST_MM
  FixtureReset();
  fixturePlay.pauseCtx.debugState = 1;
  MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
  ExpectEmpty("SoH debug pause accepted a receipt marker");
  FixtureReset();
  MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
  fixturePlay.pauseCtx.debugState = 1;
  Draw();
  Check(!frames.back().visible, "SoH debug pause left a pending shaft visible");
#endif
}

// Catches duplicate rendering and markers/envelopes leaking across draw cycles
// or teardown.
static void TestConsumeReset() {
  FixtureReset();
  MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
  MARK_NATIVE(&fixturePlay, &fixturePlayer, 180);
  Draw();
  Check(frames.size() == 1 && frames.back().visible,
        "two receipt markers did not yield one visible frame");
  Draw();
  Check(frames.size() == 2 && !frames.back().visible,
        "consumed receipt marker was drawn a second time");
  MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
  sMarker.Begin();
  Draw();
  Check(!frames.back().visible,
        "new world draw retained the previous frame's marker");

  FixtureReset();
  for (int frame = 0; frame < 5; ++frame) {
    ++fixturePlay.gameplayFrames;
    MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
    Draw();
  }
  const auto settled = frames.back();
  Check(settled.visible && settled.opacity > .25f,
        "active receipt never advanced its envelope");
  MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
  Reset();
  ExpectEmpty("reset retained a pending receipt marker");
  ++fixturePlay.gameplayFrames;
  MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
  Draw();
  Check(frames.back().visible && frames.back().opacity < settled.opacity,
        "reset retained the preceding receipt's animation envelope");
  Check(frames.back().generation != settled.generation,
        "reset retained interpolation generation");

  MARK_NATIVE(&fixturePlay, &fixturePlayer, 174);
  const auto submitted = frames.size();
  gPlayState = nullptr;
  Draw();
  Check(frames.size() == submitted,
        "destroyed play state reached the shaft renderer");
  ExpectEmpty("null-play draw did not clear pending work");
}

// Catches applying adult-only offsets to native forms or ignoring a scaled
// adult rig.
static void TestHeights() {
#ifdef GI_SHAFT_TEST_MM
  struct NativeForm {
    u8 form;
    float height;
  };
  const NativeForm nativeForms[] = {{PLAYER_FORM_HUMAN, 44},
                                    {PLAYER_FORM_DEKU, 36},
                                    {PLAYER_FORM_GORON, 80},
                                    {PLAYER_FORM_ZORA, 68},
                                    {PLAYER_FORM_FIERCE_DEITY, 124}};
  for (auto form : nativeForms) {
    FixtureReset();
    fixturePlayer.transformation = form.form;
    const auto value = MarkAndTake();
    Near(value.playerHeight, form.height,
         "native form used the wrong physical height");
    Near(value.origin.y, 100, "native form received an adult visual offset");
  }
  FixtureReset();
  adultMode = 1;
  auto adult = MarkAndTake();
  Near(adult.playerHeight, 68,
       "ready ordinary adult used the human-child height");
  modelScale = .015f;
  modelYOffset = 7;
  adult = MarkAndTake();
  Near(adult.playerHeight, 102, "scaled adult height was not used");
  Near(adult.origin.y, 107, "adult visual foot offset was not used");
  Near(adult.itemHeight, 67,
       "adult visual offset was counted twice or omitted from item height");

  for (int exclusion = 0; exclusion < 6; ++exclusion) {
    FixtureReset();
    adultMode = 1;
    modelScale = .015f;
    modelYOffset = 7;
    if (exclusion == 0)
      sReady = 0;
    if (exclusion == 1)
      sSkel = nullptr;
    if (exclusion == 2)
      sIsChildRig = 1;
    if (exclusion == 3)
      customForm = CUSTOM_FORM_RITO;
    if (exclusion == 4)
      mario = true;
    if (exclusion == 5)
      wolf = true;
    const auto value = MarkAndTake();
    Near(value.playerHeight, 44,
         "excluded adult renderer changed native shaft height");
    Near(value.origin.y, 100,
         "excluded adult renderer changed native shaft origin");
  }
  for (auto form : nativeForms) {
    if (form.form == PLAYER_FORM_HUMAN)
      continue;
    FixtureReset();
    adultMode = 1;
    modelYOffset = 7;
    fixturePlayer.transformation = form.form;
    const auto value = MarkAndTake();
    Near(value.playerHeight, form.height,
         "adult mode overrode a native transformation's height");
    Near(value.origin.y, 100, "adult mode offset a native transformation");
  }
#else
  FixtureReset();
  Near(MarkAndTake().playerHeight, 44, "OoT child height was not preserved");
  gSaveContext.linkAge = LINK_AGE_ADULT;
  Near(MarkAndTake().playerHeight, 68, "OoT adult height was not preserved");
  transformHeight = 80;
  Near(MarkAndTake().playerHeight, 80,
       "OoT transformation height was not honored");
#endif
}

#ifdef GI_SHAFT_TEST_MM
static EnItem00 CustomReceipt() {
  EnItem00 item{};
  item.actor.id = ACTOR_EN_ITEM00;
  item.actor.params = ITEM00_NOTHING;
  item.actor.update = CustomItem00_Update;
  item.actor.draw = OrdinaryReceiptDraw;
  item.actor.home.rot.x =
      CustomItem::GIVE_ITEM_CUTSCENE | CustomItem::CALLED_ACTION |
      CustomItem::HIDE_TILL_OVERHEAD | CustomItem::KEEP_ON_PLAYER;
  item.actor.world.pos = {11, 160, 21};
  item.actor.shape.yOffset = 900;
  item.actor.scale = {.007f, .007f, .007f};
  item.unk152 = 20;
  return item;
}

// Catches a draw-function identity gate and failure to include the actor's
// actual shape lift.
static void TestCustom() {
  FixtureReset();
  auto item = CustomReceipt();
  item.unk152 = 19;
  MarkCustom(&item.actor);
  ExpectEmpty("custom receipt became visible before frame 20");
  item.unk152 = 20;
  MarkCustom(&item.actor);
  auto value = TakePresentation("ready custom cutscene receipt was not marked");
  Near(value.itemHeight, 66.3f, "custom receipt lost the shape-offset lift");
  Near(value.origin.x, 10,
       "shaft followed the spinning custom item instead of Link");
  Near(value.origin.z, 20,
       "shaft followed the spinning custom item instead of Link");
  item.actor.draw = ForeignReceiptDraw;
  MarkCustom(&item.actor);
  TakePresentation("an overridden foreign receipt draw function was excluded");
  adultMode = 1;
  modelScale = .015f;
  modelYOffset = 7;
  MarkCustom(&item.actor);
  value = TakePresentation("adult custom receipt was not marked");
  Near(value.playerHeight, 102,
       "custom receipt bypassed adult height eligibility");
  Near(value.origin.y, 107, "custom receipt bypassed adult visual offset");
  Near(value.itemHeight, 59.3f,
       "custom receipt counted adult translation incorrectly");

  FixtureReset(false);
  item = CustomReceipt();
  MarkCustom(&item.actor);
  ExpectEmpty("default-off setting allowed a custom receipt");
}

// Catches shop/world/overhead leakage and marking killed or ungranted custom
// actors.
static void TestCustomScope() {
  for (int exclusion = 0; exclusion < 13; ++exclusion) {
    FixtureReset();
    auto item = CustomReceipt();
    if (exclusion == 0)
      item.actor.id = ACTOR_EN_GIRLA;
    if (exclusion == 1)
      item.actor.params = ITEM00_RUPEE_GREEN;
    if (exclusion == 2)
      item.actor.update = OtherUpdate;
    if (exclusion == 3)
      item.actor.draw = nullptr;
    if (exclusion == 4)
      item.actor.home.rot.x &= ~CustomItem::GIVE_ITEM_CUTSCENE;
    if (exclusion == 5)
      item.actor.home.rot.x &= ~CustomItem::CALLED_ACTION;
    if (exclusion == 6)
      item.actor.home.rot.x |= CustomItem::GIVE_OVERHEAD;
    if (exclusion == 7)
      item.actor.home.rot.x |= CustomItem::KILL_ON_TOUCH;
    if (exclusion == 8)
      item.unk152 = -1;
    if (exclusion == 9)
      item.actor.scale.y = 0;
    if (exclusion == 10)
      item.actor.scale.y = -.007f;
    if (exclusion == 11)
      fixturePlayer.stateFlags1 = 0;
    if (exclusion == 12)
      item.actor.home.rot.x =
          CustomItem::GIVE_OVERHEAD | CustomItem::CALLED_ACTION;
    MarkCustom(&item.actor);
    ExpectEmpty("a shelf, world, quick-overhead, ungranted, or dead receipt "
                "actor marked a shaft");
  }
  FixtureReset();
  MarkCustom(nullptr);
  ExpectEmpty("null custom actor marked a receipt");
  auto item = CustomReceipt();
  gPlayState = nullptr;
  MarkCustom(&item.actor);
  ExpectEmpty("custom actor marked after play destruction");
}
#endif

int main(int argc, char **argv) {
  Check(argc == 2, "pass one named test case");
  currentCase = argv[1];
  const std::string test = currentCase;
  if (test == "disabled")
    TestDisabled();
  else if (test == "identity")
    TestIdentity();
  else if (test == "receipt")
    TestReceiptFlag();
  else if (test == "native_boundary")
    TestNativeBoundary();
  else if (test == "pause_transition")
    TestPauseTransition();
  else if (test == "consume_reset")
    TestConsumeReset();
  else if (test == "heights")
    TestHeights();
#ifdef GI_SHAFT_TEST_MM
  else if (test == "custom")
    TestCustom();
  else if (test == "custom_scope")
    TestCustomScope();
#endif
  else
    Check(false, "unknown native adapter test case");
  std::printf("PASS [%s] %s\n", kHost, currentCase);
}
