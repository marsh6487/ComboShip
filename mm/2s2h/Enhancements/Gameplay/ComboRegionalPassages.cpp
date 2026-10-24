#ifdef COMBO_BUILD
#include "ComboRegionalPassages.h"
#include "ComboGameMode.h"
#include "entrances/MmRegionalBridgePlacements.h"

namespace {
using namespace ComboRegional;
using ComboMmRegional::Placement;
static_assert(SCENE_IKANAMAE == ComboMmRegional::kIkana.scene && SCENE_30GYOSON == ComboMmRegional::kGreatBay.scene &&
              SCENE_10YUKIYAMANOMURA == ComboMmRegional::kMountainWinter.scene &&
              SCENE_10YUKIYAMANOMURA2 == ComboMmRegional::kMountainSpring.scene);
struct Passage {
    PlayState* play = nullptr;
    const Placement* placement = nullptr;
    int slot = -1;
    u32 seed = 0;
    bool armed = false;
} sRegional;
PlayState* sEntrancePlay = nullptr;
unsigned sEntranceCount = 0;
} // namespace

extern "C" void ComboRegionalPassagesMm_Reset(void) {
    sRegional = {};
    sEntrancePlay = nullptr;
    sEntranceCount = 0;
}
extern "C" void ComboRegionalPassagesMm_ReadEntrances(PlayState* play, unsigned count) {
    // Scene headers can reuse the same PlayState arena. The next spawn command
    // must recapture ownership before either drawing or testing a return.
    sRegional = {};
    sEntrancePlay = play;
    sEntranceCount = count;
}
extern "C" void ComboRegionalPassagesMm_ReadSpawns(PlayState* play, const ActorEntry* entries, unsigned count) {
    sRegional = {};
    if (!Combo_IsCombinedGame() || !play || !entries || !play->setupEntranceList || !count || sEntrancePlay != play)
        return;
    const auto* placement = ComboMmRegional::ForScene(play->sceneId);
    if (!placement || static_cast<unsigned>(placement->entrance) >= sEntranceCount)
        return;
    const auto native = play->setupEntranceList[placement->entrance];
    if (native.spawn >= count || native.room != placement->room)
        return;
    // The native entrance chooses arrival/start animation/camera. The authored
    // endpoint never follows spawn mutations or searches for a nearby wall.
    sRegional.play = play;
    sRegional.placement = placement;
    sRegional.slot = gSaveContext.fileNum;
    sRegional.seed = gSaveContext.save.shipSaveInfo.rando.finalSeed;
}
extern "C" int ComboRegionalPassagesMm_Get(PlayState* play, int* route, Vec3f* anchor, s16* yaw) {
    const auto* placement = sRegional.placement;
    if (!Combo_IsCombinedGame() || !play || play != sRegional.play || !placement ||
        ComboMmRegional::ForScene(play->sceneId) != placement || play->roomCtx.curRoom.num != placement->room ||
        gSaveContext.gameMode != GAMEMODE_NORMAL || !ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) ||
        sRegional.slot != gSaveContext.fileNum || sRegional.seed != gSaveContext.save.shipSaveInfo.rando.finalSeed ||
        !play->colCtx.colHeader || !GET_PLAYER(play))
        return false;
    if (route)
        *route = placement->route;
    if (anchor)
        *anchor = { placement->mouth.pos.x, placement->mouth.pos.y, placement->mouth.pos.z };
    if (yaw)
        *yaw = placement->mouth.yaw;
    return true;
}
extern "C" int ComboRegionalPassagesMm_AtReturn(PlayState* play, const Vec3f* position) {
    if (!position || !ComboRegionalPassagesMm_Get(play, nullptr, nullptr, nullptr))
        return 0;
    const auto& placement = *sRegional.placement;
    const Point point{ position->x, position->y, position->z };
    if (Distance2(point, placement.mouth.pos) > placement.armDistance * placement.armDistance)
        sRegional.armed = true;
    return sRegional.armed && InMouth(placement.mouth, point) ? placement.route : 0;
}
extern "C" int ComboRegionalPassagesMm_ApplyArrival(int route) {
    if (!Combo_IsCombinedGame() || !ComboRegionalPassages_IsRoute(route) || gSaveContext.gameMode != GAMEMODE_NORMAL ||
        !ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum))
        return 0;
    const auto* placement =
        ComboMmRegional::ForArrival(route, CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE));
    // These native approach records own floor, facing, start animation and
    // camera. No respawn coordinates or cycle/progression flags are rewritten.
    gSaveContext.save.entrance = placement->scene == SCENE_IKANAMAE   ? ENTRANCE(ROAD_TO_IKANA, placement->entrance)
                                 : placement->scene == SCENE_30GYOSON ? ENTRANCE(GREAT_BAY_COAST, placement->entrance)
                                 : placement->scene == SCENE_10YUKIYAMANOMURA2
                                     ? ENTRANCE(MOUNTAIN_VILLAGE_SPRING, placement->entrance)
                                     : ENTRANCE(MOUNTAIN_VILLAGE_WINTER, placement->entrance);
    gSaveContext.save.cutsceneIndex = 0;
    gSaveContext.nextCutsceneIndex = 0xFFEF;
    gSaveContext.cutsceneTrigger = 0;
    // A portal checkpoint retains its ordinary resume marker. This explicit
    // destination replaces that resume, so let Player_Init dispatch the native
    // entrance start mode (including the mountain owl approach), not mode D.
    gSaveContext.save.shipSaveInfo.pauseSaveEntrance = -1;
    gSaveContext.save.isOwlSave = false;
    gSaveContext.respawnFlag = 0;
    return 1;
}
#endif
