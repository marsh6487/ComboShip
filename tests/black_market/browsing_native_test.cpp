#include "global.h"
#include "overlays/actors/ovl_En_Ossan/z_en_ossan.h"
#include <libultraship/bridge/consolevariablebridge.h>
#ifndef OOT_BROWSING_TEST
#include "overlays/actors/ovl_En_Takaraya/z_en_takaraya.h"
#endif
#include <cassert>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <string>
#include <array>
#include "combo/BlackMarketEntry.h"
#include "combo/BlackMarketShelves.h"
#include "combo/BlackMarketGraphicsArena.h"
#include "combo/BlackMarketReceipt.h"
BlackMarketReceipt::State sReceipt;
#if __has_include("combo/BlackMarketBrowsing.h")
#include "combo/BlackMarketBrowsing.h"
BlackMarketBrowsing::State sBrowser;
#endif
PlayState* gPlayState;
static PlayState play;
static Player player;
static Camera mainCamera, shopCamera;
static GraphicsContext graphics;
static std::array<Gfx, 128> commands;
alignas(16) static std::array<Gfx, 64> opaqueCommands;
static std::array<Gfx, 64> translucentCommands;
static bool mirroredWorld;
static int cursorSetups;
extern "C" int32_t CVarGetInteger(const char* name, int32_t fallback) {
#ifdef OOT_BROWSING_TEST
    assert(std::string(name) == CVAR_ENHANCEMENT("MirroredWorld"));
#else
    assert(std::string(name) == "gModes.MirroredWorld.State");
#endif
    assert(fallback == 0);
    return mirroredWorld;
}
static bool active = true, accepted = true, advance;
static int state = TEXT_STATE_EVENT, purchases, allocations, cameraWrites, lastText, allocationId = 1, closes;
static int cursorSounds;
static bool purchaseAccepted;
#ifdef OOT_BROWSING_TEST
void Sfx_PlaySfxCentered(u16 id) { assert(id == NA_SE_SY_CURSOR); ++cursorSounds; }
#else
void Audio_PlaySfx(u16 id) { assert(id == NA_SE_SY_CURSOR); ++cursorSounds; }
#endif
static int sSelected;
static bool sTalking;
static s16 sBrowseCamera = -1, sBrowseParent = -1;
static s16 sBrowseCameraUid;
static Camera* sBrowseCameraPtr;
static bool sBrowsePlayerWasHidden;
static bool sBrowseTextboxSaved;
static std::array<s16, 6> sBrowseTextboxLayout{};
static int currentCs = -1, currentSub = -1, returnCamera;
struct KeeperState { SkelAnime skel; int timer = 90, eye; } sKeeper;
bool Active() { return active; }
bool Interior() { return active; }
// Observe the queued-GI boundary; native transaction fixtures own item/rupee delivery.
bool Purchase() {
    ++purchases;
    return purchaseAccepted && sReceipt.Begin(sSelected, 0, 0, 0);
}
int GetState(MessageContext*) { return state; }
bool ShouldAdvance(PlayState*) { return advance; }
bool Accept(Actor*, PlayState*) { bool result = accepted; accepted = false; return result; }
void Continue(PlayState*, u16 text) {
    lastText = text;
#ifdef OOT_BROWSING_TEST
    play.msgCtx.textId = text;
#else
    play.msgCtx.currentTextId = text;
#endif
    state = text == 0x8F13 ? TEXT_STATE_CHOICE : TEXT_STATE_EVENT;
}
void Close(PlayState*) { state = TEXT_STATE_CLOSING; ++closes; }
void NativeCameraInit(Camera *, View *, CollisionContext *, PlayState *);
void InitNativeCameraTable();
int CreateCamera(PlayState*) {
    ++allocations;
    if (allocationId < 0) return allocationId;
    InitNativeCameraTable();
    NativeCameraInit(&shopCamera, &play.view, &play.colCtx, &play);
    play.cameraPtrs[1] = &shopCamera; return 1;
}
int ActiveCamera(PlayState*) {
#ifdef OOT_BROWSING_TEST
    return play.activeCamera;
#else
    return play.activeCamId;
#endif
}
void CameraStatus(PlayState*, int id, int status) {
    if (status == 7) {
#ifdef OOT_BROWSING_TEST
        play.activeCamera = id;
#else
        play.activeCamId = id;
#endif
    }
}
#define GameInteractor_Should(...) true
#define GameInteractor_InvertControl(...) 1
#define Play_GetCamera(play, id) ((play)->cameraPtrs[id])
/* NATIVE CAMERA */
static int nativeCameraFrames, nativeSettingRequests;
int SetCameraSetting(PlayState *state, s16 id, s16 setting) {
  ++nativeSettingRequests;
  return NativeSetting(state, id, setting);
}
void SetView(PlayState*, int id, Vec3f* at, Vec3f* eye) {
    assert(id == 1); assert(NativeAtEye(&play, id, at, eye)); ++cameraWrites;
}
void SetFov(PlayState*, int id, float fov) { assert(id == 1); assert(NativeFov(&play, id, fov)); }
#define Message_GetState GetState
#define Message_ShouldAdvance ShouldAdvance
#define Message_ContinueTextbox Continue
#define Message_CloseTextbox Close
#define Actor_ProcessTalkRequest Accept
#define Actor_TalkOfferAccepted(actor, state) Accept(actor, gPlayState)
#define Actor_OfferTalk(...) ((void)0)
#define Actor_SetFocus(...) ((void)0)
#define SkelAnime_Update(...) ((void)0)
#define Play_CreateSubCamera CreateCamera
#define Play_GetActiveCamId ActiveCamera
#define Play_ChangeCameraStatus CameraStatus
#define Play_GetCamera(play,id) ((play)->cameraPtrs[id])
#define Play_ClearCamera(play,id) ((play)->cameraPtrs[id] = nullptr)
#define Play_ChangeCameraSetting SetCameraSetting
#define Play_CameraChangeSetting SetCameraSetting
#define Play_SetCameraAtEye SetView
#define Play_CameraSetAtEye SetView
#define Play_SetCameraFov SetFov
#define Play_CameraSetFov SetFov
#define CutsceneManager_GetCurrentCsId() currentCs
#define CutsceneManager_GetCurrentSubCamId(id) currentSub
#define CutsceneManager_SetReturnCamera(id) (returnCamera = id)
/* NATIVE DEBUG */
#undef OPEN_DISPS
#undef CLOSE_DISPS
#ifdef OOT_BROWSING_TEST
#define OPEN_DISPS(ctx) { GraphicsContext* __gfxCtx = (ctx); Gfx* dispRefs[4]; \
    Graph_OpenDisps(dispRefs, ctx, __FILE__, __LINE__);
#define CLOSE_DISPS(ctx) Graph_CloseDisps(dispRefs, ctx, __FILE__, __LINE__); }
#else
#define OPEN_DISPS(ctx) { GraphicsContext* __gfxCtx = (ctx); Gfx* dispRefs[3]; Gfx dispVals[3]; \
    Graph_OpenDisps(dispRefs, dispVals, ctx, __FILE__, __LINE__);
#define CLOSE_DISPS(ctx) Graph_CloseDisps(dispRefs, dispVals, ctx, __FILE__, __LINE__); }
#endif
void SetupCursorOverlay(GraphicsContext* ctx) {
    static Gfx setup[] = { gsSPEndDisplayList() };
    OPEN_DISPS(ctx);
    ++cursorSetups;
    __gSPDisplayList(OVERLAY_DISP++, setup);
    CLOSE_DISPS(ctx);
}
#define Gfx_SetupDL39_Overlay SetupCursorOverlay
#define Gfx_SetupDL_39Overlay SetupCursorOverlay
void UpdateBrowsingCamera(PlayState*);
/* PRODUCTION */

void NativeCameraFrame() {
  if (!sTalking)
    return;
  assert(ActiveCamera(&play) == 1 && play.cameraPtrs[1] == &shopCamera);
  assert(shopCamera.setting == CAM_SET_FREE0 &&
         shopCamera.mode == CAM_MODE_NORMAL);
#ifdef OOT_BROWSING_TEST
  assert(sCameraSettings[shopCamera.setting]
             .cameraModes[shopCamera.mode]
             .funcIdx == CAM_FUNC_UNIQ6);
#else
  assert(
      sCameraSettings[shopCamera.setting].cameraModes[shopCamera.mode].funcId ==
      CAM_FUNC_UNIQUE6);
#endif
  assert(Camera_Unique6(&shopCamera));
  ++nativeCameraFrames;
  assert(
      shopCamera.animState == 1 &&
      "actual Unique6 must initialize, then retain the supplied manual camera");
  assert(shopCamera.eye.x == 0 && shopCamera.eye.y == 76 &&
         shopCamera.eye.z == 196 && shopCamera.eyeNext.x == 0 &&
         shopCamera.eyeNext.y == 76 && shopCamera.eyeNext.z == 196 &&
         "actual native camera frames must retain the close fixed eye "
         "throughout the shop turn");
  assert(shopCamera.at.x == sBrowser.view.at.x &&
         shopCamera.at.y == sBrowser.view.at.y &&
         shopCamera.at.z == sBrowser.view.at.z &&
         shopCamera.fov == sBrowser.view.fov);
}

void ResetCursorArena(size_t opa = 64, size_t xlu = 64, size_t overlay = 128) {
    graphics = {}; commands = {}; opaqueCommands = {}; translucentCommands = {}; cursorSetups = 0;
    graphics.polyOpa.p = opaqueCommands.data(); graphics.polyOpa.d = opaqueCommands.data() + opa;
    graphics.polyXlu.p = translucentCommands.data(); graphics.polyXlu.d = translucentCommands.data() + xlu;
    graphics.overlay.p = commands.data(); graphics.overlay.d = commands.data() + overlay;
    play.state.gfxCtx = &graphics;
}
void SetCursorProjection() {
    // A deterministic projection: native helpers must project the selected
    // actor's focus, then emit the cursor rectangle about that screen point.
    play.viewProjectionMtxF = {};
    play.viewProjectionMtxF.xx = .005f;
    play.viewProjectionMtxF.yy = .005f;
    play.viewProjectionMtxF.ww = 1;
}
void CheckCursorArenas() {
    SetCursorProjection();
    sSelected = sBrowser.slot = 0;
    sBrowser.view = BlackMarketBrowsing::Target(0);
    for (int shortage = 0; shortage < 6; ++shortage) {
        ResetCursorArena();
        if (shortage == 0) { graphics.overlay.p = commands.data() + commands.size(); graphics.overlay.d = graphics.overlay.p; }
        if (shortage == 1) graphics.overlay.d = graphics.overlay.p + 31;
        if (shortage == 2) { graphics.polyOpa.p = opaqueCommands.data() + opaqueCommands.size(); graphics.polyOpa.d = graphics.polyOpa.p; }
        if (shortage == 3) graphics.polyOpa.d = graphics.polyOpa.p + 7;
        if (shortage == 4) { graphics.polyXlu.p = translucentCommands.data() + translucentCommands.size(); graphics.polyXlu.d = graphics.polyXlu.p; }
        if (shortage == 5) graphics.polyXlu.d = graphics.polyXlu.p + 7;
        const auto before = graphics;
        const auto opaBefore = opaqueCommands;
        const auto xluBefore = translucentCommands;
        const auto overlayBefore = commands;
        DrawBrowseCursor();
        assert(cursorSetups == 0 && "short cursor arenas must skip before native debug/setup commands");
        assert(graphics.polyOpa.p == before.polyOpa.p && graphics.polyOpa.d == before.polyOpa.d);
        assert(graphics.polyXlu.p == before.polyXlu.p && graphics.polyXlu.d == before.polyXlu.d);
        assert(graphics.overlay.p == before.overlay.p && graphics.overlay.d == before.overlay.d);
        assert(!std::memcmp(opaqueCommands.data(), opaBefore.data(), sizeof(opaqueCommands)));
        assert(!std::memcmp(translucentCommands.data(), xluBefore.data(), sizeof(translucentCommands)));
        assert(!std::memcmp(commands.data(), overlayBefore.data(), sizeof(commands)));
    }
    ResetCursorArena(8, 8, 32);
    DrawBrowseCursor();
    assert(cursorSetups == 1 && graphics.polyOpa.p <= graphics.polyOpa.d &&
           graphics.polyXlu.p <= graphics.polyXlu.d && graphics.overlay.p <= graphics.overlay.d);
    bool rectangle = false;
    for (auto* packet = commands.data(); packet < graphics.overlay.p; ++packet)
        rectangle |= (packet->words.w0 >> 24) == G_TEXRECT;
    assert(rectangle && "a tight complete cursor budget must still draw the selection rectangle");
}
void CheckCursorAnchors() {
    SetCursorProjection();
    std::array<std::array<std::array<int, 2>, 8>, 2> centers{};
    for (int mirrored = 0; mirrored < 2; ++mirrored) for (int slot = 0; slot < 8; ++slot) {
        mirroredWorld = mirrored;
        sSelected = sBrowser.slot = slot;
        sBrowser.view = BlackMarketBrowsing::Target(slot);
        Actor item{};
        item.focus.pos = kShelves[slot];
        EnOssan nativeShop{};
        EnGirlA nativeItem{}; nativeItem.actor = item; nativeShop.cursorIndex = slot;
#ifdef OOT_BROWSING_TEST
        nativeShop.shelfSlots[slot] = &nativeItem;
#else
        nativeShop.items[slot] = &nativeItem;
#endif
        NativeCursorPos(&play, &nativeShop);
#ifdef OOT_BROWSING_TEST
        const int x = nativeShop.cursorX, y = nativeShop.cursorY;
#else
        const int x = nativeShop.cursorPos.x, y = nativeShop.cursorPos.y;
#endif
        ResetCursorArena();
        DrawBrowseCursor();
        bool found = false;
        for (auto* packet = commands.data(); packet < graphics.overlay.p; ++packet) {
            if ((packet->words.w0 >> 24) != G_TEXRECT)
                continue;
            const int right = (packet->words.w0 >> 12) & 0xfff, bottom = packet->words.w0 & 0xfff;
            const int left = (packet->words.w1 >> 12) & 0xfff, top = packet->words.w1 & 0xfff;
            centers[mirrored][slot] = { (left + right) / 8, (top + bottom) / 8 };
            assert(centers[mirrored][slot][0] == x && centers[mirrored][slot][1] == y &&
                   "all cursor rectangles must match native EnOssan projection in normal and mirrored worlds");
            found = true;
        }
        assert(found && graphics.overlay.p < commands.data() + commands.size());
    }
    for (int mirrored = 0; mirrored < 2; ++mirrored) for (int a = 0; a < 8; ++a)
        for (int b = a + 1; b < 8; ++b)
            assert(centers[mirrored][a] != centers[mirrored][b] && "all eight item anchors must have distinct projected cursor centers");
    for (int slot = 0; slot < 8; ++slot)
        assert(centers[1][slot][0] == SCREEN_WIDTH - centers[0][slot][0] && centers[1][slot][1] == centers[0][slot][1]);
    mirroredWorld = false;
    sSelected = sBrowser.slot = -1;
    sBrowser.view = BlackMarketBrowsing::Target(-1);
}

void CheckMirroredInput() {
    for (bool mirrored : { false, true }) for (int source = 0; source < 4; ++source) {
        mirroredWorld = mirrored;
        sBrowser = {}; sSelected = -1;
        Continue(&play, kWelcomeText);
        play.state.input[0] = {};
        if (source < 2) play.state.input[0].rel.stick_x = source == 0 ? 60 : -60;
        else play.state.input[0].press.button = source == 2 ? BTN_DRIGHT : BTN_DLEFT;
        EnOssan nativeShop{};
        NativeJoystick(&play, &nativeShop);
        const int intent = source < 2 ? (nativeShop.stickAccumX > 0 ? 1 : -1)
                                      : NativeDpadIntent(play.state.input[0].press.button);
        const int soundBefore = cursorSounds;
        BrowseInput(&play);
        assert(sSelected == (intent > 0 ? 0 : 4) && cursorSounds == soundBefore + 1 &&
               "analog and D-pad bank direction must match host native mirrored-world controls");
    }
    mirroredWorld = false; play.state.input[0] = {};
    sBrowser = {}; sSelected = -1; cursorSounds = 0;
    Continue(&play, kWelcomeText);
}

void CheckReleaseLifecycle(Actor* keeper) {
    const int purchaseBefore = purchases;
    const auto press = [&](u16 button, bool shouldAdvance = false) {
        play.state.input[0] = {}; play.state.input[0].press.button = button; advance = shouldAdvance;
        BrowseInput(&play);
        play.state.input[0] = {}; advance = false;
    };
    const auto freshTalk = [&] {
        if (sTalking) EndBrowsing(&play);
        gPlayState = &play; active = true; accepted = true; allocationId = 1;
        state = TEXT_STATE_EVENT; player.stateFlags2 = 0;
        play.transitionTrigger = TRANS_TRIGGER_OFF; play.transitionMode = TRANS_MODE_OFF;
        play.gameOverCtx.state = GAMEOVER_INACTIVE; play.pauseCtx.state = 0;
        play.csCtx.state = CS_STATE_IDLE;
        play.cameraPtrs[0] = &mainCamera; play.cameraPtrs[1] = play.cameraPtrs[2] = nullptr;
#ifdef OOT_BROWSING_TEST
        play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
        mainCamera.childCamIdx = SUBCAM_FREE;
#else
        play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
#endif
        CameraStatus(&play, 0, 7);
        KeeperUpdate(keeper, &play);
        assert(sTalking && sSelected == -1 && sBrowser.slot == -1 &&
               sBrowser.phase == BlackMarketBrowsing::Phase::Browse && ActiveCamera(&play) == 1);
    };
    const auto released = [&] {
      assert(!sTalking && sBrowseCamera == -1 && sBrowseParent == -1 &&
             sBrowseCameraUid == -1 && sBrowseCameraPtr == nullptr &&
             !sBrowseTextboxSaved &&
             sBrowser.phase == BlackMarketBrowsing::Phase::Browse &&
             sBrowser.slot == -1 && purchases == purchaseBefore);
    };
    for (auto phase : { BlackMarketBrowsing::Phase::Browse, BlackMarketBrowsing::Phase::Confirm,
                        BlackMarketBrowsing::Phase::Result }) {
        freshTalk(); sSelected = sBrowser.slot = 0; sBrowser.phase = phase;
        Continue(&play, phase == BlackMarketBrowsing::Phase::Confirm ? kConfirmText :
                        phase == BlackMarketBrowsing::Phase::Result ? kResultText : kBrowseText);
        // B must remain usable while the keeper/shelf camera is still turning.
        press(BTN_B);
        if (phase != BlackMarketBrowsing::Phase::Browse) {
            assert(sTalking && sBrowser.phase == BlackMarketBrowsing::Phase::Browse && sSelected == 0);
            press(BTN_B);
        }
        assert(sTalking && sSelected == -1);
        press(BTN_B); released();
        assert(ActiveCamera(&play) == 0 && !play.cameraPtrs[1] && !(player.stateFlags2 & (1u << 29)));
    }
    freshTalk(); sSelected = sBrowser.slot = 0;
    sBrowser.view = BlackMarketBrowsing::Target(0);
    sBrowser.phase = BlackMarketBrowsing::Phase::Result; Continue(&play, kResultText);
    press(BTN_A, true);
    assert(sTalking && sBrowser.phase == BlackMarketBrowsing::Phase::Browse && purchases == purchaseBefore);
    press(BTN_B); press(BTN_B); released();
    for (int interruption = 0; interruption < 5; ++interruption) {
        freshTalk();
        if (interruption == 0) state = TEXT_STATE_NONE;
        // Active() is the native controller's scene/file/session predicate.
        if (interruption == 1) active = false;
        if (interruption == 2) play.transitionTrigger = TRANS_TRIGGER_START;
        if (interruption == 3) play.transitionMode = static_cast<decltype(play.transitionMode)>(1);
        if (interruption == 4) play.gameOverCtx.state = static_cast<decltype(play.gameOverCtx.state)>(1);
        press(0); released();
        assert(ActiveCamera(&play) == 0 && !play.cameraPtrs[1] && !(player.stateFlags2 & (1u << 29)));
        freshTalk(); press(BTN_B); released(); // immediate re-talk must release again
    }
    freshTalk();
    gPlayState = nullptr; MonitorBrowsing(); released();
    freshTalk(); press(BTN_B); released();
    freshTalk();
    // Teardown callbacks run after native actor/camera cleanup. Poison those
    // addresses so any accidental lookup/dereference is caught by ASan/UBSan.
#ifdef OOT_BROWSING_TEST
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = reinterpret_cast<Actor*>(uintptr_t(1));
#else
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = reinterpret_cast<Actor*>(uintptr_t(1));
#endif
    play.cameraPtrs[sBrowseCamera] = reinterpret_cast<Camera*>(uintptr_t(1));
    sBrowseCameraPtr = reinterpret_cast<Camera*>(uintptr_t(1));
    EndBrowsing(&play, true); released();
    freshTalk(); press(BTN_B); released();
}


void CheckReceiptHandoff(Actor* keeper) {
    const int before = purchases;
    sReceipt.Cancel(); purchaseAccepted = true;
    gPlayState = &play; active = true; accepted = true; allocationId = 1;
    play.transitionTrigger = TRANS_TRIGGER_OFF; play.transitionMode = TRANS_MODE_OFF;
    play.gameOverCtx.state = GAMEOVER_INACTIVE; play.pauseCtx.state = 0;
    play.csCtx.state = CS_STATE_IDLE; player.stateFlags2 = 0x100;
    play.cameraPtrs[0] = &mainCamera; play.cameraPtrs[1] = play.cameraPtrs[2] = nullptr;
#ifdef OOT_BROWSING_TEST
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    mainCamera.childCamIdx = SUBCAM_FREE;
#else
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
#endif
    CameraStatus(&play, 0, 7); Continue(&play, kWelcomeText);
    KeeperUpdate(keeper, &play);
    assert(sTalking && ActiveCamera(&play) == 1 && (player.stateFlags2 & (1u << 29)));
    sSelected = sBrowser.slot = 0; sBrowser.view = BlackMarketBrowsing::Target(0);
    sBrowser.phase = BlackMarketBrowsing::Phase::Confirm;
    Continue(&play, kConfirmText); play.msgCtx.choiceIndex = 0;
    play.state.input[0] = {}; play.state.input[0].press.button = BTN_A; advance = true;
    BrowseInput(&play);
    play.state.input[0] = {}; advance = false;
    assert(purchases == before + 1 && sReceipt.pending && !sReceipt.delivered && sReceipt.slot == 0);
    assert(!sTalking && sBrowseCamera == -1 && sBrowseParent == -1 && !play.cameraPtrs[1] &&
           ActiveCamera(&play) == 0 && state == TEXT_STATE_CLOSING && player.stateFlags2 == 0x100 &&
           "accepted GI purchase must release the manual camera and shop-only hide bit before native presentation");
    const int camerasBefore = allocations, writesBefore = cameraWrites, textBefore = lastText;
    accepted = true; state = TEXT_STATE_EVENT;
    KeeperUpdate(keeper, &play);
    assert(accepted && !sTalking && allocations == camerasBefore && cameraWrites == writesBefore &&
           lastText == textBefore && purchases == before + 1 &&
           "pending GI receipt must not consume a keeper talk offer or restart browsing");
    Camera itemCamera{}; play.cameraPtrs[2] = &itemCamera; CameraStatus(&play, 2, 7);
    MonitorBrowsing(); KeeperUpdate(keeper, &play);
    assert(ActiveCamera(&play) == 2 && play.cameraPtrs[2] == &itemCamera && !play.cameraPtrs[1] &&
           allocations == camerasBefore && cameraWrites == writesBefore &&
           "pending receipt must leave the native get-item camera in control");
    sReceipt.Cancel(); purchaseAccepted = false;
    play.cameraPtrs[2] = nullptr; CameraStatus(&play, 0, 7);
    KeeperUpdate(keeper, &play);
    assert(sTalking && allocations == camerasBefore + 1 && ActiveCamera(&play) == 1 &&
           "keeper browsing must be available again after the receipt is retired");
    EndBrowsing(&play);
    assert(player.stateFlags2 == 0x100);
}

void CheckFixedCustomerEye() {
  BlackMarketBrowsing::State camera;
  for (int slot : {-1, 0, 1, 3, 2, -1, 4, 5, 7, 6, -1}) {
    camera.slot = slot;
    for (int frame = 0; frame < 24; ++frame) {
      camera.TickCamera();
      assert(camera.view.eye.x == 0 && camera.view.eye.y == 76 &&
             camera.view.eye.z == 196 &&
             "keeper, either bank and both rows must turn from one close "
             "customer eye without pulling behind Link");
    }
    assert(camera.CameraSettled());
  }
}

void CheckShelfFraming() {
    const float radius = .32f * 38, halfHeight = .32f * 42;
    const auto extension = [](float half, float floor, float extra) {
        return std::max((floor + 1.4f) / .875f, (std::max(half, floor) + 1.4f) / .875f - half) + extra + 1.f / 64;
    };
    // The authored item-fit contract includes selected scale, bob, tall feather
    // and wrapping X seal. Arbitrary replacement models are outside this proof.
    const Vec3f low{ -radius - extension(radius, 2.4f, .85f),
                     -2 - .32f * 22 - extension(halfHeight, 3.2f, .85f),
                     -radius - extension(radius, 2.4f, .45f) };
    const Vec3f high{ -low.x, 2 + .32f * 62 + extension(halfHeight, 3.2f, .85f), -low.z };
    for (int slot = 0; slot < 8; ++slot) {
        const auto view = BlackMarketBrowsing::Target(slot);
        const auto anchor = kShelves[slot];
        const int bank = slot < 4 ? 0 : 6;
        const int first = slot < 4 ? 0 : 4;
        assert(std::abs(view.at.x) > 35 && std::abs(view.at.x) < 70 &&
               view.at.z == anchor.z &&
               "shelf cameras must face the actual offer bank rather than a "
               "separate camera-only layout");
        const auto& bottom = BlackMarketShelves::kBoxes[bank + (slot & 1 ? 4 : 3)];
        const auto& top = BlackMarketShelves::kBoxes[bank + (slot & 1 ? 5 : 4)];
        assert(anchor.x > bottom.low.x && anchor.x < bottom.high.x &&
               anchor.z > bottom.low.z && anchor.z < bottom.high.z &&
               anchor.y + low.y > bottom.high.y && anchor.y + high.y < top.low.y &&
               "physical item rows and camera banks must correspond to the actual shelf openings");
        Vec3f forward{ view.at.x - view.eye.x, view.at.y - view.eye.y, view.at.z - view.eye.z };
        const auto dot = [](Vec3f a, Vec3f b) { return a.x*b.x + a.y*b.y + a.z*b.z; };
        const float length = std::sqrt(dot(forward, forward));
        forward = { forward.x / length, forward.y / length, forward.z / length };
        const float rightLength = std::hypot(forward.x, forward.z);
        const Vec3f right{ -forward.z / rightLength, 0, forward.x / rightLength };
        const Vec3f up{ -right.z * forward.y, right.z * forward.x - right.x * forward.z, right.x * forward.y };
        const float half = std::tan(view.fov * 3.14159265f / 360);
        const auto project = [&](Vec3f p) {
            const Vec3f d{ p.x - view.eye.x, p.y - view.eye.y, p.z - view.eye.z };
            const float depth = dot(d, forward);
            assert(depth > 0);
            return std::array<float, 2>{ 160 + dot(d, right) / depth / half * 120,
                                         120 - dot(d, up) / depth / half * 120 };
        };
        const auto cursor = project(anchor);
        assert(cursor[0] - 20 > 8 && cursor[0] + 20 < 312 && cursor[1] - 20 > 4 && cursor[1] + 20 < 142);
        for (int corner = 0; corner < 8; ++corner) {
            const auto point = project({ anchor.x + (corner & 1 ? high.x : low.x),
                                         anchor.y + (corner & 2 ? high.y : low.y),
                                         anchor.z + (corner & 4 ? high.z : low.z) });
            assert(point[0] > 8 && point[0] < 312 && point[1] > 8 &&
                   point[1] < 138 &&
                   "the selected row's item and seal must clear the native "
                   "bottom textbox at 4:3");
        }
    }
}

int main() {
  CheckFixedCustomerEye();
  CheckShelfFraming();
  InitNativeCameraTable();
  NativeCameraInit(&mainCamera, &play.view, &play.colCtx, &play);
  gPlayState = &play;
#ifdef OOT_BROWSING_TEST
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
#else
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
#endif
    play.cameraPtrs[0] = &mainCamera;
#ifdef OOT_BROWSING_TEST
    Actor keeper{};
#else
    EnTakaraya gal{}; Actor& keeper = gal.actor;
#endif
    KeeperUpdate(&keeper, &play);
    NativeCameraFrame();
    assert(allocations == 1 && ActiveCamera(&play) == 1 && cameraWrites > 0 &&
           "accepting shop talk must start a native controlled browsing camera");
    assert(sTalking && sSelected == -1 && (player.stateFlags2 & (1u << 29)));
    assert(shopCamera.eye.x == 0 && shopCamera.eye.y == 76 && shopCamera.eye.z == 196 &&
           shopCamera.at.y == 61 && shopCamera.at.z == 86 && shopCamera.fov == 46 &&
           "the welcome view must face the keeper from the customer side at conversational distance");
    CheckCursorArenas();
    CheckCursorAnchors();
    CheckMirroredInput();
#ifdef OOT_BROWSING_TEST
    assert(mainCamera.childCamIdx==SUBCAM_FREE &&
           "manual shop camera must not hold native parent=MAIN cutscenes in the one-point queue");
#endif
    auto input = [&](u16 button, bool shouldAdvance = false) {
        play.state.input[0].press.button = button; advance = shouldAdvance;
        KeeperUpdate(&keeper,&play);
        NativeCameraFrame();
        play.state.input[0].press.button = 0; advance = false;
    };
    auto finishPan = [&] { for (int i = 0; i < 24; ++i) input(0); };
    play.state.input[0].rel.stick_x = 20; input(0); play.state.input[0].rel.stick_x = 0;
    assert(cursorSounds == 0 && "stick deadzone and idle keeper frames must not play cursor audio");
    input(BTN_DRIGHT); assert(sSelected == 0 && cursorSounds == 1);
    input(BTN_DUP); assert(sSelected == 0 && "do not move between items while turning toward the shelf");
    input(BTN_A,true); assert(lastText != kConfirmText && purchases == 0 &&
                             "pan input must not select an item before the shelf is visible");
    assert(cursorSounds == 1 && "blocked pan inputs must not repeat the cursor sound");
    graphics.overlay.p = commands.data(); DrawBrowseCursor();
    assert(graphics.overlay.p == commands.data() && "hide the item cursor until the shelf pan finishes");
    finishPan();
    input(BTN_DUP); assert(sSelected == 1);
    finishPan();
    input(BTN_DRIGHT); assert(sSelected == 3);
    input(BTN_DDOWN); assert(sSelected == 2);
    finishPan();
    input(BTN_DLEFT); assert(sSelected == 0);
    input(BTN_DDOWN); assert(sSelected == 0 && cursorSounds == 5 &&
                            "only actual row/column changes play one native cursor cue");
    for (int i=0;i<24;++i) input(0);
    assert(shopCamera.at.x > 35 && shopCamera.at.x < 70 &&
           shopCamera.eye.z > shopCamera.at.z && shopCamera.fov > 0);
    const float rightYaw = std::atan2(shopCamera.at.x - shopCamera.eye.x,
                                     shopCamera.eye.z - shopCamera.at.z) * 180.f / 3.14159265f;
    assert(std::abs(rightYaw - 30) < .02f && "use native OoT's 30-degree right shelf direction");
    input(BTN_DLEFT); assert(sSelected == -1 && cursorSounds == 6);
    input(BTN_DLEFT); assert(sSelected == -1 && "return through the keeper view before changing shelf banks");
    assert(cursorSounds == 6);
    finishPan();
    input(BTN_DLEFT); assert(sSelected == 4);
    finishPan();
    input(BTN_DUP); assert(sSelected == 5);
    finishPan();
    input(BTN_DLEFT); assert(sSelected == 7);
    input(BTN_DDOWN); assert(sSelected == 6 && cursorSounds == 10);
    for (int i=0;i<24;++i) input(0);
    assert(std::abs(shopCamera.at.x + 86.f * .57735026919f) < .05f);
    const float leftYaw = std::atan2(shopCamera.at.x - shopCamera.eye.x,
                                    shopCamera.eye.z - shopCamera.at.z) * 180.f / 3.14159265f;
    assert(std::abs(leftYaw + 30) < .02f && "use native OoT's 30-degree left shelf direction");
    input(BTN_A,true); assert(lastText == kConfirmText && purchases == 0);
    input(BTN_DRIGHT); assert(sSelected == 6 && purchases == 0);
    play.msgCtx.choiceIndex = 1;
    input(BTN_A,true); assert(lastText == kBrowseText && purchases == 0);
    input(BTN_A,true); assert(lastText == kConfirmText);
    play.msgCtx.choiceIndex = 0;
    input(BTN_A,true); assert(lastText == kResultText && purchases == 1 && !sReceipt.pending &&
                             sTalking && ActiveCamera(&play) == 1 &&
                             "rejected purchase must retain browsing and show its result");
    input(0); assert(purchases == 1);
    input(BTN_A,true); assert(lastText == kBrowseText && purchases == 1);
    assert(cursorSounds == 10 && "confirmation, cancellation and results do not replay selection audio");
    const int writesBeforePause = cameraWrites;
    play.pauseCtx.state = 1;
    input(BTN_DRIGHT); assert(sSelected == 6 && cameraWrites == writesBeforePause);
    assert(cursorSounds == 10 && "paused navigation must remain silent");
    play.pauseCtx.state = 0;
    input(BTN_B); assert(sSelected == -1 && sTalking && cursorSounds == 11);
    input(BTN_B); assert(!sTalking && ActiveCamera(&play)==0 && !play.cameraPtrs[1]);
    assert(cursorSounds == 11 && "leaving the keeper view does not move a merchandise cursor");
    assert(!(player.stateFlags2 & (1u << 29)));
    player.stateFlags2 = (1u<<29) | 0x100;
    accepted = true; state = TEXT_STATE_EVENT; input(0);
    state = TEXT_STATE_CLOSING; input(0);
    assert(!sTalking && player.stateFlags2 == ((1u<<29)|0x100));
    player.stateFlags2 = 0x100;
    accepted = true; state = TEXT_STATE_EVENT; allocationId = -1; input(0);
    assert(!sTalking && ActiveCamera(&play)==0 && player.stateFlags2==0x100);
    allocationId = 1; accepted = true; state = TEXT_STATE_EVENT; input(0);
    Camera otherCamera{}; play.cameraPtrs[2] = &otherCamera;
    currentCs = 8; currentSub = 2; returnCamera = 1;
#ifdef OOT_BROWSING_TEST
    shopCamera.childCamIdx = 2; otherCamera.parentCamIdx = 1; mainCamera.childCamIdx = SUBCAM_FREE;
#endif
    CameraStatus(&play,2,7); MonitorBrowsing();
    assert(!sTalking && ActiveCamera(&play)==2 && !play.cameraPtrs[1] &&
           "a cutscene taking the camera must cancel shop ownership without stealing it back");
#ifdef OOT_BROWSING_TEST
    assert(otherCamera.parentCamIdx==0 && mainCamera.childCamIdx==2);
#else
    assert(returnCamera==0 && "native CutsceneManager must have a live return camera after shop interruption");
#endif
    currentCs = currentSub = -1;
    play.cameraPtrs[2] = nullptr;
#ifdef OOT_BROWSING_TEST
    mainCamera.childCamIdx = SUBCAM_FREE;
#endif
    CameraStatus(&play,0,7); accepted = true; state = TEXT_STATE_EVENT; input(0);
    const int cameraWritesBeforeReuse = cameraWrites;
    // The engines reuse subCameras[id-1] storage; pointer equality is insufficient.
    ++shopCamera.uid; MonitorBrowsing();
    assert(!sTalking && play.cameraPtrs[1]==&shopCamera && cameraWrites==cameraWritesBeforeReuse);
    play.cameraPtrs[1] = nullptr;
#ifdef OOT_BROWSING_TEST
    mainCamera.childCamIdx = SUBCAM_FREE;
#endif
    CameraStatus(&play,0,7); accepted = true; state = TEXT_STATE_EVENT; input(0);
    input(BTN_A,true); assert(sSelected == 0 && cursorSounds == 12 && "A enters the first bank with one cursor cue");
    finishPan(); input(BTN_A,true); assert(lastText==kConfirmText);
    input(BTN_B); assert(lastText==kBrowseText && purchases==1);
    // FastText/SkipText makes native Message_ShouldAdvance true for a held B.
    play.state.input[0].cur.button = BTN_B;
    input(0,true);
    assert(lastText==kBrowseText && purchases==1 &&
           "held B must not reopen confirmation or accept the default Buy choice");
    assert(cursorSounds == 12);
    play.state.input[0].cur.button = 0;
    EndBrowsing(&play);
    accepted = true; state = TEXT_STATE_EVENT; input(0);
    Continue(&play,0x1234);
    const int closesBeforeForeignMessage = closes;
    MonitorBrowsing();
    assert(!sTalking && closes==closesBeforeForeignMessage && state==TEXT_STATE_EVENT &&
           "shop cleanup must preserve a replacement actor's dialogue");
    active = false; accepted = true;
    const int allocationsBeforeEmptyStock = allocations;
    KeeperUpdate(&keeper, &play);
    assert(lastText==BlackMarketEntry::kUnavailableText && !sTalking && purchases==1 &&
           allocations==allocationsBeforeEmptyStock &&
           "empty stock offers a normal explanation without browsing or purchasing");
    CheckReleaseLifecycle(&keeper);
    CheckReceiptHandoff(&keeper);
    assert(nativeSettingRequests > 0 && nativeCameraFrames > 100);
    BlackMarketBrowsing::Axis axis;
    assert(axis.Step(30)==0 && axis.Step(60)==1);
    for (int i=0;i<9;++i) assert(axis.Step(60)==0);
    assert(axis.Step(60)==1 && axis.Step(-60)==-1 && axis.Step(0)==0 && axis.Step(-60)==-1);
    std::puts(
        "PASS shop adapter: actual native Camera_Init/setting/Unique6 frames "
        "retain fixed close eye/eyeNext, at/FOV and active subcamera through "
        "keeper/both banks/rows/return; eight normal/mirrored cursor "
        "projections/GBI; selected-row item/seal framing; navigation, "
        "purchase/receipt and ownership cleanup regressions");
}
