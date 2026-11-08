#include "BlackMarketFairy.h"
#include "global.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "overlays/actors/ovl_En_Mag/z_en_mag.h"
#include "overlays/actors/ovl_End_Title/z_end_title.h"
#include "overlays/gamestates/ovl_file_choose/file_choose.h"
#include "overlays/ovl_End_Title/ovl_End_Title.h"
#include "soh/Enhancements/cosmetics/cosmeticsTypes.h"
#include "soh/OTRGlobals.h"
#include "tests/test_require.h"
#include "textures/nintendo_rogo_static/nintendo_rogo_static.h"
#include "textures/title_static/title_static.h"
#include "ui_visibility.h"
#include <string.h>
#ifdef COMBO_BUILD
int gComboGameMode;
#endif
typedef struct {
  Color_RGBA8 inner;
  Color_RGBA8 outer;
} NaviColor;
static NaviColor sNaviColorList[ACTORCAT_MAX];
static bool hidden;
int UiVisibility_IsHidden(void) { return hidden; }
SaveContext gSaveContext;
uintptr_t gSegments[16];
static GameInfo gameInfo;
GameInfo *gGameInfo = &gameInfo;
s32 gScreenWidth = SCREEN_WIDTH, gScreenHeight = SCREEN_HEIGHT;
static Gfx work[512], opa[512], xlu[512], overlay[512];
static Mtx matrix;
static unsigned fileUpdates, fileDraws, pulses, warnings, innerDraws;
static unsigned selectMenuDraws, selectLoadingDraws;
static s16 sWindowContentColors[2][3];
static s16 sScreenFillAlpha = 100;
static Gfx sScreenFillSetupDL[] = {gsSPEndDisplayList()};
static void fileUpdate(GameState *state) {
  ++fileUpdates;
  ++((FileChooseContext *)state)->windowAlpha;
}
static void fileDraw(GameState *state) {
  ++fileDraws;
  ++((FileChooseContext *)state)->highlightColor[3];
  state->gfxCtx->work.p++;
  state->gfxCtx->polyOpa.p++;
  state->gfxCtx->polyXlu.p++;
  state->gfxCtx->overlay.p++;
}
static void (*gFileSelectUpdateFuncs[])(GameState *) = {fileUpdate, fileUpdate,
                                                        fileUpdate};
static void (*gFileSelectDrawFuncs[])(GameState *) = {fileDraw, fileDraw,
                                                      fileDraw};
int32_t CVarGetInteger(const char *key, int32_t fallback) { return fallback; }
Color_RGB8 CVarGetColor24(const char *key, Color_RGB8 fallback) {
  return fallback;
}
void GameInteractor_ExecuteOnFileChooseMain(void *state) {}
void FileChoose_PulsateCursor(GameState *state) { ++pulses; }
void FileChoose_DrawRandoSaveVersionWarning(GameState *state) {
  ++warnings;
  state->gfxCtx->overlay.p++;
}
void FrameInterpolation_StartRecord(void) {}
void FrameInterpolation_StopRecord(void) {}
void FrameInterpolation_RecordOpenChild(const void *source, int line) {}
void FrameInterpolation_RecordCloseChild(void) {}
/* PRODUCTION_GRAPH_MARKERS */
void gSPSegment(void *command, int segment, uintptr_t target) {
  __gSPSegment((Gfx *)command, segment, target);
}
void gSPDisplayList(Gfx *command, Gfx *list) {
  __gSPDisplayList(command, list);
}
Gfx *Graph_GfxPlusOne(Gfx *command) { return command + 1; }
Gfx *Graph_BranchDlist(Gfx *branch, Gfx *target) {
  gSPBranchList(branch, target);
  return branch;
}
void Gfx_SetupFrame(GraphicsContext *gfx, u8 r, u8 g, u8 b) { gfx->work.p++; }
void View_SetViewport(View *view, Viewport *viewport) {}
void func_800AAA50(View *view, s32 arg) {}
void Select_DrawMenu(SelectContext *select) {
  ++selectMenuDraws;
  ++select->state.gfxCtx->overlay.p;
}
void Select_DrawLoadingScreen(SelectContext *select) {
  ++selectLoadingDraws;
  ++select->state.gfxCtx->overlay.p;
}
void Gfx_SetupDL_39Opa(GraphicsContext *gfx) {}
void Gfx_SetupDL_25Xlu(GraphicsContext *gfx) {}
Gfx *Gfx_SetupDL_64(Gfx *pointer) { return pointer; }
void Matrix_Mult(MtxF *matrix, u8 mode) {}
void Matrix_Translate(float x, float y, float z, u8 mode) {}
void Matrix_Scale(float x, float y, float z, u8 mode) {}
void Matrix_RotateX(float value, u8 mode) {}
void Matrix_RotateY(float value, u8 mode) {}
void Matrix_RotateZ(float value, u8 mode) {}
Mtx *Matrix_NewMtx(GraphicsContext *gfx, char *file, s32 line) {
  return &matrix;
}
void EnMag_DrawInner(Actor *actor, PlayState *play, Gfx **pointer) {
  ++innerDraws;
  ++((EnMag *)actor)->effectScroll;
  ++*pointer;
}
float OTRGetDimensionFromLeftEdge(float v) { return v; }
float OTRGetDimensionFromRightEdge(float v) { return v; }
Gfx *Gfx_SetupDL(Gfx *pointer, u32 index) { return pointer; }
Gfx *Gfx_SetupDL_52NoCD(Gfx *pointer) { return pointer; }
void Matrix_Push(void) {}
void Matrix_Pop(void) {}
void Math_Vec3f_Copy(Vec3f *destination, Vec3f *source) {
  *destination = *source;
}
void Actor_ProjectPos(PlayState *play, Vec3f *source, Vec3f *projected,
                      float *inverseW) {
  *projected = (Vec3f){0.1f, 0.2f, 0.3f};
  *inverseW = 1.0f;
}
static int titleYaw;
Hilite *func_8002EABC(Vec3f *object, Vec3f *eye, Vec3f *lightDir,
                      GraphicsContext *gfx) {
  return NULL;
}
void Title_SetupView(TitleContext *title, float x, float y, float z) {}
void Gfx_SetupDL_25Opa(GraphicsContext *gfx) {}
void Matrix_RotateZYX(s16 x, s16 y, s16 z, u8 mode) { titleYaw = y; }
void Environment_FillScreen(GraphicsContext *gfx, u8 r, u8 g, u8 b, u8 alpha,
                            u8 flags) {
  REQUIRE(alpha == 128);
  gfx->polyXlu.p++;
}
/* PRODUCTION_NATIVE_UI */
static void balanced(Gfx *start, Gfx *end) {
  int depth = 0;
  for (Gfx *command = start; command < end; ++command) {
    if ((command->words.w0 >> 24) != G_NOOP)
      continue;
    unsigned kind = (command->words.w0 >> 16) & 0xff;
    if (kind == 7)
      ++depth;
    if (kind == 8)
      --depth;
    REQUIRE(depth >= 0);
  }
  REQUIRE(depth == 0);
}
static void reset(GraphicsContext *gfx) {
  if (gfx->polyOpa.p) {
    balanced(opa, gfx->polyOpa.p);
    balanced(xlu, gfx->polyXlu.p);
    balanced(overlay, gfx->overlay.p);
  }
  memset(opa, 0, sizeof(opa));
  memset(xlu, 0, sizeof(xlu));
  memset(overlay, 0, sizeof(overlay));
  gfx->work.p = work;
  gfx->polyOpa.p = opa;
  gfx->polyXlu.p = xlu;
  gfx->overlay.p = overlay;
}
int main(void) {
  static PlayState play;
  static Player player;
  GraphicsContext gfx = {0};
  play.state.gfxCtx = &gfx;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
  play.csCtx.frames = 1200;
  for (unsigned mode = 0; mode < 2; ++mode) {
    hidden = mode;
    TitleContext boot = {0};
    boot.state.gfxCtx = &gfx;
    boot.coverAlpha = 128;
    reset(&gfx);
    Title_Draw(&boot);
    REQUIRE(titleYaw == 300 * mode);
    REQUIRE(gfx.polyXlu.p > xlu); /* Boot transition fade remains. */
    REQUIRE(gfx.polyOpa.p > opa); /* Startup navigation remains visible. */
    EndTitle title = {0};
    reset(&gfx);
    EndTitle_DrawFull(&title.actor, &play);
    REQUIRE(title.endAlpha == 7 && title.tlozAlpha == 15 &&
            title.ootAlpha == 15);
    REQUIRE(gfx.polyXlu.p > xlu); /* Physical hand Triforce stays submitted. */
    REQUIRE(hidden ? gfx.overlay.p == overlay : gfx.overlay.p > overlay);
    reset(&gfx);
    EndTitle_DrawNintendoLogo(&title.actor, &play);
    REQUIRE(title.endAlpha == 10);
    REQUIRE(hidden ? gfx.overlay.p == overlay : gfx.overlay.p > overlay);
    TargetContext target = {0};
    Actor hovered = {0};
    hovered.category = ACTORCAT_NPC;
    hovered.scale.y = 1.0f;
    target.unk_48 = 255;
    target.unk_44 = 100.0f;
    target.unk_94 = &hovered;
    reset(&gfx);
    Attention_Draw(&target, &play);
    REQUIRE(target.unk_48 == 135 && target.unk_4C == 2);
    REQUIRE(target.arr_50[2].unk_0C == 100.0f && target.arr_50[2].pos.x != 0);
    REQUIRE(hidden ? gfx.overlay.p == overlay : gfx.overlay.p > overlay);
    REQUIRE(hidden ? gfx.polyXlu.p == xlu : gfx.polyXlu.p > xlu);
    TitleCardContext card = {0};
    card.alpha = 255;
    card.width = 16;
    card.height = 16;
    reset(&gfx);
    TitleCard_Draw(&play, &card);
    REQUIRE(hidden ? gfx.overlay.p == overlay : gfx.overlay.p > overlay);
    REQUIRE(card.alpha == 255);
    EnMag mag = {0};
    reset(&gfx);
    EnMag_Draw(&mag.actor, &play);
    REQUIRE(mag.effectScroll == 1);
    REQUIRE(gfx.overlay.p > overlay && gfx.polyOpa.p > opa);
    REQUIRE(hidden == mode); /* Navigation must not change the chosen UI state. */
    for (unsigned running = 0; running < 2; ++running) {
      SelectContext select = {0};
      select.state.gfxCtx = &gfx;
      select.state.running = running;
      reset(&gfx);
      Select_Draw(&select);
      REQUIRE(gfx.work.p == work + 1 && gfx.overlay.p > overlay);
      REQUIRE(selectMenuDraws == mode + running);
      REQUIRE(selectLoadingDraws == mode + 1);
      REQUIRE(hidden == mode);
    }
    for (unsigned menu = 0; menu < 3; ++menu) {
      static FileChooseContext file;
      memset(&file, 0, sizeof(file));
      file.state.gfxCtx = &gfx;
      file.menuMode = menu;
      file.configMode = CM_MAIN_MENU;
      file.controlsAlpha = 255;
      file.state.input[0].rel.stick_x = 50;
      reset(&gfx);
      FileChoose_Main(&file.state);
      REQUIRE(file.windowAlpha == 1 && file.highlightColor[3] == 1 &&
              file.stickXDir == 1);
      REQUIRE(gfx.work.p == work + 2); /* Frame setup and menu both remain. */
      REQUIRE(gfx.overlay.p > overlay && gfx.polyXlu.p > xlu);
      REQUIRE(gfx.polyOpa.p > opa); /* Menu and scene fade both remain. */
      REQUIRE(hidden == mode);
      bool fadeFound = false;
      for (Gfx *command = opa; command < gfx.polyOpa.p; ++command)
        fadeFound |= (command->words.w0 >> 24) == G_FILLRECT;
      REQUIRE(fadeFound);
    }
  }
  reset(&gfx); /* Assert the final native marker pairs too. */
  REQUIRE(innerDraws == 2 && fileUpdates == 6 && fileDraws == 6 &&
          pulses == 6 && warnings == 6 && selectMenuDraws == 2 &&
          selectLoadingDraws == 2);
  puts("PASS OoT startup/title/file-select/debug-select stay visible while hidden; "
       "gameplay suppression, draw state, markers, physical Triforce and fades preserved");
}
