#include "global.h"
#include "variables.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#ifdef ZELDA_TEST_MM
#include "mods/forms/custom_forms.h"
#endif
#include "combo/actors/ZeldaRunPolicy.h"

PlayState *gPlayState;
SaveContext gSaveContext;
static PlayState play;
static Player player, other;
static bool enabled, transformed, wolf, mario;
static int ages = ZeldaRun::Both, form = -1;
static bool adultMode;
extern "C" s32 AdultLink_IsActive() { return adultMode; }
extern "C" s32 CustomForms_ActiveForm() { return form; }
extern "C" u8 TransformMasks_IsTransformedAny() { return transformed; }
extern "C" u8 WolfLinkForm_IsSelected() { return wolf; }
extern "C" u8 Sm64Mario_ShouldHideLink() { return mario; }
extern "C" int32_t CVarGetInteger(const char *key, int32_t fallback) {
  if (!std::strcmp(key, ZELDA_RUN_CVAR))
    return enabled;
  if (!std::strcmp(key, ZELDA_RUN_AGES_CVAR))
    return ages;
  return fallback;
}
extern "C" int ResourceMgr_OTRSigCheck(char *path) {
  return path && !std::strncmp(path, "__OTR__", 7);
}
#include "native_eligibility.inc"
#include "combo/actors/ZeldaRunBridge.inc"

static void Age(bool adult) {
  adultMode = adult;
#ifndef ZELDA_TEST_MM
  gSaveContext.linkAge = adult ? LINK_AGE_ADULT : LINK_AGE_CHILD;
#endif
}
int main() {
  gPlayState = &play;
#ifdef ZELDA_TEST_MM
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
  player.transformation = PLAYER_FORM_HUMAN;
#else
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
#endif
  char run[] = "__OTR__objects/gameplay_keep/gPlayerAnim_link_normal_run_free";
  char walk[] =
      "__OTR__objects/gameplay_keep/gPlayerAnim_link_normal_walk_free";
  char sword[] = "__OTR__objects/gameplay_keep/gPlayerAnim_link_fighter_run";
  player.skelAnime.limbCount = PLAYER_LIMB_MAX;
  Age(true);
  assert(ZeldaRun_Select(&player, run) == run);
  enabled = true;
  auto *selected =
      static_cast<LinkAnimationHeader *>(ZeldaRun_Select(&player, run));
  assert(selected != reinterpret_cast<LinkAnimationHeader *>(run));
  assert(selected->common.frameCount ==
         20); // caller sees new range before native helpers
#ifdef ZELDA_TEST_MM
  auto *motion = static_cast<s16 *>(selected->segmentVoid);
#else
  auto *motion = static_cast<s16 *>(selected->segment);
#endif
  constexpr int frameBytes = 3 * PLAYER_LIMB_MAX * sizeof(s16) + sizeof(s16);
  assert(std::memcmp(motion, reinterpret_cast<char *>(motion) + 19 * frameBytes,
                     frameBytes) == 0);
  assert(std::memcmp(motion, reinterpret_cast<char *>(motion) + 9 * frameBytes,
                     frameBytes) != 0);
  char altRun[] = "__OTR__alt/objects/gameplay_keep/gPlayerAnim_link_normal_run_free";
  assert(ZeldaRun_Select(&player, altRun) == selected);
  for (bool adult : {false, true}) {
    Age(adult);
    for (int a : {ZeldaRun::Child, ZeldaRun::Adult, ZeldaRun::Both}) {
      ages = a;
      assert((ZeldaRun_Select(&player, run) != run) ==
             ZeldaRun::AgeAllowed(a, adult));
    }
  }
  ages = ZeldaRun::Both;
  Age(true);
  assert(ZeldaRun_Select(&player, walk) == walk &&
         ZeldaRun_Select(&player, sword) == sword);
  assert(ZeldaRun_Select(&other, run) == run &&
         ZeldaRun_Select(nullptr, run) == run);
  wolf = true;
#ifndef ZELDA_TEST_MM
  transformed = true;
#endif
  assert(ZeldaRun_Select(&player, run) == run);
#ifndef ZELDA_TEST_MM
  // OoT's form state has returned to human. The custom slot still remembers Wolf.
  transformed = false;
  assert(ZeldaRun_Select(&player, run) == selected);
#endif
  wolf = false;
  assert(ZeldaRun_Select(&player, run) == selected);
  mario = true;
  assert(ZeldaRun_Select(&player, run) == run);
  mario = false;
#ifdef ZELDA_TEST_MM
  player.transformation = PLAYER_FORM_GORON;
  assert(ZeldaRun_Select(&player, run) == run);
  player.transformation = PLAYER_FORM_HUMAN;
  form = CUSTOM_FORM_GERUDO;
  assert(ZeldaRun_Select(&player, run) == run);
  form = CUSTOM_FORM_NONE;
#else
  transformed = true;
  assert(ZeldaRun_Select(&player, run) == run);
  transformed = false;
#endif
  ages = 99;
  assert(ZeldaRun_Select(&player, run) == run);
  ages = ZeldaRun::Both;
  player.skelAnime.limbCount++;
  assert(ZeldaRun_Select(&player, run) == run);
  player.skelAnime.limbCount--;
  assert(ZeldaRun_Select(&player, run) == selected);
  enabled = false;
  assert(ZeldaRun_Select(&player, run) == run);
  assert(ages == ZeldaRun::Both); // Off does not erase preference
  puts("PASS Zelda run: native ages/forms, original fallback, exact clip "
       "filter, 20-frame header, built-in clip and Alt path");
}
