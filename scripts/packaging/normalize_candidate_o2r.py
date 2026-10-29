#!/usr/bin/env python3
"""Canonicalize a newly generated candidate port archive; never use on accepted inputs.

Resource payloads, compressed streams, compression methods and entry flags remain
unchanged. Names are sorted, timestamps/permissions are fixed, and volatile ZIP
metadata is removed. Replacement is atomic and follows complete payload verification.
The current port writers produce ZIP32; larger ZIP64 outputs are rejected intact.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import struct
import tempfile
import zipfile

LOCAL = struct.Struct('<IHHHHHIIIHH')
CENTRAL = struct.Struct('<IHHHHHHIIIHHHHHII')
END = struct.Struct('<IHHHHIIH')
LIMIT = 0xFFFFFFFF
STAMP = 33  # DOS date: 1980-01-01, time zero.
# ZIP64 is regenerated as ZIP32 when its actual sizes fit; time/owner fields are volatile.
VOLATILE_EXTRA = {0x0001, 0x000A, 0x000D, 0x5455, 0x5855, 0x6375, 0x7855, 0x7875}


def _hash_file(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def _extra(data):
    result = bytearray()
    at = 0
    while at < len(data):
        if at + 4 > len(data):
            raise ValueError('Malformed ZIP extra field')
        tag, length = struct.unpack_from('<HH', data, at)
        end = at + 4 + length
        if end > len(data):
            raise ValueError('Truncated ZIP extra field')
        if tag not in VOLATILE_EXTRA:
            result.extend(data[at:end])
        at = end
    return bytes(result)


def payload_manifest(path):
    records = []
    with zipfile.ZipFile(path) as archive:
        entries = archive.infolist()
        if len(entries) >= 0xFFFF or len({info.filename for info in entries}) != len(entries):
            raise ValueError('Candidate must have unique ZIP32 entries')
        for info in sorted(entries, key=lambda entry: entry.filename):
            if info.file_size >= LIMIT or info.compress_size >= LIMIT:
                raise ValueError('ZIP64 resource sizes exceed the candidate ZIP32 writer')
            digest = hashlib.sha256()
            with archive.open(info) as resource:
                for block in iter(lambda: resource.read(1024 * 1024), b''):
                    digest.update(block)
            records.append({'name': info.filename, 'size': info.file_size, 'crc32': info.CRC,
                            'compression': info.compress_type, 'flags': info.flag_bits,
                            'sha256': digest.hexdigest()})
    return records


def _manifest_hash(records):
    return hashlib.sha256(json.dumps(records, sort_keys=True, ensure_ascii=False,
                                     separators=(',', ':')).encode('utf8')).hexdigest()


def normalize_candidate_archive(path):
    path = Path(path)
    before_hash = _hash_file(path)
    before = payload_manifest(path)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(prefix=path.name+'.canonical-', dir=path.parent, delete=False) as output:
            temporary = Path(output.name)
            central = []
            with zipfile.ZipFile(path) as archive, path.open('rb') as source:
                for info in sorted(archive.infolist(), key=lambda entry: entry.filename):
                    source.seek(info.header_offset)
                    data = source.read(LOCAL.size)
                    if len(data) != LOCAL.size:
                        raise ValueError('Truncated ZIP local header')
                    signature, needed, flags, method, _, _, _, _, _, name_len, extra_len = LOCAL.unpack(data)
                    if signature != 0x04034B50 or flags != info.flag_bits or method != info.compress_type:
                        raise ValueError('ZIP local/central entry mismatch')
                    name = source.read(name_len)
                    local_extra = _extra(source.read(extra_len))
                    offset = output.tell()
                    if offset >= LIMIT:
                        raise ValueError('Candidate exceeds ZIP32 archive size')
                    descriptor = bool(flags & 8)
                    crc, compressed, size = ((0, 0, 0) if descriptor else
                                              (info.CRC, info.compress_size, info.file_size))
                    output.write(LOCAL.pack(signature, needed, flags, method, 0, STAMP, crc, compressed, size,
                                            len(name), len(local_extra)))
                    output.write(name)
                    output.write(local_extra)
                    remaining = info.compress_size
                    while remaining:
                        block = source.read(min(remaining, 1024 * 1024))
                        if not block:
                            raise ValueError('Truncated ZIP compressed stream')
                        output.write(block)
                        remaining -= len(block)
                    if descriptor:
                        output.write(struct.pack('<IIII', 0x08074B50, info.CRC, info.compress_size, info.file_size))
                    extra = _extra(info.extra)
                    attributes = (0o40755 << 16 | 0x10) if info.is_dir() else (0o100644 << 16)
                    central.append(CENTRAL.pack(0x02014B50, 3 << 8 | max(20, needed & 255), needed, flags,
                                                method, 0, STAMP, info.CRC, info.compress_size, info.file_size,
                                                len(name), len(extra), 0, 0, 0, attributes, offset) + name + extra)
            central_offset = output.tell()
            for entry in central:
                output.write(entry)
            central_size = output.tell() - central_offset
            if output.tell() + END.size >= LIMIT:
                raise ValueError('Candidate exceeds ZIP32 archive size')
            output.write(END.pack(0x06054B50, 0, 0, len(central), len(central), central_size, central_offset, 0))
        after = payload_manifest(temporary)
        if before != after:
            raise ValueError('Candidate resource payload, compression or entry flags changed')
        after_hash = _hash_file(temporary)
        if _hash_file(path) != before_hash:
            raise ValueError('Candidate changed during normalization')
        changed = before_hash != after_hash
        if changed:
            os.chmod(temporary, path.stat().st_mode & 0o777)
            os.replace(temporary, path)
        return {'archive': str(path), 'entries': len(before), 'changed': changed,
                'archive_before': before_hash, 'archive_after': after_hash,
                'payload_manifest_before': _manifest_hash(before), 'payload_manifest_after': _manifest_hash(after)}
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path, help='newly generated candidate .o2r output')
    arguments = parser.parse_args()
    print(json.dumps(normalize_candidate_archive(arguments.archive), sort_keys=True))


if __name__ == '__main__':
    main()
