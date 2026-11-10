#include "ActorBehavior.h"
#include "Rando/MMEnvironment.h"
#include "Rando/MMEnvironmentCSMC.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/ObjectExtension/ActorListIndex.h"
#include "2s2h/CustomItem/CustomItem.h"
#include <map>
#include <set>
extern "C" {
#include "variables.h"
#include "objects/gameplay_keep/gameplay_keep.h"
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
} // namespace
static void DrawEnvironmentalMarker(Actor* actor, PlayState* play, RandoCheckId check, s8 slice) {
    if (!IS_RANDO || !play || !play->state.gfxCtx || !Rando::MMEnvironmentCSMC::IsEnvironmentalCheck(check))
        return;
    const auto& saved = RANDO_SAVE_CHECKS[check];
    if (!Rando::MMEnvironmentCSMC::CanMark(check, saved, pendingDrops.contains(check), activeDrops.contains(check)) ||
        saved.randoItemId <= RI_UNKNOWN || saved.randoItemId >= RI_MAX)
        return;
    const auto geometry = Rando::MMEnvironmentCSMC::GeometryFor(*actor);
    if (geometry.scale <= 0.0f)
        return;
    // This is the same live native/foreign category resolver as MM pots and
    // crates, including native progressives and foreign trap/importance policy.
    const auto category =
        CVarGetInteger("gRando.CSMC", 0)
            ? Rando::MMEnvironmentCSMC::CategoryForType(Rando::GetItemTypeForCheck(saved.randoItemId, check))
            : Rando::MMEnvironmentCSMC::Category::Major;
    const auto primary = ComboRando::ContainerCategoryColors::Get(category);
    const auto secondary = ComboRando::ContainerCategoryColors::Get(category, 1);
    Matrix_Push();
    Matrix_Translate(actor->world.pos.x, actor->world.pos.y + geometry.height, actor->world.pos.z, MTXMODE_NEW);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    if (actor->id == ACTOR_OBJ_SWPRIZE)
        Matrix_Translate((slice - 1) * geometry.spacing, 0.0f, 0.0f, MTXMODE_APPLY);
    Matrix_Scale(geometry.scale, geometry.scale, geometry.scale, MTXMODE_APPLY);
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL60_XluNoCD(play->state.gfxCtx);
    gDPPipeSync(POLY_XLU_DISP++);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, primary.r, primary.g, primary.b, 190);
    gDPSetEnvColor(POLY_XLU_DISP++, secondary.r, secondary.g, secondary.b, 0);
    gDPSetCombineLERP(POLY_XLU_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0, PRIMITIVE,
                      ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gLensFlareCircleDL);
    gDPPipeSync(POLY_XLU_DISP++);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
    Matrix_Pop();
}
static void DrawEnvironmentalSource(Actor* actor, PlayState* play) {
    if (actor->id == ACTOR_OBJ_SWPRIZE) {
        for (s8 slice = 0; slice < 3; ++slice)
            DrawEnvironmentalMarker(actor, play, IdentifySource(actor, slice), slice);
    } else {
        DrawEnvironmentalMarker(actor, play, Rando::ActorBehavior::GetObjectRandoCheckId(actor), 0);
    }
}
static void DrawEnvironmentalSoil(Actor* actor, PlayState* play) {
    // Native Swprize has no geometry or draw callback. A visual-only callback
    // makes its authored soil reward source renderable without changing Bean.
    DrawEnvironmentalSource(actor, play);
}
namespace {
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
            if (actor->id == ACTOR_OBJ_SWPRIZE && !actor->draw) {
                for (s8 slice = 0; slice < 3; ++slice) {
                    const auto soilCheck = IdentifySource(actor, slice);
                    if (soilCheck != RC_UNKNOWN && RANDO_SAVE_CHECKS[soilCheck].shuffled) {
                        actor->draw = DrawEnvironmentalSoil;
                        break;
                    }
                }
            }
        });
        COND_ID_HOOK(OnActorDraw, id, IS_RANDO, [](Actor* actor) {
            // Soil's visual-only draw already emitted its slices. An existing
            // non-null draw (if supplied elsewhere) keeps its native geometry.
            if (actor->draw != DrawEnvironmentalSoil)
                DrawEnvironmentalSource(actor, gPlayState);
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
