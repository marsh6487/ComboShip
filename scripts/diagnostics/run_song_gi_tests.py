"""Execute the production native OoT note palette and MM song export.

GPU submission is captured at the command boundary; no runtime rasterization claim.
"""
import os
from pathlib import Path
import subprocess
import tempfile
from run_mm_scene_randomization_tests import function
from run_time_pedestal_tests import functions

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
#include "combo/menu/ComboSongDrawMM.h"
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
    draw_song = functions((ROOT/'mm/2s2h/Rando/DrawItem.cpp').read_text())["DrawSong"]
    dispatcher = functions((ROOT/'mm/2s2h/Rando/NeiGiPresentation.cpp').read_text())["MM_TryDrawNeiGi"]
    source = r'''
#include <cassert>
#include <array>
#include <cstring>
#include <cstdint>
#include "combo/menu/ComboItemDrawABI.h"
#include "mm/2s2h/Rando/Types.h"
#include "combo/menu/ComboSongDrawMM.h"
#include "soh/soh/Enhancements/randomizer/NeiGiSongEffectPolicy.h"
struct PlayState { struct {void* gfxCtx;} state; uint32_t gameplayFrames; } play{};
PlayState* gPlayState=&play;
static int overlay=-1, season=-1, notes=0, genericShimmer=0, descriptions=0;
static std::array<uint8_t,4> env{};
using Gfx=int;
const char* gGiSongNoteDL="note";
#define OPEN_DISPS(...) ((void)0)
#define CLOSE_DISPS(...) ((void)0)
#define Gfx_SetupDL25_Xlu(...) ((void)0)
#define MATRIX_FINALIZE_AND_LOAD(...) ((void)0)
#define gDPSetEnvColor(p,r,g,b,a) (env={uint8_t(r),uint8_t(g),uint8_t(b),uint8_t(a)})
#define gSPDisplayList(...) (++notes)
void NeiGi_DrawSeasonOverlay(PlayState*,int profile,const char*) {season=profile;}
void NeiGi_DrawSongOverlay(PlayState*,int song,const char*) {overlay=song;}
void ComboDrawMaskShimmer(PlayState*,void*,const uint8_t*,const char*) {++genericShimmer;}
bool MM_DescribeNeiGi(RandoItemId,CwItemDrawInfo*) {++descriptions;return false;}
void MM_DrawNeiGi(const CwItemDrawInfo&) {assert(false);}
''' + draw_song + '\n' + dispatcher + r'''
int main() {
    for(int id=RI_UNKNOWN;id<RI_MAX;++id) {
        const int song=ComboSongForMmItem(id);
        if(song<0)continue;
        overlay=season=-1; notes=genericShimmer=descriptions=0; env={};
        assert(MM_TryDrawNeiGi(static_cast<RandoItemId>(id)) &&
               "early NEI dispatcher must claim every native/imported song before shared GI table IDs");
        assert(descriptions==0);
        if(song==CW_SONG_STORMS)assert(notes==0 && season==6);
        else {
            uint8_t color[4];assert(ComboSongShimmerColor(song,color));
            assert(notes==1 && overlay==song && genericShimmer==0);
            assert(!std::memcmp(env.data(),color,4));
        }
    }
    for(int song=0;song<CW_SONG_COUNT;++song) {
        for(uint32_t frame:{0u,42u,179u,180u,719u,720u,0xffffffffu}) {
            const auto mesh=NeiGi::SampleSong(song,frame);
            assert(mesh.count<=mesh.vertices.size() && mesh.count%3==0);
            if(song==CW_SONG_STORMS)assert(mesh.count==0);
            else {
                assert(mesh.count>0);
                bool hue=false;
                for(size_t i=0;i<mesh.count;++i) {
                    const auto& v=mesh.vertices[i];
                    assert(std::isfinite(v.p.x)&&std::isfinite(v.p.y)&&std::isfinite(v.p.z));
                    assert(std::abs(v.p.x)<60&&std::abs(v.p.y)<60&&std::abs(v.p.z)<60);
                    hue |= v.rgb==ComboSongColorHex(song);
                }
                assert(hue);
            }
        }
    }
    const auto feather=NeiGi::SampleSong(CW_SONG_SOARING,42);
    assert(feather.count==1296 && "Soaring must submit the six triangulated feather vanes/rachises");
}
'''
    path=temp/'dispatch.cpp';path.write_text(source)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),'-I'+str(ROOT/'combo/menu'),str(path),'-o',str(temp/'dispatch')],check=True)
    subprocess.run([str(temp/'dispatch')],check=True)
print('PASS native OoT palette, MM exports and early production dispatcher, all recovered song profiles and Soaring feather geometry')
