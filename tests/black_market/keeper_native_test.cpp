#include "global.h"
#include "combo/BlackMarketKeeper.h"
#ifndef OOT_KEEPER_TEST
#include "overlays/actors/ovl_En_Takaraya/z_en_takaraya.h"
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
PlayState* gPlayState;
static bool interior = true, pack = true, sTalking = true;
static int initializations, kills;
constexpr int kLimbCount = 24;
struct KeeperState { SkelAnime skel; Vec3s joints[24], morphs[24]; int eye, timer; } sKeeper;
bool Interior() { return interior; }
bool PackAvailable() { return pack; }
void KeeperUpdate(Actor*, PlayState*) {}
void KeeperDraw(Actor*, PlayState*) {}
void RecordInit(PlayState*, SkelAnime*, FlexSkeletonHeader*, AnimationHeader* animation, Vec3s*, Vec3s*, int limbs) {
    assert(limbs == 24);
    assert(std::strcmp(reinterpret_cast<const char*>(animation), BlackMarketKeeper::kAnimation) == 0 &&
           "native controller must select the crossed-arm lean, not the upright idle");
    ++initializations;
}
void Scale(Actor* actor, float value) {
    assert(value == BlackMarketKeeper::kScale);
    actor->scale = {value,value,value};
}
#define SkelAnime_InitFlex(...) RecordInit(__VA_ARGS__)
#define ActorShape_Init(...) ((void)0)
#define Actor_SetScale(actor,value) Scale(actor,value)
#define Actor_Kill(actor) (++kills)
/* PRODUCTION */
int main() {
    PlayState play{}; gPlayState = &play;
#ifdef OOT_KEEPER_TEST
    Actor keeper{}; KeeperInit(&keeper, &play);
    assert(sKeeper.timer == 90);
#else
    EnTakaraya gal{}; Actor& keeper = gal.actor; InitKeeper(&keeper);
    assert(gal.timer == 90 && gal.eyeTexIndex == 0 && !sTalking);
#endif
    assert(initializations == 1 && !kills && keeper.update == KeeperUpdate && keeper.draw == KeeperDraw);
    assert(keeper.destroy == nullptr && (keeper.flags & ACTOR_FLAG_UPDATE_CULLING_DISABLED));
    std::puts("PASS native keeper initializer: exact crossed-arm animation/scale, dedicated callbacks and retained blink start");
}
