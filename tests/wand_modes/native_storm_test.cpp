// A killed storm actor is a failed cast, even though native spawn returns it.
#include "global.h"
#include "GameInteractor/GameInteractor.h"
#include "overlays/actors/ovl_En_Okarina_Effect/z_en_okarina_effect.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>

extern ActorProfile En_Okarina_Effect_Profile;
int currentActorListIndex = -1;
ActorOverlay gActorOverlayTable[ACTOR_ID_MAX]{};
uintptr_t gSegments[NUM_SEGMENTS]{};
LightningStrike gLightningStrike{};
std::vector<void*> allocations;

#define FLAGS (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_UPDATE_DURING_OCARINA)
#define WAND_STORM_SPAWN_Y_OFFSET -30.0f
#define WAND_STORM_OKARINA_PARAMS 1

ActorProfile* Actor_LoadOverlay(ActorContext*, s16 id) {
    assert(id == ACTOR_EN_OKARINA_EFFECT);
    return &En_Okarina_Effect_Profile;
}
void Actor_FreeOverlay(ActorOverlay*) {}
void* ZeldaArena_Malloc(size_t size) {
    void* allocation = std::calloc(1, size);
    allocations.push_back(allocation);
    return allocation;
}
void SetActorListIndex(Actor*, s32) {}
bool GameInteractor_Should(GIVanillaBehavior, uint32_t result, ...) { return result; }
bool GameInteractor_ShouldActorInit(Actor*) { return true; }
void GameInteractor_ExecuteOnActorInit(Actor*) {}
void GameInteractor_ExecuteOnActorKill(Actor*) {}
void Math_Vec3f_Copy(Vec3f* dest, Vec3f* src) { *dest = *src; }
void CollisionCheck_InitInfo(CollisionCheckInfo*) {}
void ActorShape_Init(ActorShape* shape, f32 offset, ActorShadowFunc shadow, f32 scale) {
    shape->yOffset = offset;
    shape->shadowDraw = shadow;
    shape->shadowScale = scale;
}
s32 Flags_GetClear(PlayState*, s32) { return false; }
void Environment_PlayStormNatureAmbience(PlayState*) {}
void Environment_StopStormNatureAmbience(PlayState*) {}
s32 FrameAdvance_IsEnabled(PlayState*) { return false; }

#include "production.inc"

int main() {
    Player player{};
    PlayState play{};
    play.objectCtx.numEntries = 1;
    play.objectCtx.slots[0].id = GAMEPLAY_KEEP;

    assert(WandStorm_CallStorm(&player, &play));
    Actor* accepted = play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
    assert(accepted && accepted->init == nullptr && accepted->update != nullptr);
    accepted->update(accepted, &play);
    assert(play.envCtx.precipitation[PRECIP_SOS_MAX] == 60);

    play.envCtx.precipitation[PRECIP_RAIN_CUR] = 1;
    bool result = WandStorm_CallStorm(&player, &play);
    Actor* rejected = accepted->next;
    assert(rejected != nullptr && rejected->update == nullptr);
    assert(!result && "a native storm killed during Init must not count as a cast");

    for (void* allocation : allocations) std::free(allocation);
    std::cout << "PASS native Storm weather spawn success and Init rejection\n";
}
