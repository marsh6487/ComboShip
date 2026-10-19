#include "global.h"
#ifndef OOT_BROWSING_TEST
#include "overlays/actors/ovl_En_Takaraya/z_en_takaraya.h"
#endif
#include <cassert>
#include <cstdio>
#include <cmath>
#include <string>
#if __has_include("combo/BlackMarketBrowsing.h")
#include "combo/BlackMarketBrowsing.h"
BlackMarketBrowsing::State sBrowser;
#endif
PlayState* gPlayState;
static PlayState play;
static Player player;
static Camera mainCamera, shopCamera;
static bool active = true, accepted = true, advance;
static int state = TEXT_STATE_EVENT, purchases, allocations, cameraWrites, lastText, allocationId = 1, closes;
static int sSelected;
static bool sTalking;
static s16 sBrowseCamera = -1, sBrowseParent = -1;
static s16 sBrowseCameraUid;
static Camera* sBrowseCameraPtr;
static bool sBrowsePlayerWasHidden;
static int currentCs = -1, currentSub = -1, returnCamera;
struct KeeperState { SkelAnime skel; int timer = 90, eye; } sKeeper;
bool Active() { return active; }
bool Interior() { return active; }
void Purchase() { ++purchases; }
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
int CreateCamera(PlayState*) {
    ++allocations;
    if (allocationId < 0) return allocationId;
    shopCamera.uid = allocations;
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
void SetView(PlayState*, int id, Vec3f* at, Vec3f* eye) {
    assert(id == 1); shopCamera.at = *at; shopCamera.eye = *eye; ++cameraWrites;
}
void SetFov(PlayState*, int id, float fov) { assert(id == 1); shopCamera.fov = fov; }
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
#define Play_ChangeCameraSetting(...) ((void)0)
#define Play_CameraChangeSetting(...) ((void)0)
#define Play_SetCameraAtEye SetView
#define Play_CameraSetAtEye SetView
#define Play_SetCameraFov SetFov
#define Play_CameraSetFov SetFov
#define CutsceneManager_GetCurrentCsId() currentCs
#define CutsceneManager_GetCurrentSubCamId(id) currentSub
#define CutsceneManager_SetReturnCamera(id) (returnCamera = id)
void UpdateBrowsingCamera(PlayState*);
/* PRODUCTION */

int main() {
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
    assert(allocations == 1 && ActiveCamera(&play) == 1 && cameraWrites > 0 &&
           "accepting shop talk must start a native controlled browsing camera");
    assert(sTalking && sSelected == -1 && (player.stateFlags2 & (1u << 29)));
#ifdef OOT_BROWSING_TEST
    assert(mainCamera.childCamIdx==SUBCAM_FREE &&
           "manual shop camera must not hold native parent=MAIN cutscenes in the one-point queue");
#endif
    auto input = [&](u16 button, bool shouldAdvance = false) {
        play.state.input[0].press.button = button; advance = shouldAdvance;
        KeeperUpdate(&keeper,&play);
        play.state.input[0].press.button = 0; advance = false;
    };
    input(BTN_DRIGHT); assert(sSelected == 0);
    input(BTN_DUP); assert(sSelected == 1);
    input(BTN_DRIGHT); assert(sSelected == 3);
    input(BTN_DDOWN); assert(sSelected == 2);
    input(BTN_DLEFT); assert(sSelected == 0);
    for (int i=0;i<24;++i) input(0);
    assert(shopCamera.at.x>50 && shopCamera.eye.z>shopCamera.at.z && shopCamera.fov>0);
    input(BTN_DLEFT); assert(sSelected == -1);
    input(BTN_DLEFT); assert(sSelected == 4);
    input(BTN_DUP); assert(sSelected == 5);
    input(BTN_DLEFT); assert(sSelected == 7);
    input(BTN_DDOWN); assert(sSelected == 6);
    for (int i=0;i<24;++i) input(0);
    assert(std::abs(shopCamera.at.x+52.5f)<.05f);
    input(BTN_A,true); assert(lastText == kConfirmText && purchases == 0);
    input(BTN_DRIGHT); assert(sSelected == 6 && purchases == 0);
    play.msgCtx.choiceIndex = 1;
    input(BTN_A,true); assert(lastText == kBrowseText && purchases == 0);
    input(BTN_A,true); assert(lastText == kConfirmText);
    play.msgCtx.choiceIndex = 0;
    input(BTN_A,true); assert(lastText == kResultText && purchases == 1);
    input(0); assert(purchases == 1);
    input(BTN_A,true); assert(lastText == kBrowseText && purchases == 1);
    const int writesBeforePause = cameraWrites;
    play.pauseCtx.state = 1;
    input(BTN_DRIGHT); assert(sSelected == 6 && cameraWrites == writesBeforePause);
    play.pauseCtx.state = 0;
    input(BTN_B); assert(sSelected == -1 && sTalking);
    input(BTN_B); assert(!sTalking && ActiveCamera(&play)==0 && !play.cameraPtrs[1]);
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
    input(BTN_DRIGHT); input(BTN_A,true); assert(lastText==kConfirmText);
    input(BTN_B); assert(lastText==kBrowseText && purchases==1);
    // FastText/SkipText makes native Message_ShouldAdvance true for a held B.
    play.state.input[0].cur.button = BTN_B;
    input(0,true);
    assert(lastText==kBrowseText && purchases==1 &&
           "held B must not reopen confirmation or accept the default Buy choice");
    play.state.input[0].cur.button = 0;
    EndBrowsing(&play);
    accepted = true; state = TEXT_STATE_EVENT; input(0);
    Continue(&play,0x1234);
    const int closesBeforeForeignMessage = closes;
    MonitorBrowsing();
    assert(!sTalking && closes==closesBeforeForeignMessage && state==TEXT_STATE_EVENT &&
           "shop cleanup must preserve a replacement actor's dialogue");
    BlackMarketBrowsing::Axis axis;
    assert(axis.Step(30)==0 && axis.Step(60)==1);
    for (int i=0;i<9;++i) assert(axis.Step(60)==0);
    assert(axis.Step(60)==1 && axis.Step(-60)==-1 && axis.Step(0)==0 && axis.Step(-60)==-1);
    std::puts("PASS native shop: both banks/eight slots, pan, Buy/Cancel, B/back/leave, pause, held B, no camera slot, native return links, UID reuse, foreign dialogue and hide-flag restoration");
}
