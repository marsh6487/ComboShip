#!/usr/bin/env python3
"""Exercise native Din resource validation without authoring PNGs."""
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/assets/build_dins_fire_remake.py"
SPEC = importlib.util.spec_from_file_location("dins_fire_encoder", SCRIPT)
ENCODER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ENCODER)


class NativeResourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="dins-native-check-")
        cls.root = Path(cls.temp.name)
        for host in ("soh", "mm"):
            path = Path(host) / "assets/custom/custom/dins_fire_remake"
            shutil.copytree(ROOT / path, cls.root / path)
        shutil.copytree(ROOT / "tools/dins_fire_remake", cls.root / "tools/dins_fire_remake",
                        ignore=shutil.ignore_patterns("*.png"))
        path = Path("combo/render/DinsFireTiming.h")
        (cls.root / path).parent.mkdir(parents=True)
        shutil.copyfile(ROOT / path, cls.root / path)
        cls.script = cls.root / "scripts/assets/build_dins_fire_remake.py"
        cls.script.parent.mkdir(parents=True)
        shutil.copyfile(SCRIPT, cls.script)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def assert_rejected_mutation(self, paths, change):
        originals = {path: path.read_bytes() for path in paths}
        try:
            for path, data in originals.items():
                path.write_bytes(change(data))
            with self.assertRaises((ValueError, SystemExit)):
                ENCODER.verify_committed_resources(self.root)
        finally:
            for path, data in originals.items():
                path.write_bytes(data)

    def test_cli_without_source_art(self):
        result = subprocess.run([sys.executable, "-B", str(self.script), "--check-native"],
                                capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(result.stdout)
        self.assertEqual(report["host_resources"], 288)
        self.assertEqual(report["operation"], "check-native")
        self.assertFalse(report["source_regenerated"])
        self.assertFalse(report["runtime_tested"])

    def test_verifies_both_hosts_without_source_art(self):
        self.assertEqual(list((self.root / "tools/dins_fire_remake").glob("*.png")), [])
        self.assertEqual(ENCODER.verify_committed_resources(self.root)["host_resources"], 288)

    def test_rejects_missing_resource(self):
        path = self.root / "mm/assets/custom/custom/dins_fire_remake/flameTex"
        original = path.read_bytes()
        try:
            path.unlink()
            with self.assertRaises((ValueError, SystemExit)):
                ENCODER.verify_committed_resources(self.root)
        finally:
            path.write_bytes(original)

    def test_rejects_unexpected_resource(self):
        path = self.root / "soh/assets/custom/custom/dins_fire_remake/unexpectedTex"
        try:
            path.write_bytes(b"unexpected")
            with self.assertRaises((ValueError, SystemExit)):
                ENCODER.verify_committed_resources(self.root)
        finally:
            path.unlink()

    def test_rejects_equal_corruption_in_both_hosts(self):
        paths = [self.root / host / "assets/custom/custom/dins_fire_remake/ribbon0Tex"
                 for host in ("soh", "mm")]
        self.assert_rejected_mutation(paths, lambda data: data[:-1] + bytes([data[-1] ^ 1]))

    def test_rejects_changed_texture_header(self):
        paths = [self.root / host / "assets/custom/custom/dins_fire_remake/flameTex"
                 for host in ("soh", "mm")]
        self.assert_rejected_mutation(paths, lambda data: data[:64] + b"\xff" + data[65:])

    def test_rejects_rewritten_manifest(self):
        path = self.root / "tools/dins_fire_remake/manifest.json"
        self.assert_rejected_mutation([path], lambda data: data.replace(b'"cvar_default": 0', b'"cvar_default": 1'))

    def test_rejects_changed_resource_declarations(self):
        path = self.root / "tools/dins_fire_remake/native_resources.h"
        self.assert_rejected_mutation([path], lambda data: data.replace(b"ribbon0Tex", b"missingTex"))

    def test_rejects_changed_gameplay_timing(self):
        path = self.root / "combo/render/DinsFireTiming.h"
        self.assert_rejected_mutation([path], lambda data: data + b"\n/* altered */\n")

    def test_rejects_changed_pre_linger_provenance(self):
        path = self.root / "tools/dins_fire_remake/pre_linger_resource_provenance.json"
        self.assert_rejected_mutation([path], lambda data: data + b"\n")

    def test_source_regeneration_requires_supplied_art(self):
        result = subprocess.run([sys.executable, "-B", str(self.script), "--check"],
                                capture_output=True, text=True, check=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("source", result.stderr.lower())


if __name__ == "__main__":
    unittest.main()
