"""Execute the production native OoT note palette and MM song export.

GPU submission is captured at the command boundary; no runtime rasterization claim.
"""
import os
from pathlib import Path
import subprocess
import tempfile
from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]
oot = (ROOT / 'soh/src/code/z_draw.c').read_text()
mm = (ROOT / 'combo/menu/ComboItemDrawMM.h').read_text()
native = function(oot, 'GetItem_DrawGenericMusicNote')
recipe = function(mm, 'MM_FillSongDrawInfo')
with tempfile.TemporaryDirectory(prefix='song-gi-') as temp:
    temp = Path(temp)
    source = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
typedef int16_t s16; typedef int32_t s32; typedef uint8_t u8;
typedef struct { struct {void* gfxCtx;} state; } PlayState;
enum {GID_SONG_GENERIC=0x75,GID_SONG_TIME=0x7A,GID_SONG_STORM=0x7B};
void NeiGi_DrawSeasonOverlay(PlayState* play,int profile,const char* owner) {(void)play;(void)profile;(void)owner;}
static struct {void* dlists[3];} sDrawItemTable[256];
static uintptr_t tint[3];
#define OPEN_DISPS(x) ((void)0)
#define CLOSE_DISPS(x) ((void)0)
#define gSPMatrix(...) ((void)0)
#define gDPSetGrayscaleColor(p,r,g,b,a) (tint[0]=(uintptr_t)(r),tint[1]=(uintptr_t)(g),tint[2]=(uintptr_t)(b))
#define gSPGrayscale(...) ((void)0)
#define Gfx_SetupDL_25Opa(...) ((void)0)
#define gSPDisplayList(...) ((void)0)
''' + native + r'''
int main(void) {
    PlayState play={0};
    GetItem_DrawGenericMusicNote(&play,GID_SONG_TIME);
    assert(tint[0]==98 && tint[1]==177 && tint[2]==211);
    GetItem_DrawGenericMusicNote(&play,GID_SONG_GENERIC);
    assert(tint[0]==255 && tint[1]==255 && tint[2]==255);
}
'''
    path = temp / 'note.c'; path.write_text(source)
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-Wno-int-conversion', str(path), '-o', str(temp/'note')], check=True)
    subprocess.run([str(temp/'note')], check=True)
    source = r'''
#include <cassert>
#include <cstring>
#include "combo/menu/ComboItemDrawABI.h"
#include "combo/menu/ComboSongDraw.h"
#include "mm/2s2h/Rando/Types.h"
const char* gGiSongNoteDL="__OTR__objects/object_gi_melody/gGiSongNoteDL";
''' + recipe + r'''
int main() {
    for(int id=RI_SONG_DOUBLE_TIME;id<=RI_SONG_TIME;++id) {
        CwItemDrawInfo info{};
        assert(MM_FillSongDrawInfo(static_cast<RandoItemId>(id),&info)==1);
        if(id==RI_SONG_STORMS) {
            assert(info.drawKind==CW_DRAW_KIND_SEASON_GI && info.neiEffect==6 && info.dlistCount==0);
        } else {
            assert(info.itemShimmer && info.hasEnvColor && info.dlistCount==1);
            assert(!memcmp(info.itemShimmerColor,info.envColor,4));
        }
    }
}
'''
    path = temp / 'song.cpp'; path.write_text(source)
    subprocess.run([os.environ.get('CXX','c++'), '-std=c++20', '-I'+str(ROOT), str(path), '-o', str(temp/'song')], check=True)
    subprocess.run([str(temp/'song')], check=True)
print('PASS native OoT note palette indexing, MM song shimmer colors and weather-only Storms recipe')
