#!/usr/bin/env python3
"""Run focused trade consumption, native handoff, persistence, and sync regressions."""
import argparse
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
env = os.environ.copy()
json_include = env.get('NLOHMANN_JSON_INCLUDE', '/usr/include')
env['CPATH'] = os.pathsep.join(filter(None, [json_include, env.get('CPATH', '')]))
env['PYTHONDONTWRITEBYTECODE'] = '1'
for name in ['core', 'oot', 'mm', 'sync', 'persistence']:
    command = [sys.executable, '-B', str(ROOT/'tests/trade_consumption'/f'{name}_run_tests.py')]
    if args.sanitize:
        command.append('--sanitize')
    subprocess.run(command, cwd=ROOT, env=env, check=True)
print('PASS trade consumption: availability, native hand-ins, migrations, save codecs, and cross-game sync')
