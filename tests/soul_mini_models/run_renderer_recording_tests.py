#!/usr/bin/env python3
from pathlib import Path
import subprocess
import os
import tempfile
ROOT = Path(__file__).resolve().parents[2]
def function(source, name):
    start = source.index('void '+name+'(')
    brace = source.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]
with tempfile.TemporaryDirectory(prefix='soul-recorder-') as td:
    build = Path(td)
    for host, path in [('oot','soh/soh/frame_interpolation.cpp'),('mm','mm/2s2h/Enhancements/FrameInterpolation/FrameInterpolation.cpp')]:
        source = (ROOT/path).read_text()
        (build/'native_recording.inc').write_text('\n'.join(function(source, name) for name in ['FrameInterpolation_RecordOpenChild','FrameInterpolation_RecordCloseChild']))
        binary = build/host
        command = ['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-DCOMBO_BUILD', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie', '-I'+str(build), '-I'+str(ROOT/'combo/menu'), str(ROOT/'tests/soul_mini_models/renderer_recording_test.cpp'), '-o', str(binary)]
        if host == 'mm': command.insert(1, '-DCOMBO_SOUL_RECORDER_MM')
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
