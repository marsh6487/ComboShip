#!/usr/bin/env python3
"""Execute native MM draw entrypoints: hidden commands, live draw-side state."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r'^[^\n;{}]*\b' + re.escape(name) + r'\([^;{}]*\)\s*\{', source, re.M)
    if match is None:
        raise RuntimeError(name)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


prelude = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
typedef int32_t s32;
typedef struct { uintptr_t command; } Gfx;
typedef struct { Gfx *p, *d; } Arena;
typedef struct { Arena work, polyOpa, polyXlu, overlay, debug; } GraphicsContext;
typedef struct { GraphicsContext *gfxCtx; } GameState;
typedef struct { int currentTextId; } MessageContext;
typedef struct { GameState state; MessageContext msgCtx; } PlayState;
typedef struct { int unused; } Actor;
static int hidden, stateAdvances, inCutscene;
static inline int UiVisibility_IsHidden(void) { return hidden; }
#define COMBO_GUI_UI_VISIBILITY_H
#if __has_include("ui_visibility.h")
#include "ui_visibility.h"
#endif
#define OPEN_DISPS(gfx) { GraphicsContext *__gfxCtx = (gfx)
#define CLOSE_DISPS(gfx) }
#define POLY_OPA_DISP __gfxCtx->polyOpa.p
#define OVERLAY_DISP __gfxCtx->overlay.p
#define gSPDisplayList(pkt, list) ((pkt)->command = (uintptr_t)(list))
#define gSPEndDisplayList(pkt) ((pkt)->command = 0xDF)
static Gfx *Graph_GfxPlusOne(Gfx *p) { return p + 1; }
static void Graph_BranchDlist(Gfx *p, Gfx *next) { p->command = (uintptr_t)next; }
static bool Play_InCsMode(PlayState *play) { return inCutscene; }
// The renderer/text interpreter is the boundary: native wrappers must still call
// it, even when their resulting command streams will not be submitted.
static void Message_DrawMain(PlayState *play, Gfx **p) {
    ++stateAdvances;
    (*p)++;
    --play->state.gfxCtx->polyOpa.d;
    ++play->state.gfxCtx->polyXlu.p;
    ++play->state.gfxCtx->work.p;
    ++play->state.gfxCtx->debug.p;
}
static void EnMag_DrawInner(Actor *actor, PlayState *play, Gfx **p) {
    Message_DrawMain(play, p);
}
'''

test = r'''
static void check(void (*draw)(PlayState *), bool hide, bool navigation) {
    Gfx work[64], opa[64], xlu[64], overlay[64], debug[64];
    GraphicsContext gfx = { { work, work+64 }, { opa, opa+64 },
                            { xlu, xlu+64 }, { overlay, overlay+64 }, { debug, debug+64 } };
    PlayState play = { .state = { &gfx } };
    hidden = hide;
    stateAdvances = 0;
    draw(&play);
    assert(stateAdvances == 1);
    assert(gfx.polyOpa.d == opa+63); // Allocation lifetime must survive.
    assert(hidden == hide); // Drawing navigation must preserve the chosen state.
    if (hide && !navigation) {
        assert(gfx.work.p == work && gfx.polyOpa.p == opa &&
               gfx.polyXlu.p == xlu && gfx.overlay.p == overlay && gfx.debug.p == debug);
    } else {
        assert(gfx.work.p == work+1 && gfx.polyOpa.p > opa &&
               gfx.polyXlu.p == xlu+1 && gfx.overlay.p == overlay+1);
    }
}
static void drawTitle(PlayState *play) { Actor actor = {0}; EnMag_Draw(&actor, play); }
int main(void) {
    check(Message_Draw, false, false);
    check(Message_Draw, true, false);
    check(Message_Draw, false, false); // No stale hidden state after toggling back.
    check(drawTitle, false, true);
    check(drawTitle, true, true);
    check(drawTitle, false, true);
    puts("PASS MM title stays visible while hidden; messages remain suppressed, draw state and allocations live");
}
'''


# Exercise the real navigation bodies with native MM structs and display-list
# macros. Rendering/scene helpers remain fixture boundaries.
navigation_prelude = r'''
#include "global.h"
#include "overlays/gamestates/ovl_title/z_title.h"
#include "overlays/gamestates/ovl_file_choose/z_file_select.h"
#include "overlays/gamestates/ovl_select/z_select.h"
#include "misc/nintendo_rogo_static/nintendo_rogo_static.h"
#include "ui_visibility.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int hidden, rotation, fileUpdates, fileDraws, pulses, skyboxes, fills;
static int menuDraws, loadingDraws, frameStarts, frameStops;
static Mtx matrix;
static Gfx work[512], opa[2048], xlu[512], overlay[512], debug[512];
static Gfx sScreenFillSetupDL[] = { gsSPEndDisplayList() };
static TexturePtr D_808147B4[3];
static s16 D_808147C0[] = {144, 144, 152}, D_808147C8[] = {90, 90, 86};
static char gShipLogoDL[] = "ship-logo", gLUSLogoTextTex[] = "lus-logo";
int UiVisibility_IsHidden(void) { return hidden; }
s32 CVarGetInteger(const char *key, s32 fallback) { return fallback; }
void Graph_OpenDisps(Gfx **refs, Gfx *values, GraphicsContext *gfx, const char *file, s32 line) {}
void Graph_CloseDisps(Gfx **refs, Gfx *values, GraphicsContext *gfx, const char *file, s32 line) {}
void gSPSegment(void *p, int segment, uintptr_t value) { __gSPSegment(p, segment, value); }
void gSPDisplayList(Gfx *p, Gfx *list) {
  if ((uintptr_t)list == D_0E000000_TO_SEGMENTED(fillRect)) ++fills;
  __gSPDisplayList(p, list);
}
void FrameInterpolation_RecordOpenChild(const void *a, int b) {}
void FrameInterpolation_RecordCloseChild(void) {}
void FrameInterpolation_StartRecord(void) { ++frameStarts; }
void FrameInterpolation_StopRecord(void) { ++frameStops; }
void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode mode) {}
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode mode) {}
void Matrix_RotateZYX(s16 x, s16 y, s16 z, MatrixMode mode) { rotation = y; }
Mtx *Matrix_Finalize(GraphicsContext *gfx) { return &matrix; }
Hilite *Hilite_DrawOpa(Vec3f *object, Vec3f *eye, Vec3f *lightDir, GraphicsContext *gfx) { return NULL; }
void ConsoleLogo_RenderView(ConsoleLogoState *logo, f32 x, f32 y, f32 z) {}
void ConsoleLogo_PrintBuildInfo(ConsoleLogoState *logo) { ++logo->state.gfxCtx->overlay.p; }
void Gfx_SetupDL25_Opa(GraphicsContext *gfx) {}
void Gfx_SetupDL39_Opa(GraphicsContext *gfx) {}
void Environment_FillScreen(GraphicsContext *gfx, u8 r, u8 g, u8 b, u8 alpha, u8 flags) {
  assert(alpha == 128); ++gfx->polyXlu.p;
}
void func_8012CF0C(GraphicsContext *gfx, s32 clearFb, s32 clearZb, u8 r, u8 g, u8 b) { ++gfx->work.p; }
void View_SetViewport(View *view, Viewport *viewport) {}
s32 View_Apply(View *view, s32 mask) { return 0; }
static void drawNavigation(GameState *state) {
  ++state->gfxCtx->work.p; ++state->gfxCtx->polyOpa.p;
  ++state->gfxCtx->polyXlu.p; ++state->gfxCtx->overlay.p;
}
void MapSelect_DrawMenu(MapSelectState *select) { ++menuDraws; drawNavigation(&select->state); }
void MapSelect_DrawLoadingScreen(MapSelectState *select) { ++loadingDraws; drawNavigation(&select->state); }
void FileSelect_PulsateCursor(GameState *state) { ++pulses; }
void FileSelect_UpdateAndDrawSkybox(FileSelectState *file) { ++skyboxes; ++file->state.gfxCtx->work.p; }
static void updateFile(GameState *state) { ++fileUpdates; ++((FileSelectState *)state)->windowAlpha; }
static void drawFile(GameState *state) {
  ++fileDraws; ++((FileSelectState *)state)->highlightColor[3]; drawNavigation(state);
}
static void (*gFileSelectUpdateFuncs[])(GameState *) = {updateFile, updateFile, updateFile};
static void (*gFileSelectDrawFuncs[])(GameState *) = {drawFile, drawFile, drawFile};
static void reset(GraphicsContext *gfx) {
  gfx->work.p=work; gfx->polyOpa.p=opa; gfx->polyXlu.p=xlu;
  gfx->overlay.p=overlay; gfx->debug.p=debug;
}
'''

navigation_test = r'''
int main(void) {
  GraphicsContext gfx={0};
  for (int step=0; step<3; ++step) {
    hidden=step==1;
    ConsoleLogoState logo={0}; logo.state.gfxCtx=&gfx; logo.coverAlpha=128;
    reset(&gfx); ConsoleLogo_Draw(&logo.state);
    assert(rotation==300*step && gfx.polyOpa.p>opa && gfx.overlay.p>overlay);
    assert(gfx.polyXlu.p==xlu+1); // The original boot fade is still submitted.
    for (int menu=0; menu<3; ++menu) {
      FileSelectState file={0}; file.state.gfxCtx=&gfx; file.menuMode=menu;
      file.state.input[0].rel.stick_x=50; file.screenFillAlpha=128;
      reset(&gfx); FileSelect_Main(&file.state);
      assert(file.windowAlpha==1 && file.highlightColor[3]==1 && file.stickXDir==1);
      assert(gfx.work.p==work+3 && gfx.polyOpa.p>opa && gfx.polyXlu.p>xlu && gfx.overlay.p>overlay);
    }
    for (int running=0; running<2; ++running) {
      MapSelectState select={0}; select.state.gfxCtx=&gfx; select.state.running=running;
      reset(&gfx); MapSelect_Draw(&select);
      assert(gfx.work.p==work+2 && gfx.polyOpa.p>opa && gfx.overlay.p>overlay);
    }
    assert(hidden==(step==1)); // Startup rendering never changes the selected mode.
  }
  assert(fileUpdates==9 && fileDraws==9 && pulses==9 && skyboxes==9 && fills==9);
  assert(frameStarts==9 && frameStops==9 && menuDraws==3 && loadingDraws==3);
  puts("PASS native MM boot/file-select/debug navigation: visible while hidden, updates and fades preserved");
}
'''


def navigation():
    source = navigation_prelude
    for path, name in [
        ('mm/src/overlays/gamestates/ovl_title/z_title.c', 'ConsoleLogo_Draw'),
        ('mm/src/overlays/gamestates/ovl_file_choose/z_file_choose_NES.c', 'FileSelect_Main'),
        ('mm/src/overlays/gamestates/ovl_select/z_select.c', 'MapSelect_Draw'),
    ]:
        source += function((ROOT/path).read_text(), name)
    source += navigation_test
    with tempfile.TemporaryDirectory(prefix='mm-ui-navigation-') as directory:
        path=Path(directory)/'test.c'; path.write_text(source); binary=Path(directory)/'test'
        for combo in (False, True):
            command=['cc','-std=gnu11','-g','-w','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t',
                     '-DLOG_LEVEL_GAME_PRINTS=0','-Wno-int-conversion','-Wno-incompatible-pointer-types',
                     '-fsanitize=address,undefined',
                     '-fno-sanitize-recover=all','-no-pie', *(['-DCOMBO_BUILD'] if combo else []),
                     *['-I'+str(ROOT/p) for p in ['mm/include','mm/include/PR','mm/src','mm/assets','mm',
                                                'mm/2s2h','libultraship/include','combo']],
                     str(path),'-lm','-o',str(binary)]
            result=subprocess.run(command,capture_output=True,text=True)
            if result.returncode:
                print(result.stderr); result.check_returncode()
            subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})


def main():
    navigation()
    source = prelude + function((ROOT/'mm/src/code/z_message.c').read_text(), 'Message_Draw')
    source += function((ROOT/'mm/src/overlays/actors/ovl_En_Mag/z_en_mag.c').read_text(), 'EnMag_Draw') + test
    with tempfile.TemporaryDirectory(prefix='mm-ui-draw-') as directory:
        path = Path(directory)/'test.c'
        path.write_text(source)
        binary = Path(directory)/'test'
        subprocess.run(['cc', '-std=gnu11', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-no-pie', '-I'+str(ROOT/'mm/include'), '-I'+str(ROOT/'combo'),
                        str(path), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})


if __name__ == "__main__":
    main()
