#!/usr/bin/env python3
"""Restore exact cumulative runtime/authoring assets from bounded source parts."""
import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'tools/cumulative_assets'
BUNDLE = SOURCE / 'bundled'
sys.path.insert(0, str(SOURCE))
from cumulative_source_pool import decode, pack, sha, write_files


def inventory():
    entries = json.loads((SOURCE / 'source_files.json').read_text())['files']
    result = {entry['path']: entry for entry in entries}
    if len(result) != len(entries):
        raise ValueError('duplicate cumulative source identity')
    return result


def verified_files(bundle=BUNDLE):
    return decode(bundle, inventory())


def prepare(output_root=ROOT, bundle=BUNDLE):
    files = verified_files(bundle)
    count = write_files(output_root, files)
    print(f'PASS exact cumulative assets ({len(files)} paths; {count} restored)')
    return count


def pack_source():
    files = {}
    for name, entry in inventory().items():
        data = (ROOT / name).read_bytes()
        if len(data) != entry['bytes'] or sha(data) != entry['sha256']:
            raise ValueError('Refusing changed cumulative source bytes: ' + name)
        files[name] = data
    manifest = pack(files, BUNDLE)
    print(f'Packed {len(files)} exact cumulative files in {len(manifest["parts"])} bounded text parts')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pack', action='store_true', help='author parts only from pinned source bytes')
    parser.add_argument('--output-root', type=Path, default=ROOT)
    parser.add_argument('--bundle', type=Path, default=BUNDLE)
    args = parser.parse_args()
    try:
        if args.pack:
            pack_source()
        else:
            prepare(args.output_root, args.bundle)
    except (ValueError, OSError) as error:
        parser.exit(1, str(error) + '\n')
