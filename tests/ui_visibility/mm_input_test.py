#!/usr/bin/env python3
"""Run MM's real global L dispatcher, map toggle and minimal-UI setup."""
from pathlib import Path
import os
import subprocess
import tempfile
from mm_draw_test import function

ROOT = Path(__file__).resolve().parents[2]
fixture = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef int32_t s32;
typedef int16_t s16;
typedef struct {} GraphicsContext;
typedef struct { struct { unsigned button; } press; } Input;
typedef struct GameState { GraphicsContext *gfxCtx; Input input[4]; void (*main)(struct GameState*); } GameState;
typedef struct { int mapRoomNum, minimapAlpha, minigameState; } InterfaceContext;
typedef struct { int state; } PauseContext;
typedef struct { struct { struct { float y; } pos; } world; } Actor;
typedef struct { Actor actor; } Player;
typedef struct { GameState state; InterfaceContext interfaceCtx; PauseContext pauseCtx; int sceneId; Player player; } PlayState;
static int minimal, enabled, hidden = 1, blocked, inCutscene, hookAllows = 1;
static int minimapDisabled, mapUpdates, sLastRoomNum, playerFloor, toggles, sounds, mainCalls, events;
static int R_PAUSE_BG_PRERENDER_STATE;
static struct { void *mapDataScene, *minimapCurTex, *minimapPrevTex;
    int minimapCurX,minimapCurY,minimapPrevX,minimapPrevY,curRoom,prevRoom,swapAnimTimer; } sMapDisp;
static int sSceneNumRooms=1, hudEditorActiveElement, roomDraws, mapAvailable=1;
static struct { int mapIndex; } gSaveContext;
#define HUD_EDITOR_ELEMENT_MINIMAP 1
#define HUD_EDITOR_ELEMENT_NONE 0
#define DUNGEON_COMPASS 0
#define MINIGAME_STATE_NONE 0
#define CHECK_DUNGEON_ITEM(a,b) 1
static void HudEditor_SetActiveElement(int id) { hudEditorActiveElement=id; }
static bool HudEditor_ShouldOverrideDraw(void) { return false; }
static bool HudEditor_IsActiveElementHidden(void) { return false; }
static bool MapDisp_IsLocationMinimapBlocked(void *play) { return blocked; }
static bool MapDisp_CanDisplayMinimap(void *play) { return mapAvailable; }
static void MapDisp_DrawMinimapRoom(void *play, void *tex, float x,float y,int room,float alpha) { ++roomDraws; }
static bool MapDisp_AreRoomsSameStorey(int a,int b) { return false; }
static void MapDisp_Minimap_DrawDoorActors(void *play) {}
static bool Map_CurRoomHasMapI(void *play) { return true; }
static bool Inventory_IsMapVisible(int id) { return true; }
static void MapDisp_Minimap_DrawRedCompassIcon(void *play,int x,int z,int dir) {}
static void MapDisp_Minimap_DrawActors(void *play) {}
#define PAUSE_BG_PRERENDER_PROCESS 2
#define PAUSE_STATE_OPENING_2 2
#define BTN_L 0x20
#define CHECK_BTN_ALL(value, bits) (((value) & (bits)) == (bits))
#define CONTROLLER1(state) (&(state)->input[0])
#define GET_PLAYER(play) (&(play)->player)
#define IS_PAUSED(ctx) ((ctx)->state != 0)
#define R_MINIMAP_DISABLED minimapDisabled
#define R_PLAYER_FLOOR_REVERSE_INDEX playerFloor
#define FLOOR_INDEX_MAX 8
#define SET_DUNGEON_FLOOR_VISITED(scene, floor) ((void)0)
#define VB_MINIMAP_TOGGLE 1
#define NA_SE_SY_CAMERA_ZOOM_UP 10
#define NA_SE_SY_CAMERA_ZOOM_DOWN 11
static int CVarGetInteger(const char *name, int value) {
    if (!strcmp(name,"gEnhancements.MinimalUI")) return minimal;
    if (!strcmp(name,"gEnhancements.TrueHiddenUI")) return enabled;
    return value;
}
static int UiVisibility_IsEnabled(void) { return enabled; }
static int UiVisibility_IsHidden(void) { return enabled && hidden; }
static void UiVisibility_Toggle(void) { if (enabled) { hidden ^= 1; ++toggles; } }
static int Play_InCsMode(PlayState *play) { return inCutscene; }
static int MapDisp_IsMinimapToggleBlocked(PlayState *play) { return blocked; }
static int GameInteractor_Should(int id, int value) { assert(id==VB_MINIMAP_TOGGLE); return hookAllows; }
static void Audio_PlaySfx(int id) { assert(id==10 || id==11); ++sounds; }
static void MapDisp_Update(PlayState *play) { ++mapUpdates; }
static int Map_IsInDungeonScene(PlayState *play) { return 0; }
static int Map_IsInBossScene(PlayState *play) { return 0; }
static int MapDisp_GetPlayerStorey(float y) { return -1; }
static void MapDisp_GetBossIconY(void) {}
static int MapDisp_GetBossRoomStorey(void) { return 0; }
static void GameState_SetFrameBuffer(GraphicsContext *gfx) { ++events; }
static void GameInteractor_ExecuteOnGameStateMainStart(void) { ++events; }
static void GameInteractor_ExecuteOnGameStateMainFinish(void) { ++events; }
static void GameInteractor_ExecuteOnGameStateDrawFinish(void) { ++events; }
static void GameInteractor_ExecuteOnGameStateUpdate(void) { ++events; }
static void GameState_Draw(GameState *state, GraphicsContext *gfx) { ++events; }
static void GameState_DrawEnd(GraphicsContext *gfx) { ++events; }
/* PRODUCTION */
static void playMain(GameState *state) { ++mainCalls; Map_Update((PlayState*)state); }
static void otherMain(GameState *state) { ++mainCalls; }
static void reset(PlayState *play) {
    *play = (PlayState){0};
    play->state.main = playMain;
    play->state.input[0].press.button = BTN_L;
    minimal = enabled = blocked = inCutscene = minimapDisabled = sounds = toggles = events = mainCalls = 0;
    hidden = hookAllows = 1;
}
int main(void) {
    PlayState play;
    reset(&play);
    GameState_Update(&play.state);
    assert(minimapDisabled == 1 && sounds == 1 && mainCalls == 1 && events == 7);
    reset(&play); blocked = 1;
    GameState_Update(&play.state);
    assert(minimapDisabled == 0 && sounds == 0); // Native behavior without options.
    reset(&play); minimal = blocked = 1;
    GameState_Update(&play.state);
    assert(minimapDisabled == 1 && sounds == 1); // Minimal also works without a map.
    GameState_Update(&play.state);
    assert(minimapDisabled == 0 && sounds == 2);
    reset(&play); enabled = minimal = blocked = 1;
    GameState_Update(&play.state);
    assert(toggles == 1 && !UiVisibility_IsHidden() && minimapDisabled == 0 && sounds == 0);
    GameState_Update(&play.state);
    assert(toggles == 2 && UiVisibility_IsHidden() && minimapDisabled == 0);
    // Global L recovery remains available in pauses/dialogue/file select. Map
    // input must not also flip, including when normal minimap input is blocked.
    play.pauseCtx.state = 6; inCutscene = 1;
    GameState_Update(&play.state);
    assert(toggles == 3 && !UiVisibility_IsHidden() && minimapDisabled == 0);
    play.state.main = otherMain;
    GameState_Update(&play.state);
    assert(toggles == 4 && UiVisibility_IsHidden());
    play.state.input[0].press.button = 0;
    GameState_Update(&play.state);
    assert(toggles == 4);
    enabled = 0;
    assert(!UiVisibility_IsHidden());
    reset(&play); minimal = 1; inCutscene = 1;
    Map_Update(&play); assert(minimapDisabled == 0);
    inCutscene = 0; play.pauseCtx.state = 6;
    Map_Update(&play); assert(minimapDisabled == 0);
    play.pauseCtx.state = 0; hookAllows = 0;
    Map_Update(&play); assert(minimapDisabled == 0);
    reset(&play); minimal = 1; Regs_InitData(&play); assert(minimapDisabled == 1);
    minimal = 0; Regs_InitData(&play); assert(minimapDisabled == 0);
    // True Hidden's revealed state overrides Minimal without rewriting its
    // preference, while all native map-availability restrictions still apply.
    enabled=1; minimapDisabled=1; roomDraws=0;
    sMapDisp.mapDataScene=&play; play.interfaceCtx.minimapAlpha=255;
    MapDisp_DrawMinimap(&play,0,0,0);
    assert(roomDraws==1 && minimapDisabled==1 && hudEditorActiveElement==0);
    enabled=0; MapDisp_DrawMinimap(&play,0,0,0); assert(roomDraws==1);
    enabled=1; blocked=1; MapDisp_DrawMinimap(&play,0,0,0); assert(roomDraws==1);
    blocked=0; play.pauseCtx.state=6; MapDisp_DrawMinimap(&play,0,0,0); assert(roomDraws==1);
    play.pauseCtx.state=0; play.interfaceCtx.minimapAlpha=0;
    MapDisp_DrawMinimap(&play,0,0,0); assert(roomDraws==1);
    play.interfaceCtx.minimapAlpha=255; sMapDisp.mapDataScene=NULL;
    MapDisp_DrawMinimap(&play,0,0,0); assert(roomDraws==1);
    puts("PASS MM UI L: native, no-map Minimal, True Hidden priority/global recovery, gates and setup");
}
'''
production = function((ROOT/'mm/src/code/z_map_exp.c').read_text(), 'Map_Update')
production += function((ROOT/'mm/src/code/z_construct.c').read_text(), 'Regs_InitData')
production += function((ROOT/'mm/src/code/game.c').read_text(), 'GameState_Update')
production += function((ROOT/'mm/src/code/z_map_disp.c').read_text(), 'MapDisp_DrawMinimap')
with tempfile.TemporaryDirectory(prefix='mm-ui-input-') as directory:
    path = Path(directory)/'test.c'
    path.write_text(fixture.replace('/* PRODUCTION */', production))
    binary = Path(directory)/'test'
    subprocess.run(['cc', '-std=gnu11', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    '-no-pie', str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
