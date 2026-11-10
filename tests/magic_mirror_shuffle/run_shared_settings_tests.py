#!/usr/bin/env python3
"""Compile the actual shared Mirror policy; the CVar store is the native service boundary."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='shared-mirror-policy-') as directory:
    source = (ROOT/'combo/gui/ComboWidgetRender.cpp').read_text()
    start = source.index('void RenderWidget(')
    opening = source.index('{', start)
    end, depth = opening + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    (Path(directory)/'native_widget.inc').write_text(source[start:end])
    for combo in (False, True):
        binary = Path(directory) / ('combo' if combo else 'original')
        flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-I'+str(ROOT),
                 '-I'+str(ROOT/'libultraship/include'), '-I'+str(ROOT/'combo/menu'), '-I'+directory]
        if combo:
            flags += ['-DCOMBO_BUILD']
        if args.sanitize:
            flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                      '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
        subprocess.run([os.environ.get('CXX', 'c++'), *flags,
                        str(ROOT/'tests/magic_mirror_shuffle/shared_settings_test.cpp'), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS':'detect_leaks=0'})
