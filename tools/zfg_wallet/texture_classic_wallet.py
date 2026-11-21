#!/usr/bin/env python3
"""Author compact intensity materials for the native ZFG pouch UVs."""

import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import zipfile


ROOT = "objects/combo_zfg_wallet/classic/"
LOGICAL_SIZE = 64
SIZE = 128
HEADER = struct.Struct("<IIIIffI")

# Lower hem projected into the stock front and side UVs. Keeping the stitches
# on this curve makes the seam follow the pouch instead of cutting across it.
HEM = ((20, 55), (28.5, 42), (32, 39.6), (38.4, 32.9), (43.7, 30),
       (49.3, 30.2), (52, 29.2), (59.4, 33.3))


def wave(phase, period):
    """Repeat a softly beveled thread profile without a tile-edge discontinuity."""
    phase %= period
    half = period // 2
    return (half - abs(phase - half)) * 256 // half


def hem_segments():
    distance = 0
    for (x, y), (end_x, end_y) in zip(HEM, HEM[1:]):
        dx, dy = end_x - x, end_y - y
        length = math.hypot(dx, dy)
        yield x, y, dx, dy, length, distance
        distance += length


HEM_SEGMENTS = tuple(hem_segments())


def hem_at(x, y, tone):
    nearest = None
    for ox, oy, dx, dy, length, distance in HEM_SEGMENTS:
        leg = max(0, min(1, ((x - ox) * dx + (y - oy) * dy) / (length * length)))
        ex, ey = x - ox - dx * leg, y - oy - dy * leg
        gap = ex * ex + ey * ey
        if nearest is None or gap < nearest[0]:
            nearest = gap, (dx * ey - dy * ex) / length, distance + leg * length
    gap, side, along = nearest
    if gap > 2.4 * 2.4:
        return tone
    # A recessed channel and a raised lip are wide enough to read on the small
    # model. Broken diagonal highlights give the stitches their own rhythm.
    tone -= 77 * max(0, 1 - abs(side + .28) / .78)
    tone += 38 * max(0, 1 - abs(side - 1.54) / .82)
    thread = max(0, 1 - abs(side - .72) / .86)
    stitch = max(0, 1 - abs((along + .6 * side) % 4.8 - 2.4) / 2)
    return int(tone + 130 * thread * stitch)


def cloth(x, y):
    # Broad directional bunching under the cord and narrower herringbone yarns.
    fold = wave(3 * x - y + wave(x + 2 * y, 128) // 9, 64)
    chevron = abs((x % 32) - 16)
    yarn = wave(y + chevron, 16)
    warp = wave(x * 2, 16)
    cross = wave(2 * y - x, 16)
    tone = 112 + fold * 72 // 256 + yarn * 53 // 256 + min(warp, cross) * 28 // 256
    return hem_at((x + .5) / 2, (y + .5) / 2, tone)


def cord(x, y):
    # Opposing diagonal strands alternate over and under, with dark channels
    # between them and a narrower bright ridge highlighting the braid.
    left = wave(x + 2 * y, 32)
    right = wave(x - 2 * y + 16, 32)
    top = max(left, right)
    ridge = max(0, (top - 168) * 256 // 88)
    return 86 + top * 110 // 256 + ridge * 42 // 256 + wave(y + 12, 64) * 12 // 256


def badge(x, y):
    if y < 70:
        # The outer mount unwraps as a strip. Dark valleys and hand-cut notches
        # separate the rim from both the gold braid and the center stone.
        edge = wave(y - 12, 64)
        notch = max(0, wave(x + y, 16) - 128)
        return 107 + edge * 116 // 256 - notch * edge // 1536
    # The inner stone now uses a wider part of the atlas. Layer a clean satin
    # reflection across its facets, with the lower half reading as deep amber.
    sheen = max(0, wave(x + 2 * y + 22, 128) - 135)
    height = max(0, min(53, 123 - y))
    return 126 + height * 101 // 53 + sheen * 60 // 121


def texture(tone):
    pixels = bytearray()
    for y in range(SIZE):
        for x in range(SIZE):
            gray = max(0, min(255, (tone(x, y) + 1) // 3 * 3))
            pixels.extend((gray, gray, gray, 255))

    # OTEX v1 RAW. The stock lists address a logical 64×64 I8 tile. Each
    # logical byte maps to two four-byte pixels in each of two physical rows.
    resource = struct.pack("<B3xIIQ44x", 0, 0x4F544558, 1, 0xDEADBEEFDEADBEEF)
    return resource + HEADER.pack(1, SIZE, SIZE, 1, SIZE * 4 / LOGICAL_SIZE,
                                  SIZE / LOGICAL_SIZE, len(pixels)) + pixels


def resources():
    return {ROOT + name: texture(tone) for name, tone in
            (("WalletTex", cloth), ("WalletCordTex", cord), ("WalletBadgeTex", badge))}


def verify(assets):
    for name, data in assets.items():
        assert name.startswith(ROOT) and data[4:12] == b"XETO\x01\x00\x00\x00"
        kind, width, height, flags, hscale, vscale, count = HEADER.unpack_from(data, 64)
        assert (kind, width, height, flags, hscale, vscale) == (1, SIZE, SIZE, 1, 8, 2)
        assert LOGICAL_SIZE * hscale == width * 4 and LOGICAL_SIZE * vscale == height
        pixels = memoryview(data)[64 + HEADER.size:]
        assert count == len(pixels) == width * height * 4
        assert pixels[0::4] == pixels[1::4] == pixels[2::4] and all(a == 255 for a in pixels[3::4])
        assert max(pixels[0::4]) - min(pixels[0::4]) >= 40
    assert len(set(assets.values())) == 3


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="Output .o2r with only the private texture resources")
    args = parser.parse_args()
    assets = resources()
    verify(assets)
    with zipfile.ZipFile(args.output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(assets.items()):
            entry = zipfile.ZipInfo(name, (2026, 11, 21, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            entry.external_attr = 0o100644 << 16
            archive.writestr(entry, data, compresslevel=9)
    with zipfile.ZipFile(args.output) as archive:
        assert archive.testzip() is None and set(archive.namelist()) == set(assets)
        assert all(archive.read(name) == data for name, data in assets.items())
        print(json.dumps({
            "archiveBytes": args.output.stat().st_size,
            "archiveSha256": hashlib.sha256(args.output.read_bytes()).hexdigest(),
            "members": {name: {"compressedBytes": archive.getinfo(name).compress_size,
                                "sha256": hashlib.sha256(data).hexdigest()} for name, data in assets.items()},
        }, indent=2))


if __name__ == "__main__":
    main()
