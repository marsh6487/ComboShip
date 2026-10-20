"""Repair Saria private hash references without changing geometry or materials.

Archive::IndexFile indexes literal paths. A hash of a nonexistent base path
cannot select an alt-only entry. Rewrite that hash to the actual alt path.
Native references, existing literal hashes, markers and all other bytes stay
unchanged. --reference supplies full-pack names when repairing its overlay.
"""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from native_assets import o2r as asset

NAMESPACE = "objects/object_sa/NativeNpcV1/"
EXPANDED = {0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42}
REFERENCES = EXPANDED - {0x33}


def repair_records(records, reference_names=()):
    names = set(records) | set(reference_names)
    literal = {asset.crc64(name) for name in names}
    replacements = {asset.crc64(name[4:]): name for name in names
                    if name.startswith("alt/" + NAMESPACE) and name[4:] not in names}
    repaired = dict(records)
    changes = []
    for name, raw in records.items():
        if not name.startswith("alt/" + NAMESPACE) or raw[4:8] != b"TLDO":
            continue
        if raw[0] != 0 or len(raw) < 72 or raw[64] != asset.UCODE_F3DEX2:
            raise ValueError("Expected little-endian F3DEX2 display list: " + name)
        data = bytearray(raw)
        offset = 72
        while offset + 8 <= len(data):
            opcode = struct.unpack_from("<I", data, offset)[0] >> 24
            size = 16 if opcode in EXPANDED else 8
            if offset + size > len(data):
                raise ValueError("Truncated display list: " + name)
            if opcode in REFERENCES:
                high, low = struct.unpack_from("<II", data, offset + 8)
                hashed = high << 32 | low
                target = replacements.get(hashed) if hashed not in literal else None
                if target:
                    actual = asset.crc64(target)
                    struct.pack_into("<II", data, offset + 8, actual >> 32, actual & 0xffffffff)
                    changes.append(dict(resource=name, offset=offset + 8, target=target,
                                        previous_hash=f"{hashed:016x}", actual_hash=f"{actual:016x}"))
            offset += size
            if opcode == 0xDF:
                break
        repaired[name] = bytes(data)
    return repaired, changes


def repair(baseline, output, references=()):
    baseline, output = Path(baseline), Path(output)
    if baseline.resolve() == output.resolve():
        raise ValueError("Keep the baseline unchanged")
    with zipfile.ZipFile(baseline) as archive:
        records = {name: archive.read(name) for name in archive.namelist()}
    reference_names = set()
    for reference in references:
        with zipfile.ZipFile(reference) as archive:
            reference_names.update(archive.namelist())
    repaired, changes = repair_records(records, reference_names)
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w") as archive:
        for name, data in sorted(repaired.items()):
            entry = zipfile.ZipInfo(name, (2026, 10, 20, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(entry, data, compresslevel=6)
    return dict(baseline=str(baseline), baseline_sha256=hashlib.sha256(baseline.read_bytes()).hexdigest(),
                candidate=str(output), candidate_sha256=hashlib.sha256(output.read_bytes()).hexdigest(),
                changed_resources=len({entry["resource"] for entry in changes}), changes=changes,
                runtime_verified=False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--reference", type=Path, action="append", default=[])
    args = parser.parse_args()
    print(json.dumps(repair(args.baseline, args.output, args.reference), indent=2))
