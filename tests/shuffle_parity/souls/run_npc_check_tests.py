#!/usr/bin/env python3
"""Execute native MM NPC CHECK lambdas with native soul/clock/time adapters.

Inventory capabilities and a single reachable timeslice form the boundary
fixture. Native time macros, saved soul adapter and source CHECK bodies execute.
"""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
logic = (ROOT / 'mm/2s2h/Rando/Logic/Logic.h').read_text()
central = (ROOT / 'mm/2s2h/Rando/Logic/Regions/Central.cpp').read_text()


def body(source, signature):
    start = source.index(signature)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def check(name):
    start = central.index('CHECK(' + name + ',')
    end = central.index('(', start) + 1
    depth = 1
    while depth:
        depth += (central[end] == '(') - (central[end] == ')')
        end += 1
    return central[start:end]


pairs = [
    ('RC_CLOCK_TOWN_WEST_BANK_ADULTS_WALLET', 'RI_SOUL_NPC_BANKER'),
    ('RC_CLOCK_TOWN_WEST_BANK_INTEREST', 'RI_SOUL_NPC_BANKER'),
    ('RC_CLOCK_TOWN_WEST_BANK_PIECE_OF_HEART', 'RI_SOUL_NPC_BANKER'),
    ('RC_CLOCK_TOWN_LAUNDRY_GURU_GURU', 'RI_SOUL_NPC_GURU_GURU'),
    ('RC_STOCK_POT_INN_GRANDMA_LONG_STORY', 'RI_SOUL_NPC_OLD_HAG'),
    ('RC_STOCK_POT_INN_GRANDMA_SHORT_STORY', 'RI_SOUL_NPC_OLD_HAG'),
    ('RC_SWORDSMAN_SCHOOL_PIECE_OF_HEART', 'RI_SOUL_NPC_CARPET_MAN'),
]
source = '''#include "mm/2s2h/Rando/SoulShuffle.h"
#include "soh/tests/test_require.h"
#include <algorithm>
#include <map>
#include <functional>
#include <string>
unsigned options[RO_MAX]{};unsigned short flags[(RANDO_INF_MAX+15)/16]{};
#define RANDO_SAVE_OPTIONS options
bool Flags_GetRandoInf(RandoInf f){return flags[f/16] & (1u<<(f%16));}
void Own(RandoItemId i){auto*s=Rando::SoulShuffle::Find(i);REQUIRE(s);flags[s->flag/16]|=1u<<(s->flag%16);}
namespace Rando::Logic {
'''
source += body(logic, 'inline bool SoulCheckRequirement(') + '\n'
source += '// Child Wallet shuffle is off in this NPC/time boundary fixture.\n'
source += 'inline bool WalletCheckRequirement(RandoCheckId) {return true;}\n'
source += body(logic, 'enum TimeSlice') + ';\n'
source += '''int now=TIME_DAY1_AM_08_00,wallet=1;bool sword=true,allNight=true;
unsigned clocks=63;
bool RawBefore(TimeSlice s){return now<s;}
bool RawAfter(TimeSlice s){return now>=s;}
bool RawBetween(TimeSlice a,TimeSlice b){return now>=a && now<b;}
bool SettingClocks(){return true;}
bool OwnsHalfDayForMode(int i){return clocks&(1u<<i);}
enum { UPG_WALLET, ITEM_MASK_ALL_NIGHT };
#define CUR_UPG_VALUE(x) ((x)==UPG_WALLET ? wallet : 0)
#define HAS_ITEM(x) ((x)==ITEM_MASK_ALL_NIGHT && allNight)
#define CAN_USE_HUMAN_SWORD sword
'''
for line in logic.splitlines():
    if line.startswith(('#define CLOCK_DAY', '#define CLOCK_NIGHT', '#define IS_DAY1(',
                        '#define IS_DAY2(', '#define IS_DAY3(', '#define IS_NIGHT1(',
                        '#define IS_NIGHT2(', '#define IS_NIGHT3(', '#define BEFORE(', '#define BETWEEN(')):
        source += line + '\n'
source += body(logic, 'inline bool ClockFilter(') + '\n'
a = logic.index('#define CHECK(')
b = logic.index('// STAY macro', a)
source += logic[a:b] + '\nstruct LogicString{LogicString(const char*){}};\n'
source += 'std::map<RandoCheckId,std::pair<std::function<bool()>,LogicString>> checks={\n'
source += ',\n'.join(check(name) for name, _ in pairs) + '\n};\n}\nusing namespace Rando::Logic;\nint main(){\n'
for name, soul in pairs:
    time = 'TIME_NIGHT1_PM_08_00' if 'GURU_GURU' in name else 'TIME_DAY1_AM_08_00'
    source += f'''now={time};wallet=1;sword=allNight=true;clocks=63;
    std::fill(std::begin(flags),std::end(flags),0);options[RO_SHUFFLE_NPC_SOULS]=1;
    Own(RI_SOUL_NPC_CITIZEN);REQUIRE(!checks.at({name}).first());
    Own({soul});REQUIRE(checks.at({name}).first());
    std::fill(std::begin(flags),std::end(flags),0);options[RO_SHUFFLE_NPC_SOULS]=0;
    REQUIRE(checks.at({name}).first());
'''
source += '''options[RO_SHUFFLE_NPC_SOULS]=1;
Own(RI_SOUL_NPC_BANKER);Own(RI_SOUL_NPC_GURU_GURU);Own(RI_SOUL_NPC_OLD_HAG);Own(RI_SOUL_NPC_CARPET_MAN);
wallet=0;REQUIRE(checks.at(RC_CLOCK_TOWN_WEST_BANK_ADULTS_WALLET).first());
REQUIRE(!checks.at(RC_CLOCK_TOWN_WEST_BANK_INTEREST).first());
REQUIRE(!checks.at(RC_CLOCK_TOWN_WEST_BANK_PIECE_OF_HEART).first());
now=TIME_DAY1_AM_08_00;REQUIRE(!checks.at(RC_CLOCK_TOWN_LAUNDRY_GURU_GURU).first());
now=TIME_NIGHT1_PM_08_00;REQUIRE(checks.at(RC_CLOCK_TOWN_LAUNDRY_GURU_GURU).first());
clocks&=~2u;REQUIRE(!checks.at(RC_CLOCK_TOWN_LAUNDRY_GURU_GURU).first());clocks=63;
now=TIME_NIGHT3_PM_08_00;REQUIRE(!checks.at(RC_CLOCK_TOWN_LAUNDRY_GURU_GURU).first());
now=TIME_DAY1_AM_08_00;allNight=false;
REQUIRE(!checks.at(RC_STOCK_POT_INN_GRANDMA_LONG_STORY).first());
REQUIRE(!checks.at(RC_STOCK_POT_INN_GRANDMA_SHORT_STORY).first());allNight=true;
clocks=1;REQUIRE(!checks.at(RC_STOCK_POT_INN_GRANDMA_LONG_STORY).first());
REQUIRE(!checks.at(RC_STOCK_POT_INN_GRANDMA_SHORT_STORY).first());clocks=63;
now=TIME_DAY3_AM_08_00;REQUIRE(!checks.at(RC_STOCK_POT_INN_GRANDMA_LONG_STORY).first());
REQUIRE(!checks.at(RC_STOCK_POT_INN_GRANDMA_SHORT_STORY).first());
now=TIME_DAY1_AM_08_00;sword=false;REQUIRE(!checks.at(RC_SWORDSMAN_SCHOOL_PIECE_OF_HEART).first());sword=true;
now=TIME_NIGHT3_PM_11_00;REQUIRE(!checks.at(RC_SWORDSMAN_SCHOOL_PIECE_OF_HEART).first());
now=TIME_NIGHT3_PM_10_00;REQUIRE(checks.at(RC_SWORDSMAN_SCHOOL_PIECE_OF_HEART).first());
std::fill(std::begin(flags),std::end(flags),0);
for(auto c:{RC_SWORDSMAN_SCHOOL_POT_01,RC_SWORDSMAN_SCHOOL_POT_05,
            RC_STOCK_POT_INN_STAFF_ROOM_CHEST,RC_STOCK_POT_INN_GUEST_ROOM_CHEST})
    REQUIRE(SoulCheckRequirement(c));
puts("PASS native MM scheduled NPC checks: souls, original capability conditions and clock/time boundaries");
}
'''
with tempfile.TemporaryDirectory(prefix='soul-npc-checks-') as temp:
    temp = Path(temp)
    cpp = temp / 'checks.cpp'
    cpp.write_text(source)
    for mode in ('original', 'combo'):
        exe = temp / mode
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-I' + str(ROOT),
                        *(['-DCOMBO_BUILD'] if mode == 'combo' else []), str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
