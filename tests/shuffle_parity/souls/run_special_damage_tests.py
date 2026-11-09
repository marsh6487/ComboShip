#!/usr/bin/env python3
"""Execute native Dodongo bomb conditions before the native swallow functions."""
from pathlib import Path
import os,re,subprocess,tempfile
from runtime_fixture import prepare_runtime
ROOT=Path(__file__).resolve().parents[3]
def condition(path):
 s=(ROOT/path).read_text();a=s.index('Randomizer_SoulCanDamageActor(play, &this->actor)');a=s.rfind('if (',0,a)+4;e=a;d=1
 while d:d+=(s[e]=='(')-(s[e]==')');e+=1
 return re.sub(r'\bthis\b','self',s[a:e-1])
with tempfile.TemporaryDirectory(prefix='souls-special-damage-') as tmp:
 t=Path(tmp);prepare_runtime(tmp)
 dodongo='soh/src/overlays/actors/ovl_En_Dodongo/z_en_dodongo.c';baby='soh/src/overlays/actors/ovl_En_Dodojr/z_en_dodojr.c'
 s=(ROOT/dodongo).read_text();a=s.index('typedef enum {');b=s.index('} EnDodongoActionState;',a)+len('} EnDodongoActionState;')
 (t/'bomb.inc').write_text(s[a:b]+'\nbool DodongoBomb(EnDodongo*self,PlayState*play){return '+condition(dodongo)+';}\nbool BabyBomb(EnDodojr*self,PlayState*play){return '+condition(baby)+';}\n')
 for mode in ('original','combo'):
  exe=t/mode;subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-O0','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-I'+tmp,*['-I'+str(ROOT/p) for p in ['soh','soh/include','soh/src','soh/assets','libultraship/include','combo']],*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(ROOT/'tests/shuffle_parity/souls/special_damage_test.cpp'),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
