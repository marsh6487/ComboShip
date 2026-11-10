#!/usr/bin/env python3
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
deps = Path(os.environ.get('COMBO_TEST_DEPS', ROOT.parent / 'build-deps'))
with tempfile.TemporaryDirectory(prefix='solo-mm-codec-') as temp:
    binary = Path(temp) / 'codec-test'
    flags = ['-std=c++20', '-O1', '-DCOMBO_BUILD', '-DF3DEX_GBI_2', '-DCONTROLLERBUTTONS_T=uint32_t',
             '-DGBI_FLOAT_MTX', '-DGBI_S32_VTX', '-DFMT_HEADER_ONLY', '-I' + str(ROOT),
             '-isystem', str(deps / 'usr/include')]
    for path in ('mm', 'mm/2s2h', 'mm/include', 'mm/include/PR', 'mm/src', 'mm/assets', 'libultraship/include', 'combo'):
        flags += ['-isystem', str(ROOT / path)]
    subprocess.run([os.environ.get('CXX', 'c++'), *flags, str(Path(__file__).with_suffix('.cpp')),
                    str(ROOT / 'mm/2s2h/Rando/SoloEntrances.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
