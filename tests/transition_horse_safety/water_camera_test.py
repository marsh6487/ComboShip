#!/usr/bin/env python3
"""Exercise actual Zora waterbox initializer, lookup and camera reads under ASan.

The unpacked WaterBox layout comes directly from the current engine header.
Both the legacy-properties equivalence regression and camera bound regression
fail on the immutable source base. No scene/game asset or GPU is simulated.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize',action='store_true')
parser.add_argument('--ref',help='Read-only baseline control')
parser.add_argument('--case',choices=['all','initializer','bounds'],default='all')
args=parser.parse_args()


def read(path):
    return subprocess.check_output(['git','show',args.ref+':'+path],cwd=ROOT,text=True) if args.ref else (ROOT/path).read_text()


def block(source,marker):
    start=source.index(marker);opening=source.index('{',start);end=opening+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]


source=read('soh/src/code/z_bgcheck.c')
header=read('soh/include/z64bgcheck.h')
water_end=header.index('} WaterBox;')+len('} WaterBox;')
water_start=header.rindex('typedef struct {',0,water_end)
water_type=header[water_start:water_end]
globals=source[source.index('WaterBox zdWaterBox ='):source.index('/**\n * Public. Get the water surface',source.index('WaterBox zdWaterBox ='))]
functions='\n'.join(block(source,m) for m in [
    'void WaterBox_UnpackProperties(', 's32 WaterBox_GetSurface1(', 's32 WaterBox_GetSurfaceImpl(',
    'u32 WaterBox_GetCamDataIndex(', 'u16 WaterBox_GetCameraSType(', 'u32 WaterBox_GetLightSettingIndex('])
fixture=r'''
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
using s32=int32_t;using u32=uint32_t;using s16=int16_t;using u16=uint16_t;using u8=uint8_t;using f32=float;
''' + water_type + r'''
struct CamData { s16 cameraSType{}; s16 numCameras{}; void* camPosData{}; };
struct CollisionHeader { WaterBox* waterBoxes; size_t numWaterBoxes; CamData* cameraDataList; size_t cameraDataListLen; };
struct CollisionContext { CollisionHeader* colHeader; };
struct PlayState { int sceneNum; struct { struct { int num; } curRoom; } roomCtx; };
uintptr_t gSegments[16]{};
#define PHYSICAL_TO_VIRTUAL(x) ((void*)((uintptr_t)(x)+0x80000000u))
enum { SCENE_ZORAS_DOMAIN, SCENE_LAKE_HYLIA };
s32 WaterBox_GetSurfaceImpl(PlayState*,CollisionContext*,f32,f32,f32*,WaterBox**);
void Check(bool ok,const char* reason) { if(!ok) { std::fprintf(stderr,"FAIL: %s\n",reason); std::exit(1); } }
''' + globals + functions + r'''
int main() {
    WaterBox legacy{};WaterBox_UnpackProperties(&legacy,0x2104);
    CamData* cameras=new CamData[6]{};cameras[4].cameraSType=23;
    WaterBox lake{-100,-1313,-100,200,200,4,1,0,false};
    CollisionHeader header{&lake,1,cameras,6};CollisionContext context{&header};PlayState play{SCENE_ZORAS_DOMAIN,{{1}}};
    if (RUN_INITIALIZER) {
        Check(zdWaterBox.camera==legacy.camera && zdWaterBox.lightSetting==legacy.lightSetting &&
              zdWaterBox.room==legacy.room && zdWaterBox.notSwimmable==legacy.notSwimmable,
              "Zora synthetic waterbox interpreted packed 0x2104 as camera 8452");
        WaterBox* found=nullptr;float surface=850;
        Check(WaterBox_GetSurface1(&play,&context,-100,-1200,&surface,&found) && found==&zdWaterBox && surface==877,
              "native King Zora waterbox lookup changed");
        Check(WaterBox_GetCameraSType(&context,found)==23 && WaterBox_GetLightSettingIndex(&context,found)==1,
              "synthetic waterbox lost its native camera/lighting semantics");
        // Summer suppression of the ordinary lake header must not reinterpret
        // the separate King-Zora box or index outside the loaded camera table.
        play.sceneNum=SCENE_LAKE_HYLIA;play.roomCtx.curRoom.num=0;surface=-1300;found=nullptr;
        lake.ySurface=-32000;
        Check(WaterBox_GetSurface1(&play,&context,0,0,&surface,&found) && surface==-32000,
              "Summer's ordinary dry-water lookup contract changed");
        Check(WaterBox_GetCameraSType(&context,found)==23,"drained ordinary box changed native camera selection");
        lake.ySurface=-1313;
    }
    if (RUN_BOUNDS) {
        for(int invalid : {8452,-1,6,0x7fffffff}) {
            lake.camera=invalid;
            Check(WaterBox_GetCameraSType(&context,&lake)==0,"invalid camera id was not rejected");
        }
        lake.camera=4;header.cameraDataListLen=0;
        Check(WaterBox_GetCameraSType(&context,&lake)==0,"empty camera table was not rejected");
        header.cameraDataListLen=6;header.cameraDataList=nullptr;
        Check(WaterBox_GetCameraSType(&context,&lake)==0,"null camera table was not rejected");
        header.cameraDataList=(CamData*)PHYSICAL_TO_VIRTUAL(gSegments[0]);
        Check(WaterBox_GetCameraSType(&context,&lake)==0,"native segment sentinel was not rejected");
    }
    delete[] cameras;
    std::puts("PASS actual unpacked Zora waterbox, native/Summer lookups and bounded camera table reads");
}
'''
fixture='#include <initializer_list>\n#define RUN_INITIALIZER '+str(int(args.case!='bounds'))+'\n#define RUN_BOUNDS '+str(int(args.case!='initializer'))+'\n'+fixture
with tempfile.TemporaryDirectory(prefix='zora-water-camera-') as tmp:
    path=Path(tmp)/'water.cpp';binary=Path(tmp)/'water';path.write_text(fixture)
    command=['c++','-std=c++20','-O1','-g','-Wall','-Wextra','-Wno-unused-parameter','-Wno-missing-field-initializers']
    if args.sanitize:
        command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie']
    subprocess.run(command+[str(path),'-o',str(binary)],check=True)
    env=dict(os.environ);env['ASAN_OPTIONS']='detect_leaks=0'
    subprocess.run([str(binary)],check=True,env=env)
