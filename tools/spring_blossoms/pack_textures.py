"""Serialize authored transparent art as native scalable OTEX v1 resources.

No art is synthesized: this only resamples the root-provided ImageGen PNGs.
Logical RGBA32 tile: 32x32, 4096 TMEM bytes. Physical art: 256x256, raw8x.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]


def pack(source: Path, size=256):
    image = Image.open(source).convert('RGBA')
    # Pillow's RGBA resize filters premultiplied alpha, avoiding dark fringe.
    # A narrow transparent gutter keeps the authored rounded edge inside the
    # preserved mesh outline and avoids clipping the top of the source image.
    inset = 8 * size // 256
    image = image.resize((size - inset * 2, size - inset * 2), Image.Resampling.LANCZOS)
    canvas = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    canvas.paste(image, (inset, inset))
    image = canvas
    pixels = image.tobytes()
    header = struct.pack('<III', 0, 0x4F544558, 1).ljust(64, b'\0')
    payload = struct.pack('<IIIIffI', 1, size, size, 1, size / 32, size / 32, len(pixels))
    return header + payload + pixels


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=Path(__file__).parent / 'SOURCE')
    parser.add_argument('--check', action='store_true',
                        help='Verify exact serialized bytes and manifest without writing files')
    args = parser.parse_args()
    manifest = {}
    mismatches = []
    for kind in ('blossom', 'petal'):
        source = args.source / (kind + '-source.png')
        data = pack(source)
        for host in ('soh', 'mm'):
            target = ROOT / host / 'assets/custom/objects/object_spring_blossoms' / (kind + '_tex')
            if args.check:
                if not target.is_file() or target.read_bytes() != data:
                    mismatches.append(str(target.relative_to(ROOT)))
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(data)
        manifest[kind] = dict(source=str(source.relative_to(ROOT)),
                              source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                              native_sha256=hashlib.sha256(data).hexdigest(), physical_size=256,
                              logical_size=32, format='RGBA32 raw8x OTEXv1', tmem_bytes=4096,
                              transparent_gutter_pixels=8)
    manifest_path = Path(__file__).parent / 'texture-manifest.json'
    manifest_text = json.dumps(manifest, indent=2) + '\n'
    if args.check:
        if not manifest_path.is_file() or manifest_path.read_text() != manifest_text:
            mismatches.append(str(manifest_path.relative_to(ROOT)))
        if mismatches:
            raise SystemExit('FAIL spring resources differ from authored sources: ' + ', '.join(mismatches))
        print('PASS reproducible spring resources: 4 native OTEX files and exact source/hash manifest')
    else:
        manifest_path.write_text(manifest_text)
        print(json.dumps(manifest, indent=2))


if __name__ == '__main__':
    main()
