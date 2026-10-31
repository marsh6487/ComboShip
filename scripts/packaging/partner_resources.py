#!/usr/bin/env python3
"""Merge verified native partner resources into newly generated candidate archives.

Run before normalize_candidate_o2r.py and before copying the output. Shared blobs
are authored source; Solo MM's bounded OoT dependencies are pinned compressed text.
All sources and existing ZIP payloads are checked before atomic replacement.
"""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile
import zipfile
import zlib

ROOT = Path(__file__).resolve().parents[2]


def load_manifest(root):
    return json.loads((root / 'tools/partners_native_dependencies/manifest.json').read_text())


def load_shared(root):
    """Decode module-owned text resources and verify their pinned payload identity."""
    expected = load_manifest(root)['shared']
    encoded = json.loads((root / 'combo/assets/partners/shared.json').read_text())
    shared = {}
    try:
        for identity, data in encoded.items():
            module, name = identity.split('/', 1)
            if not module or not name or '..' in Path(identity).parts:
                raise ValueError('Invalid authored resource identity: ' + identity)
            blob = zlib.decompress(base64.b85decode(data))
            if name in shared and shared[name] != blob:
                raise ValueError('Conflicting authored resource: ' + name)
            shared[name] = blob
    except (ValueError, zlib.error) as error:
        raise ValueError('Invalid compressed authored source') from error
    if set(shared) != set(expected):
        raise ValueError('Partner source membership mismatch')
    for name, blob in shared.items():
        if hashlib.sha256(blob).hexdigest() != expected[name]:
            raise ValueError('Partner source hash mismatch: ' + name)
    return shared


def resources(root, game):
    if game not in ('soh', '2ship'):
        raise ValueError('Unknown partner resource game: ' + game)
    manifest = load_manifest(root)
    shared = load_shared(root)
    def verify(blobs, expected):
        if set(blobs) != set(expected):
            raise ValueError('Partner source membership mismatch')
        for name, blob in blobs.items():
            if hashlib.sha256(blob).hexdigest() != expected[name]:
                raise ValueError('Partner source hash mismatch: ' + name)
    if game == 'soh':
        return shared
    encoded = json.loads((root / 'tools/partners_native_dependencies/native.json').read_text())
    if set(encoded) != set(manifest['native']):
        raise ValueError('Native dependency membership mismatch')
    try:
        native = {name: zlib.decompress(base64.b85decode(data)) for name, data in encoded.items()}
    except (ValueError, zlib.error) as error:
        raise ValueError('Invalid compressed native dependency') from error
    verify(native, manifest['native'])
    if shared.keys() & native.keys():
        raise ValueError('Native/shared source collision')
    return {**native, **shared}


def merge(archive, root, game):
    archive, root = Path(archive), Path(root)
    additions = resources(root, game)
    before_hash = hashlib.sha256(archive.read_bytes()).digest()
    with zipfile.ZipFile(archive) as source:
        entries = source.infolist()
        if len({info.filename for info in entries}) != len(entries):
            raise ValueError('Duplicate archive resource names')
        original = {}
        for info in entries:
            blob = source.read(info)
            original[info.filename] = hashlib.sha256(blob).digest()
            if info.filename in additions:
                if blob != additions[info.filename]:
                    raise ValueError('Partner archive collision: ' + info.filename)
                del additions[info.filename]
    if not additions:
        return 0
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(prefix=archive.name + '.partners-', dir=archive.parent, delete=False) as output:
            temporary = Path(output.name)
        # Copy then append retains the existing entry payloads and compressed streams.
        shutil.copyfile(archive, temporary)
        with zipfile.ZipFile(temporary, 'a', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as output:
            for name, blob in sorted(additions.items()):
                info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.create_system = 3
                info.external_attr = 0o100644 << 16
                output.writestr(info, blob, compresslevel=9)
        expected = {**original, **{n: hashlib.sha256(b).digest() for n, b in additions.items()}}
        with zipfile.ZipFile(temporary) as output:
            if len(output.namelist()) != len(expected) or set(output.namelist()) != set(expected):
                raise ValueError('Merged archive membership mismatch')
            for name, digest in expected.items():
                if hashlib.sha256(output.read(name)).digest() != digest:
                    raise ValueError('Merged archive payload mismatch: ' + name)
        if hashlib.sha256(archive.read_bytes()).digest() != before_hash:
            raise ValueError('Archive changed during resource merge')
        os.chmod(temporary, archive.stat().st_mode & 0o777)
        os.replace(temporary, archive)
        return len(additions)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path)
    parser.add_argument('--game', choices=('soh', '2ship'), required=True)
    args = parser.parse_args()
    print(f'Added {merge(args.archive, ROOT, args.game)} native partner resources to {args.archive}')
