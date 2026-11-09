#include "ActorBehavior.h"
#include "Rando/MMEnvironment.h"
#include "2s2h/ObjectExtension/ActorListIndex.h"
#include "2s2h/CustomItem/CustomItem.h"
#include <map>
extern "C" {
#include "variables.h"
}
namespace {
std::map<RandoCheckId, Actor*> activeDrops;
struct PendingDrop {
    s16 scene;
    s8 room;
    Vec3f pos;
};
std::map<RandoCheckId, PendingDrop> pendingDrops;

void SpawnPendingIcicleDrops() {
    for (auto it = pendingDrops.begin(); it != pendingDrops.end();) {
        const auto check = it->first;
        const auto pending = it->second;
        // Room transitions do not necessarily invoke OnSceneInit. Never materialize a
        // reward at coordinates owned by another room; its renewable source can respawn.
        if (!gPlayState || pending.scene != gPlayState->sceneId || pending.room != gPlayState->roomCtx.curRoom.num) {
            it = pendingDrops.erase(it);
            continue;
        }
        const auto pos = pending.pos;
        if (!Rando::MMEnvironment::CanDrop(RANDO_SAVE_CHECKS[check]) || activeDrops.contains(check)) {
            it = pendingDrops.erase(it);
            continue;
        }
        auto* item = CustomItem::Spawn(
            pos.x, pos.y, pos.z, 0,
            CustomItem::KILL_ON_TOUCH | CustomItem::TOSS_ON_SPAWN | CustomItem::ABLE_TO_ZORA_RANG, check,
            [](Actor* actor, PlayState*) {
                auto& saved = RANDO_SAVE_CHECKS[CUSTOM_ITEM_PARAM];
                if (Rando::MMEnvironment::CanDrop(saved))
                    saved.eligible = true;
            },
            [](Actor* actor, PlayState*) {
                const auto check = static_cast<RandoCheckId>(CUSTOM_ITEM_PARAM);
                auto& saved = RANDO_SAVE_CHECKS[check];
                Matrix_Scale(30.0f, 30.0f, 30.0f, MTXMODE_APPLY);
                Rando::DrawItem(Rando::ConvertItem(saved.randoItemId, check), check, actor);
            });
        if (item == nullptr) {
            ++it; // Allocation can fail under actor pressure. Retry from the player update.
        } else {
            activeDrops[check] = &item->actor;
            it = pendingDrops.erase(it);
        }
    }
}
} // namespace

void Rando::ActorBehavior::InitBgIcicleBehavior() {
    COND_ID_HOOK(ShouldActorInit, ACTOR_BG_ICICLE, IS_RANDO, [](Actor* actor, bool*) {
        const auto check = MMEnvironment::IdentifyIcicle(gPlayState->sceneId, actor->room, GetActorListIndex(actor));
        if (check != RC_UNKNOWN && RANDO_SAVE_CHECKS[check].shuffled)
            SetObjectRandoCheckId(actor, check);
    });
    COND_VB_SHOULD(VB_ICICLE_DROP_COLLECTIBLE, IS_RANDO, {
        Actor* actor = va_arg(args, Actor*);
        const auto check = GetObjectRandoCheckId(actor);
        if (check == RC_UNKNOWN || !RANDO_SAVE_CHECKS[check].shuffled)
            return;
        if (MMEnvironment::CanDrop(RANDO_SAVE_CHECKS[check])) {
            *should = false;
            if (!activeDrops.contains(check)) {
                pendingDrops.emplace(check, PendingDrop{ gPlayState->sceneId, actor->room, actor->world.pos });
                SpawnPendingIcicleDrops();
            }
        }
    });
    COND_ID_HOOK(OnActorUpdate, ACTOR_PLAYER, IS_RANDO, [](Actor*) { SpawnPendingIcicleDrops(); });
    COND_ID_HOOK(OnActorDestroy, ACTOR_EN_ITEM00, IS_RANDO, [](Actor* actor) {
        for (auto it = activeDrops.begin(); it != activeDrops.end();) {
            if (it->second == actor)
                it = activeDrops.erase(it);
            else
                ++it;
        }
    });
    // A scene reload restores the physical icicles. Uncollected checks can be broken again.
    COND_HOOK(OnSceneInit, IS_RANDO, [](s8, s8) {
        activeDrops.clear();
        pendingDrops.clear();
    });
}
