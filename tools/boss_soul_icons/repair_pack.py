#!/usr/bin/env python3
"""Correct POC4's HD metadata while preserving every source pixel."""
import argparse
from hashlib import sha256
from pathlib import Path
import struct
from zipfile import ZipFile, ZipInfo, ZIP_DEFLATED

EXPECTED = 'ead9c8dedfbedc1468534aed2f4c12fee8dfcb44c300794d2a683fdc60967e9a'
RESOURCE = 'textures/parameter_static/gBossSoul'


def repair(source, output):
    assert sha256(source.read_bytes()).hexdigest() == EXPECTED, 'Unexpected input archive'
    with ZipFile(source) as archive:
        assert archive.namelist() == [RESOURCE]
        data = archive.read(RESOURCE)
    assert len(data) == 80 + 192 * 192 * 4
    assert struct.unpack_from('<I', data, 8)[0] == 0
    assert struct.unpack_from('<IIII', data, 64) == (1, 192, 192, 4096)
    header = bytearray(data[:64])
    struct.pack_into('<I', header, 8, 1)
    # RAW RGBA data, physical dimensions 192x192; native logical dimensions 32x32.
    candidate = bytes(header) + struct.pack('<IIIIffI', 1, 192, 192, 1, 6.0, 6.0, 147456) + data[80:]
    output.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(output, 'w', ZIP_DEFLATED, compresslevel=9) as archive:
        for name in (RESOURCE, 'alt/' + RESOURCE):
            info = ZipInfo(name, (2026, 11, 4, 0, 0, 0))
            info.compress_type = ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, candidate)
    with ZipFile(output) as archive:
        assert archive.testzip() is None
        for name in archive.namelist():
            assert archive.read(name)[92:] == data[80:], 'Artwork changed'
    print(f'{output}: {sha256(output.read_bytes()).hexdigest()} (base+Alt, pixels unchanged)')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    repair(args.source, args.output)
