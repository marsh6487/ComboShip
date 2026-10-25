#!/usr/bin/env python3
"""Verify private MM basket layout identity, closure and original preservation."""
from pathlib import Path
import re
import struct
import sys
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / 'tools/black_market'), str(ROOT / 'scripts/assets')]
import forest
import mm_native_shop
import mm_shop_presentation as presentation
import native_market


class PrivateMmPresentation(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.original = mm_native_shop.apply(native_market.read_bundle(ROOT / 'tools/native_market/bundled'))
        cls.candidate = presentation.apply(cls.original)

    def test_only_mm_room_mesh_changes_and_six_private_owners_are_added(self):
        self.assertEqual(set(self.candidate) - set(self.original), set(presentation.CLONES.values()))
        self.assertEqual({name for name in self.original if self.original[name] != self.candidate[name]},
                         {mm_native_shop.ROOM})
        before = mm_native_shop.native_commands(self.original[mm_native_shop.ROOM])
        after = mm_native_shop.native_commands(self.candidate[mm_native_shop.ROOM])
        self.assertEqual({k: v for k, v in before.items() if k != 10},
                         {k: v for k, v in after.items() if k != 10})
        old = forest.string(presentation.ROOM_ROOT)
        new = forest.string(presentation.CLONES[presentation.ROOM_ROOT])
        self.assertEqual(after[10], before[10].replace(old, new))
        self.assertEqual(presentation.apply(self.original), self.candidate)

    def test_whole_components_move_rigidly_and_keep_all_other_vertex_attributes(self):
        moved = 0
        for suffix, moves in presentation.MOVES.items():
            name = f'{presentation.GEOMETRY}_vtx_{suffix}'
            a = list(ET.fromstring(self.original[name]))
            b = list(ET.fromstring(self.candidate[presentation.CLONES[name]]))
            parts = presentation.market_interior.vertex_components(self.original,
                        f'{presentation.GEOMETRY}_tri_{suffix}')
            expected = {index: dx for bounds, dx, _, _ in moves for part in parts
                        if part['bounds'] == bounds for index in part['indices']}
            self.assertEqual(len(a), len(b))
            for index, (first, last) in enumerate(zip(a, b)):
                values = dict(first.attrib)
                if index in expected:
                    values['X'] = str(int(values['X']) + expected[index])
                    moved += 1
                self.assertEqual(values, last.attrib)
        self.assertEqual(moved, 558)

    def test_actual_mm_mesh_has_a_closed_resource_graph(self):
        data = mm_native_shop.native_commands(self.candidate[mm_native_shop.ROOM])[10]
        reader = forest.Reader(data)
        reader.offset = 4
        root, xlu = reader.text(), reader.text()
        self.assertEqual(root, presentation.CLONES[presentation.ROOM_ROOT])
        seen = set()
        def visit(name):
            self.assertIn(name, self.candidate)
            if name in seen:
                return
            seen.add(name)
            content = self.candidate[name]
            if content.lstrip().startswith(b'<'):
                for node in ET.fromstring(content).iter():
                    path = node.get('Path')
                    if path:
                        if path == native_market.STOCK_TEXTURE:
                            continue
                        visit(path)
        visit(root)
        self.assertTrue(set(presentation.CLONES.values()) <= seen)
        self.assertNotIn(presentation.ROOM_ROOT, seen)

    def test_changed_component_identity_is_rejected(self):
        malformed = dict(self.original)
        name = f'{presentation.GEOMETRY}_vtx_17'
        root = ET.fromstring(malformed[name])
        root[527].set('Y', '0')
        malformed[name] = ET.tostring(root)
        with self.assertRaisesRegex(ValueError, 'component identity'):
            presentation.apply(malformed)

    def test_runtime_requires_every_generated_private_owner(self):
        source = (ROOT / 'combo/BlackMarketMmShopScene.h').read_text()
        required = re.findall(r'"(custom/skiejer_market/mm_shop_presentation_v1/[^"\n]+)"', source)
        self.assertEqual(set(required), set(presentation.CLONES.values()))


if __name__ == '__main__':
    unittest.main()
