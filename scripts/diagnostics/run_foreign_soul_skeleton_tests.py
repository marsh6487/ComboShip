#!/usr/bin/env python3
"""Exercise the complete foreign animation header with the actual native limb drawers.

Resource I/O, animation initialization and matrix/GBI primitives are modeled. Native
rigid/flex limb traversal and foreign loading/cache/dispatch/routing are production code.
No TP archive or live scene is emulated. --baseline demonstrates the missing matrices.
"""
import os
import re
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def function(source, name):
    start = re.search(r'^void ' + re.escape(name) + r'\(', source, re.M).start()
    token = source.index(name + '(', start)
    brace = source.index('{', token)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

header = (ROOT / 'combo/menu/ComboForeignAnim.h').read_text()
if '--baseline-aura' in sys.argv:
    header = subprocess.check_output(['git', 'show', '9e3a2a6006c8d77f6e733da1987dd44681566ba7:combo/menu/ComboForeignAnim.h'], cwd=ROOT, text=True)
if '--baseline' in sys.argv:
    header = subprocess.check_output(['git', 'show', 'd1965180:combo/menu/ComboForeignAnim.h'], cwd=ROOT, text=True)
    # Fixture's factory-layout mirror was introduced with the correction.
    header = header.replace('struct CfaSkelEntry {', 'struct CfaLoadedSkeletonHeader { void** segment; uint8_t limbCount, skeletonType; };\nstruct CfaLoadedFlexSkeletonHeader { CfaLoadedSkeletonHeader sh; uint8_t dListCount; };\nstruct CfaSkelEntry {')
with tempfile.TemporaryDirectory(prefix='foreign-soul-') as td:
    build = Path(td)
    (build / 'foreign_anim.h').write_text(header)
    for path in ['ship/Context.h', 'ship/resource/ResourceManager.h', 'ship/resource/ResourceManagerScope.h', 'ship/resource/CrossRMRegistry.h']:
        p = build / path
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text('#pragma once\n')
    for host in ['mm', 'oot']:
        engine = (ROOT / ('mm/src/code/z_skelanime.c' if host == 'mm' else 'soh/src/code/z_skelanime.c')).read_text()
        production = '\n'.join(function(engine, n) for n in ['SkelAnime_DrawLimbOpa', 'SkelAnime_DrawOpa', 'SkelAnime_DrawFlexLimbOpa', 'SkelAnime_DrawFlexOpa'])
        # C's implicit void-pointer conversions need explicit casts in this C++ harness.
        production = production.replace('= Lib_SegmentedToVirtual(skeleton[', '= (StandardLimb*)Lib_SegmentedToVirtual(skeleton[')
        production = production.replace('Mtx* mtx = GRAPH_ALLOC(', 'Mtx* mtx = (Mtx*)GRAPH_ALLOC(')
        production = production.replace('Mtx* mtx = Graph_Alloc(', 'Mtx* mtx = (Mtx*)Graph_Alloc(')
        (build / 'native_draw.inc').write_text(production)
        binary = build / host
        flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', '-Wno-unused-variable', '-Wno-unused-but-set-variable']
        if host == 'mm': flags += ['-DHOST_MM']
        if '--sanitize' in sys.argv: flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
        subprocess.run([os.environ.get('CXX', 'c++'), *flags, '-I'+str(build), '-I'+str(ROOT/'combo/menu'), str(ROOT/'tests/foreign_soul/skeleton_test.cpp'), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
