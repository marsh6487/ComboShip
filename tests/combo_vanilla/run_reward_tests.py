#!/usr/bin/env python3
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
args = argparse.ArgumentParser()
args.add_argument('--sanitize', action='store_true')
options = args.parse_args()
flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT)]
if options.sanitize:
    flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
              '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
with tempfile.TemporaryDirectory(prefix='vanilla-rewards-') as directory:
    binary = Path(directory) / 'rewards'
    subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), *flags,
                    str(ROOT / 'tests/combo_vanilla/reward_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
