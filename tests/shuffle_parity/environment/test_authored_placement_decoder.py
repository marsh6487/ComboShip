#!/usr/bin/env python3
"""Synthetic format/negative controls; contains no user quest resources."""
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
import zipfile

PATH = Path(__file__).with_name('audit_authored_placements.py')
spec = importlib.util.spec_from_file_location('authored_audit', PATH)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def integer(value):
    return struct.pack('<I', value)


def string(value):
    return integer(len(value)) + value.encode()


def resource(body, kind=0x4f524f4d):
    return bytes(4) + integer(kind) + bytes(56) + body


def scene(*commands):
    return resource(integer(len(commands)) + b''.join(commands))


def collision():
    # One horizontal synthetic triangle and one room-zero synthetic water box.
    body = struct.pack('<hhhhhh', -10, -10, -10, 10, 10, 10)
    body += integer(3) + struct.pack('<hhhhhhhhh', -10, 0, -10, 10, 0, -10, 0, 0, 10)
    body += integer(1) + struct.pack('<HHHHhhhh', 0, 0, 1, 2, 0, 32767, 0, 0)
    body += integer(0) * 3
    body += integer(1) + struct.pack('<hhhhhi', -10, 5, -10, 20, 20, 0)
    return resource(body, 0x4f434f4c)


class DecoderTests(unittest.TestCase):
    def test_actor_stream_and_truncation(self):
        entry = (0x8123, 1, 2, 3, 4, 5, 6, 0xabcd)
        data = scene(integer(1) + integer(1) + struct.pack('<HhhhhhhH', *entry), integer(20))
        self.assertEqual(audit.decode_scene(data)[1], [entry])
        for corrupted in (data[:-1], data + b'X', data[:8] + integer(1) + data[12:]):
            with self.assertRaises(AssertionError):
                audit.decode_scene(corrupted)

    def test_alternate_headers_follow_serialized_path(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'synthetic.zip'
            with zipfile.ZipFile(path, 'w') as archive:
                archive.writestr('base', scene(integer(24) + integer(2) + string('different-name') + string('')))
                archive.writestr('different-name', scene(integer(1) + integer(0)))
            quest = audit.Quest(path)
            self.assertEqual(quest.effective('base', 1), ({1: []}, 'different-name'))
            self.assertEqual(quest.effective('base', 0)[1], 'base')
            self.assertEqual(quest.effective('base', 2)[1], 'base')
            self.assertEqual(quest.effective('base', 3)[1], 'base')

    def test_collision_and_projection(self):
        low, high, vertices, polygons, water = audit.decode_collision(collision())
        self.assertEqual((low, high), ((-10,-10,-10),(10,10,10)))
        self.assertEqual(len(polygons), 1)
        self.assertEqual(water, [(-10,5,-10,20,20,0)])
        self.assertEqual(audit.floor_projection(vertices, 0, 0), 0)
        self.assertIsNone(audit.floor_projection(vertices, 100, 100))
        with self.assertRaises(AssertionError):
            audit.decode_collision(collision()[:-1])

    def run_single(self, actor='ACTOR_BG_ICICLE', params=0, slice=0, wrong_id=False, objects=True, bean=None):
        row = dict(check='RC_GREAT_BAY_TEMPLE_ICICLE_COMPASS_ROOM_1', family='RCTYPE_ICICLE',
                   scene='SCENE_SEA', setup=0, room=0, index=0, slice=slice, actor=actor)
        scenes, actors = audit.names(audit.ROOT)
        name = scenes[row['scene']]
        prefix = f'scenes/nonmq/{name}/{name}'
        object_ids = dict((n,int(v,16)) for v,n in __import__('re').findall(
            r'/\* (0x[0-9A-Fa-f]+) \*/ DEFINE_OBJECT\(\s*\w+,\s*(\w+)\)',
            (audit.ROOT/'mm/include/tables/object_table.h').read_text()))
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'synthetic.zip'
            with zipfile.ZipFile(path, 'w') as archive:
                archive.writestr(prefix, scene(integer(4)+integer(1)+string('room')+bytes(8),
                                                integer(3)+string('collision')))
                actor_entries = [struct.pack('<HhhhhhhH',
                           actors[actor] + int(wrong_id),0,0,0,7,0,127,params)]
                if bean is not None:
                    actor_entries.append(struct.pack('<HhhhhhhH', actors['ACTOR_OBJ_BEAN'],
                                         bean.get('x', 0), 0, 0, 7, 0, 127, bean.get('params', 0x4000)))
                entries = [integer(1) + integer(len(actor_entries)) + b''.join(actor_entries)]
                if objects:
                    entries += [integer(11)+integer(2)+struct.pack('<HH',
                                object_ids['OBJECT_ICICLE'], object_ids['GAMEPLAY_FIELD_KEEP'])]
                archive.writestr('room', scene(*entries))
                archive.writestr('collision', collision())
            # Explicit partial-format fixture boundary: audit decodes these rows;
            # main separately applies the full production policy coverage gate.
            with patch.object(audit, 'catalog', return_value=[row]):
                return audit.audit(audit.ROOT, path)[:2]

    def test_actual_id_mode_slice_and_object_guards_reject(self):
        self.assertEqual(len(self.run_single()[0]), 1)
        for kwargs in ({'wrong_id': True}, {'params': 1}, {'objects': False},
                       {'actor': 'ACTOR_EN_ISHI', 'params': 1},
                       {'actor': 'ACTOR_OBJ_MURE2', 'params': 0},
                       {'actor': 'ACTOR_OBJ_MURE2', 'params': 2, 'slice': 8}):
            rows, failures = self.run_single(**kwargs)
            self.assertEqual(rows, [])
            self.assertEqual(len(failures), 1)

    def test_soil_pair_type_switch_position_and_slice(self):
        kwargs = dict(actor='ACTOR_OBJ_SWPRIZE', params=0x200, slice=2, bean={})
        self.assertEqual(len(self.run_single(**kwargs)[0]), 1)
        for change in ({'bean': {'params': 0x4001}}, {'bean': {'params': 0}},
                       {'bean': {'x': 1}}, {'params': 0, 'slice': 1}):
            rows, failures = self.run_single(**{**kwargs, **change})
            self.assertEqual(rows, [])
            self.assertEqual(len(failures), 1)

    def test_full_policy_rejects_missing_catalog_decoded_and_water_witness(self):
        placements = audit.catalog(audit.ROOT)
        policy = set(__import__('re').findall(r'MM_IRON_BOOT_ROCK\((RC_\w+)\)',
                     (audit.ROOT/'mm/2s2h/Rando/MMEnvironmentIronBootRocks.inc').read_text()))
        # Synthetic water witnesses exercise full real policy membership without assets.
        rows = [dict(check=check, position=[0,0,0], static_water_surface=5) for check in sorted(policy)]
        audit.check_full_policy_coverage(audit.ROOT, placements, rows)
        missing = rows[0]['check']
        with self.assertRaisesRegex(AssertionError, 'missing from catalog'):
            audit.check_full_policy_coverage(audit.ROOT,
                [p for p in placements if p['check'] != missing], rows)
        with self.assertRaisesRegex(AssertionError, 'missing from decoded results'):
            audit.check_full_policy_coverage(audit.ROOT, placements, rows[1:])
        with self.assertRaisesRegex(AssertionError, 'no authored underwater witness'):
            audit.check_full_policy_coverage(audit.ROOT, placements,
                [{**rows[0], 'static_water_surface': None}, *rows[1:]])


if __name__ == '__main__':
    unittest.main()
