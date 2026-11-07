#!/usr/bin/env python3
"""Run collision/waterbox and debug-object UI entrypoints without altering configuration/data."""
from pathlib import Path
import subprocess
import tempfile
from mm_draw_test import function

ROOT=Path(__file__).resolve().parents[2]
fixture=r'''
#include <cassert>
#include <cstdio>
#include <vector>
#include <cstring>
#include <cstdint>
struct Gfx { int command; };
struct Arena { Gfx* p; };
struct GraphicsContext { Arena polyOpa,polyXlu,overlay; };
struct PlayState { struct { GraphicsContext* gfxCtx; } state; };
static PlayState* gPlayState;
static bool hidden;
static int enabled=1, sceneDraws, waterDraws, debugDraws;
static int UiVisibility_IsHidden() { return hidden; }
static int CVarGetInteger(const char* name,int value) { return !strcmp(name,"gCollisionViewer.Enabled") ? enabled : value; }
static std::vector<Gfx> opaDl,xluDl;
static std::vector<int> vtxDl,mtxDl;
static void DrawSceneCollision() { ++sceneDraws; opaDl.push_back({1}); vtxDl.push_back(1); }
static void DrawBgActorCollision() {}
static void DrawColCheckCollision() {}
static void DrawWaterboxList() { ++waterDraws; xluDl.push_back({2}); }
#define OPEN_DISPS(gfx) { GraphicsContext* __gfxCtx=(gfx)
#define CLOSE_DISPS(gfx) }
#define POLY_OPA_DISP __gfxCtx->polyOpa.p
#define POLY_XLU_DISP __gfxCtx->polyXlu.p
#define G_EX_INVERT_CULLING 1
static void gSPSetExtraGeometryMode(Gfx* p,int mode) {}
static void gSPClearExtraGeometryMode(Gfx* p,int mode) {}
static void gSPDisplayList(Gfx* p,Gfx* list) { p->command=1; }
static Gfx gsSPEndDisplayList() { return {0xDF}; }
struct DebugDispObject { int type; DebugDispObject* next; };
struct DebugDispObjectInfo { int drawType; void* drawArg; };
static DebugDispObject* sDebugObjectListHead;
static void drawDebug(DebugDispObject* object,void* arg,PlayState* play) {
 ++debugDraws; ++play->state.gfxCtx->polyXlu.p;
}
static DebugDispObjectInfo sDebugObjectInfoTable[1]={{0,nullptr}};
static void (*sDebugObjectDrawFuncTable[1])(DebugDispObject*,void*,PlayState*)={drawDebug};
'''
collision=(ROOT/'mm/2s2h/DeveloperTools/CollisionViewer.cpp').read_text()
fixture+=function(collision,'ResetVector')+function(collision,'DrawCollisionViewer')
debug=(ROOT/'mm/src/code/z_debug_display.c').read_text()
fixture+=function(debug,'DebugDisplay_Init')+function(debug,'DebugDisplay_DrawObjects')
fixture+=r'''
int main() {
 Gfx opa[64],xlu[64],overlay[64]; GraphicsContext gfx={{opa},{xlu},{overlay}};
 PlayState play={{&gfx}}; gPlayState=&play;
 DebugDispObject second={0,nullptr},first={0,&second}; sDebugObjectListHead=&first;
 for (int step=0;step<3;++step) {
  hidden=step==1; gfx.polyOpa.p=opa; gfx.polyXlu.p=xlu;
  int scenes=sceneDraws,water=waterDraws,debugBefore=debugDraws;
  DrawCollisionViewer(); DebugDisplay_DrawObjects(&play);
  assert(enabled==1 && sDebugObjectListHead==&first && first.next==&second);
  if (hidden) {
   assert(gfx.polyOpa.p==opa && gfx.polyXlu.p==xlu);
   assert(sceneDraws==scenes && waterDraws==water && debugDraws==debugBefore);
  } else {
   assert(gfx.polyOpa.p>opa && gfx.polyXlu.p>xlu);
   assert(sceneDraws>scenes && waterDraws>water && debugDraws==debugBefore+2);
  }
 }
 hidden=true; assert(DebugDisplay_Init()==nullptr && sDebugObjectListHead==nullptr);
 puts("PASS MM collision/waterbox/debug overlays: hidden, restored; configuration and list lifecycle preserved");
}
'''
with tempfile.TemporaryDirectory(prefix='mm-ui-diagnostic-') as directory:
    path=Path(directory)/'test.cpp';path.write_text(fixture);binary=Path(directory)/'test'
    subprocess.run(['c++','-std=c++20',str(path),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
