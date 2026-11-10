#!/usr/bin/env python3
"""Audit privately supplied MM quest scene resources; never copy asset bytes to output.

The binary reader mirrors MM SceneFactory and scenecommand/*Factory.cpp v0.
Default output is aggregate only. --private-details is for a private local JSON,
which must not be published alongside this source-only audit.
"""
import argparse
from collections import Counter
import hashlib
import json
import math
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[3]


class Reader:
    def __init__(self, data):
        assert len(data) >= 64 and data[0] == 0, 'unsupported resource header/endian'
        assert struct.unpack_from('<I', data, 8)[0] == 0, 'unsupported resource version'
        self.data, self.pos = data, 64

    def take(self, fmt):
        fmt = '<' + fmt
        size = struct.calcsize(fmt)
        assert self.pos + size <= len(self.data), 'truncated scene command'
        values = struct.unpack_from(fmt, self.data, self.pos)
        self.pos += size
        return values[0] if len(values) == 1 else values

    def skip(self, size):
        assert 0 <= size <= len(self.data) - self.pos, 'truncated command payload'
        self.pos += size

    def string(self):
        size = self.take('I')
        assert size <= len(self.data) - self.pos, 'invalid string length'
        value = self.data[self.pos:self.pos + size].decode('utf-8')
        self.pos += size
        return value


def decode_scene(data):
    r = Reader(data)
    assert struct.unpack_from('<I', data, 4)[0] == 0x4f524f4d, 'not a scene resource'
    result = {}
    count = r.take('I')
    assert count < 100, 'invalid command count'
    fixed = {5: 4, 8: 6, 16: 3, 17: 4, 18: 2, 20: 0,
             21: 3, 22: 1, 25: 0}
    for _ in range(count):
        cmd = r.take('I')
        if cmd in fixed:
            r.skip(fixed[cmd])
        elif cmd in (0, 1):
            entries = [r.take('HhhhhhhH') for _ in range(r.take('I'))]
            result[cmd] = entries
        elif cmd in (3, 26):
            result[cmd] = r.string()
        elif cmd in (13, 24):
            result[cmd] = [r.string() for _ in range(r.take('I'))]
        elif cmd == 4:
            result[cmd] = [(r.string(), r.take('ii')) for _ in range(r.take('I'))]
        elif cmd == 2:
            for _ in range(r.take('I')):
                _, n = r.take('HH')
                r.skip(6 * n)
        elif cmd == 7:
            result[cmd] = r.take('bh')
        elif cmd == 11:
            result[cmd] = [r.take('H') for _ in range(r.take('I'))]
        elif cmd in (6, 19):
            r.skip(r.take('I') * 2)
        elif cmd == 12:
            r.skip(r.take('I') * 13)
        elif cmd in (14, 27):
            r.skip(r.take('I') * 16)
        elif cmd == 15:
            r.skip(r.take('I') * 22)
        elif cmd == 28:
            n = r.take('I')
            r.skip(2 + n * 10)
        elif cmd == 30:
            r.skip(r.take('I') * 10)
        elif cmd == 31:
            for _ in range(r.take('B')):
                r.string()
                r.skip(4)
        elif cmd == 10:
            _, kind = r.take('bb')
            n = r.take('b') if kind != 1 else 1
            assert kind in (0, 1, 2) and n >= 0, 'unsupported mesh'
            for _ in range(n):
                r.skip(1)
                if kind == 1:
                    r.string()
                    r.string()
                    for _ in range(r.take('I')):
                        r.skip(3)
                        r.string()
                        r.skip(18)
                    r.skip(1)
                elif kind == 2:
                    r.skip(8)
                r.string()
                r.string()
        else:
            raise AssertionError(f'unsupported scene command {cmd}')
    assert r.pos == len(data), f'unconsumed resource tail ({len(data)-r.pos})'
    return result


def decode_collision(data):
    r = Reader(data)
    assert struct.unpack_from('<I', data, 4)[0] == 0x4f434f4c, 'not a collision resource'
    low, high = r.take('hhh'), r.take('hhh')
    vertices = [r.take('hhh') for _ in range(r.take('I'))]
    polygons = [r.take('HHHHhhhh') for _ in range(r.take('I'))]
    r.skip(r.take('I') * 8)  # Surface types.
    r.skip(r.take('I') * 8)  # Camera records.
    r.skip(r.take('I') * 6)  # Camera positions.
    water = [r.take('hhhhhi') for _ in range(r.take('I'))]
    assert r.pos == len(data), 'unconsumed collision resource tail'
    assert all((i & 0x1fff) < len(vertices) for p in polygons for i in p[1:4])
    return low, high, vertices, polygons, water


def floor_projection(vertices, x, z):
    a, b, c = vertices
    denominator = (b[2]-c[2])*(a[0]-c[0]) + (c[0]-b[0])*(a[2]-c[2])
    if not denominator:
        return None
    u = ((b[2]-c[2])*(x-c[0]) + (c[0]-b[0])*(z-c[2])) / denominator
    v = ((c[2]-a[2])*(x-c[0]) + (a[0]-c[0])*(z-c[2])) / denominator
    if u < -1e-6 or v < -1e-6 or u+v > 1+1e-6:
        return None
    return u*a[1] + v*b[1] + (1-u-v)*c[1]


class Quest:
    def __init__(self, path):
        self.zip = zipfile.ZipFile(path)
        self.cache = {}
        self.collision_cache = {}

    def scene(self, path):
        path = path.removeprefix('__OTR__')
        if path not in self.cache:
            self.cache[path] = decode_scene(self.zip.read(path))
        return self.cache[path]

    def collision(self, path):
        if path not in self.collision_cache:
            self.collision_cache[path] = decode_collision(self.zip.read(path))
        return self.collision_cache[path]

    def effective(self, path, layer):
        scene = self.scene(path)
        headers = scene.get(24, [])
        if layer and len(headers) >= layer and headers[layer - 1]:
            return self.scene(headers[layer - 1]), headers[layer - 1]
        return scene, path


def catalog(root):
    text = (root / 'mm/2s2h/Rando/MMEnvironmentPlacements.inc').read_text()
    return [dict(zip(('check', 'family', 'scene', 'setup', 'room', 'index', 'slice', 'actor'),
                     (parts[0], parts[1], parts[2], *map(int, parts[3:7]), parts[7])))
            for match in re.finditer(r'^MM_ENVIRONMENT_PLACEMENT\(([^\n]+)\)$', text, re.M)
            for parts in [[s.strip() for s in match[1].split(',')]]]


def names(root):
    scenes = dict(re.findall(r'DEFINE_SCENE\(\s*(\w+),\s*(SCENE_\w+),',
                            (root / 'mm/include/tables/scene_table.h').read_text()))
    actors = {name: int(value, 16) for value, name in re.findall(
        r'/\* (0x[0-9A-Fa-f]+) \*/ DEFINE_ACTOR(?:_INTERNAL)?\(\s*\w+,\s*(ACTOR_\w+),',
        (root / 'mm/include/tables/actor_table.h').read_text())}
    return {v: k for k, v in scenes.items()}, actors


def audit(root, archive):
    quest = Quest(archive)
    scene_names, actor_ids = names(root)
    object_ids = {name: int(value, 16) for value, name in re.findall(
        r'/\* (0x[0-9A-Fa-f]+) \*/ DEFINE_OBJECT\(\s*\w+,\s*(\w+)\)',
        (root / 'mm/include/tables/object_table.h').read_text())}
    rows, failures = [], []
    placements = catalog(root)
    for p in placements:
        try:
            name = scene_names[p['scene']]
            prefix = f'scenes/nonmq/{name}/{name}'
            scene, _ = quest.effective(prefix, p['setup'])
            room_path = scene[4][p['room']][0]
            room, effective_room = quest.effective(room_path, p['setup'])
            entries = room[1]
            entry = entries[p['index']]
            identity, x, y, z, rx, ry, rz, params = entry
            assert identity & 0x1fff == actor_ids[p['actor']], 'actor identity differs'
            actor = p['actor']
            assert (actor != 'ACTOR_EN_ISHI' or params & 1 == 0), 'not a small rock'
            assert (actor != 'ACTOR_BG_ICICLE' or params & 3 in (0, 3)), 'nonbreaking icicle mode'
            assert (actor != 'ACTOR_OBJ_MURE2' or params & 3 == 2), 'not a rock ring'
            assert (actor != 'ACTOR_OBJ_MURE2' or 0 <= p['slice'] < 8), 'invalid ring slice'
            assert (actor != 'ACTOR_OBJ_SWPRIZE' or p['slice'] < (3 if (params >> 8) & 3 == 2 else 1)), 'invalid soil slice'
            # Actor_SpawnSetupActors treats zero as HALFDAYBIT_ALL.
            half_days = ((rx & 7) << 7) | (rz & 0x7f) or 0x3ff
            if half_days != 0x3ff:
                ranch = (root / 'mm/2s2h/Rando/Logic/Regions/MilkRoad.cpp').read_text()
                rule = next(line for line in ranch.splitlines() if 'CHECK('+p['check']+',' in line)
                if half_days & 0xfc == 0xc0:
                    assert 'FIRST_DAY()' in rule, 'Day 1 actor lacks matching logical day predicate'
                elif half_days & 0xfc == 0x3c:
                    assert 'SECOND_DAY() || FINAL_DAY()' in rule, 'Days 2/3 actor lacks matching logical day predicate'
                else:
                    raise AssertionError('unreviewed restricted actor half-day mask')
            required_object = {'ACTOR_BG_ICICLE': 'OBJECT_ICICLE',
                               'ACTOR_OBJ_BOMBIWA': 'OBJECT_BOMBIWA',
                               'ACTOR_OBJ_HAMISHI': 'GAMEPLAY_FIELD_KEEP'}.get(actor)
            if actor in ('ACTOR_EN_ISHI', 'ACTOR_OBJ_MURE2'):
                required_object = 'OBJECT_ISHI' if actor == 'ACTOR_EN_ISHI' and params & 8 else 'GAMEPLAY_FIELD_KEEP'
            available_objects = set(room.get(11, []))
            if 7 in scene:
                available_objects.add(scene[7][1])
            if required_object:
                assert object_ids[required_object] in available_objects, 'required native object unavailable'
            row = dict(p, resource=effective_room, actor_params=params, position=[x, y, z],
                       rotation=[rx, ry, rz], half_days=half_days)
            if actor == 'ACTOR_OBJ_MURE2':
                angle = p['slice'] * math.pi / 4
                row['child_position'] = [x + 80 * math.sin(angle), y, z + 80 * math.cos(angle)]
            if actor == 'ACTOR_OBJ_SWPRIZE':
                beans = [(i, e) for i, e in enumerate(entries)
                         if e[0] & 0x1fff == actor_ids['ACTOR_OBJ_BEAN'] and e[7] & 0x7f == params & 0x7f]
                assert len(beans) == 1, 'soil does not have exactly one matching bean switch'
                i, bean = beans[0]
                assert bean[7] >> 14 in (1, 2), 'bean cannot accept released bugs'
                bean_days = ((bean[4] & 7) << 7) | (bean[6] & 0x7f) or 0x3ff
                assert half_days & bean_days == half_days, 'soil active without matching bean'
                assert [x, y, z] == list(bean[1:4]), 'soil source and bean positions differ'
                row['bean_index'] = i
                row['bean_distance'] = math.dist([x,y,z], bean[1:4])
            low, high, vertices, polygons, waters = quest.collision(scene[3])
            px, py, pz = row.get('child_position', row['position'])
            row['within_static_bounds'] = all(a <= b <= c for a,b,c in zip(low, (px,py,pz), high))
            projections = [floor_projection([vertices[i & 0x1fff] for i in poly[1:4]], px, pz)
                           for poly in polygons if poly[5] > 0]
            floors = [h for h in projections if h is not None and h <= py + 30]
            row['static_floor_below'] = max(floors) if floors else None
            row['native_levitation'] = actor == 'ACTOR_EN_ISHI' and bool(params & 4)
            row['static_water_surface'] = next((h for a,h,c,dx,dz,prop in waters
                if not prop & 0x80000 and ((prop >> 13) & 0x3f) in (p['room'], 0x3f)
                and a < px < a+dx and c < pz < c+dz), None)
            rows.append(row)
        except (AssertionError, KeyError, IndexError) as exc:
            failures.append(dict(p, error=str(exc)))
    # No floor is required by Bombiwa's init or native levitating rocks.
    # Great Bay clear-state aliases move onto the same floor in setup 1.
    for check in {r['check'] for r in rows}:
        aliases = [r for r in rows if r['check'] == check]
        if not any(r['actor'] not in ('ACTOR_EN_ISHI', 'ACTOR_OBJ_MURE2')
                   or r['native_levitation'] or r['static_floor_below'] is not None for r in aliases):
            failures.append({'check': check, 'error': 'no alias has a native levitating or static-floor rock witness'})
    return rows, failures, quest


def check_full_policy_coverage(root, placements, rows):
    """Full-production gate; partial decoder fixtures deliberately call audit only."""
    iron_checks = set(re.findall(r'MM_IRON_BOOT_ROCK\((RC_\w+)\)',
                                 (root / 'mm/2s2h/Rando/MMEnvironmentIronBootRocks.inc').read_text()))
    assert len(iron_checks) == 30, 'incomplete underwater source policy'
    missing_catalog = iron_checks - {p['check'] for p in placements}
    missing_decoded = iron_checks - {r['check'] for r in rows}
    assert not missing_catalog, 'Iron Boots policy checks missing from catalog: ' + ', '.join(sorted(missing_catalog))
    assert not missing_decoded, 'Iron Boots policy checks missing from decoded results: ' + ', '.join(sorted(missing_decoded))
    for check in iron_checks:
        assert any(r['check'] == check and r['static_water_surface'] is not None
                   and r['static_water_surface'] > r['position'][1] for r in rows), \
                   'Iron Boots source has no authored underwater witness: ' + check


def native_checks(root, rows):
    """Compile exact production Identify/MatchesParams against private decoded values."""
    with tempfile.TemporaryDirectory(prefix='mm-private-placements-') as directory:
        source = Path(directory) / 'native.cpp'
        lines = ['#include "global.h"', '#include "2s2h/Rando/MMEnvironment.h"']
        for row in rows:
            lines += [f"static_assert(Rando::MMEnvironment::MatchesParams({row['actor']}, {row['actor_params']}));",
                      f"static_assert(Rando::MMEnvironment::Identify({row['scene']}, {row['setup']}, {row['room']}, {row['index']}, {row['slice']}, {row['actor']}) == {row['check']});",
                      f"static_assert(Rando::MMEnvironment::NormalizeSetup({row['scene']}, {row['setup']}) == {row['setup']});"]
        source.write_text('\n'.join(lines) + '\n')
        flags = ['-std=c++20', '-DF3DEX_GBI_2', '-DGBI_FLOAT_MTX', '-DGBI_S32_VTX',
                 '-DCONTROLLERBUTTONS_T=uint32_t', '-I'+str(root), '-fsyntax-only']
        for path in ('mm/include', 'mm/include/PR', 'mm/2s2h', 'mm', 'mm/src',
                     'mm/assets', 'libultraship/include', 'combo', 'combo/menu'):
            flags += ['-isystem', str(root/path)]
        for combo in (False, True):
            subprocess.run([os.environ.get('CXX', 'c++'), *flags,
                            *(['-DCOMBO_BUILD'] if combo else []), str(source)], check=True,
                           capture_output=True, text=True)
            print('PASS actual production Identify/MatchesParams/NormalizeSetup: 279 decoded aliases, ' +
                  ('Combo' if combo else 'original'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--repo', type=Path, default=ROOT)
    parser.add_argument('--private-details', type=Path)
    parser.add_argument('--native-checks', action='store_true')
    args = parser.parse_args()
    rows, failures, quest = audit(args.repo, args.archive)
    check_full_policy_coverage(args.repo, catalog(args.repo), rows)
    assert len(catalog(args.repo)) == 279 and len({r['check'] for r in rows}) == 211, 'incomplete catalog coverage'
    if args.native_checks and not failures:
        native_checks(args.repo, rows)
    summary = {'archive_sha256': hashlib.sha256(args.archive.read_bytes()).hexdigest(),
               'catalog_aliases': len(catalog(args.repo)), 'matched_aliases': len(rows),
               'matched_checks': len({r['check'] for r in rows}),
               'decoded_resources': len(quest.cache),
               'decoded_collision_resources': len(quest.collision_cache),
               'aliases_inside_static_bounds': sum(r['within_static_bounds'] for r in rows),
               'aliases_with_static_floor_below': sum(r['static_floor_below'] is not None for r in rows),
               'soil_aliases_with_colocated_native_bean': sum('bean_index' in r for r in rows),
               'native_levitating_rock_aliases': sum(r['native_levitation'] for r in rows),
               'families': dict(Counter(r['family'] for r in rows)),
               'failures': failures}
    if args.private_details:
        args.private_details.write_text(json.dumps({'summary': summary, 'placements': rows}, indent=2) + '\n')
    print(json.dumps(summary, indent=2))
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
