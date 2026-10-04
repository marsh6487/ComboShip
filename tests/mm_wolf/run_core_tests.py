#!/usr/bin/env python3
"""Run actual Wolf loader/procs on real MM structures, sanitizer and fast-math modes."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_nei_tests import flags

def run(command):
    result=subprocess.run(command,capture_output=True,text=True)
    if result.returncode:
        print(result.stdout+result.stderr)
        raise SystemExit(result.returncode)
    if result.stdout: print(result.stdout,end='')

with tempfile.TemporaryDirectory(prefix='mm-wolf-core-') as td:
    source=ROOT/'mm/mods/transformation_masks/wolf_link_form.cpp'
    if not source.exists():
        # RED bootstrap: the actual accepted donor loader, with only header/option spelling
        # adapted to MM. After the port exists, the fixture includes that complete source.
        donor=(ROOT/'soh/mods/transformation_masks/wolf_link_form.cpp').read_text()
        donor=donor[:donor.index('// Wolf\'s own size knob')]+ '\n} // namespace\n'
        donor=donor.replace('#include "mods/transformation_masks/wolf_link_form.h"','')
        donor=donor.replace('#include <libultraship/libultraship.h>','#include <ship/Context.h>')
        donor=donor.replace('sSkin.neutralizeRootMotion = 0;','sSkin.preserveRootMotion = 1;')
        for old,new in {'COLTYPE_HIT8':'COL_MATERIAL_HIT8','ELEMTYPE_UNK0':'ELEM_MATERIAL_UNK0',
                        'TOUCH_ON':'ATELEM_ON','TOUCH_SFX_NORMAL':'ATELEM_SFX_NORMAL','BUMP_NONE':'ACELEM_NONE'}.items():
            donor=donor.replace(old,new)
        donor=donor.replace('    sSkeleton.sh.skeletonType = SKELANIME_TYPE_FLEX;','')
        source=Path(td)/'donor-loader.cpp'; source.write_text(donor)
    for name,mode in [('sanitized',['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']),('fast-math',['-O2','-ffast-math'])]:
        binary=str(Path(td)/name)
        run([os.environ.get('CXX','c++'),'-std=c++20',*flags(), '-include',str(ROOT/'mm/include/z64malloc.h'),
             '-DWOLF_IMPLEMENTATION="'+str(source)+'"','-ffunction-sections','-fdata-sections',*mode,
             str(ROOT/'tests/mm_wolf/core_runtime_test.cpp'),'-Wl,--gc-sections','-o',binary])
        run([binary,td])
