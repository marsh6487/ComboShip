"""Active preparation must apply the bounded presentation without scene edits."""
import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zipfile

from test_generator import ROOT

sys.path.insert(0, str(ROOT / 'scripts/assets'))
from prepare_native_forest import prepared_resources
from native_forest_bundle import load_bundle

CORE = 'custom/comboship/native_forest/'


class PolishTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = ROOT / 'tools/native_forest/manifest.json'
        cls.metadata = json.loads(cls.manifest.read_text())
        with zipfile.ZipFile(io.BytesIO(load_bundle(cls.manifest.parent))) as archive:
            cls.before = {name: archive.read(name) for name in archive.namelist()}
        cls.after = prepared_resources(cls.manifest)

    def test_preparation_applies_a_pinned_revision_without_touching_scene_or_texture_bytes(self):
        self.assertEqual(self.metadata.get('presentation_polish', {}).get('version'), 1)
        self.assertEqual(set(self.before), set(self.after))
        changed = {name for name in self.before if self.before[name] != self.after[name]}
        self.assertEqual(changed, set(self.metadata['presentation_polish']['resources']))
        for name in changed:
            self.assertTrue(name.startswith(CORE))
            self.assertNotEqual(self.before[name][4:8], b'XETO', name)
            self.assertEqual(hashlib.sha256(self.after[name]).hexdigest(),
                             self.metadata['presentation_polish']['resources'][name]['sha256'])
        self.assertTrue(changed)

    def test_portal_uses_building_purple_and_has_no_legacy_frame_glow(self):
        from native_forest import display_commands
        name = CORE + 'authored/room_0/ReturnTunnelMouth'
        colors = [word for _, _, word, op in display_commands(self.after[name], name) if op == 0xFA]
        self.assertEqual(colors, [0x58188060])
        calls = [node.get('Path') for node in ET.fromstring(self.after[CORE + 'portal_xlu']) if node.get('Path')]
        self.assertEqual(calls, [name])

    def test_signs_swap_atlas_cells_while_avatar_and_all_positions_stay_exact(self):
        name = CORE + 'building/recipe7624751371212635_vtx'
        before, after = self.before[name], self.after[name]
        self.assertEqual(before[:72], after[:72])
        for index in range(52):
            offset = 72 + index * 16
            old = struct.unpack_from('<hhhHhhBBBB', before, offset)
            new = struct.unpack_from('<hhhHhhBBBB', after, offset)
            self.assertEqual(old[:4] + old[5:], new[:4] + new[5:])
            self.assertEqual(new[4], old[4] + (379 if old[2] > 480 else -379) if index < 48 else old[4])

    def test_ramp_owns_its_full_single_tile_emissive_pipeline(self):
        material = ET.fromstring(self.after[CORE + 'mirror_access_material'])
        high = material.find("SetOtherMode[@Cmd='G_SETOTHERMODE_H']")
        self.assertIsNotNone(high, 'Ramp must not inherit cycle/LOD state from the previous culled draw')
        self.assertEqual(high.get('G_CYC_1CYCLE'), '1')
        self.assertEqual(high.get('G_TL_TILE'), '1')
        texture = material.find('LoadTextureBlock')
        self.assertEqual(texture.get('Path'), CORE + 'authored/prelude/black_market_wall_magic_mask')
        self.assertEqual((texture.get('Width'), texture.get('Height')), ('32', '64'))
        self.assertEqual(texture.get('Format'), '4', 'The authored mask is native I8, not IA8')
        self.assertEqual(struct.unpack_from('<I', self.before[texture.get('Path')], 64)[0], 6)
        self.assertEqual(material.find('SetPrimColor').get('A'), '255')
        self.assertEqual(material.find('SetEnvColor').get('R'), '88')
        self.assertEqual(self.before[CORE + 'mirror_access_vtx'], self.after[CORE + 'mirror_access_vtx'])

    def test_authoring_rebakes_still_receive_the_immutable_source_bytes(self):
        from prepare_native_forest import verified_resources
        self.assertEqual(verified_resources(self.manifest), self.before)

    def test_a_bad_polish_receipt_fails_before_preparing_either_port(self):
        from prepare_native_forest import prepare
        for damage in ('version', 'hash', 'membership'):
            with self.subTest(damage=damage), tempfile.TemporaryDirectory(prefix='forest-polish-receipt-') as folder:
                root = Path(folder)
                metadata = json.loads(self.manifest.read_text())
                polish = metadata['presentation_polish']
                if damage == 'version':
                    polish['version'] = 2
                elif damage == 'hash':
                    polish['resources'][CORE + 'mirror_access_material']['sha256'] = '0' * 64
                else:
                    polish['resources'].pop(CORE + 'portal_xlu')
                manifest = root / 'manifest.json'
                manifest.write_text(json.dumps(metadata))
                (root / 'parts').symlink_to(self.manifest.parent / 'parts', target_is_directory=True)
                with self.assertRaises(ValueError):
                    prepare(root / 'output', manifest)
                self.assertFalse((root / 'output').exists())


if __name__ == '__main__':
    unittest.main()
