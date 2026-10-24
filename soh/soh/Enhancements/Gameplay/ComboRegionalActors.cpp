#ifdef COMBO_BUILD
#include "ComboRegionalActors.h"
#include "ComboRegionalPassages.h"
#include "ComboGameMode.h"
#include "actors/RegionalActorPlacement.h"
extern "C" {
#include "src/overlays/actors/ovl_En_Viewer/static_story_actor.h"
}
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"

namespace {
using namespace ComboRegionalActors;
State sActors;
PlayState* sPlay = nullptr;
int sRoom = -1;
struct World {
    PlayState* play;
    Actor* Find(uintptr_t handle) const {
        if (!handle)
            return nullptr;
        for (const auto& list : play->actorCtx.actorLists)
            for (Actor* actor = list.head; actor; actor = actor->next)
                if ((uintptr_t)actor == handle && actor->update)
                    return actor;
        return nullptr;
    }
    bool Live(uintptr_t handle) const {
        return Find(handle) != nullptr;
    }
    void Kill(uintptr_t handle) {
        if (auto* actor = Find(handle))
            Actor_Kill(actor);
    }
    bool Authored(int family, const State& own) const {
        for (const auto& list : play->actorCtx.actorLists)
            for (Actor* actor = list.head; actor; actor = actor->next) {
                if (actor->id != ACTOR_EN_VIEWER || !actor->update ||
                    (actor->room != -1 && actor->room != play->roomCtx.curRoom.num) ||
                    StaticStoryActor_GetType(actor->params) != family)
                    continue;
                bool ours = false;
                for (auto handle : own.handles)
                    ours |= handle == (uintptr_t)actor;
                if (!ours)
                    return true;
            }
        return false;
    }
    bool Floor(Point* point) const {
        auto* player = GET_PLAYER(play);
        if (!player)
            return false;
        Vec3f probe{ point->x, point->y, point->z };
        CollisionPoly* poly = nullptr;
        s32 bg = BGCHECK_SCENE;
        const float y = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &poly, &bg, &player->actor, &probe);
        if (!poly || !std::isfinite(y) || y <= BGCHECK_Y_MIN || std::fabs(y - (point->y - 200)) > 100 ||
            SurfaceType_GetSceneExitIndex(&play->colCtx, poly, bg))
            return false;
        // The actor's body must fit; never perch on a wall/ceiling found above the approach floor.
        Vec3f bottom{ point->x, y + 5, point->z }, top{ point->x, y + 90, point->z }, hit{};
        CollisionPoly* blocked = nullptr;
        if (BgCheck_EntityLineTest1(&play->colCtx, &bottom, &top, &hit, &blocked, true, false, true, true, &bg))
            return false;
        point->y = y;
        return true;
    }
    uintptr_t Spawn(const Row& row, Point p, short yaw) const {
        return (uintptr_t)Actor_Spawn(&play->actorCtx, play, ACTOR_EN_VIEWER, p.x, p.y, p.z, 0, yaw, 0, row.params);
    }
};
void RegisterRegionalActors() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s16) {
        sActors = {};
        sPlay = nullptr;
        sRoom = -1;
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] {
        sActors = {};
        sPlay = nullptr;
        sRoom = -1;
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnActorDestroy>([](void* actor) {
        for (auto& handle : sActors.handles)
            if (handle == (uintptr_t)actor)
                handle = 0;
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(ComboRegionalActorsOot_Update);
}
RegisterShipInitFunc init(RegisterRegionalActors);
} // namespace
extern "C" void ComboRegionalActorsOot_Update(void) {
    auto* play = gPlayState;
    if (!play)
        return;
    World world{ play };
    if (sPlay != play || sRoom != play->roomCtx.curRoom.num) {
        if (sPlay == play)
            Clear(sActors, world);
        else
            sActors = {};
        sPlay = play;
        sRoom = play->roomCtx.curRoom.num;
    }
    if (!Combo_IsCombinedGame()) {
        Clear(sActors, world);
        return;
    }
    if (!play->state.running || play->roomCtx.status != 0 || !play->roomCtx.curRoom.segment ||
        play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF ||
        play->pauseCtx.state || play->csCtx.state != CS_STATE_IDLE || gSaveContext.cutsceneTrigger ||
        Player_InCsMode(play))
        return;
    int route = 0;
    Vec3f anchor{};
    s16 yaw = 0;
    const bool ready = ComboRegionalPassagesOot_Get(play, &route, &anchor, &yaw);
    Update(sActors, world, ready, route, { anchor.x, anchor.y, anchor.z }, yaw, true);
}
#endif
