// The runner inserts the complete production translation unit below, using
// the native game structs. Only hook dispatch, CVars and camera requests are
// service boundaries; the backup, restoration and scene-state code are real.
#include "z64.h"
extern "C" {
#include "functions.h"
#include "variables.h"
}
#include "soh/cvar_prefixes.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <memory>
#include <new>
#include <set>
#include <string>
#include <unordered_map>

namespace TestHooks {
struct OnCameraState {
    using Callback = std::function<void(PlayState*)>;
    static inline Callback callback;
};
struct OnPlayDestroy {
    using Callback = std::function<void()>;
    static inline Callback callback;
};
template <class Hook, class Callback> void Set(bool enabled, Callback callback) {
    Hook::callback = enabled ? typename Hook::Callback(callback) : typename Hook::Callback{};
}
} // namespace TestHooks
#define COND_HOOK(hook, enabled, ...) TestHooks::Set<TestHooks::hook>(enabled, __VA_ARGS__)
struct RegisterShipInitFunc {
    RegisterShipInitFunc(void (*)(), std::initializer_list<const char*>) {}
};

static int enabled = 1;
SaveContext gSaveContext;
extern "C" {
PlayState* gPlayState = nullptr;
}
extern "C" int32_t CVarGetInteger(const char* name, int32_t fallback) {
    assert(std::string(name) == "gEnhancements.DisableFixedCamera" && fallback == 0);
    return enabled;
}
extern "C" CollisionHeader* BgCheck_GetCollisionHeader(CollisionContext* ctx, s32 id) {
    assert(id == BGCHECK_SCENE);
    return ctx->colHeader;
}
extern "C" s32 Camera_RequestSetting(Camera* camera, s16 setting) {
    camera->setting = setting;
    return 1;
}
extern "C" s32 Camera_RequestMode(Camera* camera, s16 mode) {
    camera->mode = mode;
    return 1;
}

/* PRODUCTION */

static PlayState play;
static Player player;
static Vec3s cameraPose[3] = { { 0, 100, 200 }, { -1000, 0, 0 }, { 6000, 0, 0 } };

static std::unique_ptr<CamData[]> Records() {
    auto records = std::make_unique<CamData[]>(2);
    records[0] = { CAM_SET_PREREND_FIXED, 3, cameraPose };
    records[1] = { CAM_SET_NONE, 0, nullptr };
    return records;
}
static void Attach(CollisionHeader* header, CamData* records, SceneID scene) {
    header->cameraDataList = records;
    header->cameraDataListLen = 2;
    play.colCtx.colHeader = header;
    play.sceneNum = scene;
    play.mainCamera.setting = CAM_SET_PREREND_FIXED;
    play.mainCamera.camDataIdx = 0;
    gPlayState = &play;
}
static void Tick() {
    assert(TestHooks::OnCameraState::callback);
    TestHooks::OnCameraState::callback(&play);
}
static void DestroyPlay() {
    // Play_Destroy dispatches this before GameState_Destroy unloads alt/*.
    if (TestHooks::OnPlayDestroy::callback)
        TestHooks::OnPlayDestroy::callback();
    gPlayState = nullptr;
}
static void Start(bool initialTick = true) {
    gPlayState = nullptr;
    RegisterDisableFixedCamera();
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    if (initialTick) {
        gPlayState = &play;
        play.sceneNum = SCENE_HYRULE_FIELD;
        Tick();
    }
}
static void EnterMarket(CollisionHeader* header, CamData* records) {
    Attach(header, records, SCENE_MARKET_DAY);
    Tick();
    assert(header->cameraDataList != records);
    assert(header->cameraDataList[0].cameraSType == CAM_SET_NORMAL0);
    assert(records[0].cameraSType == CAM_SET_PREREND_FIXED);
}

int main(int argc, char** argv) {
    assert(argc == 2);
    const std::string test = argv[1];
    const bool firstScene = test.starts_with("first-entry-") || test == "menu-enables-before-entry" ||
                            test == "opening-cutscene-defers";
    Start(!firstScene);
    auto original = Records();
    if (test == "first-entry-market-day" || test == "first-entry-market-night" ||
        test == "menu-enables-before-entry") {
        if (test == "menu-enables-before-entry") {
            enabled = 0;
            RegisterDisableFixedCamera();
            enabled = 1;
            RegisterDisableFixedCamera();
        }
        CollisionHeader header{};
        Attach(&header, original.get(), test == "first-entry-market-night" ? SCENE_MARKET_NIGHT : SCENE_MARKET_DAY);
        Tick();
        assert(play.mainCamera.setting == CAM_SET_NORMAL0 && "saved option did not apply on first scene entry");
        assert(header.cameraDataList != original.get());
        assert(header.cameraDataList[0].cameraSType == CAM_SET_NORMAL0);
        DestroyPlay();
        assert(header.cameraDataList == original.get());
    } else if (test == "opening-cutscene-defers") {
        CollisionHeader header{};
        Attach(&header, original.get(), SCENE_LINKS_HOUSE);
        gSaveContext.cutsceneIndex = 0xFFF1;
        Tick();
        assert(header.cameraDataList == original.get());
        assert(play.mainCamera.setting == CAM_SET_PREREND_FIXED);
        gSaveContext.cutsceneIndex = 0;
        Tick();
        assert(play.mainCamera.setting == CAM_SET_NORMAL0 && "camera did not apply after protected opening cutscene");
        DestroyPlay();
        assert(header.cameraDataList == original.get());
    } else if (test == "reused-header-shop-entry") {
        alignas(CollisionHeader) unsigned char storage[sizeof(CollisionHeader)];
        auto* header = new (storage) CollisionHeader{};
        EnterMarket(header, original.get());
        DestroyPlay();
        header->~CollisionHeader();
        original.reset(); // The old alt collision resource is now unloaded.
        auto replacement = Records();
        header = new (storage) CollisionHeader{}; // Allocator reuses header address.
        Attach(header, replacement.get(), SCENE_BAZAAR);
        Tick();
        assert(header->cameraDataList == replacement.get() && "shop camera table was replaced by retired scene data");
        assert(header->cameraDataList[0].camPosData == cameraPose);
        header->~CollisionHeader();
    } else if (test == "unloaded-header-shop-entry") {
        auto* retired = new CollisionHeader{};
        EnterMarket(retired, original.get());
        DestroyPlay();
        delete retired;
        original.reset();
        CollisionHeader replacementHeader{};
        auto replacement = Records();
        Attach(&replacementHeader, replacement.get(), SCENE_BAZAAR);
        Tick(); // Must not write through the former resource's freed header.
        assert(replacementHeader.cameraDataList == replacement.get());
    } else if (test == "same-scene-reentry") {
        CollisionHeader first{}, second{};
        EnterMarket(&first, original.get());
        DestroyPlay();
        auto replacement = Records();
        Attach(&second, replacement.get(), SCENE_MARKET_DAY);
        Tick();
        assert(second.cameraDataList != replacement.get());
        assert(second.cameraDataList[0].cameraSType == CAM_SET_NORMAL0);
        DestroyPlay();
        assert(first.cameraDataList == original.get());
        assert(second.cameraDataList == replacement.get());
    } else if (test == "bottle-camera-return") {
        CollisionHeader header{};
        EnterMarket(&header, original.get());
        play.mainCamera.camDataIdx = 1;
        player.itemAction = PLAYER_IA_BOTTLE;
        player.stateFlags1 = PLAYER_STATE1_IN_ITEM_CS;
        Tick();
        assert(play.mainCamera.setting == CAM_SET_TURN_AROUND);
        player.stateFlags1 = 0;
        Tick();
        assert(play.mainCamera.setting == CAM_SET_NORMAL0 && play.mainCamera.camDataIdx == 1);
        enabled = 0;
        RegisterDisableFixedCamera();
        assert(header.cameraDataList == original.get());
    } else if (test == "option-off-restores-live-table") {
        CollisionHeader header{};
        EnterMarket(&header, original.get());
        enabled = 0;
        RegisterDisableFixedCamera();
        assert(header.cameraDataList == original.get());
        assert(header.cameraDataList[0].cameraSType == CAM_SET_PREREND_FIXED);
    } else if (test == "enabling-waits-for-scene-change") {
        enabled = 0;
        RegisterDisableFixedCamera();
        CollisionHeader header{};
        Attach(&header, original.get(), SCENE_MARKET_DAY);
        enabled = 1;
        RegisterDisableFixedCamera();
        Tick();
        assert(header.cameraDataList == original.get());
        Attach(&header, original.get(), SCENE_MARKET_NIGHT);
        Tick();
        assert(header.cameraDataList != original.get());
        enabled = 0;
        RegisterDisableFixedCamera();
    } else {
        assert(false && "unknown case");
    }
    std::puts("PASS");
}
