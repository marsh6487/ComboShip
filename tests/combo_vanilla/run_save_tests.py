#!/usr/bin/env python3
"""Compile real neutral Combo Vanilla persistence policy; native adapters follow."""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT)]
try:
    import nlohmann_json
    flags += ['-I', str(nlohmann_json.get_include())]
except ImportError:
    pass  # CI supplies nlohmann-json3-dev like the other native JSON gates.
if '--sanitize' in sys.argv:
    flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
              '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
with tempfile.TemporaryDirectory(prefix='combo-vanilla-save-') as directory:
    exe = Path(directory) / 'save-test'
    subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), *flags,
                    str(ROOT / 'tests/combo_vanilla/save_test.cpp'), '-o', str(exe)], check=True)
    subprocess.run([str(exe), directory], check=True)
