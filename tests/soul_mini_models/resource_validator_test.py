#!/usr/bin/env python3
"""Negative controls for the archive gate, using invented miniature resources."""
import struct
import tempfile
from pathlib import Path
import unittest
import zipfile

from check_native_resources import (InvalidResource, OANM, ODLT, OPAM, OSKL, Reader,
                                    Resources, crc64, material_segments, parse_animation, parse_player_pose,
                                    parse_skeleton, validate_recipe)


def header(kind):
    return struct.pack('<B3xII', 0, kind, 0).ljust(64, b'\0')


def string(value):
    data = value.encode()
    return struct.pack('<I', len(data)) + data


def skeleton(count=2, table=2, display=2):
    return header(OSKL) + struct.pack('<bbIIbI', 1, 1, count, display, 1, table) + b''.join(string(f'limb/{n}') for n in range(table))


def animation(frames=2, indices=((0, 1, 2), (3, 4, 5))):
    return (header(OANM) + struct.pack('<IhI', 0, frames, 8) + bytes(16) +
            struct.pack('<I', len(indices)) + b''.join(struct.pack('<3H', *x) for x in indices) + struct.pack('<H', 3))


def display_list(commands):
    return header(ODLT) + bytes(8) + b''.join(struct.pack('<2I', *x) for x in commands) + struct.pack('<2I', 0xDF000000, 0)


class ResourceValidatorTests(unittest.TestCase):
    def test_normal_animation_bounds(self):
        self.assertEqual(parse_animation(animation(), 2)['frames'], 2)
        for bad in (animation(frames=0), animation(indices=((0, 1, 2), (3, 4, 8))), animation()[:-1]):
            with self.assertRaises(InvalidResource):
                parse_animation(bad, 2)
        with self.assertRaises(InvalidResource):
            parse_animation(animation(), 3)
        wrong_type = bytearray(animation())
        struct.pack_into('<I', wrong_type, 64, 1)
        with self.assertRaises(InvalidResource):
            parse_animation(bytes(wrong_type), 2)

    def test_skeleton_counts_and_documented_fsn_correction(self):
        self.assertEqual(parse_skeleton(skeleton())['count'], 2)
        mismatch = skeleton(18, 17, 17)
        with self.assertRaises(InvalidResource):
            parse_skeleton(mismatch)
        with self.assertRaises(InvalidResource):
            parse_skeleton(mismatch, '__OTR__different/rig')
        corrected = parse_skeleton(mismatch, '__OTR__objects/object_fsn/gFsnSkel')
        self.assertEqual((corrected['count'], corrected['declared_count']), (17, 18))
        with self.assertRaises(InvalidResource):
            parse_skeleton(skeleton(18, 16, 16), '__OTR__objects/object_fsn/gFsnSkel')
        with self.assertRaises(InvalidResource):
            parse_skeleton(skeleton(2, 2, 3))

    def test_player_pose_vector_and_stride_bounds(self):
        count = (22 * 3 + 1) * 2
        pose = header(OPAM) + struct.pack('<I', count) + bytes(count * 2)
        self.assertEqual(parse_player_pose(pose, 22)['frames'], 2)
        with self.assertRaises(InvalidResource):
            parse_player_pose(pose[:-1], 22)
        with self.assertRaises(InvalidResource):
            parse_player_pose(pose, 21)
        with self.assertRaises(InvalidResource):
            parse_player_pose(animation(), 22)

    def test_negative_length_and_truncated_strings(self):
        with self.assertRaises(InvalidResource):
            Reader(header(ODLT)).skip(-1)
        with self.assertRaises(InvalidResource):
            Reader(header(ODLT) + struct.pack('<I', 4) + b'ab').string()

    def test_missing_dependencies_segment_and_palette_failures(self):
        with tempfile.TemporaryDirectory(prefix='soul-resource-negative-') as directory:
            path = Path(directory) / 'fixture.o2r'
            with zipfile.ZipFile(path, 'w') as archive:
                archive.writestr('fixture/body', display_list([(0xDE000000, 0x09000001), (0xDA380003, 0x0D000081)]))
                h = crc64('fixture/missing-child')
                archive.writestr('fixture/bad-hash', display_list([(0x31000000, 0), (h >> 32, h & 0xFFFFFFFF)]))
            resources = Resources('oot', path)
            recipe = dict(id='invented', owner='oot', category='enemy', scale=0.01,
                          parts=[dict(dlPath='__OTR__fixture/body')], matrixCount=3,
                          segments=[dict(segment=9, kind=0)])
            self.assertTrue(validate_recipe(recipe, resources)['passed'])
            with self.assertRaisesRegex(InvalidResource, 'unbound segments'):
                validate_recipe(dict(recipe, segments=[]), resources)
            with self.assertRaisesRegex(InvalidResource, 'exceeds authored palette'):
                validate_recipe(dict(recipe, matrixCount=2), resources)
            with self.assertRaisesRegex(InvalidResource, 'missing oot resource'):
                validate_recipe(dict(recipe, parts=[dict(dlPath='__OTR__fixture/absent')]), resources)
            with self.assertRaisesRegex(InvalidResource, 'unresolved DL hash'):
                resources.closure(['fixture/bad-hash'])
            with self.assertRaisesRegex(InvalidResource, 'wrong resource owner'):
                resources.data('__OTR__@mm:fixture/body')

    def test_xml_alt_selection_and_mesh_types(self):
        with tempfile.TemporaryDirectory(prefix='soul-alt-negative-') as directory:
            base, mod = (Path(directory) / name for name in ('base.o2r', 'mod.o2r'))
            with zipfile.ZipFile(base, 'w') as archive:
                archive.writestr('fixture/body', display_list([]))
                archive.writestr('fixture/wrong', animation())
                archive.writestr('fixture/texture', '<Texture/>')
            with zipfile.ZipFile(mod, 'w') as archive:
                archive.writestr('alt/fixture/body', '<DisplayList><Matrix Path=">0x0D000080"/></DisplayList>')
            resources = Resources('oot', base, [mod])
            recipe = dict(id='invented', owner='oot', category='enemy', scale=0.01,
                          parts=[dict(dlPath='__OTR__fixture/body')], matrixCount=3)
            self.assertTrue(validate_recipe(recipe, resources)['passed'])
            self.assertTrue(resources.selected['fixture/body']['replacement'])
            self.assertEqual(resources.dl('fixture/body')['matrices'], [(13, 2)])
            for wrong in ('fixture/wrong', 'fixture/texture'):
                with self.assertRaisesRegex(InvalidResource, 'not a display list'):
                    validate_recipe(dict(recipe, parts=[dict(dlPath=wrong)]), resources)
            with self.assertRaisesRegex(InvalidResource, 'exceeds authored palette'):
                validate_recipe(dict(recipe, matrixCount=2), resources)

    def test_limb_override_cannot_overrun_flex_palette(self):
        with tempfile.TemporaryDirectory(prefix='soul-flex-negative-') as directory:
            path = Path(directory) / 'fixture.o2r'
            with zipfile.ZipFile(path, 'w') as archive:
                archive.writestr('rig', '<Skeleton Type="Flex" LimbCount="2" DisplayListCount="1">'
                                 '<SkeletonLimb Path="limb/0"/><SkeletonLimb Path="limb/1"/></Skeleton>')
                archive.writestr('limb/0', '<SkeletonLimb ChildIndex="1" DisplayList1="body"/>')
                archive.writestr('limb/1', '<SkeletonLimb DisplayList1="gEmptyDL"/>')
                archive.writestr('body', display_list([]))
            resources = Resources('oot', path)
            recipe = dict(id='invented', owner='oot', category='npc', scale=0.01, skelPath='rig',
                          limbCount=3, bindPose=True, fixedPoseCount=3)
            self.assertTrue(validate_recipe(recipe, resources)['passed'])
            with self.assertRaisesRegex(InvalidResource, 'effective limb matrices'):
                validate_recipe(dict(recipe, limbOverrides=[dict(limbIndex=2, dlPath='body')]), resources)

    def test_material_color_array_bounds(self):
        def material(kind, length, count, keys, prim_count, env_count=0):
            return (header(0x4F54414E) + struct.pack('<IbhHHH', 1, -1, kind, length, count, len(keys)) +
                    b''.join(struct.pack('<H', k) for k in keys) + struct.pack('<H', prim_count) +
                    bytes(5 * prim_count) + struct.pack('<H', env_count) + bytes(4 * env_count))
        with tempfile.TemporaryDirectory(prefix='soul-material-negative-') as directory:
            path = Path(directory) / 'fixture.o2r'
            records = {
                'valid': material(4, 20, 2, [0, 20], 2),
                'short-keys': material(4, 20, 2, [0], 2),
                'short-prim': material(4, 20, 2, [0, 20], 1),
                'short-env': material(4, 20, 2, [0, 20], 2, 1),
                'duplicate': material(4, 20, 2, [0, 0], 2),
                'indexed-short': material(2, 3, 0, [], 2),
                'unported-handler': header(0x4F54414E) + struct.pack('<Ibh4B', 1, -1, 0, 1, 1, 32, 32),
            }
            missing_end = bytearray(records['valid'])
            struct.pack_into('<b', missing_end, 68, 1)
            records['missing-end'] = bytes(missing_end)
            bad_segment = bytearray(records['valid'])
            struct.pack_into('<b', bad_segment, 68, -7)
            records['segment-outside-consumer'] = bytes(bad_segment)
            with zipfile.ZipFile(path, 'w') as archive:
                for name, data in records.items():
                    archive.writestr(name, data)
            resources = Resources('mm', path)
            self.assertEqual(material_segments(resources, 'valid'), ({8}, [], [4]))
            for name in records.keys() - {'valid'}:
                with self.subTest(name=name), self.assertRaises(InvalidResource):
                    material_segments(resources, name)


if __name__ == '__main__':
    unittest.main(verbosity=2)
