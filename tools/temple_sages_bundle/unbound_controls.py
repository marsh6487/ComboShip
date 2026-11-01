"""Export only the built-in Temple's verified legacy controls to Unbound.

Authoring conversion only: ordinary builds restore the committed source pool.
Render resources and native collision/audio fields remain unchanged.
"""
import json
import struct

from temple_sages_pool import safe_path

ROOM = 'scenes/shared/tokinoma_scene/tokinoma_room_0'
COLLISION = 'scenes/shared/tokinoma_scene/tokinoma_sceneCollisionHeader_0032F8'
PREFIX = 'scenes/tokinoma/'
RETIRED_ALT_SCENE = 'alt/' + PREFIX + 'scene.json'


class Reader:
    def __init__(self, data, kind):
        if len(data) < 68 or data[4:8] != kind:
            raise ValueError('Unexpected Temple control resource')
        self.data, self.offset = data, 64

    def read(self, fmt):
        size = struct.calcsize('<' + fmt)
        if self.offset + size > len(self.data):
            raise ValueError('Truncated Temple control resource')
        values = struct.unpack_from('<' + fmt, self.data, self.offset)
        self.offset += size
        return values[0] if len(values) == 1 else values

    def count(self, maximum=65536):
        count = self.read('I')
        if count > maximum:
            raise ValueError('Temple control array exceeds native bounds')
        return count

    def path(self):
        size = self.count(1024)
        if self.offset + size > len(self.data):
            raise ValueError('Truncated Temple resource path')
        path = self.data[self.offset:self.offset + size].decode('utf-8')
        self.offset += size
        if path and not safe_path(path):
            raise ValueError('Invalid Temple resource path')
        return path

    def finish(self):
        if self.offset != len(self.data):
            raise ValueError('Unexpected trailing Temple control data')


def indexed(values):
    return {str(i): value for i, value in enumerate(values)}


def room_setup(data):
    reader = Reader(data, b'MORO')
    result, alternate = {}, []
    seen = set()
    for _ in range(reader.count(32)):
        command = reader.read('I')
        if command in seen:
            raise ValueError('Duplicate Temple room command')
        seen.add(command)
        if command == 0x18:
            alternate = [reader.path() for _ in range(reader.count(32))]
        elif command == 1:
            actors = []
            for _ in range(reader.count(512)):
                row = reader.read('H3h3hH')
                actors.append({'id': row[0], 'pos': list(row[1:4]), 'rot': list(row[4:7]),
                               'params': row[7] if row[7] < 32768 else row[7] - 65536})
            result['actors'] = indexed(actors)
        elif command == 11:
            result['objects'] = indexed([reader.read('h') for _ in range(reader.count(256))])
        elif command == 8:
            flags, flags2 = reader.read('bi')
            result['behavior'] = {'gameplayFlags': flags, 'gameplayFlags2': flags2}
        elif command == 16:
            hour, minute, increment = reader.read('BBb')
            result['time'] = {'hour': hour, 'minute': minute, 'increment': increment}
        elif command == 18:
            skybox, sun = reader.read('BB')
            result['skyboxModifier'] = {'skyboxDisabled': skybox, 'sunMoonDisabled': sun}
        elif command == 22:
            result['echo'] = reader.read('B')
        elif command == 10:
            _, kind, count = reader.read('BBB')
            if kind != 0:
                raise ValueError('Expected the built-in Temple polygon mesh')
            entries = []
            for _ in range(count):
                reader.read('B')  # Legacy unused polygon-type byte.
                entries.append({'opa': reader.path() or None, 'xlu': reader.path() or None})
            result['mesh'] = {'type': kind, 'entries': indexed(entries)}
        elif command != 20:
            raise ValueError('Unsupported Temple room command: ' + hex(command))
    reader.finish()
    if not {1, 8, 10, 11, 16, 18, 20, 22} <= seen:
        raise ValueError('Incomplete Temple room setup')
    return result, alternate


def collision_control(data):
    reader = Reader(data, b'LOCO')
    bounds = reader.read('6h')
    vertices = [reader.read('3h') for _ in range(reader.count(8192))]
    polygons = [reader.read('4H4h') for _ in range(reader.count(65535))]
    surface_fields = (
        (('material', 0, 4), ('floorEffect', 4, 2), ('lightSetting', 6, 5), ('echo', 11, 6),
         ('canHookshot', 17, 1), ('conveyorSpeed', 18, 3), ('conveyorDirection', 21, 6), ('isWallDamage', 27, 1)),
        (('camera', 0, 8), ('exit', 8, 5), ('floorType', 13, 5), ('wallFlags', 18, 3),
         ('wallType', 21, 5), ('floorProperty', 26, 4), ('isSoft', 30, 1), ('isHorseBlocked', 31, 1)))
    surfaces = []
    for _ in range(reader.count()):
        data1, data0 = reader.read('II')
        if data1 & 0xF0000000:
            raise ValueError('Unsupported legacy surface padding bits')
        surface = {name: (word >> shift) & ((1 << width) - 1)
                   for word, fields in zip((data1, data0), surface_fields) for name, shift, width in fields}
        surfaces.append(surface)
    cameras = []
    for _ in range(reader.count(256)):
        kind, count, index = reader.read('Hhi')
        cameras.append({'sType': kind, 'count': count, 'positionIndex': index})
    positions = [list(reader.read('3h')) for _ in range(reader.count(8192))]
    if reader.count() != 0:
        raise ValueError('The built-in Temple has no water boxes; explicit reauthoring is required')
    reader.finish()
    bulk = bytearray(b''.join(struct.pack('<3i', *vertex) for vertex in vertices))
    for poly in polygons:
        if poly[0] >= len(surfaces) or any((word & 0x1FFF) >= len(vertices) for word in poly[1:4]):
            raise ValueError('Invalid native Temple collision reference')
        indices = [(word & 0x1FFF) | ((word >> 13) << 29) for word in poly[1:4]]
        bulk.extend(struct.pack('<HHIIIhhhhi', poly[0], 0, *indices, *poly[4:7], 0, poly[7]))
    document = {'$schema': 'unbound/collision/3', '$replace': True,
                'bounds': {'min': list(bounds[:3]), 'max': list(bounds[3:])},
                'bulk': {'file': PREFIX + 'collision.bin', 'vertices': len(vertices), 'polys': len(polygons)},
                'surfaceTypes': indexed(surfaces), 'cameras': indexed(cameras),
                'cameraPositions': indexed(positions), 'waterBoxes': {}}
    return document, bytes(bulk)


def apply(resources):
    result = dict(resources)
    # Partial scene documents merge only at their exact path. Inherit the
    # clean canonical scene's spawn/story data when Alt Assets is enabled.
    result.pop(RETIRED_ALT_SCENE, None)
    for prefix in ('', 'alt/'):
        setup, alternate = room_setup(resources[prefix + ROOM])
        setups = {'0': setup}
        for index, path in enumerate(alternate):
            if path:
                setups[str(index + 1)], nested = room_setup(resources[prefix + path])
                if nested:
                    raise ValueError('Unexpected nested Temple alternate header')
        collision, bulk = collision_control(resources[prefix + COLLISION])
        documents = {'scene.json': {'$schema': 'unbound/scene/1', 'collision': PREFIX + 'collision.json',
                                    'rooms': {'0': PREFIX + 'rooms/0.json'}},
                     'rooms/0.json': {'$schema': 'unbound/room/1', '$replace': True, 'setups': setups},
                     'collision.json': collision}
        for name, document in documents.items():
            if prefix and name == 'scene.json':
                continue
            result[prefix + PREFIX + name] = (json.dumps(document, indent=2) + '\n').encode()
        result[prefix + PREFIX + 'collision.bin'] = bulk
    return result


if __name__ == '__main__':
    import argparse
    from pathlib import Path
    import temple_sages_pool as pool

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bundle', type=Path, default=Path(__file__).resolve().parent)
    args = parser.parse_args()
    manifest = args.bundle / 'manifest.json'
    metadata = json.loads(manifest.read_text())
    original = pool.decode(args.bundle, metadata['bundle'])
    resources = apply(original)
    metadata['bundle'] = pool.pack(resources, args.bundle)
    metadata['resources'] = {name: {'bytes': len(data), 'sha256': pool.sha(data)}
                             for name, data in sorted(resources.items())}
    metadata['unbound_controls'] = 'exact-native-room-collision-v1'
    metadata['retired_resources'] = sorted(set(metadata.get('retired_resources', [])) | {RETIRED_ALT_SCENE})
    manifest.write_text(json.dumps(metadata, indent=2, sort_keys=True) + '\n')
    print(f'PASS native Unbound chamber controls: {len(resources)} resources')
