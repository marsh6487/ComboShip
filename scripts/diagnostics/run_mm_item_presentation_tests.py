#!/usr/bin/env python3
"""Execute the production cross-game GI selection boundary without either game runtime."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def function(source, name):
    start = source.rfind('\n', 0, source.index(name + '(')) + 1
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

src = (ROOT / 'combo/menu/ComboItemDrawOOT.h').read_text()
body = function(src, 'OOT_FillItemDrawInfo')
preamble = r'''
#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <iostream>
#include "combo/menu/ComboItemDrawABI.h"
using s16=int16_t; using s32=int32_t; using f32=float;
enum RandomizerGet { RG_NONE, RG_TEST_CUSTOM, RG_TEST_NATIVE, RG_TEST_PROGRESSIVE, RG_TEST_TIER };
constexpr int TABLE_RANDOMIZER=1;
struct GetItemEntry { int tableId, drawItemId, gid; void (*drawFunc)(); int itemId=0; };
void customDraw() {}
void Randomizer_DrawCaneOfSomaria() {}
void Randomizer_DrawCanePacci() {}
void Randomizer_DrawCaneSomariaUpgrade() {}
void Randomizer_DrawCanePacciUpgrade() {}
void Randomizer_DrawCanePacciUltrahand() {}
void (*awardDraw)()=customDraw;
int tableCalls=0; bool redesigned=false;
RandomizerGet described=RG_NONE;
namespace Rando::StaticData {
struct Name { std::string english="Awarded tier"; };
struct Item {
 RandomizerGet id;
 std::shared_ptr<GetItemEntry> GetGIEntry(RandomizerGet* actual) const {
  if(id==RG_TEST_PROGRESSIVE) *actual=RG_TEST_TIER;
  return std::make_shared<GetItemEntry>(GetItemEntry{TABLE_RANDOMIZER,id==RG_TEST_PROGRESSIVE?RG_TEST_TIER:id,
      id==RG_TEST_NATIVE?7:0, id==RG_TEST_NATIVE?nullptr:awardDraw});
 }
 const Name& GetName() const { static Name n; return n; }
};
Item RetrieveItem(RandomizerGet id) { return {id}; }
}
int OOT_DescribeCustomDraw(RandomizerGet id, CwItemDrawInfo*) { described=id;return 0; }
int NeiGi_DescribeEntry(const GetItemEntry*,CwItemDrawInfo* out) {
 if(!redesigned) return 0;
 out->drawKind=CW_DRAW_KIND_SIMPLE;out->dlists[0]="__OTR__objects/nei_gi_redesign/spinner/gi_dl";
 out->dlistCount=1;return 1;
}
int GetItem_GetDrawTableEntry(int,void** out,int,int*,float*,int*,uint8_t*) {
 ++tableCalls;out[0]=(void*)"__OTR__native";return 1;
}
void GetItem_GetDrawSetupDLs(int,void**,void**) {}
int GetItem_GetShimmerColor(s16,uint8_t*) { return 0; }
'''
checks = r'''
int main() {
 CwItemDrawInfo info{};
 assert(OOT_FillItemDrawInfo(RG_TEST_CUSTOM,&info)==0);
 assert(tableCalls==0); // custom callback must never consult bottle row zero
 redesigned=true;
 assert(OOT_FillItemDrawInfo(RG_TEST_CUSTOM,&info)==1);
 assert(tableCalls==0 && std::string(info.dlists[0]).find("spinner")!=std::string::npos);
 redesigned=false;
 assert(OOT_FillItemDrawInfo(RG_TEST_PROGRESSIVE,&info)==0);
 assert(described==RG_TEST_TIER && std::string(info.resolvedName)=="Awarded tier");
 assert(OOT_FillItemDrawInfo(RG_TEST_NATIVE,&info)==1 && tableCalls==1);
 void (*skills[])()={Randomizer_DrawCaneOfSomaria,Randomizer_DrawCanePacci,
   Randomizer_DrawCaneSomariaUpgrade,Randomizer_DrawCanePacciUpgrade,Randomizer_DrawCanePacciUltrahand};
 int expected[]={1,2,3,4,6};
 for(int i=0;i<5;i++) {
   awardDraw=skills[i]; info={};
   assert(OOT_FillItemDrawInfo(RG_TEST_CUSTOM,&info)==1);
   assert(info.drawKind==CW_DRAW_KIND_NEI_CANE && info.neiLegacyCane==expected[i]);
   assert(tableCalls==1);
 }

 std::cout<<"PASS actual GI boundary: custom fallback cannot become bottle; resolved tier and native rows preserved\n";
}
'''
with tempfile.TemporaryDirectory(prefix='mm-presentation-') as td:
    test=Path(td)/'test.cpp';test.write_text(preamble+body+checks)
    binary=Path(td)/'test'
    flags=['-std=c++20','-Wall','-Wextra','-I'+str(ROOT)]
    if '--sanitize' in __import__('sys').argv: flags += ['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run([os.environ.get('CXX','c++'),*flags,str(test),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)

message = (ROOT / 'mm/src/code/z_message.c').read_text()
consumer = (ROOT / 'mm/2s2h/Rando/DrawItem.cpp').read_text()
fixture = (ROOT / 'tests/mm_presentation/icon_bridge_test.cpp').read_text()
bindings = re.findall(r'itemTable\[RG_BOTTLE_WITH_[A-Z_]+\]\.CustomIcon\([^;]+;',
                      (ROOT / 'soh/soh/Enhancements/randomizer/item_list.cpp').read_text())
fixture = fixture.replace('/* BOTTLE_ICON_BINDINGS */', '\n'.join(bindings))
fixture = fixture.replace('/* OWNER_ICON */', function(src, 'OOT_FillItemIconInfo'))
fixture = fixture.replace('/* STAGE_ICON */', function(message, 'Message_StageCustomItemIconEx') + '\n' + function(message, 'Message_StageCustomItemIcon'))
fixture = fixture.replace('/* CONSUMER_ICON */', function(consumer, 'Rando::ComboForeignMessageIcon'))
with tempfile.TemporaryDirectory(prefix='mm-icon-') as td:
    test=Path(td)/'test.cpp';test.write_text(fixture)
    binary=Path(td)/'test'
    subprocess.run([os.environ.get('CXX','c++'),*flags,'-I'+str(ROOT / 'soh/include'),str(test),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)

conversion = (ROOT / 'mm/2s2h/Rando/ConvertItem.cpp').read_text()
fixture = (ROOT / 'tests/mm_presentation/cane_progression_test.cpp').read_text()
fixture = fixture.replace('/* RESOLVE_CANE */', function(conversion, 'ResolveCanePresentation'))
fixture = fixture.replace('/* RAW_PREVIEW */', function(consumer, 'Rando::DrawItem'))
assert 'case RI_OOT_NEI_CANE_OF_SOMARIA:\n                return ResolveCanePresentation();' in conversion
queue = (ROOT / 'mm/2s2h/Rando/MiscBehavior/CheckQueue.cpp').read_text()
assert 'Rando::DrawResolvedItem(randoItemId, randoCheckId, actor);' in queue
with tempfile.TemporaryDirectory(prefix='mm-cane-') as td:
    test=Path(td)/'test.cpp';test.write_text(fixture)
    binary=Path(td)/'test'
    subprocess.run([os.environ.get('CXX','c++'),*flags,str(test),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
