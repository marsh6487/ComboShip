"""Check actual built-in chamber controls selected by native Unbound paths."""
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / 'scripts/assets'), str(ROOT / 'tools/temple_sages')]
import resource_geometry as geometry
import prepare_temple_sages as prepare

ROOM = 'scenes/shared/tokinoma_scene/tokinoma_room_0'
COLLISION = 'scenes/shared/tokinoma_scene/tokinoma_sceneCollisionHeader_0032F8'
INDEX = 'custom/temple_sages/builtin_index.json'


class UnboundChamber(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.resources = prepare.verified_resources()

    def document(self, path):
        self.assertTrue(path in self.resources, 'built-in chamber lacks native Unbound control: ' + path)
        return json.loads(self.resources[path])

    def test_native_scene_selects_authored_collision_in_both_asset_modes(self):
        self.assertFalse('alt/scenes/tokinoma/scene.json' in self.resources,
                         'Alt must inherit canonical base story/spawn setups, not select a partial scene')
        for prefix in ('', 'alt/'):
            scene = self.document('scenes/tokinoma/scene.json')
            self.assertEqual(scene['collision'], 'scenes/tokinoma/collision.json')
            self.assertEqual(scene['rooms']['0'], 'scenes/tokinoma/rooms/0.json')
            self.assertNotIn('$replace', scene, 'scene patch must retain base entrances, rooms and story commands')
            collision = self.document(prefix + scene['collision'])
            self.assertEqual(collision['bounds']['min'], [-979, -242, -3473])
            self.assertEqual(collision['bounds']['max'], [979, 1600, 2760])
            self.assertEqual(collision['bulk']['vertices'], 884)
            self.assertEqual(collision['bulk']['polys'], 885)

    def test_all_native_room_setups_keep_exact_actor_and_object_records(self):
        for prefix in ('', 'alt/'):
            document = self.document(prefix + 'scenes/tokinoma/rooms/0.json')
            root = self.resources[prefix + ROOM]
            commands = geometry.room_commands(root)
            alternates = next(info['paths'] for code, _, _, info in commands if code == 0x18)
            expected = {'0': root, **{str(i + 1): self.resources[prefix + path]
                                    for i, path in enumerate(alternates) if path}}
            self.assertEqual(set(document['setups']), set(expected))
            self.assertTrue(document['$replace'])
            for setup, old in expected.items():
                new = document['setups'][setup]
                for code, start, end, info in geometry.room_commands(old):
                    if code == 8:
                        self.assertEqual(struct.unpack_from('<bi', old, start + 4),
                                         (new['behavior']['gameplayFlags'], new['behavior']['gameplayFlags2']))
                    if code == 16:
                        self.assertEqual(struct.unpack_from('<BBb', old, start + 4),
                                         tuple(new['time'][key] for key in ('hour', 'minute', 'increment')))
                    if code == 18:
                        self.assertEqual(old[start + 4:end], bytes(new['skyboxModifier'][key]
                                         for key in ('skyboxDisabled', 'sunMoonDisabled')))
                    if code == 22:
                        self.assertEqual(old[start + 4:end], bytes([new['echo']]))
                    if code == 1:
                        actors = new['actors']
                        packed = b''.join(struct.pack('<H3h3hH', actor['id'], *actor['pos'], *actor['rot'],
                                                      actor['params'] & 0xFFFF)
                                          for actor in actors.values())
                        self.assertEqual(packed, info['actors'], 'actor/setup changed: ' + setup)
                    if code == 11:
                        objects = list(new['objects'].values())
                        self.assertEqual(old[start + 4:end], struct.pack('<I', len(objects)) +
                                         b''.join(struct.pack('<h', obj) for obj in objects))
                self.assertEqual(new['mesh']['entries']['0']['xlu'], 'custom/temple_sages/annex_xlu')
            for setup in ('0', '2'):
                actors = list(document['setups'][setup]['actors'].values())
                self.assertEqual(sorted(actor['params'] & 0xFFFF for actor in actors if actor['id'] == 302),
                                 list(range(0xFC00, 0xFC05)))
                self.assertTrue(any(actor['id'] == 42 for actor in actors))

    def test_collision_bulk_roundtrips_native_vertices_polys_and_flags(self):
        for prefix in ('', 'alt/'):
            document = self.document(prefix + 'scenes/tokinoma/collision.json')
            path = prefix + document['bulk']['file']
            self.assertTrue(path in self.resources, 'collision bulk is not packaged')
            data = self.resources[path]
            vertices, polys, _, _ = geometry.collision_parts(self.resources[prefix + COLLISION])
            self.assertEqual(len(data), len(vertices) * 12 + len(polys) * 28)
            for index, vertex in enumerate(vertices):
                self.assertEqual(struct.unpack_from('<3i', data, index * 12), vertex)
            for index, poly in enumerate(polys):
                item = struct.unpack_from('<HHIIIhhhhi', data, len(vertices) * 12 + index * 28)
                self.assertEqual((item[0], *(item[i] & 0x1FFF | ((item[i] >> 29) << 13)
                                            for i in (2, 3, 4)), *item[5:8], item[9]), poly)
                self.assertEqual((item[1], item[8]), (0, 0))

    def test_collision_surface_camera_and_water_fields_roundtrip_native(self):
        for prefix in ('', 'alt/'):
            document = self.document(prefix + 'scenes/tokinoma/collision.json')
            data0 = (('camera', 0), ('exit', 8), ('floorType', 13), ('wallFlags', 18),
                     ('wallType', 21), ('floorProperty', 26), ('isSoft', 30), ('isHorseBlocked', 31))
            data1 = (('material', 0), ('floorEffect', 4), ('lightSetting', 6), ('echo', 11),
                     ('canHookshot', 17), ('conveyorSpeed', 18), ('conveyorDirection', 21), ('isWallDamage', 27))
            vertices, polys, surfaces, tail = geometry.collision_parts(self.resources[prefix + COLLISION])
            packed = b''.join(struct.pack('<II', sum(surface[k] << shift for k, shift in data1),
                                               sum(surface[k] << shift for k, shift in data0))
                              for surface in document['surfaceTypes'].values())
            self.assertEqual(packed, surfaces, 'native material/echo or other surface flags changed')
            cameras = list(document['cameras'].values())
            positions = list(document['cameraPositions'].values())
            packed = (struct.pack('<I', len(cameras)) + b''.join(struct.pack('<Hhi', camera['sType'],
                      camera['count'], camera['positionIndex']) for camera in cameras) +
                      struct.pack('<I', len(positions)) + b''.join(struct.pack('<3h', *row) for row in positions) +
                      struct.pack('<I', len(document['waterBoxes'])))
            self.assertFalse(document['waterBoxes'])
            self.assertEqual(packed, tail, 'native camera or water payload changed')

    def test_prepared_runtime_index_names_all_payloads_and_no_other_resources(self):
        with tempfile.TemporaryDirectory() as temporary:
            for game in ('soh', 'mm'):
                stale = Path(temporary) / game / 'assets/custom/alt/scenes/tokinoma/scene.json'
                stale.parent.mkdir(parents=True)
                stale.write_text('{"collision":"stale partial scene"}')
            resources = prepare.prepare(Path(temporary))
            self.assertTrue(INDEX in resources, 'normal port has no scoped built-in mount index')
            index = json.loads(resources[INDEX])
            self.assertEqual(index['version'], 1)
            self.assertEqual(index['resources'], sorted(set(resources) - {INDEX}))
            for game in ('soh', 'mm'):
                root = Path(temporary) / game / 'assets/custom'
                self.assertFalse((root / 'alt/scenes/tokinoma/scene.json').exists())
                for path in ('scenes/tokinoma/scene.json', 'scenes/tokinoma/rooms/0.json',
                             'scenes/tokinoma/collision.json', 'scenes/tokinoma/collision.bin', INDEX):
                    self.assertEqual((root / path).read_bytes(), resources[path])
                self.assertEqual(resources['scenes/tokinoma/rooms/0.json'],
                                 resources['alt/scenes/tokinoma/rooms/0.json'])


if __name__ == '__main__':
    unittest.main()
