#include "global.h"
#include "mods/items/custom_items.h"
#include "tests/test_require.h"
#include <string.h>
static bool hidden;
#if __has_include("ui_visibility.h")
#include "../../combo/gui/UiVisibility.h"
#define UiVisibility_IsHidden() hidden
#include "ui_visibility.h"
#endif
SaveContext gSaveContext;
CustomItemState gCustomItemState;
static unsigned pauseCalls, hudCalls, messageCalls, lightsCalls, boxCalls,
    maskCalls, warpCalls;
static Gfx work[128], opa[128], xlu[128], overlay[128];
static bool boxOpen, fairyWarp;
bool BoxMenu_IsOpen(void) { return boxOpen; }
bool MmMaskWear_IsGreatFairyWarpActive(void) { return fairyWarp; }
static void emit(PlayState *play) {
  GraphicsContext *gfx = play->state.gfxCtx;
  gfx->work.p++;
  gfx->polyOpa.p++;
  gfx->polyXlu.p++;
  gfx->overlay.p++;
  /* Graph tail allocations remain valid and must never be rewound. */
  --gfx->polyOpa.d;
}
void KaleidoScopeCall_Draw(PlayState *play) {
  ++pauseCalls;
  ++play->pauseCtx.cursorPoint[0];
  emit(play);
}
static Gfx sSetupDL_80125A60[] = {gsSPEndDisplayList()};
s32 gScreenWidth = SCREEN_WIDTH, gScreenHeight = SCREEN_HEIGHT;
void FrameInterpolation_RecordOpenChild(const void *source, int line) {}
void FrameInterpolation_RecordCloseChild(void) {}
void Graph_OpenDisps(Gfx **d, GraphicsContext *g, const char *file, s32 line) {}
void Graph_CloseDisps(Gfx **d, GraphicsContext *g, const char *file, s32 line) {
}
void gSPDisplayList(Gfx *command, Gfx *list) {
  __gSPDisplayList(command, list);
}
/* HUD internals are the controlled drawing boundary; native fade body is real.
 * The HUD scope is now local to Interface_Draw so its exit fade is retained. */
void Interface_Draw(PlayState *play) {
  OotUiDrawScope scope = OotUi_BeginDraw(play->state.gfxCtx);
  ++hudCalls;
  ++gSaveContext.timerSeconds;
  emit(play);
  OotUi_EndDraw(play->state.gfxCtx, scope);
  InterfaceContext *interfaceCtx = &play->interfaceCtx;
  OPEN_DISPS(play->state.gfxCtx);
  /* PRODUCTION_INTERFACE_FADE */
  CLOSE_DISPS(play->state.gfxCtx);
}
void Message_Draw(PlayState *play) {
  ++messageCalls;
  ++play->msgCtx.textDrawPos;
  emit(play);
}
void GameOver_FadeInLights(PlayState *play) { ++lightsCalls; }
void MmMaskWear_DrawOverlay(PlayState *play) {
  ++maskCalls;
  emit(play);
}
void MinishKaleido_Draw(PlayState *play) {
  ++warpCalls;
  emit(play);
}
void PostmanKaleido_Draw(PlayState *play) {
  ++warpCalls;
  emit(play);
}
void BoxMenu_Draw(PlayState *play) {
  ++boxCalls;
  emit(play);
}
/* PRODUCTION_OVERLAY */
int main(void) {
  static PlayState play;
  GraphicsContext gfx = {0};
  play.state.gfxCtx = &gfx;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  play.pauseCtx.state = 6;
  play.gameOverCtx.state = GAMEOVER_DEATH_WAIT_GROUND;
  for (unsigned mode = 0; mode < 2; ++mode) {
    hidden = mode;
    for (unsigned warp = 0; warp < 3; ++warp) {
      gfx.work.p = work;
      gfx.polyOpa.p = opa + 1;
      gfx.polyXlu.p = xlu;
      gfx.overlay.p = overlay;
      opa[0].words.w0 =
          0xDEADBEEF; /* previously emitted pause-world framebuffer */
      gfx.polyOpa.d = opa + 128;
      gCustomItemState.minishCapWarpMode = warp == 1;
      gCustomItemState.postmanHatWarpMode = warp == 2;
      unsigned oldPause = pauseCalls;
      unsigned oldHud = hudCalls;
      unsigned oldMessage = messageCalls;
      unsigned oldLights = lightsCalls;
      unsigned oldWarp = warpCalls;
      Play_DrawOverlayElements(&play);
      unsigned expected = (warp == 0) + 4 + (warp != 0);
      unsigned visible = hidden ? 1 : expected;
      REQUIRE(pauseCalls == oldPause + (warp == 0));
      REQUIRE(hudCalls == oldHud + 1 && messageCalls == oldMessage + 1 &&
              lightsCalls == oldLights + 1);
      REQUIRE(warpCalls == oldWarp + (warp != 0));
      REQUIRE(gfx.work.p == work + visible);
      REQUIRE(gfx.polyOpa.p == opa + 1 + visible);
      REQUIRE(opa[0].words.w0 == 0xDEADBEEF);
      REQUIRE(gfx.polyXlu.p == xlu + visible);
      REQUIRE(gfx.overlay.p == overlay + visible);
      REQUIRE(gfx.polyOpa.d == opa + 128 - expected);
    }
  }
  REQUIRE(gSaveContext.timerSeconds == 6 && play.msgCtx.textDrawPos == 6);
  REQUIRE(play.pauseCtx.cursorPoint[0] == 2 && maskCalls == 6 && boxCalls == 6);
  hidden = true;
  play.interfaceCtx.unk_244 = 128;
  gfx.work.p = work;
  gfx.polyOpa.p = opa + 1;
  gfx.polyXlu.p = xlu;
  gfx.overlay.p = overlay;
  gfx.polyOpa.d = opa + 128;
  Play_DrawOverlayElements(&play);
  REQUIRE(gfx.overlay.p ==
          overlay +
              5); /* Four native exit-fade commands plus world warp effect. */
  bool fadeFound = false;
  for (Gfx *command = overlay; command < gfx.overlay.p; ++command)
    fadeFound |= (command->words.w0 >> 24) == G_FILLRECT;
  REQUIRE(fadeFound);
  puts("PASS native OoT overlay boundary: hidden HUD/message/pause/warp/box "
       "draws retain state and tail allocations");
}
