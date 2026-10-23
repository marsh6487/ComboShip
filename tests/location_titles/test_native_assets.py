#!/usr/bin/env python3
"""Real built-in artwork, default preparation and smooth HD geometry parity."""
from pathlib import Path
import base64
import importlib.util
import json
import shutil
import struct
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
MODULE = ROOT / 'tools/location_titles/native_assets.py'
PREFIX = 'custom/oot_location_titles/v1/'
sys.path.insert(0, str(ROOT / 'tools/location_titles'))


def load_module(path):
    spec = importlib.util.spec_from_file_location(path.stem, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class NativeAssets(unittest.TestCase):
    def assets(self):
        self.assertTrue(MODULE.is_file(), 'native OoT title artwork must be built in, not optional-only')
        return load_module(MODULE)

    def test_every_english_scene_title_is_built_in(self):
        module = self.assets()
        resources = module.resources()
        coverage = json.loads((ROOT / 'tools/location_titles/assets/native-v3/manifest.json').read_text())['titles']
        self.assertEqual(len(coverage), 67)
        self.assertEqual(len(resources), 69)
        self.assertEqual(set(resources), {PREFIX + '1/' + row['id'] for row in coverage} |
                         {PREFIX + 'market', PREFIX + 'lost_woods'})
        for row in coverage:
            data = resources[PREFIX + '1/' + row['id']]
            self.assertEqual(struct.unpack_from('<IIII', data, 64), (8, 240, 24, 5760))
        names = {row['text'] for row in coverage}
        self.assertTrue({'East Clock Town', 'North Clock Town', "Honey & Darling's Shop", 'Post Office',
                         'Astral Observatory', 'Oceanside Spider House'} <= names)

    def test_optional_pack_uses_smooth_hd_art_with_native_resource_geometry(self):
        module = self.assets()
        native, hd = module.resources(), module.hd_resources()
        self.assertEqual(set(native), set(hd))
        for name, data in native.items():
            kind, width, height, size = struct.unpack_from('<IIII', data, 64)
            self.assertEqual((kind, size, len(data)), (8, width * height, 80 + width * height))
            raw = data[80:]
            self.assertTrue(any(v >> 4 and v & 15 for v in raw), name)
            self.assertTrue(any(not v >> 4 and v & 15 for v in raw), name + ' baked black halo')
            if not name.endswith('/lost_woods'):
                self.assertFalse(any(v & 15 for v in raw[:width] + raw[-width:] + raw[::width] + raw[width - 1::width]), name)
            out = hd[name]
            self.assertEqual(struct.unpack_from('<IIIIffI', out, 64),
                             (1, width * 4, height * 4, 1, 16.0, 4.0, width * height * 64))
            pixels = out[92:]
            if not name.endswith('/lost_woods'):
                alpha = pixels[3::4]
                physical_width = width * 4
                self.assertFalse(any(alpha[:physical_width] + alpha[-physical_width:] +
                                     alpha[::physical_width] + alpha[physical_width - 1::physical_width]),
                                 name + ' HD must preserve the native transparent perimeter')
            self.assertGreater(len(set(pixels[3::4])), 32, name + ' needs smooth 8-bit HD alpha')
            varying_blocks = 0
            for y in range(0, height * 4, 4):
                for x in range(0, width * 4, 4):
                    block = {pixels[((y + dy) * width * 4 + x + dx) * 4 + 3]
                             for dy in range(4) for dx in range(4)}
                    varying_blocks += len(block) > 1
            self.assertGreater(varying_blocks, 20, name + ' cannot just repeat native pixels')

    def test_corrupt_encoded_source_is_rejected(self):
        module = self.assets()
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            shutil.copytree(module.ASSETS, root / 'assets')
            p = root / 'assets/cards.zlib.b64'
            p.write_bytes(p.read_bytes() + b'!')
            with self.assertRaises(ValueError):
                module.resources(root / 'assets')

    def test_hd_preserves_native_letter_contours(self):
        import hd_assets
        metrics = json.loads((self.assets().ASSETS / 'manifest.json').read_text())['glyph_layout']
        for ch, (face, _, _) in hd_assets.glyphs().items():
            native = base64.b64decode(metrics[ch]['face_coverage_b64'], validate=True)
            reduced = [sum(face[(y * 4 + dy) * 96 + x * 4 + dx] for dy in range(4) for dx in range(4)) / 16
                       for y in range(24) for x in range(24)]
            # Compare fractional coverage, not thresholded pixels: 119/136
            # native edge values straddle 128 without defining a hard edge.
            error = sum(abs(value - reduced[i]) for i, value in enumerate(native)) / sum(native)
            self.assertLessEqual(error, 0.30, repr(ch) + ' HD outline must preserve native letter shape')

        module = self.assets()
        native = module.resources()[PREFIX + 'lost_woods'][80:]
        rgba = module.hd_resources()[PREFIX + 'lost_woods'][92:]
        face = [rgba[i] * rgba[i + 3] / 255 for i in range(0, len(rgba), 4)]
        source = [(value >> 4) * (value & 15) * 255 / 225 for value in native]
        reduced = [sum(face[(y * 4 + dy) * 576 + x * 4 + dx] for dy in range(4) for dx in range(4)) / 16
                   for y in range(24) for x in range(144)]
        error = sum(abs(value - reduced[i]) for i, value in enumerate(source)) / sum(source)
        self.assertLessEqual(error, 0.25, 'HD Lost Woods must preserve the original whole-word layout and shapes')

    def test_default_preparation_repairs_both_hosts_without_mods(self):
        module = self.assets()
        prepare = load_module(ROOT / 'scripts/assets/prepare_native_titles.py')
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            target = root / 'mm/assets/custom' / (PREFIX + '1/0104')
            target.parent.mkdir(parents=True)
            target.write_bytes(b'stale MM glyph candidate')
            self.assertEqual(prepare.prepare(root), 138)
            self.assertEqual(prepare.prepare(root), 0)
            resources = module.resources()
            for host in ('mm', 'soh'):
                for name, data in resources.items():
                    self.assertEqual((root / host / 'assets/custom' / name).read_bytes(), data)


if __name__ == '__main__':
    unittest.main()
