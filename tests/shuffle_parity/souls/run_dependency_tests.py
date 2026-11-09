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
            text='#include "soh/soh/Enhancements/randomizer/SoulShuffle.h"\n'
            text+='''
struct Logic { std::set<RandomizerSettingKey> options;std::set<RandomizerGet> owned;
 bool HasItem(RandomizerGet i) {auto*s=Rando::SoulShuffle::Find(i);return !s||!options.count(s->option)||owned.count(i);}
 bool CanAccessSoulCheck(RandomizerCheck check);
} state;
auto*logic=&state;
struct Option { bool noLogic=false; bool Is(int) { return !noLogic; } };
namespace Rando { struct Context { Option option; static Context*GetInstance(){static Context c;return &c;} Option GetOption(int){return option;} }; }
'''+body(oot_src,'bool Logic::CanAccessSoulCheck(')+'\n'
            access_body=body(access[access.index('class LocationAccess {'):], 'bool GetConditionsMet() const')
            text+='struct Check {RandomizerCheck location;bool(*condition_function)();'+access_body+'};\n'
            text+='struct Pair {RandomizerCheck check;std::vector<RandomizerGet> souls;};\n'
            text+='#define SOUL_CHECK(c,...) {c,{__VA_ARGS__}},\nPair pairs[]={\n#include "'+manifest+'"\n};\n#undef SOUL_CHECK\n'
            test='''
            state.options={RSK_SHUFFLE_NPC_SOULS};
            for(auto c:{RC_MARKET_SHOOTING_GALLERY_REWARD,RC_KAK_SHOOTING_GALLERY_REWARD,
                        RC_MARKET_BOMBCHU_BOWLING_FIRST_PRIZE,RC_MARKET_BOMBCHU_BOWLING_SECOND_PRIZE})
                assert(!state.CanAccessSoulCheck(c));
            state.owned={RG_SOUL_NPC_SHOOTING_GALLERY_OWNER,RG_SOUL_NPC_BOMBCHU_BOWLING_LADY};
            for(auto c:{RC_MARKET_SHOOTING_GALLERY_REWARD,RC_KAK_SHOOTING_GALLERY_REWARD,
                        RC_MARKET_BOMBCHU_BOWLING_FIRST_PRIZE,RC_MARKET_BOMBCHU_BOWLING_SECOND_PRIZE})
                assert(state.CanAccessSoulCheck(c));
            state.owned.clear();
            for(auto p:std::vector<Pair>{
                {RC_KAK_TRADE_ODD_MUSHROOM,{RG_SOUL_NPC_OLD_HAG}},
                {RC_KAK_GRANNYS_SHOP,{RG_SOUL_NPC_OLD_HAG}},
                {RC_LH_LAB_DIVE,{RG_SOUL_NPC_SCIENTIST}},
                {RC_LH_TRADE_FROG,{RG_SOUL_NPC_SCIENTIST}},
                {RC_ZR_MAGIC_BEAN_SALESMAN,{RG_SOUL_NPC_BEAN_SALESMAN}},
                {RC_WASTELAND_BOMBCHU_SALESMAN,{RG_SOUL_NPC_CARPET_MAN}},
                {RC_SONG_FROM_WINDMILL,{RG_SOUL_NPC_GURU_GURU}}}){
                state.owned={RG_SOUL_NPC_CITIZEN};assert(!state.CanAccessSoulCheck(p.check));
                state.owned.insert(p.souls[0]);assert(state.CanAccessSoulCheck(p.check));
            }
            state.owned.clear();
            for(auto c:{RC_KAK_WINDMILL_FREESTANDING_POH,RC_HW_CARPET_SALESMAN_ARROW_SIGN,RC_WASTELAND_CHEST})
                assert(state.CanAccessSoulCheck(c));
            state.options={RSK_SHUFFLE_ANIMAL_SOULS,RSK_SHUFFLE_NPC_SOULS};
            assert(!state.CanAccessSoulCheck(RC_LW_TRADE_COJIRO));
            state.owned={RG_SOUL_NPC_GROG};assert(state.CanAccessSoulCheck(RC_LW_TRADE_COJIRO));
            state.owned.clear();assert(state.CanAccessSoulCheck(RC_LW_TRADE_ODD_POTION));
            state.owned={RG_SOUL_ANIMAL_DOG};assert(!state.CanAccessSoulCheck(RC_MARKET_LOST_DOG));
            state.owned.insert(RG_SOUL_NPC_DOG_LADY);assert(state.CanAccessSoulCheck(RC_MARKET_LOST_DOG));
            state.owned.erase(RG_SOUL_ANIMAL_DOG);assert(!state.CanAccessSoulCheck(RC_MARKET_LOST_DOG));
            state.owned={RG_SOUL_NPC_CITIZEN};
            for(auto c:{RC_MK_BEGGAR_BUGS,RC_MK_BEGGAR_FISH,RC_MK_BEGGAR_BLUE_FIRE,
                        RC_KAK_BEGGAR_BUGS,RC_KAK_BEGGAR_FISH,RC_KAK_BEGGAR_BLUE_FIRE}){
                assert(!state.CanAccessSoulCheck(c));state.owned.insert(RG_SOUL_NPC_BANKER);
                assert(state.CanAccessSoulCheck(c));state.owned.erase(RG_SOUL_NPC_BANKER);
            }
            state.owned.clear();assert(state.CanAccessSoulCheck(RC_MK_LOST_DOG_HOUSE_CRATE));
            state.options={RSK_SHUFFLE_ANIMAL_SOULS,RSK_SHUFFLE_NPC_SOULS};
            state.owned={RG_SOUL_ANIMAL_CUCCO};
            assert(!state.CanAccessSoulCheck(RC_KAK_ANJU_AS_CHILD));
            state.owned.insert(RG_SOUL_NPC_ANJU);assert(state.CanAccessSoulCheck(RC_KAK_ANJU_AS_CHILD));
            state.owned.erase(RG_SOUL_ANIMAL_CUCCO);assert(!state.CanAccessSoulCheck(RC_KAK_ANJU_AS_CHILD));
            state.options.erase(RSK_SHUFFLE_ANIMAL_SOULS);assert(state.CanAccessSoulCheck(RC_KAK_ANJU_AS_CHILD));
            state.options.insert(RSK_SHUFFLE_ANIMAL_SOULS);
            assert(state.CanAccessSoulCheck(RC_KAK_ANJU_AS_ADULT));
            assert(state.CanAccessSoulCheck(RC_KAK_TRADE_POCKET_CUCCO));
            state.owned.clear();assert(state.CanAccessSoulCheck(RC_KAK_NEAR_IMPAS_HOUSE_POT_1));
            for(auto c:{RC_JABU_JABUS_BELLY_MQ_HOLES_COW,RC_JABU_JABUS_BELLY_MQ_WONDER_ELEVATOR_COW,
                        RC_JABU_JABUS_BELLY_MQ_WONDER_BASEMENT_LEFT_COW_1,RC_JABU_JABUS_BELLY_MQ_WONDER_BEFORE_BOSS_LEFT_COW})
                assert(state.CanAccessSoulCheck(c));
            for(auto &p:pairs){
                Check c{p.check,[]{return true;}};
                state.options.clear();state.owned.clear();assert(c.GetConditionsMet());
                for(auto item:p.souls)state.options.insert(Rando::SoulShuffle::Find(item)->option);
                assert(!c.GetConditionsMet());
                state.owned.insert(p.souls.begin(),p.souls.end());assert(c.GetConditionsMet());
                for(auto item:p.souls){state.owned.erase(item);assert(!c.GetConditionsMet());state.owned.insert(item);}
                c.condition_function=[]{return false;};assert(!c.GetConditionsMet());
                Rando::Context::GetInstance()->option.noLogic=true;assert(c.GetConditionsMet());
                Rando::Context::GetInstance()->option.noLogic=false;
            }'''
        else:
            text='#include "mm/2s2h/Rando/SoulShuffle.h"\n'
            text+='''unsigned options[RO_MAX]{};unsigned short flags[(RANDO_INF_MAX+15)/16]{};
#define RANDO_SAVE_OPTIONS options
bool Flags_GetRandoInf(RandoInf f){return flags[f/16] & (1u<<(f%16));}
void Own(RandoItemId item,bool owned){auto*s=Rando::SoulShuffle::Find(item);assert(s);if(owned)flags[s->flag/16]|=1u<<(s->flag%16);else flags[s->flag/16]&=~(1u<<(s->flag%16));}
namespace Rando::Logic {
'''+body(mm_logic,'inline bool SoulCheckRequirement(')+'\n'
            text+='// Child Wallet shuffle is off in this soul boundary fixture.\ninline bool WalletCheckRequirement(RandoCheckId) {return true;}\n'
            text+='struct Pair {RandoCheckId check;std::vector<RandoItemId> souls;};\n'
            text+='#define SOUL_CHECK(c,...) {c,{__VA_ARGS__}},\nPair pairs[]={\n#include "'+manifest+'"\n};\n#undef SOUL_CHECK\n'
            start=mm_logic.index('#define CHECK(');end=mm_logic.index('// STAY macro',start)
            text+=mm_logic[start:end]+'\nstruct LogicString {LogicString(const char*){}};\n'
            text+='}\nusing namespace Rando::Logic;\n'
            test='''
            std::map<RandoCheckId,std::pair<std::function<bool()>,LogicString>> checks={
                CHECK(RC_ROMANI_RANCH_GROG,true), CHECK(RC_DOGGY_RACETRACK_PIECE_OF_HEART,false)};
            options[RO_SHUFFLE_ANIMAL_SOULS]=options[RO_SHUFFLE_NPC_SOULS]=1;
            for(auto c:{RC_CLOCK_TOWN_EAST_SHOOTING_GALLERY_HIGH_SCORE,RC_CLOCK_TOWN_EAST_SHOOTING_GALLERY_PERFECT_SCORE,
                        RC_SWAMP_SHOOTING_GALLERY_HIGH_SCORE,RC_SWAMP_SHOOTING_GALLERY_PERFECT_SCORE}){
                assert(!SoulCheckRequirement(c));Own(RI_SOUL_NPC_SHOOTING_GALLERY_OWNER,true);
                assert(SoulCheckRequirement(c));Own(RI_SOUL_NPC_SHOOTING_GALLERY_OWNER,false);
            }
            Own(RI_SOUL_ANIMAL_CUCCO,true);assert(!checks.at(RC_ROMANI_RANCH_GROG).first());
            Own(RI_SOUL_NPC_GROG,true);assert(checks.at(RC_ROMANI_RANCH_GROG).first());
            Own(RI_SOUL_ANIMAL_CUCCO,false);assert(!checks.at(RC_ROMANI_RANCH_GROG).first());
            options[RO_SHUFFLE_ANIMAL_SOULS]=0;assert(checks.at(RC_ROMANI_RANCH_GROG).first());
            std::fill(std::begin(flags),std::end(flags),0);options[RO_SHUFFLE_NPC_SOULS]=1;
            for(auto c:{RC_CUCCO_SHACK_LARGE_CRATE_01,RC_CUCCO_SHACK_TREE,RC_DOGGY_RACETRACK_CHEST,RC_DOGGY_RACETRACK_POT_01})
                assert(SoulCheckRequirement(c));
            for(auto&p:pairs){
                std::fill(std::begin(options),std::end(options),0);
                std::fill(std::begin(flags),std::end(flags),0);assert(SoulCheckRequirement(p.check));
                for(auto item:p.souls)options[Rando::SoulShuffle::Find(item)->option]=1;
                assert(!SoulCheckRequirement(p.check));
                for(auto item:p.souls)Own(item,true);assert(SoulCheckRequirement(p.check));
                for(auto item:p.souls){Own(item,false);assert(!SoulCheckRequirement(p.check));Own(item,true);}
            }
            std::fill(std::begin(options),std::end(options),0);
            assert(!checks.at(RC_DOGGY_RACETRACK_PIECE_OF_HEART).first());
            '''
        cpp=temp/(game+'.cpp');cpp.write_text('#include <cassert>\n#include <algorithm>\n#include <set>\n#include <vector>\n#include <map>\n#include <functional>\n'+text+'\nint main(){'+test+'}\n')
        for mode in ('original','combo'):
            exe=temp/(game+mode)
            subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),'-I'+str(ROOT/folder),'-I'+str(ROOT/'soh/include'),*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(cpp),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)
print('PASS actual native check boundaries: every required soul independently missing, owned/off and unrelated conditions')
