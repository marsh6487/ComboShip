#include "global.h"
#include "mods/items/helpers/equip_helper.h"
#include "2s2h/resource/type/Scene.h"
#include "2s2h/resource/type/scenecommand/SetAlternateHeaders.h"
#include "2s2h/resource/type/scenecommand/SetEntranceList.h"
#include "2s2h/resource/type/scenecommand/SetStartPositionList.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
// Resource ownership/lifetime is a service boundary; scene and command data
// use the actual engine classes and their native implementations below.
namespace Ship {
IResource::IResource(std::shared_ptr<ResourceInitData> data) : mInitData(std::move(data)) {
}
IResource::~IResource() = default;
} // namespace Ship
SaveContext gSaveContext{};
PlayState* gPlayState;
GameState* gGameState;
static bool vanilla = true;
static bool Combo_IsVanilla() {
    return vanilla;
}
static bool blocked;
extern "C" u8 ItemInput_IsBlocked(Player*, PlayState*) {
    return blocked;
}
static ActorEntry anchor{};
static bool missingAnchor, missingFloor, dynamicFloor, steep, exitFloor, submerged, wall;
static CollisionPoly floorPoly{};
extern "C" f32 BgCheck_EntityRaycastFloor5_3(PlayState*, CollisionContext*, CollisionPoly** poly, s32* bg, Actor*,
                                             Vec3f*) {
    *poly = missingFloor ? nullptr : &floorPoly;
    *bg = dynamicFloor ? 1 : BGCHECK_SCENE;
    floorPoly.normal.y = steep ? 10000 : 32767;
    return missingFloor ? BGCHECK_Y_MIN : anchor.pos.y;
}
extern "C" u32 SurfaceType_GetSceneExitIndex(CollisionContext*, CollisionPoly*, s32) {
    return exitFloor;
}
extern "C" s32 WaterBox_GetSurface1_2(PlayState*, CollisionContext*, f32, f32, f32* height, WaterBox**) {
    *height = anchor.pos.y + 1;
    return submerged;
}
extern "C" s32 BgCheck_EntityLineTest1(CollisionContext*, Vec3f*, Vec3f*, Vec3f*, CollisionPoly**, s32, s32, s32, s32,
                                       s32*) {
    return wall;
}
constexpr float kAngle = 6.28318530718f / 65536.0f;
/* BINDINGS */
/* PRODUCTION_FUNCTIONS */
/* NATIVE_SPAWN_YAW */

int main() {
    PlayState play{};
    Player player{};
    gPlayState = &play;
    gGameState = &play.state;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    play.state.running = 1;
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    gSaveContext.fileNum = 0;
    gSaveContext.save.saveInfo.playerData.threeDayResetCount = 1;
    assert(Safe(&play));
    gSaveContext.save.saveInfo.playerData.threeDayResetCount = 0;
    assert(!Safe(&play));
    gSaveContext.save.saveInfo.playerData.threeDayResetCount = 1;
    vanilla = false;
    assert(!Safe(&play));
    vanilla = true;
    blocked = true;
    assert(!Safe(&play));
    blocked = false;
    player.actor.bgCheckFlags = 0;
    assert(!Safe(&play));
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    player.stateFlags1 |= PLAYER_STATE1_DEAD;
    assert(!Safe(&play));
    player.stateFlags1 = 0;
    play.transitionTrigger = TRANS_TRIGGER_START;
    assert(!Safe(&play));
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    play.pauseCtx.state = 1;
    assert(!Safe(&play));
    play.pauseCtx.state = 0;
    play.csCtx.state = CS_STATE_RUN;
    assert(!Safe(&play));
    play.csCtx.state = CS_STATE_IDLE;
    gSaveContext.timerStates[0] = TIMER_STATE_COUNTING;
    assert(!Safe(&play));
    gSaveContext.timerStates[0] = TIMER_STATE_OFF;
    auto scene = std::make_shared<SOH::Scene>();
    auto starts = std::make_shared<SOH::SetStartPositionList>(nullptr);
    auto entrances = std::make_shared<SOH::SetEntranceList>(nullptr);
    starts->cmdId = SOH::SceneCommandID::SetStartPositionList;
    entrances->cmdId = SOH::SceneCommandID::SetEntranceList;
    scene->commands = { entrances, starts };
    starts->startPositions.resize(1);
    play.sceneSegment = scene.get();
    for (const auto& binding : kBindings) {
        entrances->entrances.resize(binding.entrance + 1);
        entrances->entrances[binding.entrance] = { 0, static_cast<u8>(binding.room) };
        for (unsigned degrees : { 0u, 90u, 180u, 270u }) {
            for (unsigned cutscene : { 0u, 1u, 55u, 127u }) {
                anchor.pos = { 1000, 20, 2000 };
                starts->startPositions[0] = { ACTOR_PLAYER,
                                              anchor.pos,
                                              { 0, static_cast<s16>((degrees << 7) | cutscene), 0 },
                                              PLAYER_PARAMS(0xFF, PLAYER_START_MODE_E) };
                const auto unmodified = starts->startPositions[0];
                Vec3f first{}, second{};
                assert(Placement(&play, binding, 0, first));
                assert(Placement(&play, binding, 1, second));
                ActorEntry nativeEntry{};
                assert(NativeAnchor(&play, binding, nativeEntry));
                const float angle = NativeSpawnYaw(&nativeEntry) * kAngle;
                const float expectedX = 1000 + binding.right * std::cos(angle) + binding.forward * std::sin(angle);
                const float expectedZ = 2000 - binding.right * std::sin(angle) + binding.forward * std::cos(angle);
                assert(std::abs(first.x - expectedX) < .01f && std::abs(first.z - expectedZ) < .01f);
                assert(std::memcmp(&unmodified, &starts->startPositions[0], sizeof(unmodified)) == 0);
                const float distance = std::hypot(first.x - anchor.pos.x, first.z - anchor.pos.z);
                assert(std::abs(distance - std::hypot(binding.right, binding.forward)) < .01f);
                assert(std::abs(std::hypot(second.x - first.x, second.z - first.z) - 80) < .01f);
                assert(first.y == 20 && second.y == 20);
                for (auto* failure :
                     { &missingAnchor, &missingFloor, &dynamicFloor, &steep, &exitFloor, &submerged, &wall }) {
                    *failure = true;
                    if (failure == &missingAnchor)
                        play.sceneSegment = nullptr;
                    Vec3f rejected{};
                    assert(!Placement(&play, binding, 0, rejected));
                    play.sceneSegment = scene.get();
                    *failure = false;
                }
            }
        }
        ActorEntry rejected{};
        entrances->entrances[binding.entrance].room = binding.room + 1;
        assert(!NativeAnchor(&play, binding, rejected));
        entrances->entrances[binding.entrance].room = binding.room;
        entrances->entrances[binding.entrance].spawn = 1;
        assert(!NativeAnchor(&play, binding, rejected));
        entrances->entrances[binding.entrance].spawn = 0;
    }
    // Native alternates inherit lists already executed from the base header.
    const auto& binding = kBindings[0];
    entrances->entrances = { { 0, static_cast<u8>(binding.room) } };
    auto alternate = std::make_shared<SOH::Scene>();
    auto headers = std::make_shared<SOH::SetAlternateHeaders>(nullptr);
    headers->cmdId = SOH::SceneCommandID::SetAlternateHeaders;
    headers->headers = { alternate };
    scene->commands.push_back(headers);
    gSaveContext.sceneLayer = 1;
    ActorEntry inherited{};
    assert(NativeAnchor(&play, binding, inherited));
    assert(inherited.pos.x == starts->startPositions[0].pos.x);
    auto overrideStarts = std::make_shared<SOH::SetStartPositionList>(nullptr);
    overrideStarts->cmdId = SOH::SceneCommandID::SetStartPositionList;
    overrideStarts->startPositions = starts->startPositions;
    overrideStarts->startPositions[0].pos.x += 400;
    alternate->commands = { overrideStarts };
    assert(NativeAnchor(&play, binding, inherited));
    assert(inherited.pos.x == overrideStarts->startPositions[0].pos.x);
    headers->headers[0] = scene;
    assert(!NativeAnchor(&play, binding, inherited));
    headers->headers.clear(); // Break the deliberately malformed resource cycle.
    std::cout
        << "PASS MM fixed native-anchor transforms, unsafe geometry rejection and native idle/first-cycle guards\n";
}
