// Characterize the exact native Woods of Mystery guide voice, independently
// of the SoH-side sound aliases and custom actor caller.
#include "global.h"
#include "overlays/actors/ovl_En_Mnk/z_en_mnk.h"
#include <assert.h>
#include <stdio.h>

#define MONKEY_FOLLOWPATH_RESULT_FOLLOWING 0

static unsigned calls;
static bool onFrame;
s32 Animation_OnFrame(SkelAnime *animation, f32 frame) {
  (void)animation;
  assert(frame == 1.0f);
  return onFrame;
}
void Actor_PlaySfx(Actor *actor, u16 id) {
  assert(actor != NULL);
  assert(id == (calls % 2 == 0 ? 0x2900 : 0x28EF));
  ++calls;
}
s32 SkelAnime_Update(SkelAnime *animation) {
  (void)animation;
  return 0;
}
s32 EnMnk_FollowPath(EnMnk *monkey, u16 flags) {
  (void)monkey;
  assert(flags == 0);
  return 0;
}
void Actor_Kill(Actor *actor) {
  (void)actor;
  assert(false);
}

/* NATIVE_GUIDE */

int main(void) {
  EnMnk monkey = {0};
  PlayState play = {0};
  EnMnk_Monkey_GuideThroughWoods(&monkey, &play);
  assert(calls == 0);
  onFrame = true;
  EnMnk_Monkey_GuideThroughWoods(&monkey, &play);
  assert(calls == 2);
  puts("PASS native MM Woods of Mystery guide emits voice 0x2900 and footstep "
       "0x28EF at animation frame 1");
}
