#!/usr/bin/env python3
"""Independently verify the bounded POC11 -> POC12 pot-identity repair."""
import argparse
import contextlib
import copy
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import re
import struct
import zipfile

ROOM = "scenes/spot01/rooms/0.json"
EXPECTED = {
    "/setups/0/actors/8/pos": ([225, 200, -380], [222, 200, -377]),
    "/setups/0/actors/10/pos": ([285, 200, -353], [284, 200, -356]),
    "/setups/1/actors/7/pos": ([225, 200, -380], [222, 200, -377]),
    "/setups/1/actors/9/pos": ([285, 200, -353], [284, 200, -356]),
}
POTS = (("Near Potion Shop Pot 1", [222, 200, -377], 16643),
        ("Near Potion Shop Pot 2", [255, 200, -366], 17155),
        ("Near Potion Shop Pot 3", [284, 200, -356], 17667))


def sha(data):
    return hashlib.sha256(data).hexdigest()


def merge(base, overlay):
    if not isinstance(base, dict) or not isinstance(overlay, dict) or overlay.get("$replace") is True:
        return copy.deepcopy(overlay)
    result = copy.deepcopy(base)
    for key, value in overlay.items():
        if value is None:
            result.pop(key, None)
        elif isinstance(value, dict) and isinstance(result.get(key), dict):
            result[key] = merge(result[key], value)
        else:
            result[key] = copy.deepcopy(value)
    return result


def delta(before, after, path=""):
    if isinstance(before, dict) and isinstance(after, dict):
        result = []
        for key in sorted(set(before) | set(after)):
            if key not in before or key not in after:
                result.append({"path": path + "/" + key, "before": before.get(key), "after": after.get(key)})
            else:
                result.extend(delta(before[key], after[key], path + "/" + key))
        return result
    return [] if before == after else [{"path": path, "before": before, "after": after}]


def compressed(raw, info):
    local = struct.unpack_from("<IHHHHHIIIHH", raw, info.header_offset)
    if local[0] != 0x04034B50:
        raise ValueError("invalid local ZIP header")
    start = info.header_offset + 30 + local[-2] + local[-1]
    return raw[start:start + info.compress_size]


def identity_key(position):
    # TWO_ACTOR_PARAMS((s16)world.x, (s16)world.z), with integral,
    # in-range positions verified below. No nearest-position matching.
    x, _, z = position
    if any(not isinstance(v, (int, float)) or not -32768 <= v <= 32767 for v in (x, z)):
        raise ValueError("pot identity outside the native s16 coordinate domain")
    return ((int(x) & 0xFFFF) << 16) | (int(z) & 0xFFFF)


def verify(parent_path, candidate_path, base_path, donor_path, dog_verifier):
    parent_raw, candidate_raw = Path(parent_path).read_bytes(), Path(candidate_path).read_bytes()
    errors, manifest, identities = [], [], []
    def check(condition, message):
        if not condition:
            errors.append(message)
    with zipfile.ZipFile(parent_path) as parent, zipfile.ZipFile(candidate_path) as candidate, zipfile.ZipFile(base_path) as base:
        pnames, cnames = parent.namelist(), candidate.namelist()
        check(len(pnames) == len(set(pnames)) and len(cnames) == len(set(cnames)), "duplicate ZIP members")
        check(set(pnames) == set(cnames), "ZIP member membership changed")
        check(parent.testzip() is None and candidate.testzip() is None, "ZIP CRC/integrity failure")
        changed, compressed_changed = [], []
        for name in sorted(set(pnames) & set(cnames)):
            before, after = parent.read(name), candidate.read(name)
            pinfo, cinfo = parent.getinfo(name), candidate.getinfo(name)
            pstream, cstream = compressed(parent_raw, pinfo), compressed(candidate_raw, cinfo)
            if before != after:
                changed.append(name)
            if pstream != cstream:
                compressed_changed.append(name)
            check(pinfo.compress_type == cinfo.compress_type, "compression method changed: " + name)
            manifest.append({"path": name, "before_bytes": len(before), "after_bytes": len(after),
                "before_sha256": sha(before), "after_sha256": sha(after),
                "before_compressed_sha256": sha(pstream), "after_compressed_sha256": sha(cstream)})
        check(changed == [ROOM], "expected exactly one changed room JSON payload, got " + repr(changed))
        check(compressed_changed == [ROOM], "compressed streams outside the room changed")
        before_bytes, after_bytes = parent.read(ROOM), candidate.read(ROOM)
        before_doc, after_doc = json.loads(before_bytes), json.loads(after_bytes)
        changes = delta(before_doc, after_doc)
        expected = [{"path": key, "before": before, "after": after} for key, (before, after) in sorted(EXPECTED.items())]
        check(changes == expected, "semantic changes differ from the four authorized position fields")
        masked = []
        for body, positions in ((before_bytes, ((225, -380), (285, -353))),
                                (after_bytes, ((222, -377), (284, -356)))):
            for number, (x, z) in enumerate(positions, 1):
                pattern = rb"\[\s*" + str(x).encode() + rb"\s*,\s*200\s*,\s*" + str(z).encode() + rb"\s*\]"
                body, count = re.subn(pattern, f"[BOUNDED_POT_{number}]".encode(), body)
                check(count == 2, "unexpected number of target position arrays in room text")
            masked.append(body)
        outside_equal = masked[0] == masked[1]
        check(outside_equal, "room JSON text outside the four position arrays changed")
        native = json.loads(base.read(ROOM))
        before_room, after_room = merge(native, before_doc), merge(native, after_doc)
        for setup, keys in (("0", ("8", "9", "10")), ("1", ("7", "8", "9"))):
            actors = after_room["setups"][setup]["actors"]
            pot_keys = {}
            for key, actor in actors.items():
                if isinstance(actor, dict) and actor.get("id") == 273:
                    pot_keys.setdefault(identity_key(actor["pos"]), []).append(key)
            for actor_key, (label, position, params) in zip(keys, POTS):
                actor = actors[actor_key]
                expected_key = identity_key(position)
                check(actor.get("id") == 273 and actor.get("pos") == position and actor.get("params") == params,
                      f"setup {setup} actor {actor_key}: {label} native identity mismatch")
                matches = pot_keys.get(expected_key, [])
                check(matches == [actor_key], f"setup {setup} {label}: identity-key collision {matches}")
                coincident = [key for key, other in actors.items() if key != actor_key and isinstance(other, dict)
                              and other.get("pos") == position]
                check(not coincident, f"setup {setup} {label}: coincident actor placement {coincident}")
                identities.append({"setup": setup, "time": "child day" if setup == "0" else "child night",
                    "actor_key": actor_key, "check": label, "id": actor.get("id"), "params": actor.get("params"),
                    "position": actor.get("pos"), "identity_key": f"0x{identity_key(actor['pos']):08X}",
                    "expected_identity_key": f"0x{expected_key:08X}", "matching_pot_actor_keys": matches,
                    "coincident_other_actor_keys": coincident})
            middle = keys[1]
            check(before_room["setups"][setup]["actors"][middle] == actors[middle],
                  f"setup {setup}: successful middle Pot 2 changed")
    spec = importlib.util.spec_from_file_location("poc12_original_dog_verifier", dog_verifier)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.BASE, module.DONOR = Path(base_path), Path(donor_path)
    dog_output = io.StringIO()
    with contextlib.redirect_stdout(dog_output):
        dog_status = module.verify(Path(candidate_path))
    check(dog_status == 0, "authored dog-path continuity check failed")
    return {"exit_code": int(bool(errors)), "errors": errors,
        "parent_sha256": sha(parent_raw), "candidate_sha256": sha(candidate_raw),
        "member_count": len(manifest), "changed_members": changed,
        "unchanged_payload_members": len(manifest) - len(changed), "changed_compressed_members": compressed_changed,
        "semantic_delta": changes, "effective_pot_identities": identities,
        "room_text_outside_position_fields_identical": outside_equal,
        "dog_paths": {"exit_code": dog_status, "output": dog_output.getvalue()},
        "member_manifest": manifest,
        "runtime": "not run; static identity, archive preservation and authored-path checks only"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("parent", "candidate", "base", "donor", "dog-verifier", "report"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    report = verify(args.parent, args.candidate, args.base, args.donor, args.dog_verifier)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print("PASS" if report["exit_code"] == 0 else "FAIL", "POC12 pot identity/payload/path checks")
    for error in report["errors"]:
        print(error)
    print(report["dog_paths"]["output"].strip())
    return report["exit_code"]


if __name__ == "__main__":
    raise SystemExit(main())
