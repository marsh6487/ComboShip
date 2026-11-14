#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>
#include "global.h"
#include "regs.h"
extern "C" {
#include "overlays/gamestates/ovl_opening/z_opening.h"
#include "overlays/gamestates/ovl_file_choose/z_file_select.h"
}
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/Enhancements/Gameplay/ComboForestActors.h"
#include "2s2h/Enhancements/Gameplay/ComboForestEpona.h"
#include "entrances/ForestActorPlacements.h"

SaveContext gSaveContext{};
PlayState* gPlayState;
GameState* gGameState;
int gComboGameMode = COMBO_GAME_BOTH;
int gComboVanillaMode;
static GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;
static RegEditor editor{};
RegEditor* gRegEditor = &editor;
Input D_801F6C18{};
u8 D_801D0D54;
s32 gTransitionTileState;
static TransitionTile sTransitionTile;
static std::unordered_map<Actor*, int> sStates;
static bool resourcesReady = true;
struct {
    bool Resolve() const { return resourcesReady; }
} sBridge;
namespace CustomMessage {
struct Entry { std::string msg; };
void LoadCustomMessageIntoFont(const Entry&) {}
}
namespace Ship::CrossRMRegistry {
void RegisterTeardownListener(void (*)()) {}
}

static PlayState play;
static Player player;
static GraphicsContext graphics;
static CollisionPoly floorPoly;
static bool collisionReady = true, allocationReady = true;
static float floorHeight;
static std::array<Actor, 8> actors;
static int allocated, frame, firstSpawnFrame;
static bool stepTransition;
struct DrawFrame {
    int frame, actors, alpha;
};
static std::vector<DrawFrame> draws;

static void require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

extern "C" f32 BgCheck_EntityRaycastFloor5(CollisionContext*, CollisionPoly** poly, s32* bgId, Actor*, Vec3f*) {
    *poly = collisionReady ? &floorPoly : nullptr;
    *bgId = BGCHECK_SCENE;
    return floorHeight;
}

static Actor* InsertActor(ActorContext* context, s16 id, s16 params, float x, float y, float z, s16 yaw) {
    require(allocated < static_cast<int>(actors.size()), "allocation boundary exhausted");
    auto* actor = &actors[allocated++];
    actor->id = id;
    actor->params = params;
    actor->world.pos = { x, y, z };
    actor->world.rot.y = yaw;
    const int category = id == ACTOR_EN_HORSE ? ACTORCAT_BG : ACTORCAT_NPC;
    actor->category = category;
    actor->next = context->actorLists[category].first;
    context->actorLists[category].first = actor;
    ++context->actorLists[category].length;
    return actor;
}

extern "C" Actor* Actor_Spawn(ActorContext* context, PlayState*, s16 id, f32 x, f32 y, f32 z,
                              s16, s16 yaw, s16, s32 params) {
    if (!allocationReady)
        return nullptr;
    if (!firstSpawnFrame)
        firstSpawnFrame = frame;
    return InsertActor(context, id, params, x, y, z, yaw);
}

extern "C" void GameState_SetFrameBuffer(GraphicsContext*) {}
void GameState_UpdateUiVisibility(GameState*) {}
extern "C" void GameState_Draw(GameState*, GraphicsContext*) {}
extern "C" void GameState_DrawEnd(GraphicsContext*) {}
extern "C" void DebugDisplay_Init() {}
extern "C" void FrameInterpolation_StartRecord() {}
extern "C" void FrameInterpolation_StopRecord() {}
extern "C" s16 CutsceneManager_Update() { return 0; }
extern "C" void CutsceneManager_ClearWaiting() {}
extern "C" int Combo_MM_CompleteCrossDungeonTransition() { return 0; }
extern "C" void Combo_RequestOwlSaveQuit() {}
extern "C" void TitleSetup_Init(GameState*) {}
extern "C" void FileSelect_Init(GameState*) {}
extern "C" void Play_Init(GameState*) {}
extern "C" void TransitionTile_Destroy(TransitionTile*) {}
extern "C" void GameState_SetFramerateDivisor(GameState* state, s32 divisor) { state->framerateDivisor = divisor; }
extern "C" void Play_ClearTransition(PlayState*) {}
static void NativeTransitionStep(PlayState*);
extern "C" void Play_Update(PlayState* state) {
    if (!stepTransition)
        return;
    if (state->transitionTrigger == TRANS_TRIGGER_END && state->transitionMode == TRANS_MODE_OFF)
        state->transitionMode = TRANS_MODE_INSTANCE_INIT;
    NativeTransitionStep(state);
}
extern "C" void Play_Draw(PlayState* state) {
    auto* fade = reinterpret_cast<TransitionFade*>(&state->transitionCtx.instanceData);
    draws.push_back({ frame, allocated, stepTransition ? fade->color.a : 0 });
}

/* NATIVE */
/* PRODUCTION */

static void Reset(bool solo = false, bool incoming = false) {
    play = {};
    player = {};
    gSaveContext = {};
    actors = {};
    editor = {};
    allocated = frame = firstSpawnFrame = 0;
    draws.clear();
    resourcesReady = collisionReady = allocationReady = true;
    floorHeight = 0;
    stepTransition = incoming;
    gComboGameMode = solo ? COMBO_GAME_MM : COMBO_GAME_BOTH;
    gComboVanillaMode = 0;
    gPlayState = &play;
    gGameState = &play.state;
    play.state.running = true;
    play.state.main = Play_Main;
    play.state.gfxCtx = &graphics;
    play.state.framerateDivisor = 3;
    play.sceneId = solo ? COMBO_SOLO_FOREST_MM_SCENE : SCENE_LOST_WOODS;
    play.roomCtx.curRoom.num = 0;
    play.roomCtx.curRoom.segment = reinterpret_cast<void*>(1);
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].length = 1;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.fileNum = 0;
    gSaveContext.save.entrance = solo ? COMBO_SOLO_FOREST_MM_ENTRANCE : ENTRANCE(LOST_WOODS, 0);
    gSaveContext.save.saveInfo.playerData.threeDayResetCount = 1;
    if (incoming) {
        play.transitionTrigger = TRANS_TRIGGER_END;
        play.transitionCtx.transitionType = TRANS_TYPE_FADE_BLACK;
        play.transitionCtx.init = TransitionFade_Init;
        play.transitionCtx.destroy = TransitionFade_Destroy;
        play.transitionCtx.update = TransitionFade_Update;
        play.transitionCtx.setType = TransitionFade_SetType;
        play.transitionCtx.setColor = TransitionFade_SetColor;
        play.transitionCtx.start = TransitionFade_Start;
        play.transitionCtx.isDone = TransitionFade_IsDone;
    }
    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnSceneInit>(play.sceneId, 0);
}

static void Tick() {
    ++frame;
    GameState_Update(&play.state);
}

static void CheckIncoming() {
    for (bool walking : { false, true }) {
        for (bool solo : { false, true }) {
            Reset(solo, true);
            // Native StartMode_E sets this entrance-walk bit. Motion itself is a
            // controlled input; the real Player_InCsMode must report it as blocked.
            player.stateFlags1 = walking ? PLAYER_STATE1_20000000 : 0;
            if (walking)
                require(Player_InCsMode(&play), "native entrance movement must block player control in this case");
            for (int tick = 0; tick < 28; ++tick) {
                if (tick == 25)
                    player.stateFlags1 = 0;
                Tick();
            }
            const auto visible = std::find_if(draws.begin(), draws.end(), [](const auto& draw) { return draw.alpha < 255; });
            const auto companion = std::find_if(draws.begin(), draws.end(), [](const auto& draw) { return draw.actors == 2; });
            std::printf("arrival mode=%s, entrance walk=%d: first visible scene=%d, spawned=%d, "
                        "first draw with companion actors=%d\n",
                        solo ? "solo MM" : "combined", walking, visible == draws.end() ? -1 : visible->frame,
                        firstSpawnFrame, companion == draws.end() ? -1 : companion->frame);
            require(visible != draws.end() && visible->actors == 2,
                    "Saria and Epona must exist in the first partially revealed native arrival frame");
            require(draws.front().actors == 2 && firstSpawnFrame == 1,
                    "loaded incoming forest companions must be created before the first scene draw");
            require(allocated == 2, "arrival frames must not duplicate companions");
            require(play.transitionTrigger == TRANS_TRIGGER_OFF && play.transitionMode == TRANS_MODE_OFF,
                    "native arrival fade must still finish normally");
        }
    }
}

static void CheckIdle() {
    Reset();
    Tick();
    require(draws.front().actors == 2, "idle loaded room must create companions before its first draw");
    for (int tick = 0; tick < 8; ++tick)
        Tick();
    require(allocated == 2, "idle frames must not duplicate companions");

    Reset();
    player.stateFlags1 = PLAYER_STATE1_20000000;
    require(Player_InCsMode(&play), "controlled entrance walk must use the actual native player guard");
    Tick();
    require(draws.front().actors == 2, "visual creation must not wait for native entrance movement to end");

    Reset();
    play.roomCtx.status = 1;
    Tick();
    require(allocated == 0, "incomplete room must not spawn companions");
    play.roomCtx.status = 0;
    Tick();
    require(draws.back().actors == 2, "first ready room tick must create companions before drawing");

    Reset(false, true);
    play.roomCtx.status = 1;
    Tick();
    require(allocated == 0, "incoming fade must not bypass room loading");
    play.roomCtx.status = 0;
    Tick();
    require(draws.back().actors == 2 && draws.back().alpha < 255,
            "room readiness during an incoming fade must spawn before that tick's revealed draw");

    Reset();
    InsertActor(&play.actorCtx, ACTOR_EN_VIEWER, kWaitingParams, -695, 0, -313, 0);
    InsertActor(&play.actorCtx, ACTOR_EN_HORSE, COMBO_FOREST_EPONA_PARAMS, -400, 0, -313, 0);
    Tick();
    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnRoomInit>(play.sceneId, 0);
    Tick();
    require(allocated == 2 && firstSpawnFrame == 0, "authored companions must win across room-init resets");

    Reset();
    Tick();
    play.actorCtx.actorLists[ACTORCAT_NPC] = {};
    play.actorCtx.actorLists[ACTORCAT_BG] = {};
    allocated = 0;
    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnRoomInit>(play.sceneId, 0);
    Tick();
    require(draws.back().actors == 2 && allocated == 2, "a later room arrival must recreate removed companions once");
}

static void CheckGuards() {
    using Change = void (*)();
    const Change blocked[] = {
        [] { gPlayState = nullptr; },
        [] { gGameState = nullptr; },
        [] { play.state.running = false; },
        [] { gComboGameMode = COMBO_GAME_OOT; },
        [] { gComboGameMode = 99; },
        [] { play.sceneId = SCENE_CLOCKTOWER; },
        [] { play.roomCtx.curRoom.num = 1; },
        [] { play.roomCtx.status = 1; },
        [] { play.roomCtx.curRoom.segment = nullptr; },
        [] { play.actorCtx.actorLists[ACTORCAT_PLAYER].first = nullptr; },
        [] { player.stateFlags1 = PLAYER_STATE1_DEAD; },
        [] { play.gameOverCtx.state = GAMEOVER_DEATH_START; },
        [] { gSaveContext.gameMode = GAMEMODE_FILE_SELECT; },
        [] { gSaveContext.fileNum = 3; },
        [] { gSaveContext.sceneLayer = 1; },
        [] { gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 1); },
        [] { gSaveContext.save.cutsceneIndex = 0xFFF0; },
        [] { gComboVanillaMode = 1; gSaveContext.save.saveInfo.playerData.threeDayResetCount = 0; },
        [] { play.transitionTrigger = TRANS_TRIGGER_START; },
        [] { play.transitionTrigger = 7; },
        [] { play.transitionTrigger = TRANS_TRIGGER_OFF; play.transitionMode = TRANS_MODE_INSTANCE_RUNNING; },
        [] { play.pauseCtx.state = PAUSE_STATE_MAIN; },
        [] { play.csCtx.state = CS_STATE_RUN; },
        [] { gSaveContext.cutsceneTrigger = 1; },
        [] { play.soaringCsOrSoTCsPlaying = true; },
    };
    for (bool incoming : { false, true }) {
        for (auto change : blocked) {
            Reset(false, incoming);
            change();
            Tick();
            require(allocated == 0, "nonexploration, transition, cutscene, room or teardown guard allowed a spawn");
        }
    }
    Reset();
    resourcesReady = false;
    Tick();
    require(allocated == 1 && actors[0].id == ACTOR_EN_HORSE,
            "unavailable Saria bridge must prevent Saria without suppressing independent Epona");
    Reset();
    collisionReady = false;
    Tick();
    require(allocated == 0, "missing loaded floor polygon must prevent both spawns");
    collisionReady = true;
    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnRoomInit>(play.sceneId, 0);
    Tick();
    require(draws.back().actors == 2, "room retry after missing collision must retain before-draw ordering");
    for (float invalid : { BGCHECK_Y_MIN, std::numeric_limits<float>::quiet_NaN() }) {
        Reset();
        floorHeight = invalid;
        Tick();
        require(allocated == 0, "invalid floor must not produce a floating companion");
    }
    Reset();
    allocationReady = false;
    Tick();
    require(allocated == 0, "failed native allocation boundary must remain unspawned");
    std::printf("PASS: %zu readiness guards during idle and arrival plus resource, floor, allocation and duplicate controls\n",
                std::size(blocked));
}

static void CheckField() {
    Reset();
    play.sceneId = SCENE_00KEIKOKU;
    player.stateFlags1 = PLAYER_STATE1_20000000;
    Tick();
    require(allocated == 0, "Termina Field song-giver must retain its native player-control guard");
    player.stateFlags1 = 0;
    play.transitionTrigger = TRANS_TRIGGER_END;
    Tick();
    require(allocated == 0, "Termina Field song-giver must retain its transition guard");
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    Tick();
    require(draws.back().actors == 0 && allocated == 1 && actors[0].params == kSongGiftParams,
            "Termina Field song-giver must retain the existing post-draw hook and gift identity");
    require(actors[0].world.pos.x == 975 && actors[0].world.pos.y == -99 && actors[0].world.pos.z == 2101,
            "Termina Field authored song-giver pose must remain unchanged");
    Tick();
    require(draws.back().actors == 1 && allocated == 1, "Termina Field must contain one Saria and no Epona");
}

int main(int argc, char** argv) {
    RegisterSaria();
    const std::string selected = argc > 1 ? argv[1] : "all";
    if (selected == "all" || selected == "incoming") CheckIncoming();
    if (selected == "all" || selected == "idle") CheckIdle();
    if (selected == "all" || selected == "guards") CheckGuards();
    if (selected == "all" || selected == "field") CheckField();
    std::puts("PASS: native forest companion hook/draw sequencing and preserved context guards");
}
