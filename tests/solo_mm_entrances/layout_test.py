#!/usr/bin/env python3
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
deps = Path(os.environ.get('COMBO_TEST_DEPS', ROOT.parent / 'build-deps'))
with tempfile.TemporaryDirectory(prefix='solo-mm-layout-') as temp:
    binary = Path(temp) / 'layout-test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-I' + str(ROOT),
                    '-I' + str(deps / 'usr/include'), str(Path(__file__).with_suffix('.cpp')), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
