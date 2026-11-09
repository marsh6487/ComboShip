#!/usr/bin/env python3
"""Actual MM first-drop source selection, native naming and combined clue composition."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tests/ui_visibility'))
from run_control_input_tests import compiler_flags, function


def main():
    items = (ROOT / 'mm/2s2h/Rando/StaticData/Items.cpp').read_text()
    saria = (ROOT / 'mm/2s2h/Rando/MiscBehavior/SariasSongHint.cpp').read_text()
    runtime = (ROOT / 'mm/2s2h/Rando/Rando.cpp').read_text()
    checks = (ROOT / 'mm/2s2h/Rando/StaticData/Checks.cpp').read_text()
    cpp = (ROOT / 'tests/shuffle_parity/hints/native_test.cpp').read_text().split('int main() {')[0]
    # Keep native engine boundaries from the established fixture, replace its
    # scene rendering boundary with the complete production location function.
    old = function(cpp, 'Rando::GetHintLocationText')
    cpp = cpp.replace(old, '''namespace Rando::Spoiler { std::string GetOotAreaForItem(const std::string&) { return {}; } }
std::string Rando::GetItemLocationHintName(RandoItemId, bool) { return "in an Unknown Location"; }
''' + function(runtime, 'Rando::GetHintLocationText'))
    cpp = '#include "2s2h/CustomMessage/CustomMessage.h"\n#include <sstream>\n#include "rando/CrossHints.h"\n' + cpp
    cpp += '''\nstd::string convertEnumToReadableName(const std::string&, const std::string& prefix = "RC_");
'''
    cpp += function((ROOT / 'mm/2s2h/ShipUtils.cpp').read_text(), 'convertEnumToReadableName')
    cpp += function((ROOT / 'mm/2s2h/CustomMessage/CustomMessage.cpp').read_text(), 'CustomMessage::Replace')
    cpp += function((ROOT / 'mm/2s2h/CustomMessage/CustomMessage.cpp').read_text(), 'CustomMessage::ReplaceColorChars')
    start = saria.index('                entry.msg = "%g... Link?')
    end = saria.index('                Rando::RemoveItem(RI_SONG_SARIA);',start)
    cpp += 'std::string NativeSariaText(const Rando::HintTarget& target){ CustomMessage::Entry entry;\n' + saria[start:end] + '\nreturn entry.msg;}\n'
    gossip = (ROOT / 'mm/2s2h/Rando/ActorBehavior/EnGs.cpp').read_text()
    start = gossip.index('                entry.msg = "They say %g{{item}}')
    end = gossip.index('                ComboReportStoneHint(',start)
    cpp += 'std::string NativeGossipText(RandoCheckId randoCheckId,bool showExact){CustomMessage::Entry entry;auto&saveCheck=RANDO_SAVE_CHECKS[randoCheckId];\n' + gossip[start:end] + '\nreturn entry.msg;}\n'

    cpp += '''\nextern "C" s16 Play_GetOriginalSceneId(s16 scene) { return scene; }
const char* Ship_GetSceneName(s16) { return "Fixture scene boundary"; }
namespace Rando::StaticData {
std::array<std::string, RC_MAX> CheckNames;
static std::set<SceneId> scenesToCheckParent;
static std::unordered_map<RandoCheckId,std::string> checkLocationHints;
'''
    cpp += function(checks, 'PopulateCheckNames') + function(checks, 'GetLocationNameForHint') + '}\n'
    cpp += (ROOT / 'tests/shuffle_parity/hints/mm_first_drop_cases.inc').read_text()
    with tempfile.TemporaryDirectory(prefix='mm-first-drop-hints-') as temporary:
        build = Path(temporary)
        start = items.index('std::map<RandoItemId, RandoStaticItem> Items = {')
        end = items.index('\n};', start) + 3
        (build / 'items.inc').write_text(items[start:end])
        rows = [line.strip() for line in checks.splitlines()
                if re.match(r'\s*RC\(RC_ENEMY_DROP_.*RCTYPE_ENEMY_DROP', line)]
        assert len(rows) == 56
        (build / 'first_drop_checks.inc').write_text('\n'.join(rows))
        (build / 'convert.inc').write_text(function((ROOT / 'mm/2s2h/Rando/ConvertItem.cpp').read_text(), 'Rando::ConvertItem'))
        rpg = (ROOT / 'mm/mods/combo_rpg.cpp').read_text()
        (build / 'native_magic.inc').write_text(function(rpg,'ComboRpg_IsEnabled') + function(rpg,'ComboRpg_NativeMagicTier'))
        bodies = [function(saria,name) for name in ('IsExcludedFromSariaPriorityItemCandidates',
                  'Rando::GetSariaPriorityItemCandidates','Rando::GetSariaPriorityItemsFromSpoiler',
                  'Rando::SetSariaPriorityItemsInSpoiler','Rando::GetSariaPriorityItemsFromSave',
                  'Rando::SetSariaPriorityItemsInSave')]
        bodies += ['namespace Rando::StaticData {\n' + function(items,'GetItemIdFromName') + function(items,'GetItemName') + '}\n']
        (build / 'saria.inc').write_text('\n'.join(bodies))
        menu = (ROOT / 'mm/2s2h/BenGui/BenMenu.cpp').read_text()
        (build / 'sensor_menu.inc').write_text(function(menu,'SensorItemChoices') + function(menu,'SensorDesireName'))
        (build / 'placement.inc').write_text(function(runtime,'Rando::FindMultiItemPlacement'))
        (build / 'selection.inc').write_text(function(saria,'GetProgressiveCheckInLogic') +
            function(runtime,'SensorFindOutstandingCheck') + function(runtime,'Rando::GetCrossWorldHintTargets') +
            function(saria,'Rando::GetSariaHintTarget') + function(runtime,'Rando::GetDesiredItemHintTarget') +
            function(runtime,'SensorGetDesire') + function(runtime,'SensorRune_PrepareHint'))
        (build / 'audit.cpp').write_text(cpp)
        for combo in (False, True):
            flags = [f for f in compiler_flags('mm',combo) if not f.startswith('-Wno-') and not f.startswith('-std=') and f != '-Werror=implicit-function-declaration']
            binary = build / ('combo' if combo else 'original')
            subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(build),'-include',
                            str(ROOT/'tests/dungeon_rewards/headless.h'),*flags,str(build/'audit.cpp'),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
            print('PASS actual MM first-drop hint paths', 'Combo' if combo else 'original',flush=True)
if __name__ == '__main__': main()
