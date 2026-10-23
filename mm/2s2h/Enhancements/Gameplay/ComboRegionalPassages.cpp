#ifdef COMBO_BUILD
#include "ComboRegionalPassages.h"
#include "ComboGameMode.h"
#include "entrances/RegionalPassages.h"
#include <cmath>
namespace {
using namespace ComboRegional;
struct Passage {
    PlayState* play = nullptr;
    Pose entry{}, mouth{};
    int route = 0, room = -1, slot = -1;
    u32 seed = 0;
    bool queried = false, ready = false, armed = false;
} sRegional;
PlayState* sEntrancePlay = nullptr;
unsigned sEntranceCount = 0;
int SceneRoute(int scene) {
    if (scene == SCENE_IKANAMAE)
        return COMBO_PORTAL_IKANA;
    if (scene == SCENE_30GYOSON)
        return COMBO_PORTAL_GREAT_BAY;
    if (scene == SCENE_10YUKIYAMANOMURA || scene == SCENE_10YUKIYAMANOMURA2)
        return COMBO_PORTAL_SNOWHEAD;
    return 0;
}
struct NativeWorld {
    PlayState* play;
    bool Floor(Point p, float expected, float* height) {
        Vec3f point{ p.x, expected + 100, p.z };
        CollisionPoly* floor = nullptr;
        s32 bg = BGCHECK_SCENE;
        const float y =
            BgCheck_EntityRaycastFloor5_3(play, &play->colCtx, &floor, &bg, &GET_PLAYER(play)->actor, &point);
        if (!floor || bg != BGCHECK_SCENE || !std::isfinite(y) || y == BGCHECK_Y_MIN || std::fabs(y - expected) > 35 ||
            floor->normal.y < 23170 || SurfaceType_GetSceneExitIndex(&play->colCtx, floor, bg) != 0)
            return false;
        WaterBox* water = nullptr;
        float waterY;
        if (WaterBox_GetSurface1_2(play, &play->colCtx, p.x, p.z, &waterY, &water) && waterY >= y - .5f)
            return false;
        *height = y;
        return true;
    }
    bool Wall(Point a, Point b, Point* hit, Point* normal) {
        Vec3f from{ a.x, a.y, a.z }, to{ b.x, b.y, b.z }, found{};
        CollisionPoly* wall = nullptr;
        s32 bg = BGCHECK_SCENE;
        if (!BgCheck_EntityLineTest1(&play->colCtx, &from, &to, &found, &wall, true, false, false, true, &bg) ||
            !wall || bg != BGCHECK_SCENE || std::abs(wall->normal.y) > 4096)
            return false;
        *hit = { found.x, found.y, found.z };
        *normal = { wall->normal.x / 32767.f, 0, wall->normal.z / 32767.f };
        return true;
    }
    bool Clear(Point a, Point b) {
        Vec3f from{ a.x, a.y, a.z }, to{ b.x, b.y, b.z }, found{};
        CollisionPoly* polygon = nullptr;
        s32 bg = BGCHECK_SCENE;
        // A hit of any kind obstructs the apron, including dynamic geometry,
        // floors/ceilings and backfaces. Only Wall() filters a support wall.
        return !BgCheck_EntityLineTest1(&play->colCtx, &from, &to, &found, &polygon, true, true, true, false, &bg);
    }
    bool NearWater(Point p) {
        for (float radius : { 120.f, 280.f, 500.f })
            for (int step = 0; step < 8; ++step) {
                const auto point = Add(p, Forward(static_cast<short>(step * 8192)), radius);
                WaterBox* box = nullptr;
                float y;
                if (WaterBox_GetSurface1_2(play, &play->colCtx, point.x, point.z, &y, &box) && std::isfinite(y))
                    return true;
            }
        return false;
    }
    bool Occupied(Point p, float radius) {
        for (int category = 0; category < ACTORCAT_MAX; ++category) {
            if (category == ACTORCAT_PLAYER || category == ACTORCAT_MISC || category == ACTORCAT_EXPLOSIVES)
                continue;
            for (Actor* a = play->actorCtx.actorLists[category].first; a; a = a->next) {
                if (!a->update || (a->room >= 0 && a->room != play->roomCtx.curRoom.num) ||
                    std::fabs(a->home.pos.y - p.y) > 180)
                    continue;
                if (Distance2({ a->home.pos.x, a->home.pos.y, a->home.pos.z }, p) < radius * radius)
                    return true;
            }
        }
        return false;
    }
};
} // namespace
extern "C" void ComboRegionalPassagesMm_Reset(void) {
    sRegional = {};
}
extern "C" void ComboRegionalPassagesMm_ReadEntrances(PlayState* play, unsigned count) {
    sEntrancePlay = play;
    sEntranceCount = count;
}
extern "C" void ComboRegionalPassagesMm_ReadSpawns(PlayState* play, const ActorEntry* entries, unsigned count) {
    sRegional = {};
    if (!Combo_IsCombinedGame() || !play || !SceneRoute(play->sceneId) || !entries || !play->setupEntranceList ||
        !count)
        return;
    if (sEntrancePlay != play || sEntranceCount == 0)
        return;
    const auto native = play->setupEntranceList[0];
    if (native.spawn >= count)
        return;
    const auto& source = entries[native.spawn];
    s16 yaw = (source.rot.y >> 7) & 0x1FF;
    if (!(source.id & 0x8000))
        yaw = DEG_TO_BINANG(yaw);
    else if (yaw > 180)
        yaw -= 360;
    sRegional.play = play;
    sRegional.entry = {
        { static_cast<float>(source.pos.x), static_cast<float>(source.pos.y), static_cast<float>(source.pos.z) }, yaw
    };
    sRegional.route = SceneRoute(play->sceneId);
    sRegional.room = native.room;
    sRegional.slot = gSaveContext.fileNum;
    sRegional.seed = gSaveContext.save.shipSaveInfo.rando.finalSeed;
}
extern "C" int ComboRegionalPassagesMm_Get(PlayState* play, int* route, Vec3f* anchor, s16* yaw) {
    if (!Combo_IsCombinedGame() || !play || play != sRegional.play || SceneRoute(play->sceneId) != sRegional.route ||
        play->roomCtx.curRoom.num != sRegional.room || gSaveContext.gameMode != GAMEMODE_NORMAL ||
        !ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) || sRegional.slot != gSaveContext.fileNum ||
        sRegional.seed != gSaveContext.save.shipSaveInfo.rando.finalSeed || !play->colCtx.colHeader ||
        !GET_PLAYER(play))
        return false;
    if (!sRegional.queried) {
        sRegional.queried = true;
        NativeWorld world{ play };
        sRegional.ready =
            FindMouth(world, sRegional.entry, &sRegional.mouth, sRegional.route == COMBO_PORTAL_GREAT_BAY);
    }
    if (!sRegional.ready)
        return false;
    if (route)
        *route = sRegional.route;
    if (anchor)
        *anchor = { sRegional.mouth.pos.x, sRegional.mouth.pos.y, sRegional.mouth.pos.z };
    if (yaw)
        *yaw = sRegional.mouth.yaw;
    return true;
}
extern "C" int ComboRegionalPassagesMm_AtReturn(PlayState* play, const Vec3f* position) {
    if (!position || !ComboRegionalPassagesMm_Get(play, nullptr, nullptr, nullptr))
        return 0;
    const Point point{ position->x, position->y, position->z };
    if (Distance2(point, sRegional.mouth.pos) > 110 * 110)
        sRegional.armed = true;
    return sRegional.armed && InMouth(sRegional.mouth, point) ? sRegional.route : 0;
}
extern "C" int ComboRegionalPassagesMm_ApplyArrival(int route) {
    if (!Combo_IsCombinedGame() || !ComboRegionalPassages_IsRoute(route) || gSaveContext.gameMode != GAMEMODE_NORMAL ||
        !ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum))
        return 0;
    // Native regional approach records own floor, facing and camera. Never
    // rewrite progression to make winter/spring, the coast or Ikana available.
    gSaveContext.save.entrance = route == COMBO_PORTAL_IKANA       ? ENTRANCE(ROAD_TO_IKANA, 0)
                                 : route == COMBO_PORTAL_GREAT_BAY ? ENTRANCE(GREAT_BAY_COAST, 0)
                                 : CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE)
                                     ? ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 0)
                                     : ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 0);
    gSaveContext.save.cutsceneIndex = 0;
    gSaveContext.nextCutsceneIndex = 0xFFEF;
    gSaveContext.cutsceneTrigger = 0;
    gSaveContext.save.isOwlSave = false;
    gSaveContext.respawnFlag = 0;
    return 1;
}
#endif
