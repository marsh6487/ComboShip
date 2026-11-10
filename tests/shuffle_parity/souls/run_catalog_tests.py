#!/usr/bin/env python3
"""Compile append-only soul identities with native grant/remove/oracle switch bodies."""
import os, re, subprocess, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
CAT=ROOT/'combo/rando/SoulShuffleCatalog.inc'
assert CAT.exists(), 'missing semantic soul catalog'
rows=re.findall(r'SOUL\((OOT|MM), (ENEMY|NPC|ANIMAL|MISC), (\w+), "([^"]+)"\)',CAT.read_text())
assert len(rows)==154, f'incomplete donor native inventory: {len(rows)}'
assert len({(g,f,n) for g,f,n,_ in rows})==154
assert len({(g,name) for g,_,_,name in rows})==154
print('PASS semantic catalog: 43 OoT enemy, 46/53 NPC, 4/4 animals, 2/2 misc')
for game, family, key, name in rows:
    if game=='OOT':
        assert 'RANDO_ENUM_ITEM(RG_SOUL_'+family+'_'+key+')' in (ROOT/'soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h').read_text()
    else:
        assert 'RI_SOUL_'+family+'_'+key+',' in (ROOT/'mm/2s2h/Rando/Types.h').read_text()
paths={
 'mm_give':'mm/2s2h/Rando/GiveItem.cpp',
 'mm_remove':'mm/2s2h/Rando/RemoveItem.cpp',
 'mm_oracle':'mm/2s2h/BenPort.cpp',
 'oot_has':'soh/soh/Enhancements/randomizer/logic.cpp',
 'oot_effect':'soh/soh/Enhancements/randomizer/logic.cpp',
 'oot_pool':'soh/soh/Enhancements/randomizer/3drando/item_pool.cpp',
}
blocks={}
for name,path in paths.items():
    source=(ROOT/path).read_text()
    begin='// Soul catalog '+name+' begin'
    end='// Soul catalog '+name+' end'
    assert begin in source, f'missing actual native soul path: {name}'
    blocks[name]=source.split(begin,1)[1].split(end,1)[0]
# Compile unchanged production bodies, with native enums and actual packed flag storage.
# Only unrelated outer grant effects/graphics/UI are outside this bounded fixture.
with tempfile.TemporaryDirectory(prefix='native-soul-') as tmp:
    tmp=Path(tmp)
    for game in ('mm','oot'):
        if game=='mm':
            pre='#include "mm/2s2h/Rando/SoulShuffle.h"\n'
            pre+='unsigned short flags[(RANDO_INF_MAX+15)/16]{};\n'
            pre+='void Flags_SetRandoInf(RandoInf f){flags[f/16]|=1u<<(f%16);}\n'
            pre+='void Flags_ClearRandoInf(RandoInf f){flags[f/16]&=~(1u<<(f%16));}\n'
            pre+='void Give(RandoItemId randoItemId){'+blocks['mm_give']+'}\n'
            pre+='void Remove(RandoItemId randoItemId){'+blocks['mm_remove']+'}\n'
            pre+='void Oracle(RandoItemId ri){'+blocks['mm_oracle']+'}\n'
            body='''
#include "tests/shuffle_parity/legacy_ids.inc"
for(auto &s:Rando::SoulShuffle::Catalog) {
                Give(s.item); Give(s.item); assert(flags[s.flag/16] == (1u<<(s.flag%16)));
                Remove(s.item); for(auto flag:flags) assert(flag==0);
                Oracle(s.item); assert(flags[s.flag/16] == (1u<<(s.flag%16)));
                Remove(s.item); for(auto flag:flags) assert(flag==0);
            }'''
        else:
            # Include real native ID definitions; the heavy seed/UI include is unnecessary here.
            pre='#define RANDO_ENUM_BEGIN(n) enum n {\n#define RANDO_ENUM_ITEM(n,...) n,\n#define RANDO_ENUM_END(n) };\n'
            for h in ('RandomizerGet','RandomizerInf','RandomizerSettingKey'):
                pre+=f'#include "soh/soh/Enhancements/randomizer/randomizerEnums/{h}.h"\n'
            hdr=(ROOT/'soh/soh/Enhancements/randomizer/SoulShuffle.h').read_text().replace('#pragma once','').replace('#include "randomizerTypes.h"','').replace('../../../../combo/rando/', 'combo/rando/').replace('#include "SoulShuffleLegacy.inc"','#include "soh/soh/Enhancements/randomizer/SoulShuffleLegacy.inc"')
            pre+=hdr+'\nusing namespace Rando;\nunsigned short flags[(RAND_INF_MAX+15)/16]{};\n'
            pre+='bool options[RSK_MAX]{}; struct Context{bool GetOption(RandomizerSettingKey k){return options[k];}} context; auto*ctx=&context;\n'
            pre+='bool CheckRandoInf(RandomizerInf f){return flags[f/16] & (1u<<(f%16));}\n'
            pre+='void SetRandoInf(RandomizerInf f,bool v){if(v)flags[f/16]|=1u<<(f%16);else flags[f/16]&=~(1u<<(f%16));}\n'
            pre+='bool Has(RandomizerGet itemName){'+blocks['oot_has']+'return false;}\n'
            pre+='void Effect(RandomizerGet randoGet,bool state){'+blocks['oot_effect']+'}\n'
            pre+='int pool[RG_MAX]{}; void AddItemToPool(RandomizerGet i,int a,int b,int c,int d){assert(a==1&&b==1&&c==1&&d==1);pool[i]++;}\n'
            pre+='void Pool(){'+blocks['oot_pool']+'}\n'
            body='''for(auto &s:Rando::SoulShuffle::Catalog) {
                Pool(); assert(pool[s.item]==0); assert(Has(s.item)); options[s.option]=true; Pool(); assert(pool[s.item]==1); for(auto &n:pool)n=0; assert(!Has(s.item));
                Effect(s.item,true); Effect(s.item,true); assert(Has(s.item));
                assert(flags[s.flag/16] == (1u<<(s.flag%16)));
                Effect(s.item,false); assert(!Has(s.item)); options[s.option]=false;
                for(auto flag:flags) assert(flag==0);
            }'''
        source=tmp/(game+'.cpp');source.write_text('#include <cassert>\n'+pre+'int main(){'+body+'}\n')
        for mode in ('original','combo'):
            exe=tmp/(game+mode)
            subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(source),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)
print('PASS actual native soul grant/removal/oracle/ownership bodies, original and Combo guards')
for game, family, key, name in rows:
    native=('soh/soh/Enhancements/randomizer/item_list.cpp' if game=='OOT' else 'mm/2s2h/Rando/StaticData/Items.cpp')
    assert name in (ROOT/native).read_text(), f'missing receipt/catalog presentation: {game} {name}'
