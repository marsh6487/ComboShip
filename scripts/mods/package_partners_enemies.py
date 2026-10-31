#!/usr/bin/env python3
"""Build deterministic native companion/enemy resources from tracked donor assets and an OoT base."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import re
from zipfile import ZipFile, ZipInfo, ZIP_DEFLATED
from build_mm_tp_dungeon_bottles import crc64, commands

ROOT = Path(__file__).resolve().parents[2]
PIN = 'abf41181d690643f74ab72926840918b15fbba3e'


def collect(root):
    # Share the native build's decoder and hash validation; authored source stays
    # text-only without changing any resource path or payload in either pack.
    import importlib.util
    source_root = root.resolve().parents[2]
    spec = importlib.util.spec_from_file_location(
        'partner_resources', source_root / 'scripts/packaging/partner_resources.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.load_shared(source_root)


def dependencies(blob, hashes):
    found = set()
    if blob[4:8] == b'TLDO':
        for opcode, w0, w1, key, offset in commands(blob):
            if key is not None:
                if key not in hashes:
                    raise ValueError(f'Unresolved display-list resource {key:016x}')
                found.add(hashes[key])
    # The resource serializers use a u32 byte count before each UTF-8 path. Reading
    # that count avoids treating the next field's printable byte as part of a path.
    for match in re.finditer(rb'(?:__OTR__)?(?:objects|textures|animations)/', blob):
        offset = match.start()
        if offset < 4:
            continue
        size, = struct.unpack_from('<I', blob, offset - 4)
        if not (len(match[0]) <= size <= 1024 and offset + size <= len(blob)):
            continue
        target = blob[offset:offset + size].decode().removeprefix('__OTR__')
        found.add(target)
    return found


def write_pack(path, resources):
    path.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(path, 'w', ZIP_DEFLATED, compresslevel=9) as z:
        for name, data in sorted(resources.items()):
            info = ZipInfo(name, (2026, 10, 31, 0, 0, 0))
            info.compress_type = ZIP_DEFLATED
            z.writestr(info, data)
    with ZipFile(path) as z:
        assert z.testzip() is None
        assert len(z.namelist()) == len(resources)
        assert all(z.read(n) == b for n, b in resources.items())
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build(base, output):
    shared = collect(ROOT / 'combo/assets/partners')
    with ZipFile(base) as z:
        native_names = {n for n in z.namelist() if not n.endswith('/')}
        hashes = {crc64(n): n for n in native_names | shared.keys()}
        # OoT models and icon are used through explicit resource names in the MM port.
        pending = [n for n in native_names if n.startswith(('objects/object_ma1/', 'objects/object_ma2/',
                                                           'objects/object_niw/')) and not n.endswith('.json')]
        pending.append('textures/icon_item_static/gItemIconCojiroTex')
        native = {}
        while pending:
            name = pending.pop()
            if name in native or name in shared:
                continue
            if name not in native_names:
                raise ValueError('Missing native dependency: ' + name)
            blob = native[name] = z.read(name)
            pending.extend(dependencies(blob, hashes) - native.keys() - shared.keys())
        for name, blob in shared.items():
            for target in dependencies(blob, hashes):
                if target not in shared and target not in native:
                    raise ValueError(f'{name}: unpackaged dependency {target}')
    report = {'donor_repository': 'skijer/Shipwright', 'donor_commit': PIN,
              'oot_base_sha256': hashlib.sha256(base.read_bytes()).hexdigest(),
              'runtime': 'Not verified in game', 'packs': {}}
    for game in ('soh', '2ship'):
        resources = shared if game == 'soh' else {**native, **shared}
        path = output / 'mods' / game / 'Native_Partners_Enemies_POC.o2r'
        report['packs'][game] = {'sha256': write_pack(path, resources), 'resources': len(resources)}
    (output / 'partners_enemies_assets.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--oot-base', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(build(args.oot_base, args.output), indent=2))
