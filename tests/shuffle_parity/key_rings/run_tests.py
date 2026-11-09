#!/usr/bin/env python3
"""Execute native MM catalogues, full pools and placement for key rings."""
import argparse
import os
import re
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tests/shuffle_parity/boss_keys'))
from run_tests import catalog, function

p = argparse.ArgumentParser()
p.add_argument('--json-include', type=Path, default=Path('/usr/include'))
p.add_argument('--sanitize',action='store_true')
args = p.parse_args()
with tempfile.TemporaryDirectory(prefix='mm-native-key-rings-') as td:
    build = Path(td)
    source = (ROOT / 'mm/2s2h/FleetShipCombo/FleetShipCombo.cpp').read_text()
    (build / 'unified_pool.inc').write_text(function(source,'FleetCombo_UnifiedPoolActive'))
    (build / 'checks.inc').write_text(catalog(ROOT / 'mm/2s2h/Rando/StaticData/Checks.cpp', 'RC',
                                            'std::map<RandoCheckId, RandoStaticCheck> Checks'))
    source = (ROOT / 'mm/2s2h/Rando/StartingItems.cpp').read_text()
    (build / 'starting.inc').write_text('\n'.join(function(source,n) for n in (
        'GetComputedStartingItems','GetStartingItemsFromSave','SetStartingItemsInSave')))
    source = (ROOT / 'mm/2s2h/Rando/MiscBehavior/OnCycleSave.cpp').read_text()
    (build / 'cycle.inc').write_text('\n'.join(function(source,n) for n in (
        'IsRenewable','Rando::MiscBehavior::BeforeEndOfCycleSave','Rando::MiscBehavior::AfterEndOfCycleSave')))
    inventory = (ROOT / 'mm/src/code/z_inventory.c').read_text()
    tables = '\n'.join(re.search(r'^(?:u16|u8) '+name+r'\[\] = \{.*?^\};',inventory,re.M|re.S)[0]
                       for name in ('gEquipMasks','gEquipNegMasks','gEquipShifts'))
    actors = (ROOT / 'mm/src/code/z_actor.c').read_text()
    (build / 'cycle_services.c').write_text('#include "global.h"\n#include "2s2h/GameInteractor/GameInteractor.h"\n'+tables+
        '\nvoid GameInteractor_ExecuteOnFlagSet(FlagType type, u32 flag) {}\n'
        'void GameInteractor_ExecuteOnFlagUnset(FlagType type, u32 flag) {}\n'+
        '\n'.join(function(actors,n) for n in ('Flags_SetWeekEventReg','Flags_GetRandoInf','Flags_SetRandoInf','Flags_ClearRandoInf')))
    flags = ['-std=c++20','-DF3DEX_GBI_2','-DGBI_FLOAT_MTX','-DGBI_S32_VTX',
             '-DCONTROLLERBUTTONS_T=uint32_t','-include',str(ROOT / 'tests/shuffle_parity/souls/headless.h'),
             '-I'+str(ROOT),'-I'+str(build),'-ffunction-sections','-fdata-sections','-Wl,--gc-sections']
    for path in ('mm/include','mm/include/PR','mm/2s2h','mm','mm/src','mm/assets','libultraship/include','combo','combo/menu'):
        flags += ['-isystem',str(ROOT / path)]
    if args.json_include.resolve() != Path('/usr/include'):
        flags += ['-isystem',str(args.json_include.resolve())]
    if args.sanitize:
        flags += ['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    for combo in (False,True):
        binary = build / ('combo' if combo else 'original')
        native = build / ('services-combo.o' if combo else 'services-original.o')
        c_flags = [f for f in flags if f not in ('-std=c++20','-include',str(ROOT / 'tests/shuffle_parity/souls/headless.h'))]
        subprocess.run([os.environ.get('CC','cc'),'-std=c11',*c_flags,
                        *(['-DCOMBO_BUILD'] if combo else []),'-c',str(build / 'cycle_services.c'),'-o',str(native)],check=True)
        subprocess.run([os.environ.get('CXX','c++'),*flags,*(['-DCOMBO_BUILD'] if combo else []),
                        str(Path(__file__).with_name('native_test.cpp')),
                        *[str(ROOT / ('mm/2s2h/Rando/'+p)) for p in ('Logic/GeneratePools.cpp',
                            'Logic/PlacementConstraints.cpp','StaticData/Items.cpp','StaticData/Options.cpp')],
                        str(native),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,timeout=90,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
        print('PASS native MM key rings', 'Combo' if combo else 'original',flush=True)
