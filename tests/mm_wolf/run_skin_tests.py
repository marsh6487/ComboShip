#!/usr/bin/env python3
"""Production MM skin and native matrix fixture, using real MM headers."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_nei_tests import flags

def run(command):
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode:
        print(result.stdout + result.stderr)
        raise SystemExit(result.returncode)

with tempfile.TemporaryDirectory(prefix='mm-wolf-skin-') as td:
    options = ['-DMM_WOLF_SKIN_OPTIONS'] if 'preserveRootMotion' in (ROOT/'mm/expansions/ssbb/ssbb_skin.h').read_text() else []
    objects = []
    for i, source in enumerate(['mm/expansions/ssbb/ssbb_skin.c', 'mm/src/code/z_skin_matrix.c']):
        obj = str(Path(td)/f'{i}.o')
        run([os.environ.get('CC','cc'), '-std=gnu11', *flags(), '-include', str(ROOT/'mm/include/variables.h'),
                        '-include', str(ROOT/'mm/include/functions.h'), '-include', str(ROOT/'mm/include/z64malloc.h'),
                        '-include', str(ROOT/'libultraship/include/libultraship/bridge/consolevariablebridge.h'),
                        '-O1', '-g', '-ffunction-sections', '-fdata-sections', '-c', str(ROOT/source), '-o', obj])
        objects.append(obj)
    binary = str(Path(td)/'skin')
    run([os.environ.get('CXX','c++'), '-std=c++20', *flags(), *options,
         str(ROOT/'tests/mm_wolf/skin_runtime_test.cpp'), *objects, '-Wl,--gc-sections', '-o', binary])
    subprocess.run([binary], check=True)
