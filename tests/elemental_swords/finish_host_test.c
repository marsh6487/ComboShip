// Execute the complete production finish binding with live engine structs and
// real collision/fatal functions. Graphics/resources/audio are boundaries only.
#ifdef SWORD_FINISH_TEST_MM
#include "../../mm/tests/din_fire_fixture.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "mods/combo_rpg.h"
#define FINISH_PRESENT __has_include("../../mm/src/code/elemental_sword_finish.c")
#else
#define main LegacyFireRendererMain
#include "../../soh/tests/din_fire_sword_test.c"
#undef main
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
PlayState *gPlayState = &play;
#define FINISH_PRESENT __has_include("../../soh/src/code/elemental_sword_finish.c")
#endif

#if FINISH_PRESENT
#ifdef SWORD_FINISH_TEST_MM
#include "../../mm/src/code/elemental_sword_finish.c"
#else
#include "../../soh/src/code/elemental_sword_finish.c"
#endif
#ifdef SWORD_FINISH_TEST_MM
void CollisionCheck_ApplyDamage(PlayState *, CollisionCheckContext *,
                                Collider *, ColliderElement *);
#else
void CollisionCheck_ApplyDamage(PlayState *, CollisionCheckContext *,
                                Collider *, ColliderInfo *);
#endif
static int finishSoundCount;
static u16 finishSound;
static int finishStopCount;
static Vec3f *finishSoundPos;
static void LiveUpdate(Actor *actor, PlayState *state) {}
bool GameInteractor_Should(GIVanillaBehavior behavior, uint32_t result, ...) {
  return result;
}
#ifdef SWORD_FINISH_TEST_MM
void GameInteractor_ExecuteOnActorKill(Actor *actor) {}
#else
void GameInteractor_ExecuteOnActorKill(void *actor) {}
#endif
u8 gIvanPossessActive;
u8 Sm64Mario_IsReady(void) { return 0; }
#ifdef SWORD_FINISH_TEST_MM
u8 TridentChargeBall_GetFierceDamage(Actor *actor) { return 0; }
int ComboRpg_IsEnabled(int stat) { return 0; }
uint8_t ComboRpg_ApplyPower(uint8_t damage, float roll) { return damage; }
f32 Rand_ZeroOne(void) { return 0.5f; }
void Audio_PlaySfx_AtPos(Vec3f *pos, u16 sound) {
  ++finishSoundCount;
  finishSound = sound;
  finishSoundPos = pos;
}
void AudioSfx_StopByPosAndId(Vec3f *pos, u16 sound) {
  ++finishStopCount;
  REQUIRE(pos >= sFinishSoundPos &&
          pos < sFinishSoundPos + SWORD_FINISH_EFFECT_CAPACITY &&
          sound == 0x31F7);
}
#else
u8 TransformMasks_IsFDSkinMode(void) { return 0; }
static GameInfo gameInfo;
GameInfo *gGameInfo = &gameInfo;
s32 MmSfx_PlayAtPos(u16 sound, Vec3f *pos) {
  ++finishSoundCount;
  finishSound = sound;
  finishSoundPos = pos;
  return 1;
}
void MmSfx_StopAtPos(u16 sound, Vec3f *pos) {
  ++finishStopCount;
  REQUIRE(pos >= sFinishSoundPos &&
          pos < sFinishSoundPos + SWORD_FINISH_EFFECT_CAPACITY &&
          sound == 0x31F7);
}
Gfx *MmAssets_LoadDisplayListGraphStrict(const char *path) { return NULL; }
void MmAssets_EnsureStrictTextureBindings(void) {}
void Trirod_NotifyEnemyDown(PlayState *state, Actor *actor) {}
#endif
void SkinMatrix_Vec3fMtxFMultXYZ(MtxF *matrix, Vec3f *source, Vec3f *dest) {
  *dest = *source;
}
#ifdef SWORD_FINISH_TEST_MM
void SoundSource_PlaySfxAtFixedWorldPos(PlayState *state, Vec3f *pos,
                                        u32 duration, u16 sound) {
#else
void SoundSource_PlaySfxAtFixedWorldPos(PlayState *state, Vec3f *pos,
                                        s32 duration, u16 sound) {
#endif
  ++finishSoundCount;
  finishSound = sound;
}
#include "finish_native.inc"
#include "finish_tables.inc"

static int Effects(void) {
  int count = 0;
  for (int i = 0; i < SWORD_FINISH_EFFECT_CAPACITY; ++i)
    count += sFinish.effects[i].life > 0;
  return count;
}
static void World(void) {
#ifdef SWORD_FINISH_TEST_MM
  Setup();
#else
  setup();
  static int scene;
  play.sceneNum = ++scene;
#endif
  gfx.polyOpa.p = opa;
  gfx.polyXlu.p = xlu;
  finishSoundCount = finishSound = finishStopCount = 0;
  finishSoundPos = NULL;
  elementalSelection = elementalActive = 3;
  ++play.gameplayFrames;
}
static void Accept(ColliderCylinder *cylinder, int element) {
  elementalSelection = elementalActive = element;
#ifdef SWORD_FINISH_TEST_MM
  ColliderElement *attack = &player.meleeWeaponQuads[0].elem;
  attack->atDmgInfo.dmgFlags =
      DinFireSword_SetDamageFlags(&play, &player, 0, DMG_SWORD);
  attack->atDmgInfo.damage = 4;
  player.meleeWeaponQuads[0].base.actor = &player.actor;
  cylinder->elem.acElemFlags = ACELEM_HIT;
  cylinder->elem.acDmgInfo.dmgFlags = 0xFFFFFFFF;
  cylinder->elem.acHitElem = attack;
  cylinder->elem.acHit = &player.meleeWeaponQuads[0].base;
  CollisionCheck_ApplyDamage(&play, &play.colChkCtx, &cylinder->base,
                             &cylinder->elem);
#else
  ColliderInfo *attack = &player.meleeWeaponQuads[0].info;
  attack->toucher.dmgFlags =
      DinFireSword_SetDamageFlags(&play, &player, 0, DMG_SLASH_MASTER);
  attack->toucher.damage = 4;
  cylinder->info.bumperFlags = BUMP_HIT;
  cylinder->info.bumper.dmgFlags = 0xFFFFFFFF;
  cylinder->info.acHitInfo = attack;
  cylinder->info.acHit = &player.meleeWeaponQuads[0].base;
  CollisionCheck_ApplyDamage(&play, &play.colChkCtx, &cylinder->base,
                             &cylinder->info);
#endif
}
static void FatalBindings(void) {
  for (int element = 1; element <= 3; ++element) {
    World();
    Actor actor = {0};
    actor.id = ACTOR_EN_DEKUBABA;
    actor.category = ACTORCAT_ENEMY;
    actor.update = LiveUpdate;
    actor.colChkInfo.health = 4;
    actor.world.pos = (Vec3f){10, 20, 30};
    ColliderCylinder cylinder = {0};
    cylinder.base.actor = &actor;
    cylinder.base.acFlags = AC_HIT;
    cylinder.base.shape = COLSHAPE_CYLINDER;
    cylinder.dim = (Cylinder16){20, 60, 0, {10, 20, 30}};
    Actor before = actor;
    Accept(&cylinder, element);
    REQUIRE(actor.colChkInfo.damage == 4);
    DinFireSword_FinishBeginActor(&play, &actor);
    REQUIRE(Actor_ApplyDamage(&actor) == 0);
    DinFireSword_FinishEndActor(&play, &actor);
    REQUIRE(Effects() == 1 && sFinish.effects[0].element == element);
    REQUIRE(sFinish.effects[0].shape.count >= 6);
    REQUIRE(sFinish.effects[0].shape.points[0].radius > 0);
    REQUIRE(actor.update == before.update && actor.draw == before.draw &&
            actor.category == before.category);
    DinFireSword_FinishFatal(&play, &actor);
    REQUIRE(Effects() == 1);
    DinFireSword_FinishForget(&play, &actor);
    memset(&actor, 0xDE, sizeof(actor));
    REQUIRE(sFinish.effects[0].shape.origin.x == 10);
    DinFireSword_FinishPlaySound(&play, &sFinish.effects[0]);
    REQUIRE(finishSoundCount == 1);
    if (element == 3)
      REQUIRE(finishSound == 0x31F7); // Native 0x39F7 - SFX_FLAG held burn.
    DinFireSword_FinishPlaySound(&play, &sFinish.effects[0]);
    REQUIRE(finishSoundCount == 1);
    if (element == 3) {
      REQUIRE(finishSoundPos == &sFinishSoundPos[0]);
      ++play.gameplayFrames;
      DinFireSword_FinishContext(&play);
      DinFireSword_FinishPlaySound(&play, &sFinish.effects[0]);
      REQUIRE(finishSoundCount ==
              2); // Native loop keepalive on the next simulation frame.
      play.pauseCtx.state = 1;
      ++play.gameplayFrames;
      DinFireSword_FinishContext(&play);
      DinFireSword_FinishPlaySound(&play, &sFinish.effects[0]);
      REQUIRE(finishSoundCount == 2 && finishStopCount == 1);
      DinFireSword_FinishContext(&play);
      REQUIRE(finishStopCount == 1); // Pause stops an owned loop exactly once.
      play.pauseCtx.state = 0;
      DinFireSword_FinishContext(&play);
      DinFireSword_FinishPlaySound(&play, &sFinish.effects[0]);
      REQUIRE(finishSoundCount == 3); // Same-frame resume restarts the loop.
      for (int frame = 0; frame < SWORD_FINISH_DURATION; ++frame) {
        ++play.gameplayFrames;
        DinFireSword_FinishContext(&play);
      }
      REQUIRE(Effects() == 0 && finishStopCount == 2);
    }
  }
}
static void DrawBudget(void) {
#ifdef SWORD_FINISH_DRAW_POINT_LIMIT
  World();
  gfx.polyOpa.d = opa + ARRAY_COUNT(opa);
  gfx.polyXlu.d = xlu + ARRAY_COUNT(xlu);
  REQUIRE(DinFireSword_FinishCanDrawPoint(&gfx));
  gfx.polyOpa.d = gfx.polyOpa.p;
  REQUIRE(!DinFireSword_FinishCanDrawPoint(&gfx));
  gfx.polyOpa.d = opa + ARRAY_COUNT(opa);
  gfx.polyXlu.d = gfx.polyXlu.p;
  REQUIRE(!DinFireSword_FinishCanDrawPoint(&gfx));
  REQUIRE(SWORD_FINISH_DRAW_POINT_LIMIT <= 32);
#else
  REQUIRE(!"finish drawing has no arena/point budget");
#endif
}
static void IdleObservers(void) {
  DinFireSword_FinishContext(NULL);
  World();
  Gfx *beforeOpa = gfx.polyOpa.p, *beforeXlu = gfx.polyXlu.p;
  // No accepted hits: even reading this Actor would trip the sanitizers.
  Actor *unreadable = (Actor *)(uintptr_t)1;
  DinFireSword_FinishBeginActor(&play, unreadable);
  DinFireSword_FinishEndActor(&play, unreadable);
  REQUIRE(Effects() == 0 && sFinish.hitCount == 0);
  REQUIRE(gfx.polyOpa.p == beforeOpa && gfx.polyXlu.p == beforeXlu &&
          finishSoundCount == 0);
}
static void LightLoopContextCleanup(void) {
  for (int teardown = 0; teardown < 5; ++teardown) {
    DinFireSword_FinishContext(NULL);
    World();
    DinFireSword_FinishContext(&play);
    sFinish.effects[0].life = SWORD_FINISH_DURATION;
    sFinish.effects[0].element = SWORD_ELEMENT_LIGHT;
    DinFireSword_FinishPlaySound(&play, &sFinish.effects[0]);
    REQUIRE(finishSoundCount == 1 && finishStopCount == 0);
    if (teardown == 0)
      ++FINISH_SCENE(&play);
    else if (teardown == 1)
      ++play.roomCtx.curRoom.num;
    else if (teardown == 2)
      play.transitionTrigger = TRANS_TRIGGER_START;
    if (teardown == 3) {
      // An unrelated teardown must not read that context or erase our loop.
      CollisionCheck_DestroyContext((PlayState *)(uintptr_t)1, NULL);
      REQUIRE(Effects() == 1 && finishStopCount == 0);
      CollisionCheck_DestroyContext(&play, &play.colChkCtx);
      REQUIRE(Effects() == 0 && finishStopCount == 1);
      CollisionCheck_DestroyContext(&play, &play.colChkCtx);
      REQUIRE(finishStopCount == 1);
      continue;
    }
    PlayState *context = teardown == 4 ? NULL : &play;
    DinFireSword_FinishContext(context);
    REQUIRE(Effects() == 0 && finishStopCount == 1);
    DinFireSword_FinishContext(context);
    REQUIRE(finishStopCount == 1);
  }
}
static void ImmuneElementKeepsFatalSwordDamage(void) {
  World();
  Actor actor = {0};
  ColliderCylinder receiver = {0};
#ifdef SWORD_FINISH_TEST_MM
  actor.id = ACTOR_EN_AM;
  const int damage = 4;
#else
  actor.id = ACTOR_EN_RD;
  const int damage = 2;
#endif
  actor.category = ACTORCAT_ENEMY;
  actor.update = LiveUpdate;
  actor.colChkInfo.health = damage;
  actor.colChkInfo.damageTable = &sFinishImmuneTable;
  receiver.base.actor = &actor;
  receiver.base.shape = COLSHAPE_CYLINDER;
  receiver.base.acFlags = AC_HIT;
  Accept(&receiver, 2);
  REQUIRE(actor.colChkInfo.damage == damage);
  DinFireSword_FinishBeginActor(&play, &actor);
  REQUIRE(Actor_ApplyDamage(&actor) == 0);
  DinFireSword_FinishEndActor(&play, &actor);
  REQUIRE(Effects() == 1 && sFinish.effects[0].element == 2);
#ifndef SWORD_FINISH_TEST_MM
  for (int larva = 0; larva <= 1; ++larva) {
    World();
    memset(&actor, 0, sizeof(actor));
    memset(&receiver, 0, sizeof(receiver));
    actor.id = ACTOR_EN_PEEHAT;
    actor.params = larva ? 1 : -1;
    actor.category = ACTORCAT_ENEMY;
    actor.update = LiveUpdate;
    actor.colChkInfo.health = 1;
    actor.colChkInfo.damageTable = &sFinishPeahatTable;
    receiver.base.actor = &actor;
    receiver.base.shape = COLSHAPE_CYLINDER;
    receiver.base.acFlags = AC_HIT;
    Accept(&receiver, 1);
    DinFireSword_FinishBeginActor(&play, &actor);
    Actor_Kill(&actor);
    DinFireSword_FinishEndActor(&play, &actor);
    REQUIRE(Effects() == larva); // Larvae bypass adult native fire drawing.
  }
#endif
}
#ifndef SWORD_FINISH_ADAPTER_TEST
int main(void) {
  FatalBindings();
  IdleObservers();
  LightLoopContextCleanup();
  DrawBudget();
  ImmuneElementKeepsFatalSwordDamage();
  puts("PASS real finish hosts: collider bounds, preserved damage/death, "
       "copied lifetime and MM deadlight bank");
}
#endif
#else
int main(void) {
  REQUIRE(!"production elemental finish host binding is missing");
}
#endif
