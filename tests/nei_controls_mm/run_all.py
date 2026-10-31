#!/usr/bin/env python3
"""Run the complete native NEI control regression group."""
import argparse
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
tests = sorted(path for path in HERE.glob('run_*tests.py'))
tests = [HERE / 'run_tests.py'] + [path for path in tests if path.name != 'run_tests.py']
modes = [(), ('--solo',)]
for mode in modes:
    for test in tests:
        command = [sys.executable, '-B', str(test)]
        if args.sanitize:
            command.append('--sanitize')
        command.extend(mode)
        print('RUN ' + test.name + (' solo' if mode else ' combo/native'), flush=True)
        subprocess.run(command, cwd=ROOT, check=True)
print('PASS: complete NEI MM control group', flush=True)
