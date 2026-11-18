#include "global.h"
#include "variables.h"
#include <cassert>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#ifdef ZELDA_TEST_MM
#include "2s2h/resource/type/Animation.h"
#include "2s2h/resource/type/PlayerAnimation.h"
#include "mods/forms/custom_forms.h"
#else
#include "soh/resource/type/Animation.h"
#include "soh/resource/type/PlayerAnimation.h"
#endif
#include "combo/actors/ZeldaRunPolicy.h"

namespace Ship {
IResource::IResource(std::shared_ptr<ResourceInitData>) {}
IResource::~IResource() = default;
} // namespace Ship
namespace SOH {
AnimationData *Animation::GetPointer() { return &animationData; }
size_t Animation::GetPointerSize() { return sizeof(animationData); }
int16_t *PlayerAnimation::GetPointer() { return limbRotData.data(); }
size_t PlayerAnimation::GetPointerSize() { return limbRotData.size() * 2; }
} // namespace SOH
PlayState *gPlayState;
SaveContext gSaveContext;
static PlayState play;
static Player player, other;
static bool enabled, alt, transformed, wolf, mario;
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
static std::map<std::string, std::shared_ptr<Ship::IResource>> files;
static std::shared_ptr<Ship::IResource> ZeldaRun_Load(const std::string &path) {
  if (alt && files.contains("alt/" + path))
    return files.at("alt/" + path);
  return files.contains(path) ? files.at(path) : nullptr;
}
#include "native_eligibility.inc"
#include "combo/actors/ZeldaRunBridge.inc"

static void Install(const std::string &prefix = "") {
  auto data = std::make_shared<SOH::PlayerAnimation>();
  data->limbRotData.resize(20 * 67);
  for (int i = 0; i < 20 * 67; ++i)
    data->limbRotData[i] = i;
  auto header = std::make_shared<SOH::Animation>();
  header->type = SOH::AnimationType::Link;
  header->animationData.linkAnimationHeader.common.frameCount = 20;
  header->animationData.linkAnimationHeader.segment = data->GetPointer();
  files[prefix + ZeldaRun::kHeader] = header;
  files[prefix + ZeldaRun::kData] = data;
}
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
  Age(true);
  Install();
  assert(ZeldaRun_Select(&player, run) == run);
  enabled = true;
  auto *selected =
      static_cast<LinkAnimationHeader *>(ZeldaRun_Select(&player, run));
  assert(selected != reinterpret_cast<LinkAnimationHeader *>(run));
  assert(selected->common.frameCount ==
         20); // caller sees new range before native helpers
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
  assert(ZeldaRun_Select(&player, run) == run);
  wolf = false;
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
  auto header =
      std::dynamic_pointer_cast<SOH::Animation>(files.at(ZeldaRun::kHeader));
  auto data = std::dynamic_pointer_cast<SOH::PlayerAnimation>(
      files.at(ZeldaRun::kData));
  header->animationData.linkAnimationHeader.common.frameCount = 19;
  assert(ZeldaRun_Select(&player, run) == run);
  header->animationData.linkAnimationHeader.common.frameCount = 20;
  header->type = SOH::AnimationType::Normal;
  assert(ZeldaRun_Select(&player, run) == run);
  header->type = SOH::AnimationType::Link;
  header->animationData.linkAnimationHeader.segment = nullptr;
  assert(ZeldaRun_Select(&player, run) == run);
  header->animationData.linkAnimationHeader.segment = data->GetPointer();
  data->limbRotData.resize(19 * 67);
  assert(ZeldaRun_Select(&player, run) == run);
  data->limbRotData.resize(20 * 67);
  header->animationData.linkAnimationHeader.segment = data->GetPointer();
  auto keep = selected;
  Install("alt/");
  alt = true;
  auto *alternate =
      static_cast<LinkAnimationHeader *>(ZeldaRun_Select(&player, run));
  assert(alternate != selected && alternate->common.frameCount == 20);
  // A mismatched Alt header/data pair never gets used.
  files.erase(std::string("alt/") + ZeldaRun::kData);
  assert(ZeldaRun_Select(&player, run) == run);
  files.clear();
  assert(ZeldaRun_Select(&player, run) == run);
  assert(keep->common.frameCount ==
         20); // active animation remains alive after cache removal
  Install();
  enabled = false;
  assert(ZeldaRun_Select(&player, run) == run);
  assert(ages == ZeldaRun::Both); // Off does not erase preference
  puts("PASS Zelda run: native ages/forms, original fallback, exact clip "
       "filter, 20-frame header, Alt and lifetime");
}
