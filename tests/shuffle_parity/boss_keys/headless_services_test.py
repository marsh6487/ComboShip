#!/usr/bin/env python3
"""Actual MM headless initializer and real GameInteractor; graph/name service boundaries."""
import os,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
s=(ROOT/'mm/2s2h/BenPort.cpp').read_text();a=s.index('extern "C" COMBO_EXPORT void MM_InitRandoHeadless(void)');b=s.index('{',a);e=b+1;d=1
while d:d+=(s[e]=='{')-(s[e]=='}');e+=1
program='''
#include "Rando/Logic/Logic.h"
#include <cassert>
#define COMBO_EXPORT
GameInteractor* GameInteractor::Instance=nullptr;
int graphInitializations=0,nameInitializations=0;
namespace ShipInit {void Init(const char*){++graphInitializations;}}
namespace Rando::StaticData {void PopulateCheckNames(){++nameInitializations;}}
'''+s[a:e]+'''
int main(){MM_InitRandoHeadless();assert(GameInteractor::Instance);assert(GameInteractor::Instance->nextHookId==1 && GameInteractor::Instance->events.empty());auto*p=GameInteractor::Instance;MM_InitRandoHeadless();assert(GameInteractor::Instance==p && graphInitializations==1 && nameInitializations==1);delete p;GameInteractor::Instance=nullptr;}
'''
with tempfile.TemporaryDirectory(prefix='headless-service-') as td:
 p=Path(td);cpp=p/'test.cpp';cpp.write_text(program);flags=['-std=c++20','-DCOMBO_BUILD','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX','-DCONTROLLERBUTTONS_T=uint32_t','-include',str(ROOT/'tests/dungeon_rewards/headless.h')]
 for name in ('.','combo','mm/include','mm/include/PR','mm','mm/2s2h','mm/src','mm/assets','libultraship/include'):flags+=['-I'+str(ROOT/name)]
 subprocess.run([os.environ.get('CXX','c++'),*flags,str(cpp),'-o',str(p/'test')],check=True);subprocess.run([str(p/'test')],check=True)
print('PASS real headless service object, native initializer idempotence; graph execution not claimed')
