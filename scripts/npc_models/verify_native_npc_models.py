"""Read exported resources back, validate native contracts and inspect poses."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

import numpy as np

from native_npc_models import PROFILES, VARIANTS, NativeRig, SourceModel, asset, read_limb, animation_frames


class Archive:
    def __init__(self, native, candidate):
        self.native = zipfile.ZipFile(native)
        self.candidate = zipfile.ZipFile(candidate)
        self.hashes = {}
        for name in [*self.native.namelist(), *self.candidate.namelist()]:
            self.hashes[asset.crc64(name)] = name
            if name.startswith("alt/"):
                self.hashes[asset.crc64(name[4:])] = name[4:]
        self.vertices, self.textures = {}, {}

    def read(self, name):
        name = name.removeprefix("__OTR__")
        if not name.startswith("alt/") and "alt/"+name in self.candidate.namelist():
            return self.candidate.read("alt/"+name)
        if name in self.candidate.namelist():
            return self.candidate.read(name)
        return self.native.read(name)

    def texture(self, path):
        if path not in self.textures:
            self.textures[path] = asset.read_texture(self.read(path))
        return self.textures[path]

    def vertex_buffer(self, path):
        if path not in self.vertices:
            raw = self.read(path)
            if raw[4:8] != b"RRAO" or struct.unpack_from("<I", raw, 64)[0] != 25:
                raise ValueError("Display list references a non-vertex array")
            count = struct.unpack_from("<I", raw, 68)[0]
            dtype = np.dtype([("xyz", "<i2", 3), ("flag", "<u2"), ("uv", "<i2", 2), ("normal", "i1", 3), ("alpha", "u1")])
            self.vertices[path] = np.frombuffer(raw, dtype=dtype, offset=72, count=count)
        return self.vertices[path]

    def geometry(self, path, rig, host, triangles_only=False):
        """Interpret the actual cache, recursive calls and flex-matrix loads."""
        state = dict(cache=[None]*32, slot=rig.skel.slot_to_limb.index(host), pending=None, texture=None)
        records, textures = [], []

        def walk(path, depth=0):
            if depth > 16:
                raise ValueError("Display-list recursion exceeds the model contract")
            raw = self.read(path)
            if raw[4:8] != b"TLDO" or raw[64] != 4:
                raise ValueError("Expected an F3DEX2 display list")
            offset = 72
            while offset+8 <= len(raw):
                first, second = struct.unpack_from("<II", raw, offset)
                op = first >> 24
                size = 16 if op in (0x20, 0x31, 0x32, 0x33, 0x35, 0x36) else 8
                if size == 16:
                    high, low = struct.unpack_from("<II", raw, offset+8)
                    hashed = high << 32 | low
                    if op != 0x33 and hashed not in self.hashes:
                        raise ValueError(f"Unresolved model reference {hashed:016x} in {path}")
                    target = self.hashes.get(hashed)
                    if op == 0x31:
                        walk(target, depth+1)
                    elif op == 0x32:
                        count = (first >> 12) & 255
                        start = ((first >> 1) & 127)-count
                        data = self.vertex_buffer(target)
                        source_offset = second//16
                        if start < 0 or start+count > 32 or source_offset+count > len(data):
                            raise ValueError("Vertex-cache load exceeds its actual buffer")
                        if state["slot"] not in range(rig.dlist_count):
                            raise ValueError("Vertex-cache load uses an invalid native matrix")
                        for i, record in enumerate(data[source_offset:source_offset+count]):
                            state["cache"][start+i] = (record, state["slot"])
                    elif op == 0x20:
                        state["pending"] = target
                elif op == 0xDA and second >> 24 == 0x0D:
                    state["slot"] = (second & 0xFFFFFF)//64
                elif op in (0xF3, 0xF4):
                    state["texture"] = state["pending"]
                elif op in (0x05, 0x06):
                    commands = [first & 0xFFFFFF] + ([second & 0xFFFFFF] if op == 0x06 else [])
                    for command in commands:
                        indices = [(command >> shift & 255)//2 for shift in (16, 8, 0)]
                        if any(i >= 32 or state["cache"][i] is None for i in indices):
                            raise ValueError("Triangle references an unloaded native vertex")
                        records.append([state["cache"][i] for i in indices])
                        textures.append(state["texture"])
                elif op == 0xDF:
                    return
                offset += size
            raise ValueError("Display list has no END command")

        walk(path)
        if triangles_only:
            return len(records)
        xyz, normal, uv, bones = [], [], [], []
        for triangle in records:
            for vertex, slot in triangle:
                xyz.append(vertex["xyz"])
                normal.append(vertex["normal"])
                uv.append(vertex["uv"])
                bones.append(rig.skel.slot_to_limb[slot])
        return dict(local=np.asarray(xyz, float).reshape(-1, 3), normals=np.asarray(normal, float).reshape(-1, 3)/127,
                    uv=np.asarray(uv, float).reshape(-1, 2)/(512*32), bones=np.asarray(bones),
                    faces=np.arange(len(xyz)).reshape(-1, 3), textures=textures)


def verify(native, source_dir, candidate, allow_fitted_rig=False):
    archive = Archive(native, candidate)
    if archive.candidate.testzip() is not None:
        raise ValueError("Candidate ZIP integrity failure")
    metadata = [name for name in archive.candidate.namelist() if name.endswith("_NativeNpcModelInfo.json")]
    if len(metadata) != 1:
        raise ValueError("Candidate needs one unique model manifest")
    report = json.loads(archive.candidate.read(metadata[0]))
    variant = report["variant"]
    source = SourceModel(Path(source_dir)/VARIANTS[variant]["file"])
    if report["source_sha256"] != hashlib.sha256(source.raw).hexdigest() or report["native_sha256"] != hashlib.sha256(Path(native).read_bytes()).hexdigest():
        raise ValueError("Source/native input differs from the model manifest")
    rig = NativeRig(archive.native, VARIANTS[variant]["profile"])
    expected = sum(len(m["faces"]) for m in source.meshes)
    count = 0
    rig_identical = True
    for i, limb in enumerate(rig.limbs):
        path = rig.ns+f"/Rig/Limb{i:02d}"
        original = archive.candidate.read(path)
        alternate = archive.candidate.read("alt/"+path)
        base_fields, base_dl, _ = read_limb(original)
        alt_fields, _, _ = read_limb(alternate)
        if base_dl != rig.near[i] or base_fields[3:] != alt_fields[3:]:
            raise ValueError("Private rig changes native hierarchy or fallback geometry")
        rig_identical = rig_identical and base_fields == alt_fields
        if not allow_fitted_rig and base_fields != alt_fields:
            raise ValueError("Fitted rig translations require --allow-fitted-rig")
        if base_fields != (limb.x, limb.y, limb.z, limb.child, limb.sibling):
            raise ValueError("Private rig differs from the actual native source")
        if limb.dlist:
            count += archive.geometry(rig.ns+f"/Body/Limb{i:02d}", rig, i, triangles_only=True)
    if count != expected or count != report["source_triangles"]:
        raise ValueError("Source triangles were lost or duplicated")
    texture_type, w, h, pixels = archive.texture(rig.ns+"/Textures/Diffuse")
    if texture_type != 1 or pixels != np.asarray(source.image).tobytes() or (w, h) != source.image.size:
        raise ValueError("Source diffuse pixels or dimensions changed")
    for name in archive.candidate.namelist():
        if name != metadata[0] and not name.removeprefix("alt/").startswith(rig.ns+"/"):
            raise ValueError("Archive overwrites a public or unrelated resource")
        raw = archive.candidate.read(name)
        if raw[4:8] == b"TLDO" and "/Body/" in name:
            # ENV alpha is the caller's distance/cutscene fade; native actor
            # code, rather than an OPA render-mode override, owns the blend.
            if struct.pack("<II", 0xFC127E03, 0xFFFFFBFD) not in raw:
                raise ValueError("Custom material loses native fade alpha")
        if raw[4:8] == b"RRAO":
            records = archive.vertex_buffer(name)
            if np.any(records["flag"] >> 8 >= rig.dlist_count) or np.any(records["alpha"] != 255):
                raise ValueError("Native vertex flags or alpha are invalid")
    for left in range(5):
        for right in range(5):
            for mouth in range(5):
                archive.geometry(rig.ns+f"/Faces/L{left}R{right}M{mouth}", rig, rig.profile["head"], triangles_only=True)
    if rig.name == "saria":
        combined = archive.geometry(rig.ns+"/Props/RightHandOcarina", rig, rig.profile["right"][-1], triangles_only=True)
        hand = int(report["body_triangles"]["14"])
        if combined <= hand or combined-hand != 36:
            raise ValueError("The native ocarina prop batch changed")
    return dict(candidate=Path(candidate).name, sha256=hashlib.sha256(Path(candidate).read_bytes()).hexdigest(),
                body_triangles=count, limb_count=len(rig.limbs), matrix_count=rig.dlist_count,
                expression_routes=125, diffuse_pixels_identical=True, native_rig_identical=rig_identical,
                native_fallback=True, native_fade_alpha=True, private_resources_only=True,
                runtime_tested=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("native", type=Path)
    parser.add_argument("sources", type=Path)
    parser.add_argument("candidates", type=Path)
    parser.add_argument("--allow-fitted-rig", action="store_true", help="Allow Alt limb offsets; preserve native fallback and hierarchy")
    args = parser.parse_args()
    results = []
    for variant in VARIANTS.values():
        result = verify(args.native, args.sources, args.candidates/variant["archive"], args.allow_fitted_rig)
        results.append(result)
        print("PASS", json.dumps(result), flush=True)
    (args.candidates/"Verification.json").write_text(json.dumps(results, indent=2)+"\n")


if __name__ == "__main__":
    main()
