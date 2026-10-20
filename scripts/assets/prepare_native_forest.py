#!/usr/bin/env python3
"""Verify geometry-only forest source, then prepare both normal port trees."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/black_market'))
import native_forest

DEFAULT_MANIFEST = ROOT / 'tools/native_forest/manifest.json'


def verified_resources(manifest_path, bundle_path=None):
    manifest_path = Path(manifest_path)
    metadata = json.loads(manifest_path.read_text())
    if metadata.get('schema') != 1 or metadata.get('status') != 'generated':
        raise ValueError('Native forest source is blocked: the immutable native MM archive is still required; '
                         'run tools/black_market/native_forest.py --native-mm with the real donor first')
    bundle = metadata.get('bundle', {})
    name = bundle.get('name', '')
    if not name or Path(name).name != name or not native_forest.safe_resource_path(name):
        raise ValueError('Invalid native forest bundle filename')
    bundle_path = Path(bundle_path) if bundle_path else manifest_path.parent / name
    if not bundle_path.is_file():
        raise ValueError('Missing verified native forest bundle; generate it from the immutable native MM archive')
    if bundle_path.stat().st_size != bundle.get('bytes') or native_forest.file_sha(bundle_path) != bundle.get('sha256'):
        raise ValueError('Native forest bundle hash/size mismatch')
    expected = metadata.get('resources', {})
    if not expected:
        raise ValueError('Native forest source has no resource inventory')
    resources = {}
    # Validate every member before creating an output directory. A corrupt
    # source must never partially replace a previously prepared candidate.
    with zipfile.ZipFile(bundle_path) as archive:
        names = archive.namelist()
        if len(names) != len(set(names)) or set(names) != set(expected):
            raise ValueError('Native forest bundle resource membership mismatch')
        for name in names:
            if not native_forest.safe_resource_path(name):
                raise ValueError(f'Unsafe native forest resource path: {name}')
            entry = expected[name]
            if archive.getinfo(name).file_size != entry['bytes']:
                raise ValueError(f'Native forest resource size mismatch: {name}')
            data = archive.read(name)
            if hashlib.sha256(data).hexdigest() != entry['sha256']:
                raise ValueError(f'Native forest resource hash mismatch: {name}')
            resources[name] = data
    native_forest.validate_resources(resources)
    return resources


def prepare(output_root, manifest_path=DEFAULT_MANIFEST, bundle_path=None):
    resources = verified_resources(manifest_path, bundle_path)
    restored = 0
    for game in ('soh', 'mm'):
        root = Path(output_root) / game / 'assets/custom'
        for name, data in resources.items():
            target = root / name
            if target.is_file() and target.stat().st_size == len(data) and native_forest.file_sha(target) == native_forest.sha(data):
                continue
            target.parent.mkdir(parents=True, exist_ok=True)
            temporary = None
            try:
                with tempfile.NamedTemporaryFile(dir=target.parent, prefix='.native-forest-', delete=False) as stream:
                    temporary = Path(stream.name)
                    stream.write(data)
                os.replace(temporary, target)
                restored += 1
            finally:
                if temporary:
                    temporary.unlink(missing_ok=True)
    print(f'PASS native forest source: {len(resources)} geometry/framing resources in both port trees, '
          f'no custom textures ({restored} restored)')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-root', type=Path, default=ROOT)
    parser.add_argument('--manifest', type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument('--bundle', type=Path)
    args = parser.parse_args()
    try:
        prepare(args.output_root, args.manifest, args.bundle)
    except (ValueError, OSError, zipfile.BadZipFile) as error:
        parser.exit(1, f'{error}\n')


if __name__ == '__main__':
    main()
