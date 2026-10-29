#!/usr/bin/env python3
"""Execute the production 3D-background draw hook and following projection call."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(os.environ.get('COMBO_TEST_ROOT',Path(__file__).resolve().parents[2]))


def draw_hook(source):
    start = source.index('COND_HOOK(OnPlayDrawBegin,')
    opening = source.index('{',start)
    depth = 1
    end = opening+1
    while depth:
        depth += (source[end]=='{')-(source[end]=='}')
        end += 1
    return source[start:source.index(';',end)+1]


class BackgroundDrawDistance(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source=(ROOT/'soh/soh/Enhancements/Graphics/Disable2DBackgrounds.cpp').read_text()
        play=(ROOT/'soh/src/code/z_play.c').read_text()
        room=(ROOT/'soh/src/code/z_room.c').read_text()
        declaration=re.search(r'std::set<SceneID> fogControlList\s*=\s*\{.*?\};',source,re.S).group()
        scene_names=re.findall(r'\bSCENE_[A-Z0-9_]+\b',declaration)
        projection=re.search(r'func_800AA460\(&play->view, play->view.fovy,.*?\);',play).group()
        room_cutoff=re.search(r'temp_f2 < play->lightCtx\.\w+',room).group()
        fixture=r'''
#include <algorithm>
#include <cassert>
#include <functional>
#include <set>
#include <string>
#include <cstdint>
enum SceneID { __SCENE_NAMES__, SCENE_HYRULE_FIELD=999 };
struct View { float fovy=60,zNear=10,zFar=2000; };
struct LightContext { int16_t fogNear=992,fogFar=2000; float zNear=10,zFar=2000; bool worldFog=false; };
struct PlayState { int sceneNum=0; View view; LightContext lightCtx; };
PlayState state;
PlayState* gPlayState=&state;
bool enabled=true;
int h80=0,h82=0;
std::function<void()> callback;
#define HREG(index) ((index)==80?h80:h82)
#define CVAR_VALUE enabled
#define COND_HOOK(event,condition,...) callback=[](){ if (condition) { (__VA_ARGS__)(); } }
__SCENE_LIST__
void RegisterActualHook() { __HOOK__ }
void func_800AA460(View* view,float fovy,float near,float far) {
    view->fovy=fovy;view->zNear=near;view->zFar=far;
}
void DrawProjection() { PlayState* play=&state; __PROJECTION__ }
bool RoomEntryVisible(float temp_f2) { PlayState* play=&state; return __ROOM_CUTOFF__; }
void Reset(SceneID scene,float far) {
    state=PlayState{};state.sceneNum=scene;state.lightCtx.zFar=far;
    state.lightCtx.fogFar=far>32767?32767:static_cast<int16_t>(far);
    enabled=true;h80=0;h82=0;RegisterActualHook();
}
int main(int argc,char** argv) {
    assert(argc==2);std::string test=argv[1];
    if(test=="projection") {
        for(auto scene:fogControlList) for(float far:{2000.0f,3000.0f}) {
            Reset(scene,far);callback();DrawProjection();
            assert(state.view.zFar==12800.0f);
            assert(state.lightCtx.zFar==12800.0f);
            assert(state.lightCtx.fogNear==1000 && state.lightCtx.fogFar==12800);
        }
    } else if(test=="room") {
        Reset(SCENE_MARKET_NIGHT,2000);callback();DrawProjection();
        assert(RoomEntryVisible(6000));assert(!RoomEntryVisible(13000));
    } else if(test=="long-distance") {
        Reset(SCENE_MARKET_DAY,250000);state.lightCtx.worldFog=true;state.lightCtx.zNear=25;
        callback();DrawProjection();assert(state.view.zFar==250000);
        assert(state.lightCtx.zFar==250000 && state.view.zNear==25);
    } else if(test=="disabled") {
        Reset(SCENE_MARKET_NIGHT,2000);enabled=false;callback();DrawProjection();
        assert(state.view.zFar==2000 && state.lightCtx.fogNear==992);
    } else if(test=="unrelated") {
        Reset(SCENE_HYRULE_FIELD,2000);callback();DrawProjection();
        assert(state.view.zFar==2000 && state.lightCtx.fogNear==992);
    } else if(test=="debug-bypass") {
        Reset(SCENE_MARKET_NIGHT,2000);h80=10;h82=0;callback();
        assert(state.lightCtx.zFar==2000 && state.lightCtx.fogNear==992);
    } else { return 2; }
}
'''
        fixture=fixture.replace('__SCENE_NAMES__',','.join(scene_names)).replace('__SCENE_LIST__',declaration)
        fixture=fixture.replace('__HOOK__',draw_hook(source)).replace('__PROJECTION__',projection)
        fixture=fixture.replace('__ROOM_CUTOFF__',room_cutoff)
        cls.temporary=tempfile.TemporaryDirectory()
        path=Path(cls.temporary.name)
        (path/'control.cpp').write_text(fixture)
        cls.executable=path/'control'
        subprocess.run(['g++','-std=c++20','-Wall','-Wextra','-Werror','-fsanitize=undefined',
                        '-fno-sanitize-recover=all',str(path/'control.cpp'),'-o',str(cls.executable)],check=True)

    @classmethod
    def tearDownClass(cls):
        cls.temporary.cleanup()

    def run_case(self,name):
        result=subprocess.run([str(self.executable),name],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)

    def test_market_projection_keeps_3d_distance_after_unbound_rebuild(self):
        self.run_case('projection')

    def test_room_visibility_uses_the_same_extended_distance(self):
        self.run_case('room')

    def test_longer_authored_unbound_distance_and_near_plane_survive(self):
        self.run_case('long-distance')

    def test_disabled_feature_preserves_scene_distance(self):
        self.run_case('disabled')

    def test_unrelated_scene_preserves_scene_distance(self):
        self.run_case('unrelated')

    def test_environment_debug_bypass_stays_untouched(self):
        self.run_case('debug-bypass')


if __name__=='__main__':
    unittest.main()
