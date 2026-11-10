#!/usr/bin/env python3
"""Round-trip actual MM NEI JSON conversion without changing legacy masks or item IDs."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
s=(ROOT/'mm/2s2h/BenJsonConversions.hpp').read_text()
def extract(signature):
 start=s.index(signature);pos=s.index('{',start);depth=0
 for i in range(pos,len(s)):
  if s[i]=='{':depth+=1
  if s[i]=='}':
   depth-=1
   if depth==0:return s[start:i+1]
code=r'''#include "mods/nei_save.h"
#include "combo/rando/RpgStatsJson.h"
#include <nlohmann/json.hpp>
#include <cstring>
#include <cassert>
#include <cstdio>
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
using json=nlohmann::json;
'''+extract('inline void to_json(json& j, const NeiSaveData& n)')+'\n'+extract('inline void from_json(const json& j, NeiSaveData& n)')+r'''
int main(){
 NeiSaveData n{},loaded{};n.ritoMaskOwned=1;n.ritoMaskSelected=1;n.ootMasksOwned=0x91;n.ootMaskCursor=8;n.activeCustomForm=5;
 json j;to_json(j,n);from_json(j,loaded);
 assert(loaded.ritoMaskOwned==1&&loaded.ritoMaskSelected==1&&loaded.ootMasksOwned==0x91&&loaded.ootMaskCursor==8);
 json old=json::object();from_json(old,loaded);assert(!loaded.ritoMaskOwned&&!loaded.ritoMaskSelected);
 for(json bad:{json(-1),json(2),json(257),json(true),json("1")}){old["ritoMaskSelected"]=bad;from_json(old,loaded);assert(!loaded.ritoMaskSelected);}
 old["ritoMaskOwned"]=1;old["ritoMaskSelected"]=1;from_json(old,loaded);assert(loaded.ritoMaskOwned&&loaded.ritoMaskSelected);
 puts("PASS MM Rito native JSON round-trip, old-save defaults and malformed selection validation");
}
'''
with tempfile.TemporaryDirectory(prefix='rito-mm-save-') as d:
 p=Path(d);(p/'test.cpp').write_text(code)
 subprocess.run(['c++','-std=c++20','-I'+str(ROOT),'-I'+str(ROOT/'mm'),'-I'+str(ROOT/'mm/include'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
