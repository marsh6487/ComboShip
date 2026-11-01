#!/usr/bin/env python3
"""Compare the loaded dogs' routes with the original authored Kakariko paths."""
import copy
import json
import struct
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "kak_base/oot-unbound.o2r"
DONOR = ROOT / "kak_audit/Kakariko_POC9_Fences_Restored.prelude.o2r"
SCENE = "scenes/spot01/scene.json"
ROOM = "scenes/spot01/rooms/0.json"
SETUP_DONORS = {"0": "0003A4", "1": "009CC4", "2": "00951C"}


def merge(base, overlay):
    if not isinstance(base, dict) or not isinstance(overlay, dict) or overlay.get("$replace") is True:
        return copy.deepcopy(overlay)
    result = copy.deepcopy(base)
    for key, val in overlay.items():
        if val is None:
            result.pop(key, None)
        elif isinstance(val, dict) and isinstance(result.get(key), dict):
            result[key] = merge(result[key], val)
        else:
            result[key] = copy.deepcopy(val)
    return result


def load_json(base, candidate, name):
    original = json.loads(base.read(name))
    return merge(original, json.loads(candidate.read(name))) if name in candidate.namelist() else original


def donor_paths(archive, suffix):
    name = f"alt/scenes/shared/spot01_scene/spot01_scenePathwayList_{suffix}"
    blob = archive.read(name)
    if blob[4:8] != b"HTPO":
        raise ValueError(f"unexpected donor resource: {name}")
    pos = 64
    count, = struct.unpack_from("<I", blob, pos)
    pos += 4
    result = []
    for _ in range(count):
        points, = struct.unpack_from("<I", blob, pos)
        pos += 4
        result.append([list(struct.unpack_from("<hhh", blob, pos + 6 * i)) for i in range(points)])
        pos += points * 6
    if pos != len(blob):
        raise ValueError(f"unexpected trailing bytes in {name}")
    return result


def verify(candidate_path):
    errors = []
    with zipfile.ZipFile(BASE) as base, zipfile.ZipFile(DONOR) as donor, zipfile.ZipFile(candidate_path) as candidate:
        scene = load_json(base, candidate, SCENE)
        room = load_json(base, candidate, ROOM)
        for setup, suffix in SETUP_DONORS.items():
            actual = []
            for path_name in scene["setups"][setup]["paths"]:
                doc = load_json(base, candidate, path_name)
                paths = doc["paths"]
                actual.extend(paths[k]["points"] for k in sorted(paths, key=int) if not k.startswith("$"))
            expected = donor_paths(donor, suffix)
            dogs = [(k, a) for k, a in room["setups"][setup]["actors"].items()
                    if isinstance(a, dict) and a.get("id") == 0x19B]
            if len(dogs) != 2:
                errors.append(f"setup {setup}: expected two dogs, found {len(dogs)}")
            for key, actor in dogs:
                index = (actor["params"] & 0xF0) >> 4
                if index >= len(actual):
                    errors.append(f"setup {setup} dog {key}: path {index} exceeds loaded count {len(actual)}")
                elif index >= len(expected) or actual[index] != expected[index]:
                    errors.append(f"setup {setup} dog {key}: path {index} does not match authored route")
                else:
                    print(f"setup {setup} dog {key}: authored path {index} resolves ({len(actual[index])} points)")
        if candidate.testzip() is not None:
            errors.append("candidate ZIP integrity check failed")
    if errors:
        print("FAIL:\n" + "\n".join(errors))
        return 1
    print("PASS: all six dog route lookups match the authored scene")
    return 0


if __name__ == "__main__":
    raise SystemExit(verify(Path(sys.argv[1])))
