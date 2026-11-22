"""Author the ZFG colorways in the TP pouch's existing atlas coordinates."""

import struct


SOURCE_SIZE = (256, 512)
SIZE = (128, 256)
LOGICAL_SIZE = (32, 64)
HEADER = struct.Struct("<IIIIffI")

CLOTH = (
    (16, (50, 9, 22)),
    (72, (112, 23, 43)),
    (124, (156, 42, 61)),
    (198, (205, 83, 100)),
)
LEATHER = (
    (12, (26, 9, 13)),
    (52, (70, 26, 34)),
    (104, (124, 49, 52)),
    (174, (160, 78, 68)),
)
GOLD = (
    (10, (48, 23, 11)),
    (43, (115, 62, 24)),
    (83, (179, 116, 48)),
    (132, (249, 200, 110)),
    (203, (255, 235, 161)),
)
BADGE = (
    (105, (151, 101, 35)),
    (174, (242, 220, 137)),
    (245, (255, 245, 198)),
)


def shade(stops, tone):
    # Small material-specific ramps keep the raw RGBA upload compact without
    # putting visible dither into the smoothly shaded folds.
    tone = max(stops[0][0], min(stops[-1][0], (tone + 2) // 4 * 4))
    for (low, dark), (high, light) in zip(stops, stops[1:]):
        if tone <= high:
            return tuple(lo + (tone - low) * (hi - lo) // (high - low) for lo, hi in zip(dark, light))
    raise AssertionError(tone)


def color(x, y, red, green, blue):
    donor_x = x * 2
    donor_y = y * 2
    if donor_y < 320 and (donor_y >= 8 or green > red + 8):
        # Crossed 2x2 threads with a diagonal repeat add a broad twill glint
        # while the source luma carries the embroidered folds and seams.
        warp = (0, 5, 3, -4)
        weft = (-3, 2, 4, 0)
        thread = warp[(x + y // 2) % 4] + weft[(y - x // 2) % 4]
        tone = (2 * red + 5 * green + 3 * blue) // 10 + thread
        return shade(CLOTH, tone)

    tone = (red + 2 * green + blue) // 4
    if donor_y >= 469 and donor_x < 109 and red > 155 and red - green > 33:
        return shade(BADGE, (2 * red + green) // 3)
    if donor_y >= 413 and (red > green + 11 or donor_y >= 462):
        # Alternating broad highlights break up the polished gold on the cord.
        relief = (0, 3, 6, 3, -1, -2)[(x * 2 - y) % 6]
        return shade(GOLD, tone + relief)

    grain = (0, 3, 1, -2, -1, 1, -1)[(x + 3 * y) % 7]
    return shade(LEATHER, tone + grain)


def author_atlas(source):
    """Generate deterministic, opaque RGBA8 art inside the native raw texture."""
    kind, width, height, flags, hscale, vscale, length = HEADER.unpack_from(source, 64)
    pixels = memoryview(source)[64 + HEADER.size:]
    assert source[4:8] == b"XETO" and struct.unpack_from("<I", source, 8)[0] == 1
    assert (kind, width, height, flags, hscale, vscale) == (2, *SOURCE_SIZE, 1, 16, 8)
    assert length == len(pixels) == width * height * 4
    assert all(alpha == 255 for alpha in pixels[3::4])
    out = bytearray()
    for y in range(SIZE[1]):
        for x in range(SIZE[0]):
            offset = (y * 2 * width + x * 2) * 4
            offsets = (offset, offset + 4, offset + width * 4, offset + width * 4 + 4)
            rgb = tuple((sum(pixels[i + ch] for i in offsets) + 2) // 4 for ch in range(3))
            out.extend((*color(x, y, *rgb), 255))
    return source[:64] + HEADER.pack(kind, *SIZE, flags, 2 * SIZE[0] / LOGICAL_SIZE[0],
                                    SIZE[1] / LOGICAL_SIZE[1], len(out)) + out


def verify_atlas(data):
    kind, width, height, flags, hscale, vscale, length = HEADER.unpack_from(data, 64)
    pixels = data[64 + HEADER.size:]
    assert data[4:8] == b"XETO" and struct.unpack_from("<I", data, 8)[0] == 1
    assert (kind, width, height, flags, hscale, vscale) == (2, *SIZE, 1, 8, 4)
    logical_row_bytes = LOGICAL_SIZE[0] * 2
    # These are the dimensions tested by ImportTextureRaw before the direct upload.
    assert logical_row_bytes * hscale == width * 4 and LOGICAL_SIZE[1] * vscale == height
    assert length == len(pixels) == width * height * 4
    assert all(alpha == 255 for alpha in pixels[3::4])
    return {"width": width, "height": height, "logicalSize": LOGICAL_SIZE, "rgba8Bytes": length,
            "hByteScale": hscale, "vPixelScale": vscale}
