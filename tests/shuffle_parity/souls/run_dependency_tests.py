#!/usr/bin/env python3
"""Execute native check-gate adapters and actual stored-predicate boundaries."""
from pathlib import Path
import os, re, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[3]
for game,folder in [('oot','soh/soh/Enhancements/randomizer'),('mm','mm/2s2h/Rando')]:
    assert (ROOT/folder/'SoulShuffleChecks.inc').exists(), f'missing explicit {game} soul check dependencies'
# Audit source categories against explicit IDs. This is an author/test audit;
# production lookups use enum IDs and never infer meaning from display names.
oot_native='\n'.join(p.read_text() for p in (ROOT/'soh/soh/Enhancements/randomizer').glob('*.cpp'))
mm_native=(ROOT/'mm/2s2h/Rando/StaticData/Checks.cpp').read_text()
for game,native,patterns in [
    ('oot',oot_native,[r'locationTable\[(RC_\w+)\].*?RCTYPE_COW\b',r'locationTable\[(RC_\w+)\].*?Location::ButterflyFairy\(',r'locationTable\[(RC_\w+)\].*?Location::GSToken\(',r'locationTable\[(RC_\w+)\].*?RCTYPE_SCRUB\b']),
    ('mm',mm_native,[r'RC\((RC_\w+),\s*RCTYPE_(?:COW|BUTTERFLY|SKULL_TOKEN)\b'])]:
    folder='soh/soh/Enhancements/randomizer' if game=='oot' else 'mm/2s2h/Rando'
    covered=set(re.findall(r'SOUL_CHECK\((RC_\w+),',(ROOT/folder/'SoulShuffleChecks.inc').read_text()))
    for pattern in patterns:
        missing=set(re.findall(pattern,native))-covered
        assert not missing, f'unclassified native soul-backed checks: {game} {sorted(missing)}'
print('PASS explicit native dependency manifests and native category coverage')
oot_src=(ROOT/'soh/soh/Enhancements/randomizer/logic.cpp').read_text()
access=(ROOT/'soh/soh/Enhancements/randomizer/location_access.h').read_text()
mm_logic=(ROOT/'mm/2s2h/Rando/Logic/Logic.h').read_text()
assert 'CanAccessSoulCheck(location)' in access, 'actual OoT LocationAccess bypasses soul dependency'
assert 'SoulCheckRequirement(check)' in mm_logic, 'actual MM stored CHECK lambda bypasses soul dependency'

def body(source, signature):
    start=source.index(signature);brace=source.index('{',start);end=brace+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]

with tempfile.TemporaryDirectory(prefix='soul-dependency-') as temp:
    temp=Path(temp)
    for game,folder in [('oot','soh/soh/Enhancements/randomizer'),('mm','mm/2s2h/Rando')]:
        manifest=f'{folder}/SoulShuffleChecks.inc'
        if game=='oot':
            text='#include "soh/soh/Enhancements/randomizer/randomizerEnums.h"\n'
            text+='''
struct Logic { bool enabled=false; RandomizerGet owned=RG_NONE;
 bool HasItem(RandomizerGet i) { return !enabled || owned==i; }
 bool CanAccessSoulCheck(RandomizerCheck check);
} state;
auto*logic=&state;
struct Option { bool noLogic=false; bool Is(int) { return !noLogic; } };
namespace Rando { struct Context { Option option; static Context*GetInstance(){static Context c;return &c;} Option GetOption(int){return option;} }; }
'''+body(oot_src,'bool Logic::CanAccessSoulCheck(')+'\n'
            access_body=body(access[access.index('class LocationAccess {'):], 'bool GetConditionsMet() const')
            text+='struct Check {RandomizerCheck location;bool(*condition_function)();'+access_body+'};\n'
            text+='struct Pair {RandomizerCheck check;RandomizerGet soul;};\n'
            text+='#define SOUL_CHECK(c,s) {c,s},\nPair pairs[]={\n#include "'+manifest+'"\n};\n#undef SOUL_CHECK\n'
            test='''
            // Later MQ cow-labelled wonders are independent EnWonderItem colliders.
            state.enabled=true;state.owned=RG_NONE;
            for(auto c:{RC_JABU_JABUS_BELLY_MQ_HOLES_COW,
                        RC_JABU_JABUS_BELLY_MQ_WONDER_ELEVATOR_COW,
                        RC_JABU_JABUS_BELLY_MQ_WONDER_BASEMENT_LEFT_COW_1,
                        RC_JABU_JABUS_BELLY_MQ_WONDER_BEFORE_BOSS_LEFT_COW})
                assert(state.CanAccessSoulCheck(c));
            for(auto &p:pairs){
                Check c{p.check,[]{return true;}};
                state.enabled=false;state.owned=RG_NONE;assert(c.GetConditionsMet());
                state.enabled=true;assert(!c.GetConditionsMet());
                state.owned=p.soul;assert(c.GetConditionsMet());
                c.condition_function=[]{return false;};assert(!c.GetConditionsMet());
                Rando::Context::GetInstance()->option.noLogic=true;assert(c.GetConditionsMet());
                Rando::Context::GetInstance()->option.noLogic=false;
            }'''
        else:
            text='#include "mm/2s2h/Rando/SoulShuffle.h"\n'
            text+='''unsigned options[RO_MAX]{};unsigned short flags[(RANDO_INF_MAX+15)/16]{};
#define RANDO_SAVE_OPTIONS options
bool Flags_GetRandoInf(RandoInf f){return flags[f/16] & (1u<<(f%16));}
namespace Rando::Logic {
'''+body(mm_logic,'inline bool SoulCheckRequirement(')+'\n'
            text+='struct Pair {RandoCheckId check;RandoItemId soul;};\n'
            text+='#define SOUL_CHECK(c,s) {c,s},\nPair pairs[]={\n#include "'+manifest+'"\n};\n#undef SOUL_CHECK\n'
            start=mm_logic.index('#define CHECK(');end=mm_logic.index('// STAY macro',start)
            text+=mm_logic[start:end]+'\nstruct LogicString {LogicString(const char*){}};\n'
            text+='}\nusing namespace Rando::Logic;\n'
            test='''for(auto&p:pairs){
                auto*s=Rando::SoulShuffle::Find(p.soul);assert(s);
                std::fill(std::begin(options),std::end(options),0);
                std::fill(std::begin(flags),std::end(flags),0);
                assert(SoulCheckRequirement(p.check));
                options[s->option]=1;assert(!SoulCheckRequirement(p.check));
                flags[s->flag/16]|=1u<<(s->flag%16);assert(SoulCheckRequirement(p.check));
            }
            // Execute the exact native macro that every original/combined caller stores.
            std::map<RandoCheckId,std::pair<std::function<bool()>,LogicString>> checks={
                CHECK(RC_ROMANI_RANCH_GROG,true), CHECK(RC_DOGGY_RACETRACK_PIECE_OF_HEART,false)};
            std::fill(std::begin(flags),std::end(flags),0);options[RO_SHUFFLE_ANIMAL_SOULS]=1;
            assert(!checks.at(RC_ROMANI_RANCH_GROG).first());
            auto*s=Rando::SoulShuffle::Find(RI_SOUL_ANIMAL_CUCCO);flags[s->flag/16]|=1u<<(s->flag%16);
            assert(checks.at(RC_ROMANI_RANCH_GROG).first());
            options[RO_SHUFFLE_ANIMAL_SOULS]=0;assert(!checks.at(RC_DOGGY_RACETRACK_PIECE_OF_HEART).first());
            '''
        cpp=temp/(game+'.cpp');cpp.write_text('#include <cassert>\n#include <algorithm>\n#include <map>\n#include <functional>\n'+text+'\nint main(){'+test+'}\n')
        for mode in ('original','combo'):
            exe=temp/(game+mode)
            subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),'-I'+str(ROOT/folder),*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(cpp),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)
print('PASS actual check-predicate boundaries: all explicit dependencies, missing/present/off and unrelated conditions')
