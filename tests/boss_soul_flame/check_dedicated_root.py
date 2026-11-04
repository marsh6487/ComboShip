#!/usr/bin/env python3
"""Prove soul recipes cannot select the bottle pack's invisible flame root.

The shipped native DL material/scroll/texture commands must stay byte-exact;
only its vertex dependency moves to a private copy of the native quad.
This is resource/source verification, not configured gameplay acceptance.
"""
from pathlib import Path
import struct
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/temple_sages'))
from resource_geometry import crc64, words

PRIVATE = 'objects/object_boss_soul/gBossSoulFlameDL'
VERTEX = 'objects/object_boss_soul/gBossSoulFlameVtx'
NATIVE_VERTEX = 'objects/object_gi_fire/object_gi_fireVtx_000C20'

def check():
    # XML snapshots from zTPMiscObjects(1).otr (trailing whitespace normalized): the bottle folds
    # its flame into the chamberstick and deliberately suppresses this root.
    fixtures = Path(__file__).parent / 'fixtures'
    bottle = ET.fromstring((fixtures / 'gGiBlueFireFlameDL').read_bytes())
    material = ET.fromstring((fixtures / 'mat_gGiBlueFireFlameDL_ItemInvis').read_bytes())
    assert bottle[0].attrib['Path'].endswith('/mat_gGiBlueFireFlameDL_ItemInvis')
    prim_alpha = int(material.find('SetPrimColor').attrib['A']) / 255
    combiner = material.find('SetCombineLERP').attrib
    assert [combiner[k] for k in ('Aa0','Ab0','Ac0','Ad0')] == ['G_ACMUX_0','G_ACMUX_0','G_ACMUX_0','G_ACMUX_1']
    assert [combiner[k] for k in ('Aa1','Ab1','Ac1','Ad1')] == ['G_ACMUX_COMBINED','G_ACMUX_0','G_ACMUX_PRIMITIVE','G_ACMUX_0']
    assert ((0 - 0) * 0 + 1) * prim_alpha == 0, 'Negative control must actually draw zero alpha'
    original = (ROOT / 'tools/boss_soul_flames/native/OoT-flame-DL').read_bytes()
    dedicated = ROOT / 'soh/assets/custom' / PRIVATE
    assert dedicated.exists(), 'Soul still lacks an independent root; TP bottle Alt root has primitive alpha zero'
    candidate = dedicated.read_bytes()
    expected = bytearray(original)
    remaps = 0
    for offset, opcode, _, dependency in words(original):
        if dependency == crc64(NATIVE_VERTEX):
            assert opcode >> 24 == 0x32
            new_hash = crc64(VERTEX)
            struct.pack_into('<II', expected, offset + 8, new_hash >> 32, new_hash & 0xffffffff)
            remaps += 1
    assert remaps == 1 and candidate == expected, 'Native material/scroll/textures or geometry commands changed'
    vertex = (ROOT / 'soh/assets/custom' / VERTEX).read_bytes()
    assert vertex[4:8] == b'RRAO' and len(vertex) == 136
    assert struct.unpack_from('<II', vertex, 64) == (25, 4)
    rows = [struct.unpack_from('<hhhHhhBBBB', vertex, i) for i in range(72, 136, 16)]
    assert rows == [(-12,42,0,0,0,0,0,0,120,255), (-12,2,0,0,0,1024,0,0,120,255),
                    (12,2,0,0,512,1024,0,0,120,255), (12,42,0,0,512,0,0,0,120,255)]
    # The stock combiner retains its own visible alpha (170), native LOD 128,
    # segment-8 scroll call, and standard texture hashes, so POC5 still applies.
    commands = list(words(candidate))
    assert any(a == 0xfa000080 and b == 0xaaffffff for _,a,b,_ in commands)
    assert any(a == 0xde000000 and b == 0x08000001 for _,a,b,_ in commands)
    for suffix in ('000000', '000200'):
        assert any(h == crc64('objects/object_gi_fire/object_gi_fireTex_' + suffix) for _,_,_,h in commands)
    header = (ROOT / 'soh/assets/soh_assets.h').read_text()
    assert '__OTR__' + PRIVATE in header
    draw = (ROOT / 'soh/soh/Enhancements/randomizer/draw.cpp').read_text()
    soul = draw.split('void Randomizer_DrawBossSoul(',1)[1].split('// Draw the generic boss soul model',1)[0]
    assert '(Gfx*)gBossSoulFlameDL' in soul and 'gGiBlueFireFlameDL' not in soul
    # Remaining bottle/effect users retain their original generic root.
    assert draw.count('(Gfx*)gGiBlueFireFlameDL') == 2
    recipe = (ROOT / 'combo/menu/ComboItemDrawOOT.h').read_text()
    assert 'out->dlists[0] = gBossSoulFlameDL;' in recipe
    assert 'out->flameDlPath = gBossSoulFlameDL;' in recipe
    print('PASS: dedicated native soul root/quad, unchanged material/scroll/Alt texture paths, native and exported recipes')

if __name__ == '__main__':
    check()
