#!/usr/bin/env python3
"""Run the complete native MM pool generator in ComboShip and standalone modes."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--json-include', type=Path, default=Path('/usr/include'))
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='dungeon-reward-pool-') as directory:
        build = Path(directory)
        source = (ROOT / 'mm/2s2h/FleetShipCombo/FleetShipCombo.cpp').read_text()
        start = source.index('int FleetCombo_UnifiedPoolActive(void) {')
        end = source.index('\n}', start) + 2
        (build / 'unified_pool.inc').write_text(source[start:end])
        flags = ['-std=c++20', '-DF3DEX_GBI_2', '-include',
                 str(ROOT / 'tests/dungeon_rewards/headless.h'), '-I' + str(build)]
        for path in ('mm/include', 'mm/include/PR', 'mm/2s2h', 'mm', 'mm/src', 'mm/assets',
                     'libultraship/include'):
            flags += ['-isystem', str(ROOT / path)]
        # Re-adding the compiler's system root with -isystem moves it before
        # libstdc++ and breaks its #include_next <stdlib.h> on Ubuntu CI.
        if args.json_include.resolve() != Path('/usr/include'):
            flags += ['-isystem', str(args.json_include)]
        failures = 0
        for mode in ('combo', 'standalone'):
            binary = build / mode
            subprocess.run([os.environ.get('CXX', 'c++'), *flags,
                            *(['-DCOMBO_BUILD'] if mode == 'combo' else []),
                            str(ROOT / 'tests/dungeon_rewards/pool_test.cpp'),
                            str(ROOT / 'mm/2s2h/Rando/Logic/GeneratePools.cpp'),
                            '-o', str(binary)], check=True)
            failures += subprocess.run([str(binary)], timeout=10).returncode != 0
        raise SystemExit(bool(failures))


if __name__ == '__main__':
    main()
