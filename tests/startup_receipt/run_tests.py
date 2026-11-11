#!/usr/bin/env python3
"""Focused native startup grant/receipt regression gate for both games."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
parser.add_argument('--native', action='store_true', help='Also compile both full OoT player variants')
parser.add_argument('--json-include', default=os.environ.get('NLOHMANN_JSON_INCLUDE', '/usr/include'))
args = parser.parse_args()
env = dict(os.environ)
# JSON consumers below receive an explicit -I argument. Prepending a system
# root to CPLUS_INCLUDE_PATH reorders GCC's include_next search for stdlib.h.
if args.sanitize:
    # LeakSanitizer is unsupported by the managed ptrace runtime; address and
    # undefined-behavior checks remain enabled in the state/host/MM fixtures.
    env['ASAN_OPTIONS'] = 'detect_leaks=0'
flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT), '-I', args.json_include]
if args.sanitize:
    flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
with tempfile.TemporaryDirectory(prefix='startup-state-') as temporary:
    binary = Path(temporary) / 'state'
    subprocess.run([*shlex.split(env.get('CXX', 'c++')), *flags,
                    str(ROOT / 'tests/startup_receipt/state_test.cpp'), '-o', str(binary)], check=True, env=env)
    subprocess.run([str(binary)], check=True, env=env)
for suite in ('host', 'oot', 'mm'):
    command = [sys.executable, '-B', str(ROOT / f'tests/startup_receipt/run_{suite}_tests.py')]
    if suite != 'oot':
        command += ['--json-include', args.json_include]
        if args.sanitize:
            command += ['--sanitize']
    elif args.native:
        command += ['--native']
    subprocess.run(command, check=True, cwd=ROOT, env=env)
print('PASS Impa startup receipt integration gate')
