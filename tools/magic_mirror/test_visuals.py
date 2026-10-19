"""Run with python -m unittest discover -s tools/magic_mirror -v."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]


class MagicMirrorVisualTests(unittest.TestCase):
    def test_shared_mesh_behavior(self):
        header = ROOT / "combo/gameplay/MagicMirrorGeometry.h"
        self.assertTrue(header.exists(), "the shared native mirror mesh is missing")
        with tempfile.TemporaryDirectory(prefix="magic-mirror-test-") as temp:
            exe = Path(temp) / "geometry-test"
            subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-pedantic",
                            str(Path(__file__).with_name("test_geometry.cpp")), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)

    def test_icon_is_identical_in_both_games(self):
        paths = [ROOT / game / "assets/custom/textures/icon_item_custom/gItemIconMagicMirrorTex.rgba32.png"
                 for game in ("soh", "mm")]
        self.assertTrue(all(p.exists() for p in paths), "both native inventory icons must exist")
        self.assertEqual(paths[0].read_bytes(), paths[1].read_bytes())
        image = Image.open(paths[0])
        self.assertEqual(image.size, (32, 32))
        self.assertEqual(image.mode, "RGBA")
        alpha = image.getchannel("A")
        self.assertEqual(alpha.getextrema(), (0, 255))
        self.assertGreater(sum(a > 0 for a in alpha.tobytes()), 200)
        self.assertLess(sum(a > 0 for a in alpha.tobytes()), 800)

    def test_name_is_native_ia4_in_both_games(self):
        paths = [ROOT / game / "assets/custom/textures/item_name_custom/gMagicMirrorNameTex.ia4.png"
                 for game in ("soh", "mm")]
        self.assertTrue(all(p.exists() for p in paths), "both native item names must exist")
        self.assertEqual(paths[0].read_bytes(), paths[1].read_bytes())
        image = Image.open(paths[0]).convert("RGBA")
        self.assertEqual(image.size, (128, 16))
        raw = image.tobytes()
        pixels = [tuple(raw[i:i+4]) for i in range(0, len(raw), 4)]
        self.assertLessEqual({p[3] for p in pixels}, {0, 255})
        visible = {(r, g, b) for r, g, b, a in pixels if a}
        self.assertTrue(visible)
        self.assertLessEqual(len(visible), 8)
        self.assertTrue(all(r == g == b for r, g, b in visible))
        bbox = image.getchannel("A").getbbox()
        self.assertGreaterEqual(bbox[0], 1)
        self.assertGreaterEqual(bbox[1], 1)
        self.assertLessEqual(bbox[2], 127)
        self.assertLessEqual(bbox[3], 15)


if __name__ == "__main__":
    unittest.main()
