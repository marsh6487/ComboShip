#!/usr/bin/env python3
"""Compare baked portal resources with the actual MM fallback builder/math."""
from functools import lru_cache
import json
import math
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET

from test_generator import COMBINED, MM, OOT, PREFIX, ROOT, fixture, load_module

BARK = 'scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0'


def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


@lru_cache(maxsize=1)
def native_control():
    passage = (ROOT / 'mm/2s2h/Enhancements/Graphics/ComboLostWoodsPassage.cpp').read_text()
    matrix = (ROOT / 'mm/src/code/sys_matrix.c').read_text()
    skin = (ROOT / 'mm/src/code/z_skin_matrix.c').read_text()
    trig = (ROOT / 'mm/src/code/z_lib.c').read_text()
    gu = (ROOT / 'mm/2s2h/gu_pc.c').read_text()
    sins = (ROOT / 'mm/src/libultra/gu/sins.c').read_text()
    coss = (ROOT / 'mm/src/libultra/gu/coss.c').read_text()
    declarations = passage[passage.index('constexpr int kArchSegments'):passage.index('void Vertex(')]
    source = r'''
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <libultraship/libultra/types.h>
#include <libultraship/libultra/gbi.h>
#include "sys_matrix.h"
#define SHT_MINV (1.0f/32767.0f)
MtxF current{};MtxF* sCurrentMatrix=&current;
void FrameInterpolation_RecordMatrixTranslate(float,float,float,MatrixMode){}
'''
    source += re.sub(r'^#include.*$', '', sins, flags=re.M)
    source += re.sub(r'^#include.*$', '', coss, flags=re.M)
    for contents, signature in ((trig, 'f32 Math_CosS('), (trig, 'f32 Math_SinS('),
                                (skin, 'void SkinMatrix_SetTranslate('), (matrix, 'void Matrix_Translate('),
                                (matrix, 'void Matrix_RotateYS('), (gu, 'void guMtxF2L('),
                                (gu, 'void guMtxL2F(')):
        source += function(contents, signature) + '\n'
    # Archive v0 remains fixed-point even when native Mtx is float. Retain the
    # actual legacy packing branch as the independent byte-for-byte oracle;
    # exercise the full current native writers separately with the real types.
    fixed = function(gu, 'void guMtxF2L(').split('#else', 1)[1].split('#endif', 1)[0]
    source += 'void PackLegacyArchiveMatrix(float mf[4][4], MtxS* m) { unsigned int r,c;\n' + fixed + '\n}\n'
    source += declarations + function(passage, 'void Vertex(') + '\n' + function(passage, 'void BuildPassage(')
    source += r'''
int main(){
 BuildPassage();Matrix_Translate(-695,0,-313,MTXMODE_NEW);Matrix_RotateYS(12288,MTXMODE_APPLY);
 MtxS packed{};PackLegacyArchiveMatrix(current.mf,&packed);
 Mtx native{};float restored[4][4]{};guMtxF2L(current.mf,&native);guMtxL2F(restored,&native);
 printf("{\"vertices\":[");
 for(size_t i=0;i<sVertices.size();++i){const auto& v=sVertices[i].v;
  printf("%s[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]",i?",":"",v.ob[0],v.ob[1],v.ob[2],
         v.flag,v.tc[0],v.tc[1],v.cn[0],v.cn[1],v.cn[2],v.cn[3]);}
 printf("],\"matrix\":[");for(int i=0;i<16;++i)printf("%s%u",i?",":"",((u32*)packed.m)[i]);
 printf("],\"runtime_matrix\":[");for(int i=0;i<16;++i)printf("%s%.17g",i?",":"",(&restored[0][0])[i]);
 printf("],\"source_matrix\":[");for(int i=0;i<16;++i)printf("%s%.17g",i?",":"",(&current.mf[0][0])[i]);
#if defined(GBI_FLOATS) || defined(GBI_FLOAT_MTX)
 puts("],\"float_matrices\":true}");
#else
 puts("],\"float_matrices\":false}");
#endif
}
'''
    with tempfile.TemporaryDirectory(prefix='native-portal-control-') as folder:
        path, binary = Path(folder) / 'control.cpp', Path(folder) / 'control'
        path.write_text(source)
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-O0', '-DF3DEX_GBI_2',
                        '-I' + str(ROOT / 'mm/include'), '-I' + str(ROOT / 'mm/include/PR'),
                        '-I' + str(ROOT / 'libultraship/include'), str(path), '-o', str(binary)], check=True)
        return json.loads(subprocess.check_output([str(binary)], text=True))


def decoded_matrix(words):
    values = []
    for integer, fraction in zip(words[:8], words[8:]):
        for shift in (16, 0):
            value = ((integer >> shift) & 65535) << 16 | ((fraction >> shift) & 65535)
            if value & 0x80000000:
                value -= 1 << 32
            values.append(value / 65536)
    return [values[index:index + 4] for index in range(0, 16, 4)]


class PortalTests(unittest.TestCase):
    def test_native_matrix_roundtrip_preserves_combined_abi_and_legacy_archive_precision(self):
        control = native_control()
        if os.environ.get('COMBO_NATIVE_TEST_ABI') == 'combined':
            self.assertTrue(control['float_matrices'])
        if control['float_matrices']:
            self.assertEqual(control['runtime_matrix'], control['source_matrix'])
        else:
            self.assertEqual(control['runtime_matrix'],
                             [value for row in decoded_matrix(control['matrix']) for value in row])
        archive = [value for row in decoded_matrix(control['matrix']) for value in row]
        for packed, native in zip(archive, control['source_matrix']):
            self.assertLess(abs(packed - native), 1 / 65536)

    def test_baked_portal_matches_native_builder_and_fixed_matrix_without_extra_rounding(self):
        module = load_module(self)
        native, edit, final = fixture(module)
        resources, receipt = module.build_resources(native, edit, final)
        self.assertTrue(PREFIX + 'portal_root' in resources, 'Native OoT/MM forest needs the existing opaque portal arch')
        control = native_control()
        root = ET.fromstring(resources[PREFIX + 'portal_root'])
        vertex_path = root.find('LoadVertices').get('Path')
        raw = resources[vertex_path]
        count = struct.unpack_from('<I', raw, 68)[0]
        actual = [list(struct.unpack_from('<hhhHhhBBBB', raw, 72 + index * 16)) for index in range(count)]
        self.assertEqual(actual, control['vertices'])
        self.assertEqual(len(actual), 92)
        self.assertEqual(len(root.findall('LoadVertices')), 23)
        self.assertEqual(len(root.findall('Triangle1')), 46)
        matrix_node = root.find('Matrix')
        self.assertEqual(matrix_node.get('Param'), 'G_MTX_PUSH')
        self.assertEqual(root.find('PopMatrix').get('Param'), 'G_MTX_MODELVIEW')
        raw_matrix = resources[matrix_node.get('Path')]
        self.assertEqual(raw_matrix[4:8], b'XTMO')
        self.assertEqual(list(struct.unpack_from('<16I', raw_matrix, 64)), control['matrix'])
        self.assertEqual(receipt['portal']['anchor'], [-695, 0, -313])
        self.assertEqual(receipt['portal']['yaw'], 12288)
        self.assertEqual(receipt['portal']['triangles'], 46)

    def test_all_native_hosts_get_one_culled_portal_using_only_stock_bark(self):
        module = load_module(self)
        native, edit, final = fixture(module)
        resources, receipt = module.build_resources(native, edit, final)
        self.assertTrue(PREFIX + 'portal_root' in resources, 'A texture pack cannot supply the base arch')
        control = native_control()
        matrix = decoded_matrix(control['matrix'])
        world = [[sum(vertex[k] * matrix[k][axis] for k in range(3)) + matrix[3][axis]
                  for axis in range(3)] for vertex in control['vertices']]
        for prefix in (COMBINED, MM, OOT):
            commands = module.read_scene(resources[prefix + '_room_0'], oot=prefix == OOT)
            mesh = module.read_mesh(commands[10])
            self.assertEqual((mesh['type'], len(mesh['entries'])), (2, 10))
            entries = [entry for entry in mesh['entries'] if entry['opaque'] == PREFIX + 'portal_root']
            self.assertEqual(len(entries), 1)
            x, y, z, radius = entries[0]['bounds']
            self.assertTrue(all(sum((point[i] - (x, y, z)[i]) ** 2 for i in range(3)) <= radius ** 2
                                for point in world))
        material = ET.fromstring(resources[PREFIX + 'portal_material'])
        image = material.find('LoadTextureBlock')
        self.assertEqual(image.get('Path'), BARK)
        self.assertEqual((image.get('Width'), image.get('Height')), ('32', '32'))
        self.assertIn(BARK, receipt['external_textures'])
        self.assertTrue(all(data[4:8] != b'XETO' for data in resources.values()))


if __name__ == '__main__':
    unittest.main()
