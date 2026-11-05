#!/usr/bin/env python3
"""Production milk selector/native draw/export/consumer fragments at engine API boundaries.

No installed packs, save or runtime rendering. The resource fixture models the pinned
ResourceManager view API and archive-scoped nested loading, not a full LUS build.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE_REF = None
TEST_CASE = 'all'

def read(path):
    local = ROOT / path
    native = Path(os.environ.get('COMBOSHIP_NATIVE_SOURCE_ROOT',ROOT)) / path
    if SOURCE_REF and not path.startswith('libultraship/') and path not in ('combo/menu/ComboFairyBottle.h','combo/menu/ComboItemDrawABI.h'):
        return subprocess.check_output(['git','show',SOURCE_REF+':'+path],cwd=ROOT,text=True)
    return (local if local.is_file() else native).read_text()

def block(text, marker):
    start = text.index(marker)
    opening = text.index('{', start)
    depth = 1
    for token in re.finditer(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/|[{}]', text[opening + 1:]):
        if token.group() == '{': depth += 1
        elif token.group() == '}':
            depth -= 1
            if not depth: return text[start:opening + 1 + token.end()] + '\n'
    raise ValueError(marker)

def native_struct(text, name):
    end = text.index('} '+name+';') + len('} '+name+';')
    return text[text.rfind('typedef struct {',0,end):end]

def fixture():
    z = read('mm/src/code/z_draw.c')
    export = block(z, 's32 GetItem_GetDrawTableEntry(')
    native = block(z, 'void GetItem_DrawOpa0Xlu1(PlayState* play, s16 drawId) {')
    funcs = sorted(set(re.findall(r'\bGetItem_Draw\w+', export)))
    p = [r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <functional>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
using s8=int8_t; using s16=int16_t; using s32=int32_t; using f32=float;
NATIVE_GFX_DECLARATIONS
#include "ComboBottleGi.h"
NATIVE_SHELL_DECLARATIONS
const char gGiMilkBottleContentsDL[]="__OTR__objects/object_gi_milk/gGiMilkBottleContentsDL";
const char gGiMilkBottleGlassDL[]="__OTR__objects/object_gi_milk/gGiMilkBottleGlassDL";
const char gGiEmptyBottleCorkDL[]="__OTR__empty";
const char gGiEmptyBottleGlassDL[]="__OTR__empty-glass";
constexpr int GID_MILK=1, GID_BOTTLE=2;
#define ARRAY_COUNT(a) s32(sizeof(a)/sizeof((a)[0]))
struct PlayState { struct { void* gfxCtx; } state; };
std::vector<std::string> submitted;
int opaSetup=0,xluSetup=0,matrices=0;
#define OPEN_DISPS(x)
#define CLOSE_DISPS(x)
#define MATRIX_FINALIZE_AND_LOAD(a,b) (++matrices)
#define POLY_OPA_DISP 0
#define POLY_XLU_DISP 0
#define gSPDisplayList(a,b) submitted.emplace_back((const char*)(b))
void Gfx_SetupDL25_Opa(void*) { ++opaSetup; }
void Gfx_SetupDL25_Xlu(void*) { ++xluSetup; }
void NeiGi_ReportBlackMarketModel(const char*,const char*,const char*) {}
''']
    p += [f'void {f}(PlayState*,s16)'+ (';\n' if f == 'GetItem_DrawOpa0Xlu1' else ' {}\n') for f in funcs]
    p += [r'''
struct Row { void (*drawFunc)(PlayState*,s16); void* drawResources[8]; };
Row sDrawItemTable[4]={{},{GetItem_DrawOpa0Xlu1,{(void*)gGiMilkBottleContentsDL,(void*)gGiMilkBottleGlassDL}},
 {GetItem_DrawOpa0Xlu1,{(void*)gGiEmptyBottleCorkDL,(void*)gGiEmptyBottleGlassDL}},
 {GetItem_DrawOpa0Xlu1,{(void*)"__OTR__other-contents",(void*)"__OTR__other-glass"}}};
ComboFairyBottleShell GetItem_EmptyBottleShell(s16 id) { return {(const char*)sDrawItemTable[id].drawResources[0],(const char*)sDrawItemTable[id].drawResources[1]}; }
ComboFairyBottleShell GetItem_FairyBottleShell(s16 id) { return GetItem_EmptyBottleShell(id); }
bool contentsMod=true,glassMod=false;
int ResourceMgr_IsModAsset(const char*) { return 0; }
Gfx* ResourceMgr_LoadGfxByName(const char*) { return nullptr; }
''']
    ben = read('mm/2s2h/BenPort.cpp')
    p.append(resource_fixture())
    if 'static std::weak_ptr<Ship::ResourceManager> sMilkOwner' in ben:
        start = ben.index('static std::mutex sMilkMutex')
        end = ben.index('// End native MM milk owner.', start)
        p.append(ben[start:end])
    else:
        p.append('static void MM_BindMilkBaseOwner(const std::shared_ptr<Ship::ResourceManager>&, const std::shared_ptr<Ship::Archive>&) {}\nextern "C" void MM_MilkBottleSelectPair(const char**,const char**,const char*) {}\n')
    if 'static ComboFairyBottleShell GetItem_MilkBottleShell' in z:
        p.append(block(z, 'static ComboFairyBottleShell GetItem_MilkBottleShell'))
    p += [export,native]
    producer = read('combo/menu/ComboItemDrawMM.h')
    start = producer.index('    if (drawKind == CW_DRAW_KIND_ELEMENTAL_ARROW)\n',producer.index('const auto drawOwner'))
    end = producer.index('    for (int32_t i = 0; i < n; i++)',start)
    p += [r'''
NATIVE_APPEARANCE_DECLARATIONS
constexpr int RI_BOTTLE_EMPTY=3, RI_ARROW_FIRE=4, RI_ARROW_ICE=5, RI_ARROW_LIGHT=6;
constexpr int CW_DRAW_KIND_ELEMENTAL_ARROW=39,CW_DRAW_KIND_MM_FAIRY_BOTTLE=21,CW_DRAW_KIND_MM_FAIRY_CONTAINER=37;
int NeiArrowGi_ProfileForDrawId(int,int,int,int) { return 0; }
void appearance(int drawId, CwItemDrawInfo* out) { int id=0,drawKind=0; struct Entry { struct { int drawId; } second; } entry{{drawId}}; auto it=&entry;
''', producer[start:end], '}\n']
    for suffix,filename in [('MM','ComboForeignDrawMM.h'),('OOT','ComboForeignDrawOOT.h')]:
        src = read('combo/menu/'+filename)
        loop = block(src,'for (int32_t i = 0; i < n; i++) {')
        enumname = 'ComboForeignResolveOOT' if suffix == 'MM' else 'ComboForeignResolve'
        # Execute the actual routing loop in its real local variable/type boundary.
        p += [f'namespace Consumer{suffix} {{\nenum class {enumname} {{ Unknown }};\n',r'''
struct Info { std::string dls[2]; };
struct Raw { const char* dlists[2]; int drawKind=0; };
constexpr int CW_DRAW_KIND_MM_MASK=1,CW_DRAW_KIND_MM_REMAINS=2,CW_DRAW_KIND_MM_SPIN_ATTACK=3;
std::string ComboInternRoutedPath(const std::string& s) { return s; }
std::string ComboInternRoutedPathOOT(const std::string& s) { return s; }
''',f'{enumname} route(Raw raw, Info& info) {{\nstatic constexpr char kOtrPrefix[]="__OTR__"; int32_t n=2;\n',loop,f'return static_cast<{enumname}>(1); }}\n}}\n']
    p += [r'''
int main() {
 PlayState play{};
''']
    if not SOURCE_REF or TEST_CASE == 'owner':
        p += [resource_tests()]
    if TEST_CASE in ('all','native'):
        p.append('    CwItemDrawInfo a{}; appearance(GID_MILK,&a); assert(a.stateDependent==2);\n')
    if TEST_CASE in ('all','native','rendering'):
        p += [r'''
 submitted.clear(); GetItem_DrawOpa0Xlu1(&play,GID_MILK);
 assert(submitted.size()==2 && submitted[0]=="__OTR__@mm-gi-base:objects/object_gi_milk/gGiMilkBottleContentsDL");
 assert(submitted[1]=="__OTR__@mm-gi-base:objects/object_gi_milk/gGiMilkBottleGlassDL");
 assert(opaSetup==1 && xluSetup==1 && matrices==2);
 void* dls[8]{}; int xlu=-1,scroll=-1,kind=-1; float scale=-1;
 assert(GetItem_GetDrawTableEntry(GID_MILK,dls,8,&xlu,&scale,&scroll,&kind)==2);
 assert((const char*)dls[0]==submitted[0] && (const char*)dls[1]==submitted[1]);
 assert(xlu==1 && scale==0 && scroll==0 && kind==0);
 assert(GetItem_GetDrawTableEntry(GID_MILK,dls,1,&xlu,&scale,&scroll,&kind)==0);
 submitted.clear(); GetItem_DrawOpa0Xlu1(&play,3);
 assert(submitted[0]=="__OTR__other-contents");
''']
    for suffix in (['MM','OOT'] if TEST_CASE in ('all','consumer') else []):
        enumname = 'ComboForeignResolveOOT' if suffix=='MM' else 'ComboForeignResolve'
        p += [f'''{{ using namespace Consumer{suffix}; Info info; Raw raw{{{{"__OTR__@mm-gi-base:objects/a","__OTR__@mm-gi-base:objects/b"}}}};
 assert(route(raw,info)!={enumname}::Unknown); assert(info.dls[0]==raw.dlists[0]); assert(info.dls[1]==raw.dlists[1]);
 raw.dlists[0]="__OTR__@mm-gi-baseX:objects/a"; assert(route(raw,info)=={enumname}::Unknown);
 raw.dlists[0]="__OTR__@unknown:objects/a"; assert(route(raw,info)=={enumname}::Unknown);
 raw.dlists[0]="__OTR__@oot-gi-base:objects/a"; assert(route(raw,info)!={enumname}::Unknown); }}\n''']
    p += ['std::cout << "PASS milk production selector, lifecycle, native/export and consumers\\n"; }\n']
    gbi = read('libultraship/include/libultraship/libultra/gbi.h')
    union_start=gbi.index('typedef union Gfx {'); union_end=gbi.index('} Gfx;',union_start)+len('} Gfx;')
    gfx=native_struct(gbi,'Gwords')+'\n'+gbi[union_start:union_end]+'\n'+re.search(r'^#define G_DL_OTR_FILEPATH .+$',gbi,re.M).group()
    abi = read('combo/menu/ComboItemDrawABI.h')
    appearance='\n'.join(re.search(r'^#define '+name+r' .+$',abi,re.M).group() for name in ['CW_DRAW_MAX_DLISTS','CW_DRAW_MAX_OPS'])+'\n'+native_struct(abi,'CwDrawOp')+'\n'+native_struct(abi,'CwItemDrawInfo')
    return ''.join(p).replace('NATIVE_GFX_DECLARATIONS',gfx).replace('NATIVE_SHELL_DECLARATIONS',native_struct(read('combo/menu/ComboFairyBottle.h'),'ComboFairyBottleShell')).replace('NATIVE_APPEARANCE_DECLARATIONS',appearance)

def resource_fixture():
    return (Path(__file__).parent/'resource_fixture.inc').read_text()

def resource_tests():
    return (Path(__file__).parent/'resource_tests.inc').read_text()

def c_fixture():
    shell = native_struct(read('combo/menu/ComboFairyBottle.h'),'ComboFairyBottleShell')
    z=read('mm/src/code/z_draw.c')
    if 'static ComboFairyBottleShell GetItem_MilkBottleShell' not in z:
        return None
    return '#include <stdint.h>\n#include <string.h>\n#include <assert.h>\ntypedef int16_t s16;\n'+shell+r"""
enum { GID_MILK=1 };
static const char gGiMilkBottleContentsDL[]="native-contents";
static const char gGiMilkBottleGlassDL[]="native-glass";
int calls=0;
void MM_MilkBottleSelectPair(const char** a,const char** b,const char* host) { (void)host; ++calls; *a="base-contents"; *b="base-glass"; }
"""+block(z,'static ComboFairyBottleShell GetItem_MilkBottleShell')+r"""
int main(void) {
 ComboFairyBottleShell shell={gGiMilkBottleContentsDL,gGiMilkBottleGlassDL};
 ComboFairyBottleShell result=GetItem_MilkBottleShell(GID_MILK,shell,"mm");
 assert(calls==1 && !strcmp(result.opaque,"base-contents") && !strcmp(result.glass,"base-glass"));
 result=GetItem_MilkBottleShell(0,shell,"mm"); assert(calls==1 && result.opaque==shell.opaque);
 shell.glass="explicit-native-glass"; result=GetItem_MilkBottleShell(GID_MILK,shell,"mm");
 assert(calls==1 && result.glass==shell.glass);
 shell.opaque="ordinary-contents"; result=GetItem_MilkBottleShell(GID_MILK,shell,"mm");
 assert(calls==1 && result.opaque==shell.opaque);
 return 0;
}
"""

def main():
    global SOURCE_REF, TEST_CASE
    ap=argparse.ArgumentParser(); ap.add_argument('--sanitize',action='store_true'); ap.add_argument('--baseline-ref'); ap.add_argument('--case',choices=['all','owner','native','rendering','consumer'],default='all'); args=ap.parse_args()
    SOURCE_REF=args.baseline_ref; TEST_CASE=args.case
    with tempfile.TemporaryDirectory(prefix='milk-bottle-') as d:
        src=Path(d)/'fixture.cpp'; src.write_text(fixture()); exe=Path(d)/'fixture'
        cmd=[*shlex.split(os.environ.get('CXX','c++')),'-std=c++20','-g','-Wall','-Wextra','-Wno-unused-variable','-I'+str(ROOT/'combo/menu'),str(src),'-o',str(exe)]
        if args.sanitize: cmd += ['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
        c_source=c_fixture()
        if c_source:
            c_src=Path(d)/'selector.c'; c_src.write_text(c_source)
            c_cmd=[*shlex.split(os.environ.get('CC','cc')),'-std=c11','-Wall','-Wextra','-Werror',str(c_src),'-o',str(exe)]
            if args.sanitize: c_cmd += ['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
            subprocess.run(c_cmd,check=True)
            subprocess.run([str(exe)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
        for build in ('combo','standalone'):
            flags=['-DCOMBO_BUILD','-DGBI_FLOAT_MTX','-DGBI_S32_VTX'] if build=='combo' else []
            subprocess.run(cmd+flags,check=True)
            subprocess.run([str(exe)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})

if __name__=='__main__': main()
