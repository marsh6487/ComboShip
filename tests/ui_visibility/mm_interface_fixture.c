#include "global.h"

#include "2s2h/BenGui/CosmeticEditor.h"
#include "2s2h/BenGui/HudEditor.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/Rando/RupeeCounterDigits.h"
#include "PR/gs2dex.h"
#include "interface/parameter_static/parameter_static.h"
#include "ui_visibility.h"
#include "z64snap.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int minimal, enabled, hidden, minimapDisabled;
static int hearts, magic, buttons, bIcons, cIcons, aLabels, dIcons, clocks,
    minigames, timers;
static int photoStarts, compressed, hooks;
SaveContext gSaveContext;
u32 gUpgradeMasks[8];
u8 gUpgradeShifts[8];
u16 gUpgradeCapacities[8][4] = {{0}};
static RegEditor editor;
RegEditor *gRegEditor = &editor;
static Gfx buffer[8192];
static Gfx work[512], xlu[512], overlay[8192], debug[512];
static Vtx vertices[64];
static TexturePtr sStoryTextures[2], sStoryTLUTs[2], sCounterTextures[10];
static Color_RGB8 sRupeeCounterIconPrimColors[4], sRupeeCounterIconEnvColors[4];
static Color_RGB8 sMinigameCountdownPrimColors[4];
static TexturePtr sMinigameCountdownTextures[4];
static s16 sMinigameCountdownTexWidths[4];
static Gfx sScreenFillSetupDL[1];
static s16 sPictoState;
enum {
  PICTO_BOX_STATE_OFF,
  PICTO_BOX_STATE_LENS,
  PICTO_BOX_STATE_SETUP_PHOTO,
  PICTO_BOX_STATE_PHOTO
};
enum { ucode_s2dex = 1 };
GfxMasterList D_0E000000;
u8 gWorkBuffer[PICTO_PHOTO_WIDTH * PICTO_PHOTO_HEIGHT];
HudEditorElement hudEditorElements[HUD_EDITOR_ELEMENT_MAX];
HudEditorElementID hudEditorActiveElement;

int UiVisibility_IsEnabled(void) { return enabled; }
int UiVisibility_IsHidden(void) { return enabled && hidden; }
s32 CVarGetInteger(const char *key, s32 fallback) {
  if (!strcmp(key, "gEnhancements.MinimalUI"))
    return minimal;
  if (!strcmp(key, "gEnhancements.Dpad.DpadEquips"))
    return 1;
  return fallback;
}
bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t result, ...) {
  return result;
}
void GameInteractor_ExecuteOnInterfaceDrawStart(void) { ++hooks; }
void Graph_OpenDisps(Gfx **refs, Gfx *values, GraphicsContext *gfx,
                     const char *file, s32 line) {}
void Graph_CloseDisps(Gfx **refs, Gfx *values, GraphicsContext *gfx,
                      const char *file, s32 line) {}
void FrameInterpolation_RecordOpenChild(const void *a, int b) {}
void FrameInterpolation_RecordCloseChild(void) {}
void *Graph_Alloc(GraphicsContext *gfx, size_t size) { return vertices; }
void gSPSegment(void *p, int segment, uintptr_t value) {}
void gSPInvalidateTexCache(Gfx *p, uintptr_t value) {}
void gSPDisplayList(Gfx *p, Gfx *list) {}
void gSPVertex(Gfx *p, uintptr_t v, int n, int v0) {}
void gDPSetPrimColorOverride(Gfx *p, u8 m, u8 l, u8 r, u8 g, u8 b, u8 a,
                             const char *id) {}
void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode mode) {}
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode mode) {}
Mtx *Matrix_Finalize(GraphicsContext *gfx) { return (Mtx *)vertices; }
void Gfx_SetupDL39_Overlay(GraphicsContext *gfx) {}
void Gfx_SetupDL39_Opa(GraphicsContext *gfx) {}
void Gfx_SetupDL42_Overlay(GraphicsContext *gfx) {}
void Interface_SetVertices(PlayState *play) {}
void Interface_SetOrthoView(InterfaceContext *ctx) {}
void LifeMeter_Draw(PlayState *play) { ++hearts; }
void Magic_DrawMeter(PlayState *play) { ++magic; }
void Map_DrawMinimap(PlayState *play) {}
void Attention_Draw(Attention *attention, PlayState *play) {}
void Interface_DrawItemButtons(PlayState *play) { ++buttons; }
void Interface_DrawBButtonIcons(PlayState *play) { ++bIcons; }
void Interface_DrawCButtonIcons(PlayState *play) { ++cIcons; }
void Interface_DrawDButtonIcons(PlayState *play) { ++dIcons; }
void Interface_DrawAButton(PlayState *play) { ++aLabels; }
void Interface_DrawPauseMenuEquippingIcons(PlayState *play) {}
void Interface_DrawClock(PlayState *play) {
  ++clocks;
  ++play->state.gfxCtx->overlay.p;
}
void Interface_DrawPerfectLetters(PlayState *play) {}
void Interface_DrawMinigameIcons(PlayState *play) { ++minigames; }
void Interface_DrawTimers(PlayState *play) { ++timers; }
void Sm64CapsHud_DrawImGui(void) {}
void Message_StartTextbox(PlayState *play, u16 id, Actor *actor) {
  assert(id == 0xF8);
  ++photoStarts;
}
void Interface_SetHudVisibility(u16 value) {
  gSaveContext.hudVisibility = value;
}
void Play_CompressI8ToI5(void *input, void *output, size_t size) {
  ++compressed;
}
void Prerender_DrawBackground2D(Gfx **p, void *image, void *tlut, u16 width,
                                u16 height, u8 fmt, u8 siz, u16 tt, u16 tw,
                                f32 x, f32 y, f32 scaleX, f32 scaleY,
                                u32 flags) {}
uint32_t SysUcode_GetUCode(void) { return 0; }
s16 Inventory_GetSkullTokenCount(s16 scene) { return 0; }
void HudEditor_SetActiveElement(HudEditorElementID id) {}
bool HudEditor_ShouldOverrideDraw(void) { return false; }
void HudEditor_ModifyDrawValues(s16 *x, s16 *y, s16 *w, s16 *h, s16 *dx,
                                s16 *dy) {}
f32 OTRGetRectDimensionFromLeftEdge(f32 value) { return value; }
f32 OTRGetRectDimensionFromRightEdge(f32 value) { return value; }
Gfx *Gfx_DrawTexRectIA8(Gfx *p, TexturePtr tex, s16 w, s16 h, s16 x, s16 y,
                        s16 rw, s16 rh, u16 dx, u16 dy) {
  return p + 1;
}
Gfx *Gfx_DrawTexRectI8(Gfx *p, TexturePtr tex, s16 w, s16 h, s16 x, s16 y,
                       s16 rw, s16 rh, u16 dx, u16 dy) {
  return p + 1;
}
Gfx *Gfx_DrawTexQuadIA8(Gfx *p, TexturePtr tex, s16 w, s16 h, u16 point) {
  return p + 1;
}
/* PRODUCTION */

int main(void) {
  GraphicsContext gfx = {0};
  PlayState play = {0};
  Player player = {0};
  ActionLabel labels[2] = {0};
  char *icons[4] = {0};
  play.state.gfxCtx = &gfx;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
  play.interfaceCtx.actionVtx = vertices;
  play.interfaceCtx.doActionSegment = labels;
  play.interfaceCtx.iconItemSegment = icons;
  player.transformation = PLAYER_FORM_HUMAN;
  gSaveContext.save.playerForm = PLAYER_FORM_HUMAN;
  gSaveContext.save.saveInfo.playerData.health = 0x30;
  gSaveContext.save.saveInfo.playerData.healthCapacity = 0x30;
  gSaveContext.magicState = MAGIC_STATE_IDLE;
  for (int hide = 0; hide < 3; ++hide) {
    gfx.work.p = work;
    gfx.work.d = work + 512;
    gfx.polyOpa.p = buffer;
    gfx.polyOpa.d = buffer + 8192;
    gfx.polyXlu.p = xlu;
    gfx.polyXlu.d = xlu + 512;
    gfx.overlay.p = overlay;
    gfx.overlay.d = overlay + 8192;
    gfx.debug.p = debug;
    gfx.debug.d = debug + 512;
    minimal = 1;
    enabled = 0;
    R_MINIMAP_DISABLED = 1;
    hearts = magic = buttons = bIcons = cIcons = aLabels = dIcons = 0;
    Interface_Draw(&play);
    assert(hearts == 0 && magic == 0 && buttons == 0);
    assert(cIcons == 1 && dIcons == 1 && aLabels == 1);
    --gSaveContext.save.saveInfo.playerData.health;
    gSaveContext.magicState = MAGIC_STATE_CONSUME_SETUP;
    Interface_Draw(&play);
    assert(hearts == 1 && magic == 1);
    ++gSaveContext.save.saveInfo.playerData.health;
    gSaveContext.magicState = MAGIC_STATE_IDLE;
    R_MINIMAP_DISABLED = 0;
    Interface_Draw(&play);
    assert(hearts == 2 && magic == 2 && buttons == 1);
    R_MINIMAP_DISABLED = 1;
    play.pauseCtx.state = PAUSE_STATE_MAIN;
    Interface_Draw(&play);
    assert(hearts == 3 && buttons == 2);
    play.pauseCtx.state = PAUSE_STATE_OFF;
    enabled = 1;
    hidden = hide == 1;
    sPictoState = PICTO_BOX_STATE_SETUP_PHOTO;
    play.actorCtx.flags |= ACTORCTX_FLAG_PICTO_BOX_ON;
    Gfx *start = gfx.overlay.p;
    int oldPhoto = photoStarts, oldClock = clocks;
    Interface_Draw(&play);
    assert(photoStarts == oldPhoto + 1 && sPictoState == PICTO_BOX_STATE_PHOTO);
    assert(clocks == oldClock + 1 && hearts == 4 && buttons == 3);
    assert(hidden ? gfx.overlay.p == start : gfx.overlay.p > start);
    play.actorCtx.flags &= ~ACTORCTX_FLAG_PICTO_BOX_ON;
    Interface_Draw(&play);
    assert(compressed == hide + 1 && sPictoState == PICTO_BOX_STATE_OFF);
    play.interfaceCtx.screenFillAlpha = 80;
    start = gfx.overlay.p;
    Interface_Draw(&play);
    assert(gfx.overlay.p > start); // World fade survives even when hidden.
    play.interfaceCtx.screenFillAlpha = 0;
  }
  assert(hooks > 0 && timers > 0 && minigames > 0);
  puts("PASS complete MM Interface_Draw: Minimal conditions, True priority, "
       "photo/clock progress, fade and restore");
}
