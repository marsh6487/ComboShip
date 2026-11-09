#include "EnemyFirstDrops.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include <algorithm>
#include <iterator>

extern "C" {
#include "functions.h"
#include "variables.h"
extern PlayState* gPlayState;
// Existing C bridge declarations from OTRGlobals.h; keep this engine callback
// independent of the generator/UI object graph.
u8 Randomizer_GetSettingValue(RandomizerSettingKey key);
GetItemEntry Randomizer_GetItemFromKnownCheck(RandomizerCheck check, GetItemID original);
}

extern void EnItem00_DrawRandomizedItem(EnItem00* item, PlayState* play);

namespace Rando::EnemyFirstDrops {
// Active actors are transient; both defeat eligibility and receipt are native
// saved flags. This preserves unique rewards through scene changes/save reloads.
static Actor* pending[RC_MAX] = {};

void ResetPending() {
    std::fill(std::begin(pending), std::end(pending), nullptr);
}

void OnDropDestroyed(void* actor) {
    for (const auto& entry : Catalog) {
        auto& drop = pending[entry.check];
        if (drop == actor) {
            drop = nullptr;
        }
    }
}

static bool Spawn(const Entry& entry, Vec3f position) {
    if (pending[entry.check] || Flags_GetRandomizerInf(entry.flag))
        return false;
    EnItem00* drop = (EnItem00*)Item_DropCollectible2(gPlayState, &position, ITEM00_SOH_DUMMY);
    if (drop == nullptr)
        return false; // Saved eligibility remains queued for the next player update.
    drop->randoInf = entry.flag;
    drop->itemEntry = Randomizer_GetItemFromKnownCheck(entry.check, GI_NONE);
    drop->actor.draw = (ActorFunc)EnItem00_DrawRandomizedItem;
    drop->actor.velocity.y = 8.0f;
    drop->actor.speedXZ = 0.0f;
    pending[entry.check] = &drop->actor;
    return true;
}

bool OnDefeat(Actor* actor) {
    if (!IS_RANDO || !Randomizer_GetSettingValue(RSK_SHUFFLE_ENEMY_DROPS) || actor == nullptr ||
        gPlayState == nullptr || gSaveContext.gameMode == GAMEMODE_END_CREDITS)
        return false;
    const auto entry = ForActor(actor->id, actor->params, gSaveContext.linkAge == LINK_AGE_ADULT,
                                gPlayState->sceneNum == SCENE_DEKU_TREE_BOSS);
    if (entry == nullptr || !HasSeededCheck(entry->check) || !IsAvailable(entry->check, SeedUsesMQ) ||
        pending[entry->check] || Flags_GetRandomizerInf(entry->flag))
        return false;

    // Only native confirmed death/victory emits eligibility, never unload or
    // damage. Save it before allocation: unique foes may never spawn again.
    Flags_SetRandomizerInf(entry->defeatedFlag);
    return Spawn(*entry, actor->world.pos);
}

void DeliverUncollected() {
    if (!IS_RANDO || !Randomizer_GetSettingValue(RSK_SHUFFLE_ENEMY_DROPS) || !gPlayState ||
        gSaveContext.gameMode == GAMEMODE_END_CREDITS)
        return;
    Actor* player = gPlayState->actorCtx.actorLists[ACTORCAT_PLAYER].head;
    if (!player)
        return;
    for (const auto& entry : Catalog) {
        if (HasSeededCheck(entry.check) && IsAvailable(entry.check, SeedUsesMQ) &&
            Flags_GetRandomizerInf(entry.defeatedFlag) && !Flags_GetRandomizerInf(entry.flag) && !pending[entry.check])
            Spawn(entry, player->world.pos);
    }
}

static void RegisterRuntime() {
    const bool enabled = IS_RANDO && Randomizer_GetSettingValue(RSK_SHUFFLE_ENEMY_DROPS);
    ResetPending();
    RefreshSeedExclusions();
    COND_HOOK(OnSceneInit, enabled, [](int16_t) {
        ResetPending();
        RefreshSeedExclusions();
    });
    COND_HOOK(OnPlayerUpdate, enabled, []() { DeliverUncollected(); });
    COND_HOOK(OnActorDestroy, enabled, [](void* actor) { OnDropDestroyed(actor); });
    COND_HOOK(OnEnemyDefeat, enabled, [](void* actor) { OnDefeat(static_cast<Actor*>(actor)); });
}
static RegisterShipInitFunc initRuntime(RegisterRuntime, { "IS_RANDO" });
} // namespace Rando::EnemyFirstDrops
