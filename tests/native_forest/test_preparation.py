#!/usr/bin/env python3
"""Exercise verified dual-tree preparation and the real archive-only CMake gate."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile

from test_generator import COMBINED, MM, OOT, PREFIX, ROOT, fixture, load_module

PREPARE = ROOT / 'scripts/assets/prepare_native_forest.py'
CMAKE = ROOT / 'CMake/NativeForestAssets.cmake'


def load_prepare(test):
    test.assertTrue(PREPARE.is_file(), 'Verified dual-tree native forest preparation is missing')
    spec = importlib.util.spec_from_file_location('prepare_native_forest', PREPARE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def source_bundle(test, folder):
    generator = load_module(test)
    resources, report = generator.build_resources(*fixture(generator))
    bundle = folder / 'resources.zip'
    manifest = folder / 'manifest.json'
    metadata = generator.write_bundle(resources, report, bundle)
    manifest.write_text(json.dumps(metadata))
    return resources, manifest, bundle


class PreparationTests(unittest.TestCase):
    def test_verified_native_geometry_prepares_identical_trees_and_repairs_only_stale_files(self):
        module = load_prepare(self)
        with tempfile.TemporaryDirectory(prefix='native-forest-prepare-') as name:
            directory = Path(name)
            resources, manifest, _ = source_bundle(self, directory)
            output = directory / 'source'
            module.prepare(output, manifest)
            paths = [output / game / 'assets/custom' / key for game in ('soh', 'mm') for key in resources]
            before = {path: path.stat().st_mtime_ns for path in paths}
            for game in ('soh', 'mm'):
                for key, data in resources.items():
                    self.assertEqual((output / game / 'assets/custom' / key).read_bytes(), data)
            module.prepare(output, manifest)
            self.assertEqual(before, {path: path.stat().st_mtime_ns for path in paths})
            damaged = paths[0]
            damaged.write_bytes(b'broken geometry')
            module.prepare(output, manifest)
            for game in ('soh', 'mm'):
                root = output / game / 'assets/custom'
                self.assertEqual({path.relative_to(root).as_posix(): path.read_bytes()
                                  for path in root.rglob('*') if path.is_file()}, resources)
            self.assertTrue(all(path == damaged or before[path] == path.stat().st_mtime_ns for path in paths))

    def test_bundle_corruption_and_missing_real_donor_fail_before_any_output_write(self):
        module = load_prepare(self)
        with tempfile.TemporaryDirectory(prefix='native-forest-invalid-') as name:
            directory = Path(name)
            _, manifest, bundle = source_bundle(self, directory)
            damaged = bytearray(bundle.read_bytes())
            damaged[len(damaged) // 2] ^= 1
            bundle.write_bytes(damaged)
            output = directory / 'source'
            with self.assertRaisesRegex(ValueError, 'bundle.*(hash|SHA)'):
                module.prepare(output, manifest)
            self.assertFalse(output.exists())
            manifest.write_text(json.dumps({'schema': 1, 'status': 'blocked_missing_native_mm',
                                            'required_native_mm_sha256': 'f10167'}))
            with self.assertRaisesRegex(ValueError, 'native MM|native-mm'):
                module.prepare(output, manifest)
            self.assertFalse(output.exists())

    def test_resource_hash_and_texture_exclusion_are_checked_beyond_the_zip_hash(self):
        module = load_prepare(self)
        with tempfile.TemporaryDirectory(prefix='native-forest-members-') as name:
            directory = Path(name)
            resources, manifest, bundle = source_bundle(self, directory)
            metadata = json.loads(manifest.read_text())
            first = next(iter(resources))
            with zipfile.ZipFile(bundle, 'w') as archive:
                for key, data in resources.items():
                    archive.writestr(key, b'changed bytes' if key == first else data)
            metadata['bundle']['sha256'] = hashlib.sha256(bundle.read_bytes()).hexdigest()
            metadata['bundle']['bytes'] = bundle.stat().st_size
            manifest.write_text(json.dumps(metadata))
            with self.assertRaisesRegex(ValueError, 'resource.*(hash|SHA|length|size)'):
                module.prepare(directory / 'output', manifest)
            key, data = PREFIX + 'forbidden_image', bytes(4) + b'XETO' + bytes(60)
            resources[key] = data
            with zipfile.ZipFile(bundle, 'w') as archive:
                for key, data in resources.items():
                    archive.writestr(key, data)
            metadata['bundle'].update(sha256=hashlib.sha256(bundle.read_bytes()).hexdigest(), bytes=bundle.stat().st_size)
            metadata['resources'][key] = {'sha256': hashlib.sha256(data).hexdigest(), 'bytes': len(data)}
            manifest.write_text(json.dumps(metadata))
            with self.assertRaisesRegex(ValueError, 'texture'):
                module.prepare(directory / 'output', manifest)
            self.assertFalse((directory / 'output').exists())

    def test_resource_contracts_include_only_each_engines_headers_and_common_stock_dependencies(self):
        module = load_module(self)
        resources, receipt = module.build_resources(*fixture(module))
        text = module.native_resource_header(resources, receipt)
        mm, oot = text.split('kNativeForestOotResources')
        self.assertIn('kNativeForestMmResources', mm)
        self.assertIn(COMBINED, mm)
        self.assertIn(MM, mm)
        self.assertNotIn(OOT, mm)
        self.assertIn(OOT, oot)
        self.assertNotIn(COMBINED, oot)
        self.assertNotIn(MM, oot)
        for path in receipt['external_resources']:
            self.assertIn('"' + path + '"', mm)
            self.assertIn('"' + path + '"', oot)

    def test_real_cmake_gate_blocks_only_archive_generation_until_a_verified_bundle_exists(self):
        self.assertTrue(CMAKE.is_file(), 'Native archive preparation dependency is missing')
        self.assertTrue(PREPARE.is_file())
        with tempfile.TemporaryDirectory(prefix='native-forest-cmake-') as name:
            directory = Path(name)
            source, build = directory / 'source', directory / 'build'
            source.mkdir()
            manifest = directory / 'manifest.json'
            manifest.write_text(json.dumps({'schema': 1, 'status': 'blocked_missing_native_mm'}))
            (source / 'ready.py').write_text(
                'from pathlib import Path\n'
                "assert (Path(__file__).parent/'soh/assets/custom/custom/comboship/native_forest/materials_v1').is_file()\n"
                "assert (Path(__file__).parent/'mm/assets/custom/custom/comboship/native_forest/materials_v1').is_file()\n")
            (source / 'CMakeLists.txt').write_text(
                'cmake_minimum_required(VERSION 3.22)\nproject(NativeForestPackaging NONE)\n'
                'find_package(Python3 REQUIRED COMPONENTS Interpreter)\n'
                'add_custom_target(CheckAssetCollisions)\n'
                'add_custom_target(GenerateSohOtr COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/ready.py")\n'
                'add_custom_target(Generate2ShipOtr COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/ready.py")\n'
                f'set(COMBO_NATIVE_FOREST_MANIFEST "{manifest.as_posix()}")\n'
                f'include("{CMAKE.as_posix()}")\n')
            configured = subprocess.run(['cmake', '-S', str(source), '-B', str(build)], capture_output=True, text=True)
            self.assertEqual(configured.returncode, 0, configured.stdout + configured.stderr)
            ordinary = subprocess.run(['cmake', '--build', str(build), '--target', 'CheckAssetCollisions'], capture_output=True, text=True)
            self.assertEqual(ordinary.returncode, 0, ordinary.stdout + ordinary.stderr)
            blocked = subprocess.run(['cmake', '--build', str(build), '--target', 'GenerateSohOtr'], capture_output=True, text=True)
            self.assertNotEqual(blocked.returncode, 0)
            self.assertIn('native MM', blocked.stdout + blocked.stderr)
            self.assertFalse((source / 'soh').exists())
            resources, _, _ = source_bundle(self, directory)
            for target in ('GenerateSohOtr', 'Generate2ShipOtr'):
                result = subprocess.run(['cmake', '--build', str(build), '--target', target], capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            for game in ('soh', 'mm'):
                for path, data in resources.items():
                    self.assertEqual((source / game / 'assets/custom' / path).read_bytes(), data)


if __name__ == '__main__':
    unittest.main()
