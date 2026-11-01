#!/usr/bin/env python3
"""Create isolated Kakariko POC12 from the pinned POC11; never edit the input.

Only four pos fields are patched, retaining all other JSON bytes. Unchanged ZIP
members retain their exact compressed streams. Output ZIP metadata is canonical.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import struct
import tempfile
import os
import zipfile
import zlib

PARENT_SHA256 = "4d3059750f71cfb907a1311990b88764b88f925ab5444a7a91a5d5116b64e558"
ROOM = "scenes/spot01/rooms/0.json"
EDITS = (("0", "8", [225, 200, -380], [222, 200, -377]),
         ("0", "10", [285, 200, -353], [284, 200, -356]),
         ("1", "7", [225, 200, -380], [222, 200, -377]),
         ("1", "9", [285, 200, -353], [284, 200, -356]))
LOCAL = struct.Struct("<IHHHHHIIIHH")
CENTRAL = struct.Struct("<IHHHHHHIIIHHHHHII")
END = struct.Struct("<IHHHHIIH")


def sha(data):
    return hashlib.sha256(data).hexdigest()


def encoded_entry(data, method=8):
    if method == 8:
        compressor = zlib.compressobj(9, zlib.DEFLATED, -15)
        packed = compressor.compress(data) + compressor.flush()
    elif method == 0:
        packed = data
    else:
        raise ValueError("only ZIP stored/deflated entries are supported")
    return {"size": len(data), "crc": zlib.crc32(data) & 0xFFFFFFFF, "method": method, "packed": packed}


def canonical_zip(path, entries):
    """Write ZIP32 with deterministic names/metadata and supplied raw streams."""
    if len(entries) >= 65535:
        raise ValueError("ZIP32 member limit exceeded")
    records = []
    with Path(path).open("wb") as output:
        for name, entry in sorted(entries.items()):
            name_bytes = name.encode("utf8")
            packed = entry["packed"]
            offset = output.tell()
            if max(offset, len(packed), entry["size"]) >= 0xFFFFFFFF or len(name_bytes) > 65535:
                raise ValueError("ZIP32 entry limit exceeded")
            # 1980-01-01 00:00:00, UTF-8 names, regular 0644 files, no extras/comments.
            output.write(LOCAL.pack(0x04034B50, 20, 0x800, entry["method"], 0, 33,
                                    entry["crc"], len(packed), entry["size"], len(name_bytes), 0))
            output.write(name_bytes)
            output.write(packed)
            records.append(CENTRAL.pack(0x02014B50, 3 << 8 | 20, 20, 0x800, entry["method"], 0, 33,
                                        entry["crc"], len(packed), entry["size"], len(name_bytes), 0, 0, 0, 0,
                                        0o100644 << 16, offset) + name_bytes)
        directory = output.tell()
        for record in records:
            output.write(record)
        directory_size = output.tell() - directory
        if output.tell() + END.size >= 0xFFFFFFFF:
            raise ValueError("ZIP32 archive limit exceeded")
        output.write(END.pack(0x06054B50, 0, 0, len(records), len(records), directory_size, directory, 0))


def repair(parent_path, output_path):
    parent_path, output_path = Path(parent_path).resolve(), Path(output_path).resolve()
    if parent_path == output_path:
        raise ValueError("output must be a separate candidate")
    raw = parent_path.read_bytes()
    if sha(raw) != PARENT_SHA256:
        raise ValueError("input is not the pinned, unchanged POC11")
    entries = {}
    with zipfile.ZipFile(parent_path) as source:
        if len(set(source.namelist())) != len(source.namelist()) or source.testzip() is not None:
            raise ValueError("invalid POC11 archive")
        original = source.read(ROOM)
        before = json.loads(original)
        expected = copy.deepcopy(before)
        for setup, key, old, new in EDITS:
            actor = expected["setups"][setup]["actors"][key]
            if actor.get("pos") != old:
                raise ValueError("unexpected POC11 position")
            actor["pos"] = new
        text = original.decode("utf8")
        for old_x, old_z, new_x, new_z in ((225, -380, 222, -377), (285, -353, 284, -356)):
            pattern = re.compile(r"(\[\s*)" + str(old_x) + r"(\s*,\s*)200(\s*,\s*)" + str(old_z) + r"(\s*\])")
            text, count = pattern.subn(lambda m: m[1] + str(new_x) + m[2] + "200" + m[3] + str(new_z) + m[4], text)
            if count != 2:
                raise ValueError("unexpected count of bounded position replacements")
        modified = text.encode("utf8")
        if json.loads(modified) != expected or len(modified) != len(original):
            raise ValueError("repair changed more than the four allowed position fields")
        for info in source.infolist():
            if info.flag_bits & 1 or info.compress_type not in (0, 8):
                raise ValueError("unsupported encrypted/compression mode")
            if info.filename == ROOM:
                entries[ROOM] = encoded_entry(modified, info.compress_type)
                continue
            header = LOCAL.unpack_from(raw, info.header_offset)
            if header[0] != 0x04034B50:
                raise ValueError("invalid local header")
            start = info.header_offset + LOCAL.size + header[-2] + header[-1]
            packed = raw[start:start + info.compress_size]
            if len(packed) != info.compress_size:
                raise ValueError("truncated compressed stream")
            entries[info.filename] = {"size": info.file_size, "crc": info.CRC,
                                      "method": info.compress_type, "packed": packed}
    output_path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=output_path.parent, prefix=".poc12-", delete=False) as stream:
            temporary = Path(stream.name)
        canonical_zip(temporary, entries)
        with zipfile.ZipFile(temporary) as candidate:
            if candidate.testzip() is not None or set(candidate.namelist()) != set(entries):
                raise ValueError("candidate integrity/membership failure")
        if sha(parent_path.read_bytes()) != PARENT_SHA256:
            raise ValueError("source changed while building candidate")
        os.replace(temporary, output_path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    return {"exit_code": 0, "parent_sha256": PARENT_SHA256,
            "candidate_sha256": sha(output_path.read_bytes()), "member_count": len(entries),
            "room_before_sha256": sha(original), "room_after_sha256": sha(modified),
            "field_delta": [{"path": f"/setups/{setup}/actors/{key}/pos", "before": old, "after": new}
                            for setup, key, old, new in EDITS],
            "unchanged_compressed_members": len(entries) - 1,
            "output_metadata": "sorted UTF-8 names; 1980-01-01; regular 0644; ZIP32; no extras/comments",
            "zlib_build_version": zlib.ZLIB_VERSION, "zlib_runtime_version": zlib.ZLIB_RUNTIME_VERSION}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--parent", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    report = repair(args.parent, args.output)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print("PASS POC12 deterministic bounded repair", report["candidate_sha256"])


if __name__ == "__main__":
    main()
