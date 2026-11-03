#!/usr/bin/env python3
"""Run MM's real cane ownership/selection and first-grant functions."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(path, signature):
    source = (ROOT / path).read_text(encoding='utf-8-sig')
    start = source.index(signature)
    brace = source.index('{', start)
    end, depth = brace + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


native = 'mm/mods/nei_save.cpp'
cane = 'mm/mods/items/logic/item_cane_of_somaria.c'
header = (ROOT / 'mm/mods/items/logic/item_cane_of_somaria.h').read_text()
defines = '\n'.join(line for line in header.splitlines()
                    if re.match(r'#define CANE_(?:TYPE_\w+|SKILL_\w+)(?:\([^)]*\))? ', line)
                    and not line.endswith('\\'))
defines += '\n' + '\n'.join(re.findall(r'^#define NEI_CANE_.*$', (ROOT / native).read_text(), re.M))
code = r'''
#include "mm/mods/nei_save.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
using u8=uint8_t; using s8=int8_t;
struct Player {}; struct PlayState {};
static NeiSaveData save{};
static bool vanilla;
static int wheelMode;
[[maybe_unused]] static bool Combo_IsVanilla() { return vanilla; }
static int CVarGetInteger(const char*,int) { return wheelMode; }
extern "C" NeiSaveData* Nei_Save() { return &save; }
extern "C" void Nei_SetOwnedItem(uint8_t slot,uint16_t item) { assert(slot>=24 && slot<72);save.ownedItems[slot-24]=item; }
#define SLOT_CANE_OF_SOMARIA 45
#define ITEM_CANE_OF_SOMARIA 0xC3
static int gSfxDefaultPos,gSfxDefaultFreqAndVolScale,gSfxDefaultReverb;
#define NA_SE_SY_CURSOR 1
static void Audio_PlaySoundGeneral(int,void*,int,void*,void*,void*) {}
'''
code += defines + '\n'
for signature in ('static uint8_t Nei_CaneTypeGateBit', 'extern "C" uint8_t Nei_CaneHasSkill',
                  'extern "C" void Nei_CaneGrantSkill', 'extern "C" uint8_t Nei_CaneOwned',
                  'extern "C" uint8_t Nei_CaneTypeOwned', 'extern "C" uint8_t Nei_CaneTypeVisible',
                  'extern "C" uint8_t Nei_CaneGetType', 'extern "C" void Nei_CaneSetType',
                  'static uint8_t Nei_SomariaSummonOwned', 'extern "C" uint8_t Nei_CaneGetSkillSlot',
                  'extern "C" void Nei_CaneSetSkillSlot', 'extern "C" uint8_t Nei_CaneActiveSkill'):
    code += function(native, signature) + '\n'
for signature in ('u8 Cane_HasSkill', 'u8 Cane_GetType', 'u8 Cane_GiveSkill',
                  'static u8 Cane_SummonSlotOwned', 'static void Cane_CycleSummon'):
    code += function(cane, signature) + '\n'
code += r'''
int main() {
    vanilla=true;
    constexpr std::array<int,6> family={0,0,1,2,2,3};
    for(unsigned skill=0;skill<6;++skill) {
        save={};std::fill(std::begin(save.ownedItems),std::end(save.ownedItems),0xFF);
        assert(Cane_GiveSkill(skill));
        assert(save.caneSkills==(1u<<skill));
        assert(save.ownedItems[SLOT_CANE_OF_SOMARIA-24]==ITEM_CANE_OF_SOMARIA);
        assert(Nei_CaneTypeOwned(family[skill]));
        assert(Nei_CaneGetType()==family[skill]);
        if(skill!=2) assert(Nei_CaneActiveSkill()==skill);
        const auto selected=save.caneType;
        assert(!Cane_GiveSkill(skill));
        assert(save.caneType==selected);
    }
    for(unsigned mask=0;mask<64;++mask) {
        save={};save.caneSkills=mask;
        for(wheelMode=0;wheelMode<4;++wheelMode) {
            for(unsigned type=0;type<4;++type) {
                constexpr std::array<unsigned,4> gates={3,4,24,32};
                assert(bool(Nei_CaneTypeOwned(type))==bool(mask&gates[type]));
                assert(Nei_CaneTypeVisible(type)==Nei_CaneTypeOwned(type));
            }
        }
    }
    save={};save.caneSkills=(1<<3)|(1<<4);save.caneType=2;
    Player player; PlayState play;
    assert(Nei_CaneActiveSkill()==3);
    Cane_CycleSummon(&player,&play,1);
    assert(Nei_CaneActiveSkill()==4);
    Cane_CycleSummon(&player,&play,-1);
    assert(Nei_CaneActiveSkill()==3);
    save.caneSkillSel[0]=2;
    Nei_CaneSetSkillSlot(2,1);
    assert(save.caneSkillSel[0]==2 && save.caneSkillSel[1]==1);
    // Seeded/solo retain the old four-entry gate and ordinary Pacci treatment.
    vanilla=false;wheelMode=0;save={};save.caneSkills=1<<1;
    assert(!Nei_CaneTypeOwned(0));
    save.caneSkills=(1<<3)|(1<<4);save.caneType=2;save.caneSkillSel[1]=1;
    assert(Nei_CaneActiveSkill()==3);
    Cane_CycleSummon(&player,&play,1);
    assert(Nei_CaneActiveSkill()==3);
    std::cout<<"PASS MM independent cane families, exact first grants, Pacci choice and seeded/solo controls\n";
}
'''
with tempfile.TemporaryDirectory(prefix='vanilla-mm-cane-') as tmp:
    source=Path(tmp)/'test.cpp';source.write_text(code)
    flags=['-std=c++20','-Wall','-Wextra','-Werror','-Wno-unused-parameter',
           '-I'+str(ROOT),'-I'+str(ROOT/'mm'),'-I'+str(ROOT/'mm/2s2h')]
    if '--sanitize' in sys.argv:
        flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie']
    binary=Path(tmp)/'test'
    subprocess.run([*shlex.split(os.environ.get('CXX','c++')),*flags,str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
