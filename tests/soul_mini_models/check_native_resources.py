#!/usr/bin/env python3
"""Validate compiled soul recipes against private native resource archives, read-only.

No ROM, resource contents, animation samples, or geometry leave this process. Reports
contain identities, paths, hashes and validation counts only. This is an asset/recipe
gate; GPU rendering and visual acceptance are separate checks.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import struct
import sys
import xml.etree.ElementTree as ET
from read_only_archive import ReadOnlyArchive

ROOT = Path(__file__).resolve().parents[2]
_spec = importlib.util.spec_from_file_location('native_resource_geometry', ROOT / 'tools/temple_sages/resource_geometry.py')
_geometry = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_geometry)
crc64 = _geometry.crc64

OSKL = 0x4F534B4C
OSLB = 0x4F534C42
OANM = 0x4F414E4D
OPAM = 0x4F50414D
ODLT = 0x4F444C54
EXPANDED = {0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42}
PATH_FIELDS = {'skelPath', 'animPath', 'posePath', 'texAnimPath', 'texAnimPath2',
               'flameDlPath', 'postModelDlPath', 'fallbackTokenPath', 'dlPath',
               'alternateDlPath', 'postDlPath', 'path'}


class InvalidResource(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise InvalidResource(message)


class Reader:
    def __init__(self, data, offset=64):
        self.data, self.offset = data, offset

    def take(self, fmt):
        fmt = '<' + fmt
        size = struct.calcsize(fmt)
        require(self.offset + size <= len(self.data), f'truncated resource at {self.offset}')
        result = struct.unpack_from(fmt, self.data, self.offset)
        self.offset += size
        return result[0] if len(result) == 1 else result

    def skip(self, size):
        require(0 <= size <= len(self.data) - self.offset, f'invalid resource span {size}')
        self.offset += size

    def string(self):
        size = self.take('I')
        require(size <= len(self.data) - self.offset, 'truncated resource string')
        result = self.data[self.offset:self.offset + size].decode('utf-8').rstrip('\0')
        self.offset += size
        return result


def resource_kind(data):
    if data.lstrip().startswith(b'<'):
        kind = ET.fromstring(data).tag
        return {'Skeleton': OSKL, 'SkeletonLimb': OSLB, 'DisplayList': ODLT,
                'Vertex': 0x4F565458, 'Texture': 0x4F544558}.get(kind, 0)
    require(len(data) >= 64, 'truncated resource header')
    require(data[0] == 0, 'unsupported resource byte order')
    return struct.unpack_from('<I', data, 4)[0]


def expect_kind(data, kind):
    actual = resource_kind(data)
    require(actual == kind, f'wrong resource type {actual:08x}, expected {kind:08x}')
    require(struct.unpack_from('<I', data, 8)[0] == 0, 'unsupported native resource version')


def parse_skeleton(data, path=None):
    if data.lstrip().startswith(b'<'):
        root = ET.fromstring(data)
        require(root.tag == 'Skeleton', 'XML resource is not a skeleton')
        kind = {'Normal': 0, 'Flex': 1}.get(root.get('Type', 'Flex'))
        require(kind is not None, 'unsupported XML skeleton type')
        count = int(root.get('LimbCount', 0))
        paths = [e.get('Path') for e in root if e.tag == 'SkeletonLimb']
        display = int(root.get('DisplayListCount', 0))
        require(0 < count <= 127 and count == len(paths) and all(paths), 'invalid XML skeleton limb table size')
        require(0 <= display <= count, 'invalid XML flex matrix count')
        return dict(type=kind, limb_type=2, count=count, declared_count=count,
                    display_count=display, table_type=2, paths=paths)
    expect_kind(data, OSKL)
    r = Reader(data)
    skeleton_type, limb_type = r.take('b'), r.take('b')
    count, display_count, table_type, table_count = r.take('I'), r.take('I'), r.take('b'), r.take('I')
    require(skeleton_type in (0, 1), f'unsupported skeleton type {skeleton_type}')
    require(limb_type in (1, 2), f'unsupported limb type {limb_type}')
    declared_count = count
    # The native En_Fsn actor documents this shipped header error and corrects its
    # joint count. The soul renderer makes the same correction on its own header
    # copy after validating the actual typed resource's 17-entry segment vector.
    fsn_path = 'objects/object_fsn/gFsnSkel'
    if path and path.removeprefix('__OTR__') == fsn_path and (skeleton_type, limb_type, count, display_count, table_count) == (1, 1, 18, 17, 17):
        count = 17
    require(0 < count <= 127 and count == table_count, 'invalid skeleton limb table size')
    require(display_count <= count, 'invalid flex matrix count')
    paths = [r.string() for _ in range(table_count)]
    require(all(paths), 'empty native limb path')
    return dict(type=skeleton_type, limb_type=limb_type, count=count, declared_count=declared_count,
                display_count=display_count, table_type=table_type, paths=paths)


def parse_limb(data):
    if data.lstrip().startswith(b'<'):
        root = ET.fromstring(data)
        require(root.tag == 'SkeletonLimb', 'XML resource is not a skeleton limb')
        dl = root.get('DisplayList1', '')
        # Both XML factories currently materialize the LOD ABI and use its first
        # display list for standard native traversal.
        return dict(type=2, child=int(root.get('ChildIndex', 255)), sibling=int(root.get('SiblingIndex', 255)),
                    translation=tuple(int(float(root.get('LegTrans' + axis, 0))) for axis in 'XYZ'),
                    dl='' if dl == 'gEmptyDL' else dl, dl2='', extra_paths=[])
    expect_kind(data, OSLB)
    r = Reader(data)
    limb_type, skin_type = r.take('b'), r.take('b')
    skin_dl = r.string()
    r.take('H')
    modifiers = r.take('I')
    require(modifiers <= len(data), 'invalid skin modifier count')
    for _ in range(modifiers):
        r.take('H')
        count = r.take('i')
        r.skip(count * 10)
        count = r.take('i')
        r.skip(count * 8)
    skin_dl2 = r.string()
    legacy_translation, legacy_rotation = r.take('3f'), r.take('3H')
    child_path, sibling_path, dl, dl2 = (r.string() for _ in range(4))
    translation, child, sibling = r.take('3h'), r.take('B'), r.take('B')
    require(limb_type in (1, 2), f'unsupported loaded limb type {limb_type}')
    return dict(type=limb_type, child=child, sibling=sibling, translation=translation,
                dl=dl, dl2=dl2, extra_paths=[x for x in (skin_dl, skin_dl2, child_path, sibling_path) if x])


def parse_animation(data, joints):
    expect_kind(data, OANM)
    r = Reader(data)
    kind, frames = r.take('I'), r.take('h')
    require(kind == 0, 'normal skeleton recipe requires normal OANM, not player/curve animation')
    require(frames > 0, 'animation has no frames')
    values = r.take('I')
    r.skip(values * 2)
    count = r.take('I')
    require(joints <= count <= len(data) // 6, 'animation joint table is smaller than skeleton')
    indices = [r.take('3H') for _ in range(count)]
    static_max = r.take('H')
    require(static_max <= values, 'animation static table exceeds values')
    for joint in indices[:joints]:
        for index in joint:
            last = index if index < static_max else index + frames - 1
            require(last < values, 'animation frame index exceeds data vector')
    return dict(frames=frames, joints=count, values=values, static_max=static_max)


def parse_player_pose(data, joints):
    expect_kind(data, OPAM)
    r = Reader(data)
    count = r.take('I')
    require(count * 2 <= len(data) - r.offset, 'player animation vector exceeds resource size')
    require(count >= joints * 3, 'player animation has insufficient pose vectors')
    # Native player frames append two bytes of face data to the joint vectors.
    stride = joints * 3 + 1
    require(count % stride == 0, 'player animation frame stride does not match skeleton')
    return dict(frames=count // stride, joint_values=count, pose_joints=joints)


def normalize_path(path, owner):
    require(isinstance(path, str) and path, 'empty resource path')
    if path.startswith('__OTR__'):
        path = path[7:]
    if path.startswith('@'):
        prefix, path = path[1:].split(':', 1)
        require(prefix == owner, f'wrong resource owner {prefix}, expected {owner}')
    require(not path.startswith('/') and '..' not in path.split('/'), 'unsafe resource path')
    return path


class Resources:
    def __init__(self, owner, path, mods=(), stormlib=None):
        self.owner, self.path = owner, path
        self.archive = ReadOnlyArchive(path, stormlib)
        self.names = self.archive.names
        self.mods = [ReadOnlyArchive(mod, stormlib) for mod in mods]
        self.alt = bool(self.mods)
        custom = ROOT / ('soh' if owner == 'oot' else 'mm') / 'assets/custom'
        self.overlay = {str(p.relative_to(custom)): p for p in custom.rglob('*') if p.is_file()}
        names = self.names | self.overlay.keys()
        for mod in self.mods:
            names = names | mod.names
        self.hashes = {crc64(n): n for n in names}
        for n in names:
            if n.startswith('alt/'):
                self.hashes[crc64(n[4:])] = n[4:]
        self.read = {}
        self.selected = {}
        self.dl_cache = {}

    def data(self, path):
        path = normalize_path(path, self.owner)
        if path not in self.read:
            candidates = ['alt/' + path, path] if self.alt and not path.startswith('alt/') else [path]
            for candidate in candidates:
                source = next((mod for mod in reversed(self.mods) if candidate in mod.names), None)
                if source:
                    self.read[path] = source.read(candidate)
                    self.selected[path] = dict(path=candidate, archive=source.path.name, replacement=True)
                    break
                if candidate in self.overlay:
                    self.read[path] = self.overlay[candidate].read_bytes()
                    self.selected[path] = dict(path=candidate, archive='source-owned custom assets', replacement=False)
                    break
                if candidate in self.names:
                    self.read[path] = self.archive.read(candidate)
                    self.selected[path] = dict(path=candidate, archive=self.archive.path.name, replacement=candidate.startswith('alt/'))
                    break
            require(path in self.read, f'missing {self.owner} resource: {path}')
        return self.read[path]

    def dl(self, path):
        path = normalize_path(path, self.owner)
        if path in self.dl_cache:
            return self.dl_cache[path]
        data = self.data(path)
        dependencies, segments, matrices = set(), set(), []
        if data.lstrip().startswith(b'<'):
            root = ET.fromstring(data)
            require(root.tag == 'DisplayList', f'XML mesh is not a display list: {path}')
            for element in root.iter():
                target = element.get('Path')
                if not target:
                    continue
                if target.startswith('>'):
                    address = int(target[1:], 0)
                    segment, offset = address >> 24, address & 0xFFFFFE
                    if 8 <= segment <= 13:
                        segments.add(segment)
                        if element.tag == 'Matrix':
                            require(offset % 64 == 0, f'unaligned XML matrix in {path}')
                            matrices.append((segment, offset // 64))
                else:
                    dependencies.add(normalize_path(target, self.owner))
        else:
            expect_kind(data, ODLT)
            r = Reader(data, 72)
            ended = False
            while r.offset < len(data):
                a, b = r.take('2I')
                op = a >> 24
                if op in EXPANDED:
                    hi, lo = r.take('2I')
                    value = (hi << 32) | lo
                    if op != 0x33:  # marker metadata is not a dereferenced dependency
                        require(value in self.hashes, f'unresolved DL hash {value:016x} in {path}')
                        dependencies.add(self.hashes[value])
                if op in (0x01, 0xDA, 0xDE, 0xFD):
                    segment = b >> 24
                    if 8 <= segment <= 13:
                        segments.add(segment)
                        if op == 0xDA:
                            # Native extracted commands tag segmented addresses with bit 0;
                            # libultraship SegAddr removes it before calculating the offset.
                            offset = b & 0xFFFFFE
                            require(offset % 64 == 0, f'unaligned matrix in {path}')
                            matrices.append((segment, offset // 64))
                if op == 0xDF or (op == 0xDE and ((a >> 16) & 0xFF) == 1):
                    ended = True
                    break
            require(ended, f'display list has no terminating command: {path}')
        result = dict(dependencies=dependencies, segments=segments, matrices=matrices)
        self.dl_cache[path] = result
        return result

    def closure(self, paths):
        pending = list(paths)
        seen, segments, matrices = set(), set(), []
        while pending:
            path = normalize_path(pending.pop(), self.owner)
            if path in seen:
                continue
            seen.add(path)
            data = self.data(path)
            if resource_kind(data) == ODLT:
                dl = self.dl(path)
                pending.extend(dl['dependencies'] - seen)
                segments.update(dl['segments'])
                matrices.extend(dl['matrices'])
        return seen, segments, matrices


def path_values(value, key=''):
    if isinstance(value, dict):
        for name, child in value.items():
            yield from path_values(child, name)
    elif isinstance(value, list):
        for child in value:
            yield from path_values(child, key)
    elif isinstance(value, str) and value and (key in PATH_FIELDS or key.endswith('Paths') or value.startswith('__OTR__')):
        yield value


def material_segments(resources, path):
    """Read the native material list and its texture-cycle resource dependencies."""
    data = resources.data(path)
    expect_kind(data, 0x4F54414E)
    r = Reader(data)
    count = r.take('I')
    require(0 < count <= 8, 'animated material count exceeds shared renderer capacity')
    segments, paths, types = set(), [], []
    for entry_index in range(count):
        segment, kind = r.take('b'), r.take('h')
        # All catalog material kinds are exercised with their actual shapes in
        # the production renderer sweep. Do not count an unported handler as a
        # valid detailed model merely because its archive data is well formed.
        require(kind in (1, 2, 3, 4), 'material handler is not supported by the shared soul renderer')
        types.append(kind)
        empty = count == 1 and segment == 0 and kind == 6
        require(empty or 1 <= abs(segment) <= 6, 'material segment exceeds shared renderer range')
        require(empty or ((segment < 0) == (entry_index == count - 1)),
                'material list terminator does not match its final entry')
        if segment:
            segments.add(abs(segment) + 7)
        if kind in (0, 1):
            r.skip(4 * (kind + 1))
        elif kind in (2, 3, 4):
            length, frame_count = r.take('H'), r.take('H')
            n = r.take('H'); keys = [r.take('H') for _ in range(n)]
            prim_count = r.take('H'); r.skip(prim_count * 5)
            env_count = r.take('H'); r.skip(env_count * 4)
            require(length > 0 and (kind == 2 or frame_count > 0), 'empty color material animation')
            required = length if kind == 2 else frame_count
            require(prim_count >= required and (env_count == 0 or env_count >= required),
                    'material color vector smaller than frame requirement')
            if kind in (3, 4):
                require(len(keys) >= frame_count, 'material key vector smaller than frame count')
            if kind == 3:
                require(frame_count >= 2, 'material linear interpolation needs two keys')
                require(all(a < b for a, b in zip(keys[:frame_count], keys[1:frame_count])),
                        'material interpolation keys are not strictly increasing')
            if kind == 4:
                require(frame_count <= 50, 'material Lagrange table exceeds native capacity')
                require(len(set(keys[:frame_count])) == frame_count, 'material Lagrange keys are duplicated')
        elif kind == 5:
            length, n = r.take('H'), r.take('I')
            require(n <= len(data) and length > 0, 'invalid material texture cycle')
            paths.extend(r.string() for _ in range(n))
            indices = [r.take('B') for _ in range(length)]
            require(all(i < n for i in indices), 'material texture cycle index out of range')
        else:
            require(kind == 6, f'unsupported material animation type {kind}')
    return segments, paths, types


def validate_recipe(recipe, resources):
    info = recipe.get('info', recipe)
    name = recipe.get('name', recipe.get('id', '<unnamed>'))
    require(math.isfinite(info.get('scale', 0)) and info.get('scale', 0) > 0, 'nonpositive/nonfinite miniature scale')
    paths = set(path_values(info))
    roots, limb_info = set(), None
    bound = {int(s['segment']) for s in info.get('segs', info.get('segments', []))}
    bound.update(int(s['segment'] if isinstance(s, dict) else s) for s in info.get('colorSegments', []))
    if info.get('matrixCount', 0):
        bound.add(info.get('matrixSegment', 0) or 13)
    materials = {}
    for key in ('texAnimPath', 'texAnimPath2'):
        if info.get(key):
            segments, deps, types = material_segments(resources, info[key])
            bound.update(segments); paths.update(deps); materials[info[key]] = types
    if info.get('skelPath'):
        skeleton = parse_skeleton(resources.data(info['skelPath']), info['skelPath'])
        joints = info.get('limbCount', 0)
        require(joints == skeleton['count'] + 1, 'recipe limb count does not match loaded skeleton + root')
        if not resources.alt:
            require(bool(info.get('nonFlexSkeleton', 0)) == (skeleton['type'] == 0), 'recipe skeleton kind differs from native rig')
        limbs = [parse_limb(resources.data(p)) for p in skeleton['paths']]
        paths.update(skeleton['paths'])
        visited, stack = set(), set()
        max_depth = 0
        def visit(index, depth=1):
            nonlocal max_depth
            if index == 255:
                return
            require(0 <= index < len(limbs), 'skeleton link outside limb array')
            require(index not in stack and index not in visited, 'skeleton limb hierarchy cycles/reuses a limb')
            visited.add(index); stack.add(index)
            max_depth = max(max_depth, depth)
            visit(limbs[index]['child'], depth + 1); visit(limbs[index]['sibling'], depth)
            stack.remove(index)
        visit(0)
        require(len(visited) == len(limbs), 'unreachable skeleton limbs')
        overrides = {}
        for override in info.get('limbDLs', info.get('limbOverrides', [])):
            overrides.setdefault(int(override['limbIndex']), []).append(override)
        hidden_mask = int(info.get('hiddenLimbMask', 0))
        matrix_consumers = 0
        for i, limb in enumerate(limbs, 1):
            paths.update(limb['extra_paths'])
            rules = overrides.get(i, [])
            override = rules[0] if rules else {}
            hidden = override.get('hide', 0) or (not override.get('dlPath') and
                     (i == info.get('hiddenLimb', -1) or bool(hidden_mask & (1 << i))))
            effective = None if hidden else override.get('dlPath') or limb['dl']
            if limb['dl'] or effective:
                matrix_consumers += 1
            if not hidden:
                if effective: roots.add(effective)
            for rule in rules:
                if rule.get('postDlPath'): roots.add(rule['postDlPath'])
                if rule.get('postSelf') and limb['dl']: roots.add(limb['dl'])
        if skeleton['type'] == 1:
            bound.add(13)
            require(matrix_consumers <= skeleton['display_count'], 'effective limb matrices exceed native flex palette')
        if info.get('bindPose'):
            require(not info.get('animPath'), 'bind pose also supplies incompatible normal animation')
            if info.get('posePath'):
                animation = parse_player_pose(resources.data(info['posePath']), joints)
            else:
                require(info.get('fixedPoseCount', 0) >= joints, 'skeleton has no authored or native static pose')
                animation = dict(fixed_pose=True)
        else:
            require(info.get('animPath'), 'skeletal recipe has no animation')
            animation = parse_animation(resources.data(info['animPath']), joints)
        limb_info = dict(native_limbs=skeleton['count'], declared_native_limbs=skeleton['declared_count'],
                         joint_count=joints, type=skeleton['type'], animation=animation,
                         hierarchy_depth=max_depth, effective_matrix_consumers=matrix_consumers)
    else:
        require(info.get('parts'), 'recipe has neither skeleton nor static geometry')
    for part in info.get('parts', []):
        if isinstance(part, str):
            roots.add(part)
            continue
        for key in ('dlPath', 'alternateDlPath'):
            if part.get(key): roots.add(part[key])
        roots.update(p for p in part.get('framePaths', []) if p)
    for key in ('postModelDlPath',):
        if info.get(key): roots.add(info[key])
    # Resolve all explicit resources, then all resources reached by actual visible body DLs.
    for path in paths:
        resources.data(path)
    for path in roots:
        require(resource_kind(resources.data(path)) == ODLT, f'visible mesh is not a display list: {path}')
    closure, used, matrices = resources.closure(roots)
    require(roots, 'miniature contains no visible geometry')
    unresolved = used - bound
    require(not unresolved, f'model reads unbound segments {sorted(unresolved)}')
    for segment, index in matrices:
        if segment == (info.get('matrixSegment', 0) or 13) and info.get('matrixCount', 0):
            require(index < info['matrixCount'], f'model matrix {index} exceeds authored palette')
        elif segment == 13 and limb_info and limb_info['type'] == 1:
            require(index < skeleton['display_count'], 'model matrix exceeds native flex palette')
    # Flame and token are independent passes and can legally reuse model segments.
    fallback_paths = [info[k] for k in ('flameDlPath', 'fallbackTokenPath') if info.get(k)]
    for path in fallback_paths:
        require(resource_kind(resources.data(path)) == ODLT, f'fallback mesh is not a display list: {path}')
    fallback_closure, _, _ = resources.closure(fallback_paths)
    all_paths = {normalize_path(p, resources.owner) for p in paths} | closure | fallback_closure
    return dict(owner=resources.owner, name=name, id=recipe.get('id'), category=recipe.get('category'),
                passed=True, skeletal=limb_info, direct_paths=len(paths), visible_body_roots=len(roots),
                body_dependency_resources=len(closure), resources_checked=len(all_paths),
                bound_segments=sorted(bound), used_segments=sorted(used),
                matrix_references=len(matrices), material_animations=len(materials),
                material_handler_types=materials,
                selected_skeleton=resources.selected.get(normalize_path(info['skelPath'], resources.owner)) if info.get('skelPath') else None,
                replacement_body_resources=sum(resources.selected[p]['replacement'] for p in closure))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--oot-archive', type=Path)
    parser.add_argument('--mm-archive', type=Path)
    parser.add_argument('--oot-mod', type=Path, action='append', default=[], help='Read-only overlay, in increasing load priority')
    parser.add_argument('--mm-mod', type=Path, action='append', default=[], help='Read-only overlay, in increasing load priority')
    parser.add_argument('--stormlib', type=Path, help='Optional native StormLib shared library for MPQ inputs')
    parser.add_argument('--recipes', type=Path, action='append', required=True)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    owners = {owner: Resources(owner, path, mods, args.stormlib) for owner, path, mods in
              (('oot', args.oot_archive, args.oot_mod), ('mm', args.mm_archive, args.mm_mod)) if path}
    recipes = []
    for source in args.recipes:
        data = json.loads(source.read_text())
        recipes.extend(data if isinstance(data, list) else data['recipes'])
    results, errors = [], []
    identities = set()
    expected_counts = {('oot', 'enemy'): 43, ('oot', 'npc'): 46, ('mm', 'enemy'): 47, ('mm', 'npc'): 53}
    counts = Counter((r.get('owner'), str(r.get('category', '')).lower()) for r in recipes)
    for owner in owners:
        for category in ('enemy', 'npc'):
            expected = expected_counts[(owner, category)]
            if counts[(owner, category)] != expected:
                errors.append(dict(recipe=f'{owner}:{category}', error=f'catalog has {counts[(owner, category)]} records; expected {expected}'))
    for recipe in recipes:
        owner = recipe.get('owner')
        label = f"{owner}:{recipe.get('name', recipe.get('id'))}"
        try:
            require(owner in owners, f'archive not supplied for {owner}')
            key = (owner, recipe.get('id', recipe.get('name')))
            require(key not in identities, 'duplicate recipe identity')
            identities.add(key)
            results.append(validate_recipe(recipe, owners[owner]))
        except (InvalidResource, KeyError, ValueError, struct.error, ET.ParseError) as error:
            errors.append(dict(recipe=label, error=str(error)))
    hashes = {owner: hashlib.file_digest(Path(resource.path).open('rb'), 'sha256').hexdigest()
              for owner, resource in owners.items()}
    report = dict(check='native-resource-recipe-validation', passed=not errors,
                  scope='Resource paths/types, rigs, animation bounds, body dependencies, segment and matrix closure; no GPU claim.',
                  archive_sha256=hashes, recipe_count=len(recipes), checked=len(results),
                  overlays={o: [dict(name=m.path.name, sha256=hashlib.file_digest(m.path.open('rb'), 'sha256').hexdigest()) for m in r.mods]
                            for o, r in owners.items()},
                  owner_category_counts=dict(Counter(f"{r['owner']}:{r.get('category', 'unknown')}" for r in recipes)),
                  unique_resources={o: len(r.read) for o, r in owners.items()}, results=results, errors=errors)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k:report[k] for k in ('passed', 'recipe_count', 'checked', 'unique_resources', 'errors')}, indent=2))
    return bool(errors)


if __name__ == '__main__':
    sys.exit(main())
