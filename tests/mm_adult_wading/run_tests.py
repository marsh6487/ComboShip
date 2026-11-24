#!/usr/bin/env python3
"""Exercise 2Ship's swim transitions with native player data and controlled water/floor input."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PLAYER = 'mm/src/overlays/actors/ovl_player_actor/z_player.c'
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from


def production(source):
    table = block_from(source, source.index('PlayerAgeProperties sPlayerAgeProperties[')) + ';'
    bodies = [table]
    if 'Player_GetWaterAgeProperties(' in source:
        bodies.append(block_from(source, source.index('static const PlayerAgeProperties* Player_GetWaterAgeProperties(')))
    for name in ('func_8082DC64', 'func_8083A878', 'func_8083B32C', 'func_8083BB4C', 'func_808475B4'):
        start = re.search(r'^(?:void|s32) ' + name + r'\([^;{}]+\) \{', source, re.M).start()
        bodies.append(block_from(source, start))
    # The age table carries animation pointers unused by water physics.
    return re.sub(r'&gPlayerAnim_\w+', 'NULL', bodies[0]) + '\n' + '\n'.join(bodies[1:])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--parent', help='Exercise the same cases against a given revision')
    args = parser.parse_args()
    source = (ROOT / PLAYER).read_text()
    if args.parent:
        source = subprocess.check_output(['git', 'show', f'{args.parent}:{PLAYER}'], cwd=ROOT, text=True)
    includes = ['-I' + str(ROOT / p)
                for p in ('', 'mm', 'mm/include', 'mm/include/PR', 'mm/src', 'mm/assets', 'mm/2s2h', 'libultraship/include')]
    with tempfile.TemporaryDirectory(prefix='mm-adult-wading-') as td:
        work = Path(td)
        (work / 'production.inc').write_text(production(source))
        binary = work / 'test'
        subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu17', '-DF3DEX_GBI_2', '-I' + td, *includes,
                        str(Path(__file__).with_name('native_test.c')), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
