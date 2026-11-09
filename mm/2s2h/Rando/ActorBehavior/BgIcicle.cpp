#include "ActorBehavior.h"
#include "Rando/MMEnvironment.h"
#include "2s2h/ObjectExtension/ActorListIndex.h"
#include "2s2h/CustomItem/CustomItem.h"
#include <map>
#include <set>
extern "C" {
#include "variables.h"
#include "overlays/actors/ovl_Obj_Mure2/z_obj_mure2.h"
#include "overlays/actors/ovl_Obj_Bean/z_obj_bean.h"
#include "overlays/actors/ovl_Obj_Swprize/z_obj_swprize.h"
void ObjSwprize_DoNothing(ObjSwprize*, PlayState*);
void func_80C25640(ObjSwprize*);
}
namespace {
std::map<RandoCheckId, Actor*> activeDrops;
std::set<std::pair<s8, s16>> freshSoilBurrows;
s16 environmentalScene = -1;
s8 environmentalRoom = -1;
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
        if (!gPlayState || pending.scene != gPlayState->sceneId || pending.room != gPlayState->roomCtx.curRoom.num) {
            it = pendingDrops.erase(it);
            continue;
        }
        if (!Rando::MMEnvironment::CanDrop(RANDO_SAVE_CHECKS[check]) || activeDrops.contains(check)) {
            it = pendingDrops.erase(it);
            continue;
        }
        const auto pos = pending.pos;
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
        if (!item)
            ++it;
        else {
            activeDrops[check] = &item->actor;
            it = pendingDrops.erase(it);
        }
    }
}
RandoCheckId IdentifySource(Actor* actor, s8 slice = 0) {
    if (!gPlayState || !Rando::MMEnvironment::MatchesParams(actor->id, actor->params))
        return RC_UNKNOWN;
    return Rando::MMEnvironment::Identify(
        gPlayState->sceneId, Rando::MMEnvironment::NormalizeSetup(gPlayState->sceneId, gSaveContext.sceneLayer),
        actor->room, GetActorListIndex(actor), slice, actor->id);
}
bool HasUncollectedSoil(Actor* soil) {
    for (s8 slice = 0; slice < 3; ++slice) {
        const auto check = IdentifySource(soil, slice);
        if (check != RC_UNKNOWN && Rando::MMEnvironment::CanDrop(RANDO_SAVE_CHECKS[check]))
            return true;
    }
    return false;
}
bool BeanHasUncollectedSoil(Actor* bean) {
    const auto flag = OBJBEAN_GET_SWITCH_FLAG_1(bean, 0);
    for (auto* actor = gPlayState->actorCtx.actorLists[ACTORCAT_PROP].first; actor; actor = actor->next)
        if (actor->id == ACTOR_OBJ_SWPRIZE && actor->room == bean->room && OBJ_SWPRIZE_GET_SWITCH_FLAG(actor) == flag &&
            HasUncollectedSoil(actor))
            return true;
    return false;
}
void QueueSourceDrop(Actor* actor, RandoCheckId check, bool* should) {
    if (check == RC_UNKNOWN || !Rando::MMEnvironment::CanDrop(RANDO_SAVE_CHECKS[check]))
        return;
    *should = false;
    if (!activeDrops.contains(check)) {
        // Carried rocks retain their authored identity but break in the current physical room.
        pendingDrops.emplace(check,
                             PendingDrop{ gPlayState->sceneId, gPlayState->roomCtx.curRoom.num, actor->world.pos });
        SpawnPendingIcicleDrops();
    }
}
} // namespace
void Rando::ActorBehavior::InitBgIcicleBehavior() {
    for (const auto id : { ACTOR_BG_ICICLE, ACTOR_EN_ISHI, ACTOR_OBJ_BOMBIWA, ACTOR_OBJ_HAMISHI, ACTOR_OBJ_SWPRIZE }) {
        COND_ID_HOOK(ShouldActorInit, id, IS_RANDO, [](Actor* actor, bool*) {
            const auto check = IdentifySource(actor);
            if (check != RC_UNKNOWN && RANDO_SAVE_CHECKS[check].shuffled)
                SetObjectRandoCheckId(actor, check);
        });
    }
    COND_VB_SHOULD(VB_OBJ_MURE2_SET_CHILD_ROOM, IS_RANDO, {
        auto* parent = va_arg(args, ObjMure2*);
        const auto slice = va_arg(args, int);
        if (slice < 0 || slice >= OBJ_MURE2_CHILD_MAX || !parent->actors[slice] ||
            !MMEnvironment::MatchesParams(ACTOR_OBJ_MURE2, parent->actor.params))
            return;
        const auto check = MMEnvironment::Identify(
            gPlayState->sceneId, MMEnvironment::NormalizeSetup(gPlayState->sceneId, gSaveContext.sceneLayer),
            parent->actor.room, GetActorListIndex(&parent->actor), slice, ACTOR_OBJ_MURE2);
        if (check != RC_UNKNOWN && RANDO_SAVE_CHECKS[check].shuffled)
            SetObjectRandoCheckId(parent->actors[slice], check);
    });
    COND_VB_SHOULD(VB_ICICLE_DROP_COLLECTIBLE, IS_RANDO, {
        auto* actor = va_arg(args, Actor*);
        QueueSourceDrop(actor, GetObjectRandoCheckId(actor), should);
    });
    COND_VB_SHOULD(VB_ENVIRONMENT_DROP_COLLECTIBLE, IS_RANDO, {
        auto* actor = va_arg(args, Actor*);
        const auto slice = va_arg(args, int);
        const auto check = actor->id == ACTOR_OBJ_SWPRIZE ? IdentifySource(actor, slice) : GetObjectRandoCheckId(actor);
        QueueSourceDrop(actor, check, should);
    });
    COND_VB_SHOULD(VB_ENVIRONMENT_SOURCE_USED, IS_RANDO, {
        auto* actor = va_arg(args, Actor*);
        const auto check = GetObjectRandoCheckId(actor);
        if ((actor->id == ACTOR_OBJ_SWPRIZE && HasUncollectedSoil(actor)) ||
            (check != RC_UNKNOWN && MMEnvironment::CanDrop(RANDO_SAVE_CHECKS[check])))
            *should = false;
    });
    COND_VB_SHOULD(VB_ENVIRONMENT_SOIL_USED, IS_RANDO, {
        auto* bean = va_arg(args, Actor*);
        if (BeanHasUncollectedSoil(bean))
            *should = false;
    });
    COND_VB_SHOULD(VB_ENVIRONMENT_SOIL_BURROW, IS_RANDO, {
        auto* bean = va_arg(args, Actor*);
        if (BeanHasUncollectedSoil(bean)) {
            const auto flag = OBJBEAN_GET_SWITCH_FLAG_1(bean, 0);
            freshSoilBurrows.emplace(bean->room, flag);
            // A timed-out/unloaded reward may be retried in the same room, but only
            // after a new native three-bug interaction. Never restart an active cutscene.
            for (auto* actor = gPlayState->actorCtx.actorLists[ACTORCAT_PROP].first; actor; actor = actor->next) {
                if (actor->id != ACTOR_OBJ_SWPRIZE || actor->room != bean->room ||
                    OBJ_SWPRIZE_GET_SWITCH_FLAG(actor) != flag || !HasUncollectedSoil(actor))
                    continue;
                auto* soil = reinterpret_cast<ObjSwprize*>(actor);
                if (soil->actionFunc == ObjSwprize_DoNothing)
                    func_80C25640(soil);
            }
        }
    });
    COND_VB_SHOULD(VB_ENVIRONMENT_SOIL_TRIGGER, IS_RANDO, {
        auto* soil = va_arg(args, Actor*);
        if (!HasUncollectedSoil(soil))
            return;
        const auto key = std::make_pair(soil->room, static_cast<s16>(OBJ_SWPRIZE_GET_SWITCH_FLAG(soil)));
        *should = freshSoilBurrows.erase(key) != 0;
    });
    COND_ID_HOOK(OnActorUpdate, ACTOR_OBJ_BEAN, IS_RANDO, [](Actor* actor) {
        auto* bean = reinterpret_cast<ObjBean*>(actor);
        // Bean may initialize before its reward actor. Renew only native bug acceptance.
        if (bean->unk_1E4 == 5 && BeanHasUncollectedSoil(actor))
            bean->unk_1E4 = 0;
    });
    COND_ID_HOOK(OnActorUpdate, ACTOR_PLAYER, IS_RANDO, [](Actor*) {
        if (gPlayState &&
            (environmentalScene != gPlayState->sceneId || environmentalRoom != gPlayState->roomCtx.curRoom.num)) {
            freshSoilBurrows.clear();
            environmentalScene = gPlayState->sceneId;
            environmentalRoom = gPlayState->roomCtx.curRoom.num;
        }
        SpawnPendingIcicleDrops();
    });
    COND_ID_HOOK(OnActorDestroy, ACTOR_EN_ITEM00, IS_RANDO, [](Actor* actor) {
        for (auto it = activeDrops.begin(); it != activeDrops.end();) {
            if (it->second == actor)
                it = activeDrops.erase(it);
            else
                ++it;
        }
    });
    COND_HOOK(OnSceneInit, IS_RANDO, [](s8, s8) {
        activeDrops.clear();
        pendingDrops.clear();
        freshSoilBurrows.clear();
        environmentalScene = gPlayState ? gPlayState->sceneId : -1;
        environmentalRoom = gPlayState ? gPlayState->roomCtx.curRoom.num : -1;
    });
}
