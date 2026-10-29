"""Check every native cutscene actor vertex offset against an owned MM archive.

This runs the production graph patcher on actual vertex packets. It does not
render the game, execute materials, or require redistributing any ROM assets.
"""
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import zipfile


class Vertex(C.Structure):
    _fields_ = [("position", C.c_int32 * 3), ("flag", C.c_uint16),
                ("uv", C.c_int16 * 2), ("color", C.c_uint8 * 4)]


class View(C.Structure):
    _fields_ = [("pointer", C.c_size_t), ("size", C.c_size_t),
                ("recordSize", C.c_size_t), ("stride", C.c_size_t)]


class Command(C.Structure):
    _fields_ = [("w0", C.c_uint32), ("w1", C.c_size_t)]


class Stats(C.Structure):
    _fields_ = [(name, C.c_size_t) for name in
                ("nested", "vertices", "unresolved", "malformed", "renderMode", "textures")]


def crc64(path):
    crc = (1 << 64) - 1
    for byte in path.encode():
        crc ^= byte << 56
        for _ in range(8):
            crc = ((crc << 1) ^ (0x42F0E1EBA9EA3693 if crc & (1 << 63) else 0)) & ((1 << 64) - 1)
    return crc


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    assert C.sizeof(Vertex) == 24
    catalog = (root / "soh/src/overlays/actors/ovl_En_Viewer/static_story_mm_actor.c").read_text()
    folders = set(re.findall(r'"objects/([^/]+)/', catalog))
    folders.update(re.findall(r'ORDINARY\("([^"]+)"', catalog))
    callback_type = C.CFUNCTYPE(C.c_size_t, C.c_void_p, C.c_int, C.c_uint64, C.POINTER(View))
    result = {"archive_sha256": hashlib.sha256(args.archive.read_bytes()).hexdigest(),
              "runtime_vertex_size": C.sizeof(Vertex), "objects": {}, "animations": {}}
    with tempfile.TemporaryDirectory(prefix="native-mm-archive-") as tmp, zipfile.ZipFile(args.archive) as archive:
        binary = Path(tmp) / "patch.so"
        subprocess.run(["c++", "-std=c++20", "-shared", "-fPIC",
                        str(root / "soh/mods/transformation_masks/assets/mm_display_list_patch.cpp"),
                        str(root / "soh/tests/mm_display_list_patch_bridge.cpp"), "-o", str(binary)], check=True)
        library = C.CDLL(str(binary))
        patch = library.Test_PatchMmDisplayList
        patch.argtypes = [C.POINTER(Command), C.c_size_t, callback_type, C.c_void_p, C.POINTER(Stats)]
        patch.restype = C.c_bool
        names = {crc64(name): name for name in archive.namelist() if not name.endswith("/")}
        vertices = {}
        errors = []

        @callback_type
        def resolve(_context, kind, hash_value, view):
            try:
                assert kind == 1, kind
                if hash_value not in vertices:
                    name = names[hash_value]
                    data = archive.read(name)
                    endian = "<" if data[0] == 0 else ">"
                    array_type, count = struct.unpack_from(endian + "II", data, 64)
                    assert array_type == 25 and len(data) == 72 + count * 16, name
                    runtime = (Vertex * count)()
                    for index in range(count):
                        values = struct.unpack_from(endian + "hhhHhh4B", data, 72 + index * 16)
                        runtime[index].position[:] = values[:3]
                        runtime[index].flag = values[3]
                        runtime[index].uv[:] = values[4:6]
                        runtime[index].color[:] = values[6:]
                    vertices[hash_value] = runtime
                runtime = vertices[hash_value]
                view.contents.pointer = C.addressof(runtime)
                view.contents.size = C.sizeof(runtime)
                view.contents.recordSize = 16
                view.contents.stride = C.sizeof(Vertex)
                return C.addressof(runtime)
            except Exception as error:
                errors.append(str(error))
                return 0

        for name in archive.namelist():
            if not re.match(r"objects/([^/]+)/", name):
                continue
            folder = name.split("/")[1]
            if folder not in folders or name.endswith("/"):
                continue
            data = archive.read(name)
            if data[4:8] not in (b"TLDO", b"ODLT"):
                continue
            assert len(data) >= 80 and (len(data) - 72) % 8 == 0, name
            endian = "<" if data[0] == 0 else ">"
            entry = result["objects"].setdefault(folder, {"lists": 0, "vertex_loads": 0,
                                                        "nonzero_offsets": 0})
            entry["lists"] += 1
            offset = 72
            while offset + 8 <= len(data):
                w0, w1 = struct.unpack_from(endian + "II", data, offset)
                opcode = w0 >> 24
                if opcode == 0xDF:
                    break
                if opcode == 0x32:
                    high, low = struct.unpack_from(endian + "II", data, offset + 8)
                    packet = (Command * 3)(Command(w0, w1), Command(high, low), Command(0xDF000000, 0))
                    stats = Stats()
                    assert patch(packet, 3, resolve, None, C.byref(stats)) and not errors, (name, offset, errors)
                    assert stats.vertices == 1 and not stats.unresolved and not stats.malformed, (name, offset)
                    runtime = vertices[(high << 32) | low]
                    index = w1 // 16
                    assert packet[0].w0 >> 24 == 1 and packet[0].w1 == C.addressof(runtime[index]), (name, offset)
                    actual = C.cast(packet[0].w1, C.POINTER(Vertex)).contents
                    assert bytes(actual) == bytes(runtime[index]), (name, offset)
                    entry["vertex_loads"] += 1
                    entry["nonzero_offsets"] += w1 != 0
                offset += 16 if opcode in (0x20, 0x24, 0x25, 0x27, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42) else 8
        for name, frames in (("gAnju2UmbrellaCryAnim", 43), ("gAnju2UmbrellaIdleAnim", 32)):
            data = archive.read("objects/object_an2/" + name)
            endian = "<" if data[0] == 0 else ">"
            kind, actual_frames = struct.unpack_from(endian + "Ih", data, 64)
            assert kind == 0 and actual_frames == frames
            result["animations"][name] = actual_frames
    assert result["objects"].get("object_an1", {}).get("nonzero_offsets", 0) > 0
    result["verdict"] = "PASS: exact runtime element selected for every native actor vertex packet; in-game visuals untested"
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
