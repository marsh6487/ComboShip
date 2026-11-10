"""Execute the complete native MM generator with real game/save/rando structures."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import argparse
ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import functions
with tempfile.TemporaryDirectory(prefix='mirror-shuffle-pool-') as td:
    build = Path(td)
    helpers = functions((ROOT / 'mm/2s2h/Rando/Logic/PlacementConstraints.cpp').read_text())
    placement = build / 'helpers.cpp'
    placement.write_text('#include "Rando/Logic/Logic.h"\nnamespace Rando::Logic {\n' + '\n'.join(
        helpers[name] for name in ['DungeonItemToDungeon', 'DungeonItemPlacementOption', 'StaysAtVanillaCheck', 'IsSongLocationItem']) + '\n}\n')
    flags = ['-std=c++20', '-O1', '-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DCONTROLLERBUTTONS_T=uint32_t',
             '-include', str(ROOT / 'tests/dungeon_rewards/headless.h'),
             '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections']
    if args.sanitize:
        flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                  '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
    for path in ['', 'mm/include', 'mm/include/PR', 'mm/2s2h', 'mm', 'mm/src', 'mm/assets', 'libultraship/include', 'combo']:
        flags += ['-isystem', str(ROOT / path)]
    binary = build / 'pool'
    result = subprocess.run([os.environ.get('CXX', 'c++'), *flags, str(ROOT / 'tests/magic_mirror_shuffle/pool_test.cpp'),
        str(ROOT / 'mm/2s2h/Rando/Logic/GeneratePools.cpp'), str(ROOT / 'mm/2s2h/Rando/SongEvents.cpp'),
        str(placement), '-o', str(binary)], capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    subprocess.run([str(binary)], check=True)
