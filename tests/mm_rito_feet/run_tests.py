#!/usr/bin/env python3
"""Compile the actual Rito query, C post-native seam and native matrices/trig.

Fixture provenance pins the original MM67-word clips and exact Rito triangle
matrix owners. No ROM archive, scratch import or downloaded dependency is needed.
"""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
def function(source, name):
    pattern = re.compile(r'^(?:extern "C" )?(?:static )?(?:u8|f32|void|s32|int)\s+' + re.escape(name) + r'\([^;{}]*\)\s*\{', re.M)
    match = pattern.search(source)
    assert match, 'missing production definition: ' + name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]
from run_mm_nei_tests import flags


def initializer(value):
    if isinstance(value, list):
        return '{' + ','.join(initializer(x) for x in value) + '}'
    return str(value)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--mode', choices=('original', 'combo'), default='original')
    parser.add_argument('--output-dir', type=Path)
    args = parser.parse_args()
    fixture = json.loads((ROOT / 'tests/mm_rito_feet/native_gait_fixture.json').read_text())
    clips = ('normal_wait', 'normal_walk', 'normal_run', 'normal_wait_free', 'normal_walk_free',
             'fighter_walk_long', 'normal_run_free', 'fighter_run', 'fighter_run_long')
    resources = {}
    for resource, expected in fixture['resource_sha256'].items():
        path = ROOT / resource
        blob = path.read_bytes() if path.is_file() else subprocess.check_output(
            ['git', 'show', 'HEAD:' + resource], cwd=ROOT)
        resources[resource.removeprefix('mm/assets/custom/')] = ET.fromstring(blob)
        assert hashlib.sha256(blob).hexdigest() == expected, (
            'Rito foot source resource changed: ' + resource +
            '; regenerate verified rig/support data and provenance, do not update hashes alone')
    base = 'objects/forms/rito/object_link_boy/'
    skeleton = resources[base + 'gLinkAdultSkel']
    limbs = [resources[node.get('Path')] for node in skeleton]
    offsets = [[int(node.get('LegTrans' + axis)) for axis in 'XYZ'] for node in limbs]
    parents, order = [-1] * 21, []
    def walk(index, parent):
        assert index not in order, 'cyclic Rito limb topology'
        parents[index] = parent
        order.append(index)
        child, sibling = int(limbs[index].get('ChildIndex')), int(limbs[index].get('SiblingIndex'))
        if child != 255: walk(child, index)
        if sibling != 255: walk(sibling, parent)
    walk(0, -1)
    assert offsets == fixture['offsets'] and parents == fixture['parents']
    slots = [index for index in order if limbs[index].get('DisplayList1') not in (None, '', 'gEmptyDL')]
    for owner in (5, 8):
        cache, triangles, bone = {}, [], owner
        for node in resources[limbs[owner].get('DisplayList1')]:
            if node.tag == 'Matrix':
                address = int(node.get('Path').removeprefix('>'), 16)
                assert address >> 24 == 13
                bone = slots[(address & 0xFFFFFF) // 64]
            elif node.tag == 'LoadVertices':
                vertices = list(resources[node.get('Path')])
                start, offset, count = (int(node.get(key)) for key in ('VertexBufferIndex', 'VertexOffset', 'Count'))
                for index, vertex in enumerate(vertices[offset:offset + count]):
                    cache[start + index] = [bone] + [int(vertex.get(axis)) for axis in 'XYZ']
            elif node.tag in ('Triangles2', 'Triangle1'):
                triangles.extend(cache[int(value)] for key, value in node.attrib.items() if key.startswith('V'))
        regenerated = [list(point) for point in sorted(set(tuple(point) for point in triangles))]
        assert regenerated == fixture['feet'][str(owner)], 'actual native flex-owner geometry differs; regenerate support'
    frames = []
    frame_gait, frame_animations = [], []
    for clip in clips:
        record = fixture['clips'][clip]
        packed = b''.join(struct.pack('<67h', *frame) for frame in record['frames'])
        assert hashlib.sha256(packed).hexdigest() == record['packed_frames_sha256']
        frames.extend([[frame[i:i + 3] for i in range(0, 66, 3)] for frame in record['frames']])
        frame_gait.extend([int(clip not in ('normal_wait', 'normal_wait_free'))] * len(record['frames']))
        frame_animations.extend(['gPlayerAnim_link_' + clip] * len(record['frames']))

    player_source = (ROOT / 'mm/src/code/z_player_lib.c').read_text()
    leg = function(player_source, 'Player_AdjustSingleLeg')
    assert leg.count('RitoFeet_ApplyPostNative(') == 1 and player_source.count('RitoFeet_ApplyPostNative(') == 1, \
        'post-native solve must occur exactly once inside Player_AdjustSingleLeg and nowhere else'
    assert leg.count('BgCheck_EntityRaycastFloor3(') == 1 and leg.count('EffectSsGFire_Spawn(') == 1
    native_leg = leg.replace('void Player_AdjustSingleLeg(', 'void RitoFeetTest_NativeLeg(', 1)
    # The reference executes the same unchanged native ray/angle/effect path;
    # remove only the new gated post-native call, without duplicating its math.
    native_leg, removed = re.subn(r'\bRitoFeet_ApplyPostNative\s*\([^;]*\);', '', native_leg, count=1)
    assert removed == 1, 'remove exactly the new call, including any native-format line wrapping'
    constants = player_source[player_source.index('Vec3f D_801C08C0'):
                              player_source.index('/**\n * Adjusts player', player_source.index('Vec3f D_801C08C0'))]
    form = (ROOT / 'mm/mods/forms/form_rito.cpp').read_text()
    shared = (ROOT / 'mm/mods/forms/custom_forms.cpp').read_text()
    query = '\n'.join(function(shared, name) for name in ('Forms_IsAdultAge', 'Forms_OnGround', 'Forms_CanAct',
                                                           'CustomForms_RootScale'))
    query += '\n' + function(form, 'MmRitoInputBlocked')
    query += '\n' + function(form, 'RitoForm_CanAdjustFeet')
    query += '\n' + function(re.sub(r'//[^\n]*', '', (ROOT / 'mm/2s2h/BenPort.cpp').read_text()), 'ResourceMgr_OTRSigCheck')

    declarations = 'static const Vec3s kOffsets[21] = ' + initializer(fixture['offsets']) + ';\n'
    declarations += 'static const int kParents[21] = ' + initializer(fixture['parents']) + ';\n'
    declarations += 'static Vec3s kFrames[][22] = ' + initializer(frames) + ';\n'
    declarations += 'static const bool kIsGait[] = ' + initializer(frame_gait) + ';\n'
    declarations += 'static const char* kAnimations[] = {' + ','.join(frame_animations) + '};\n'
    declarations += 'static const unsigned kGaitFeet = ' + str(sum(frame_gait) * 2) + ';\n'
    declarations += 'struct RitoFeetTestVertex { int segment; Vec3f point; };\n'
    triangles = []
    for owner in ('5', '8'):
        triangles.append([[2 if v[0] == int(owner) else 1, v[1:]] for v in fixture['feet'][owner]])
    declarations += 'static const RitoFeetTestVertex kFootTriangles[2][110] = ' + initializer(triangles) + ';\n'

    with tempfile.TemporaryDirectory(prefix='mm-rito-feet-') as temporary:
        build = args.output_dir.resolve() if args.output_dir else Path(temporary)
        build.mkdir(parents=True, exist_ok=True)
        (build / 'native_query.inc').write_text(query)
        (build / 'fixture.inc').write_text(declarations)
        (build / 'native_leg.c').write_text('#include "global.h"\n#include "mods/forms/custom_forms.h"\n'
                                           '#include "mods/forms/rito_foot_contact.h"\n' + constants + leg + '\n' + native_leg)
        common = [*flags(), '-O0' if args.sanitize else '-O2', '-g', '-ffunction-sections', '-fdata-sections',
                  '-I' + str(build)]
        if args.mode == 'combo':
            common.append('-DCOMBO_BUILD')
        if args.sanitize:
            common.append('-fsanitize=address,undefined')
        objects = []
        sources = [build / 'native_leg.c', ROOT / 'mm/src/code/sys_matrix.c', ROOT / 'mm/src/code/z_skin_matrix.c',
                   ROOT / 'mm/src/code/sys_math_atan.c', ROOT / 'mm/src/boot/O2/math64.c',
                   ROOT / 'mm/src/libultra/gu/sins.c', ROOT / 'mm/src/libultra/gu/coss.c']
        for source in sources:
            output = build / (source.stem + '.o')
            subprocess.run([os.environ.get('CC', 'gcc'), '-std=gnu17', '-Wno-incompatible-pointer-types',
                            '-Wno-int-conversion', *common, '-c', str(source), '-o', str(output)], check=True)
            objects.append(str(output))
        binary = build / 'native_feet_test'
        subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++20', *common,
                        str(ROOT / 'tests/mm_rito_feet/native_test.cpp'), *objects,
                        '-Wl,--gc-sections', '-lm', '-o', str(binary)], check=True)
        environment = dict(os.environ)
        if args.sanitize:
            environment['ASAN_OPTIONS'] = 'detect_leaks=0'
        started = time.monotonic()
        subprocess.run([str(binary)], check=True, env=environment)
        print(f'PASS actual MM Rito foot seam/query, {args.mode}, sanitize={args.sanitize}, '
              f'executable_seconds={time.monotonic() - started:.6f}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
