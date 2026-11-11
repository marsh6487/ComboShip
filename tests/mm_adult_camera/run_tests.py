#!/usr/bin/env python3
"""Compile actual MM camera bodies/math/types; collision and renderer services are controlled.
--parent REV executes the same assertions against that revision's camera (expected red).
This does not establish configured visual/runtime acceptance.
"""
from pathlib import Path
import argparse
import importlib.util
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('item_camera_fixture', ROOT/'tests/adult_mm_item_presentation/run_tests.py')
item = importlib.util.module_from_spec(spec)
spec.loader.exec_module(item)

def source(parent=None):
    # Reuse the native struct/service fixture and exact camera math dependencies.
    # Its unrelated receipt functions are discarded by section GC.
    native = item.source()
    camera = (ROOT/'mm/src/code/z_camera.c').read_text()
    if parent:
        old = subprocess.check_output(['git', 'show', parent+':mm/src/code/z_camera.c'], cwd=ROOT, text=True)
        native = native.replace(item.function(camera, 'Camera_KeepOn4'), item.function(old, 'Camera_KeepOn4'))
    native = native[:native.index('int main(')]
    data = (ROOT/'mm/src/code/z_camera_data.inc').read_text()
    first = item.block_from(data, data.index('CameraModeValue sSetNormal0ModeFirstPersonData['))+';\n'
    native += '\n'+first+item.function(camera, 'Camera_Subject1')+'\n'
    return native+(Path(__file__).parent/'native_test.cpp').read_text()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--parent')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    flags = ['-DCOMBO_BUILD', '-DF3DEX_GBI_2', '-DGBI_FLOAT_MTX', '-DGBI_S32_VTX',
             '-ffunction-sections', '-fdata-sections', '-g']
    flags += ['-I'+str(ROOT/p) for p in ('','mm','mm/include','mm/include/PR','mm/src','mm/assets','mm/2s2h','libultraship/include','combo')]
    if args.sanitize:
        flags += ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie']
    with tempfile.TemporaryDirectory(prefix='mm-adult-camera-') as td:
        temp = Path(td)
        (temp/'test.cpp').write_text(source(args.parent))
        obj = temp/'matrix.o'
        subprocess.run([os.environ.get('CC','gcc'),'-std=gnu2x',*flags,'-w','-Wno-incompatible-pointer-types',
                        '-Wno-int-conversion','-c',str(ROOT/'mm/src/code/sys_matrix.c'),'-o',str(obj)],check=True)
        binary = temp/'test'
        subprocess.run([os.environ.get('CXX','g++'),'-std=c++20',*flags,'-Wno-narrowing','-Wno-write-strings',
                        str(temp/'test.cpp'),str(obj),'-Wl,--gc-sections','-no-pie','-lm','-o',str(binary)],check=True)
        failed = []
        for case in ('ocarina','controls','transitions','firstperson'):
            print('CASE '+case, flush=True)
            result = subprocess.run([str(binary),case],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
            if result.returncode:
                failed.append(case)
        print('FAIL '+', '.join(failed) if failed else 'PASS all native MM adult camera cases', flush=True)
        return bool(failed)

if __name__ == '__main__':
    raise SystemExit(main())
