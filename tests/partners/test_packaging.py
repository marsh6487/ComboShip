"""Native archives contain the exact authored partners and Solo MM dependencies."""
import importlib.util
import base64
import json
import zlib
import sys
from pathlib import Path
import shutil
import tempfile
import unittest
from zipfile import ZipFile, ZipInfo

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location('partner_resources', ROOT / 'scripts/packaging/partner_resources.py')


class Packaging(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not Path(SPEC.origin).exists():
            return
        cls.module = importlib.util.module_from_spec(SPEC)
        SPEC.loader.exec_module(cls.module)

    def merger(self):
        self.assertTrue(Path(SPEC.origin).exists(), 'native build resource merger is missing')
        return self.module

    def test_exact_known_pack_membership(self):
        module = self.merger()
        soh = module.resources(ROOT, 'soh')
        mm = module.resources(ROOT, '2ship')
        self.assertEqual(len(soh), 403)
        self.assertEqual(len(mm), 605)
        self.assertEqual({n: mm[n] for n in soh}, soh)
        self.assertEqual(set(mm) - set(soh), set(module.load_manifest(ROOT)['native']))

    def test_shared_text_roundtrips_and_manual_packager_matches(self):
        module = self.merger()
        source = ROOT / 'combo/assets/partners/shared.json'
        self.assertTrue(source.exists(), 'bounded shared text source is missing')
        encoded = json.loads(source.read_text())
        decoded = {}
        for identity, text in encoded.items():
            self.assertIn('/', identity)
            name = identity.split('/', 1)[1]
            blob = zlib.decompress(base64.b85decode(text))
            if name in decoded:
                self.assertEqual(decoded[name], blob)
            decoded[name] = blob
        self.assertEqual(decoded, module.resources(ROOT, 'soh'))
        sys.path.insert(0, str(ROOT / 'scripts/mods'))
        try:
            import package_partners_enemies
            self.assertEqual(package_partners_enemies.collect(source.parent), decoded)
        finally:
            sys.path.pop(0)

    def test_merge_preserves_payloads_and_is_idempotent(self):
        module = self.merger()
        for game in ('soh', '2ship'):
            with self.subTest(game=game), tempfile.TemporaryDirectory() as directory:
                archive = Path(directory) / 'game.o2r'
                with ZipFile(archive, 'w') as output:
                    output.writestr('unrelated/resource', b'keep exactly\x00\xff')
                module.merge(archive, ROOT, game)
                first = archive.read_bytes()
                with ZipFile(archive) as output:
                    self.assertEqual(output.read('unrelated/resource'), b'keep exactly\x00\xff')
                    for name, data in module.resources(ROOT, game).items():
                        self.assertEqual(output.read(name), data)
                    self.assertEqual(len(output.namelist()), len(set(output.namelist())))
                module.merge(archive, ROOT, game)
                self.assertEqual(archive.read_bytes(), first)

    def test_conflicting_archive_is_rejected_without_mutation(self):
        module = self.merger()
        with tempfile.TemporaryDirectory() as directory:
            archive = Path(directory) / 'game.o2r'
            name = next(iter(module.resources(ROOT, 'soh')))
            with ZipFile(archive, 'w') as output:
                output.writestr(name, b'incorrect existing resource')
            before = archive.read_bytes()
            with self.assertRaisesRegex(ValueError, 'collision'):
                module.merge(archive, ROOT, 'soh')
            self.assertEqual(archive.read_bytes(), before)

    def test_matching_collision_retains_original_entry(self):
        module = self.merger()
        with tempfile.TemporaryDirectory() as directory:
            archive = Path(directory) / 'game.o2r'
            name, blob = next(iter(module.resources(ROOT, 'soh').items()))
            with ZipFile(archive, 'w') as output:
                output.writestr(name, blob)
            module.merge(archive, ROOT, 'soh')
            with ZipFile(archive) as output:
                self.assertEqual(output.namelist().count(name), 1)
                self.assertEqual(output.read(name), blob)

    def test_same_inputs_produce_identical_outputs(self):
        module = self.merger()
        with tempfile.TemporaryDirectory() as directory:
            outputs = []
            for index in range(2):
                archive = Path(directory) / f'{index}.o2r'
                with ZipFile(archive, 'w') as output:
                    output.writestr(ZipInfo('original', (1980, 1, 1, 0, 0, 0)), b'identical')
                module.merge(archive, ROOT, '2ship')
                outputs.append(archive.read_bytes())
            self.assertEqual(*outputs)

    def test_corrupt_source_is_rejected_before_mutation(self):
        module = self.merger()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            shutil.copytree(ROOT / 'tools/partners_native_dependencies', root / 'tools/partners_native_dependencies')
            shutil.copytree(ROOT / 'combo/assets/partners', root / 'combo/assets/partners')
            archive = root / 'game.o2r'
            with ZipFile(archive, 'w') as output:
                output.writestr('untouched', b'original')
            before = archive.read_bytes()
            blob = root / 'combo/assets/partners/shared.json'
            original_blob = blob.read_bytes()
            encoded = json.loads(original_blob)
            encoded[next(iter(encoded))] = base64.b85encode(zlib.compress(b'corrupt')).decode()
            blob.write_text(json.dumps(encoded))
            with self.assertRaises(ValueError):
                module.merge(archive, root, 'soh')
            self.assertEqual(archive.read_bytes(), before)
            blob.write_bytes(original_blob)
            dependency = root / 'tools/partners_native_dependencies/native.json'
            dependency.write_text('{}')
            with self.assertRaises(ValueError):
                module.merge(archive, root, '2ship')
            self.assertEqual(archive.read_bytes(), before)


if __name__ == '__main__':
    unittest.main()
