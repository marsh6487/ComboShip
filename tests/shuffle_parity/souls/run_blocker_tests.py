#!/usr/bin/env python3
"""Execute real NPC update bodies: barriers persist while interaction is unavailable."""
from pathlib import Path
import os,re,subprocess,tempfile
from runtime_fixture import prepare_runtime
ROOT=Path(__file__).resolve().parents[3]
def body(path,signature):
 s=(ROOT/path).read_text();a=s.index(signature);e=s.index('{',a)+1;d=1
 while d:d+=(s[e]=='{')-(s[e]=='}');e+=1
 return re.sub(r'\bthis\b','self',s[a:e])
with tempfile.TemporaryDirectory(prefix='souls-blockers-') as t:
 t=Path(t);prepare_runtime(str(t))
 (t/'blockers.inc').write_text(body('soh/src/overlays/actors/ovl_En_Md/z_en_md.c','void EnMd_Update(Actor* thisx, PlayState* play) {')+'\n'+body('soh/src/overlays/actors/ovl_En_Kz/z_en_kz.c','void EnKz_Update(Actor* thisx, PlayState* play) {'))
 for mode in ('original','combo'):
  exe=t/mode
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-O0','-fmax-errors=5','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-I'+str(t),*['-I'+str(ROOT/p) for p in ['soh','soh/include','soh/src','soh/assets','libultraship/include','combo']],*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(ROOT/'tests/shuffle_parity/souls/blocker_test.cpp'),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)

from mm_runtime_fixture import prepare_mm
with tempfile.TemporaryDirectory(prefix='souls-mm-blockers-') as tmp:
 t=Path(tmp);prepare_mm(tmp)
 def enum(path,end):
  s=(ROOT/path).read_text();b=s.index('} '+end+';')+len('} '+end+';');a=s.rfind('typedef enum {',0,b);return s[a:b]
 giant='mm/src/overlays/actors/ovl_En_Dai/z_en_dai.c';guard='mm/src/overlays/actors/ovl_En_Bombers2/z_en_bombers2.c'
 (t/'mm_blockers.inc').write_text(enum(giant,'EnDaiAnimation')+'\n'+enum(guard,'EnBombersAnimation')+'\n'+body(giant,'void func_80B3EEDC(')+'\n'+body(giant,'void EnDai_Update(Actor* thisx, PlayState* play) {')+'\n'+body(guard,'void EnBombers2_Update(Actor* thisx, PlayState* play) {'))
 source=(ROOT/giant).read_text();a=source.index("static Vec3f D_80B3FBF0");b=source.index(";",a)+1
 extra=body('mm/src/code/z_actor.c','void Flags_SetWeekEventReg(')+'\n'+body('mm/src/code/z_actor.c','void Flags_ClearWeekEventReg(')+'\n'+source[a:b]+"\n"+body(giant,"void EnDai_Init(Actor* thisx, PlayState* play) {")+"\n"
 source=(ROOT/guard).read_text();a=source.index("static ColliderCylinderInit sCylinderInit");b=source.index("};",a)+2
 extra+=source[a:b]+"\n"+body(guard,"void EnBombers2_Init(Actor* thisx, PlayState* play) {")+"\n"
 # Native C asset handles use preexisting pointer-type allowances; retain real
 # headers and cast only those native resource handles for this C++ fixture.
 extra=extra.replace('&object_dai_Skel_0130D0','(FlexSkeletonHeader*)&object_dai_Skel_0130D0').replace('&object_cs_Skel_00F82C','(FlexSkeletonHeader*)&object_cs_Skel_00F82C').replace('&gBomberIdleAnim','(AnimationHeader*)&gBomberIdleAnim')
 (t/"mm_blocker_init.inc").write_text(extra)
 for mode in ('original','combo'):
  exe=t/mode
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-fpermissive','-O0','-fmax-errors=5','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-I'+str(t),*['-I'+str(ROOT/p) for p in ['mm','mm/include','mm/include/PR','mm/src','mm/assets','mm/2s2h','libultraship/include','combo']],*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(ROOT/'tests/shuffle_parity/souls/mm_blocker_test.cpp'),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
