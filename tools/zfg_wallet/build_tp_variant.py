#!/usr/bin/env python3
"""Fit the user-provided TPObjects wallet and package its ZFG-only draw list."""

import argparse
import ctypes as ct
import hashlib
import json
import math
import os
from pathlib import Path
import xml.etree.ElementTree as ET
import zipfile

from texture_tp_wallet import LOGICAL_SIZE, author_atlas, verify_atlas


DONOR_PREFIX = "objects/object_gi_purse/"
VARIANT_PREFIX = "objects/combo_zfg_wallet/tp/"
DONOR_ROOT = DONOR_PREFIX + "gGiWalletDL"
VARIANT_ROOT = VARIANT_PREFIX + "Draw"
INPUT_HASHES = (
    "31eed67c4255fe389fb5dc39904af3a0b2773f59ad9e8690518a552bb282955f",
    "d9e325eb7f01823cb0cf1e8f4f141c5d9a1a204239bef249a735390f7ec1507c",
)
SCALE = (0.84, 0.72, 0.84)
OFFSET = (0, -8, 0)
MOUTH_CENTER = (4, 30, -1)


class Mpq:
    def __init__(self, path, library):
        self.handle = ct.c_void_p()
        self.library = library
        if not library.SFileOpenArchive(os.fsencode(path), 0, 0x100, ct.byref(self.handle)):
            raise OSError(f"Could not open {path}")

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.library.SFileCloseArchive(self.handle)

    def read(self, name):
        entry = ct.c_void_p()
        raw_name = name.encode("ascii")
        if not self.library.SFileOpenFileEx(self.handle, raw_name, 0, ct.byref(entry)):
            raise KeyError(name)
        try:
            size = self.library.SFileGetFileSize(entry, None)
            data = ct.create_string_buffer(size)
            read = ct.c_uint32()
            if not self.library.SFileReadFile(entry, data, size, ct.byref(read), None) or read.value != size:
                raise OSError(f"Could not read {name}")
            return data.raw
        finally:
            self.library.SFileCloseFile(entry)


def open_storm(path):
    library = ct.CDLL(os.fspath(path))
    pointer = ct.POINTER(ct.c_void_p)
    signatures = (
        ("SFileOpenArchive", (ct.c_char_p, ct.c_uint32, ct.c_uint32, pointer), ct.c_bool),
        ("SFileOpenFileEx", (ct.c_void_p, ct.c_char_p, ct.c_uint32, pointer), ct.c_bool),
        ("SFileGetFileSize", (ct.c_void_p, ct.POINTER(ct.c_uint32)), ct.c_uint32),
        ("SFileReadFile", (ct.c_void_p, ct.c_void_p, ct.c_uint32, ct.POINTER(ct.c_uint32), ct.c_void_p), ct.c_bool),
        ("SFileCloseFile", (ct.c_void_p,), ct.c_bool),
        ("SFileCloseArchive", (ct.c_void_p,), ct.c_bool),
    )
    for name, args, result in signatures:
        function = getattr(library, name)
        function.argtypes = args
        function.restype = result
    return library


def signed_byte(value):
    return (int(value) + 128) % 256 - 128


def fit_vertices(root):
    for vertex in root:
        for axis, factor, offset in zip("XYZ", SCALE, OFFSET):
            vertex.set(axis, str(round(int(vertex.get(axis)) * factor + offset)))
        normal = [signed_byte(vertex.get(ch)) / factor for ch, factor in zip("RGB", SCALE)]
        length = math.sqrt(sum(component * component for component in normal))
        if length:
            for ch, component in zip("RGB", normal):
                vertex.set(ch, str(round(127 * component / length) % 256))


def variant_name(path):
    if not path.startswith(DONOR_PREFIX):
        raise ValueError(f"External dependency outside wallet assets: {path}")
    return VARIANT_ROOT if path == DONOR_ROOT else VARIANT_PREFIX + path.removeprefix(DONOR_PREFIX)


def serialize_xml(root):
    ET.indent(root, space="    ")
    return ET.tostring(root, encoding="utf-8", xml_declaration=True) + b"\n"


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def build_assets(meshes, textures):
    assets = {}
    pending = [DONOR_ROOT]
    while pending:
        original = pending.pop()
        output = variant_name(original)
        if output in assets:
            continue
        if original.endswith("/WalletTex"):
            assets[output] = author_atlas(textures.read("alt/" + original))
            continue
        root = ET.fromstring(meshes.read("alt/" + original))
        if root.tag == "Vertex":
            fit_vertices(root)
        else:
            assert root.tag == "DisplayList"
            for element in list(root):
                path = element.get("Path")
                if path is not None:
                    pending.append(path)
                    element.set("Path", variant_name(path))
                if element.tag == "SetTextureImage":
                    # The atlas is RGBA. A preceding color-indexed item's TLUT
                    # must not affect it on the same graphics stream.
                    root.insert(list(root).index(element), ET.Element("SetTextureLUT", {"Mode": "G_TT_NONE"}))
        assets[output] = serialize_xml(root)
    return assets


def verify(assets):
    visited = set()
    position = []
    triangles = 0
    texture = None

    def visit(name, parents=()):
        nonlocal texture, triangles
        assert name.startswith(VARIANT_PREFIX) and name not in parents and name in assets
        visited.add(name)
        data = assets[name]
        if name.endswith("/WalletTex"):
            texture = verify_atlas(data)
            return
        root = ET.fromstring(data)
        assert root.get("Version") == "0"
        if root.tag == "Vertex":
            coordinates = [tuple(int(vertex.get(axis)) for axis in "XYZ") for vertex in root]
            position.extend(coordinates)
            return coordinates
        assert root.tag == "DisplayList" and root[-1].tag == "EndDisplayList"
        batch = {}
        for element in root:
            ref = element.get("Path")
            if ref is not None:
                child = visit(ref, (*parents, name))
            if element.tag == "LoadVertices":
                offset, start, count = [int(element.get(key)) for key in ("VertexOffset", "VertexBufferIndex", "Count")]
                assert 0 <= offset < offset + count <= len(child)
                assert 0 <= start < start + count <= 32
                batch.update({i + start: child[i + offset] for i in range(count)})
            elif element.tag == "Triangle1":
                ids = [int(element.get(f"V0{i}")) for i in range(3)]
                assert set(ids) <= batch.keys()
                a, b, c = (batch[index] for index in ids)
                ab = [end - begin for begin, end in zip(a, b)]
                ac = [end - begin for begin, end in zip(a, c)]
                assert any(ab[i] * ac[j] != ac[i] * ab[j] for i, j in ((0, 1), (1, 2), (2, 0)))
                triangles += 1
            elif element.tag == "SetRenderMode":
                assert element.get("Mode2") == "G_RM_AA_ZB_OPA_SURF2"
            elif element.tag == "LoadBlock":
                assert int(element.get("Lrs")) + 1 == math.prod(LOGICAL_SIZE)
            elif element.tag == "SetTile" and element.get("Tile") == "0":
                assert element.get("Size") == "G_IM_SIZ_16b"
                assert int(element.get("Line")) * 8 == LOGICAL_SIZE[0] * 2

    visit(VARIANT_ROOT)
    assert visited == assets.keys() and len(assets) == 10 and triangles == 266
    bounds = [[min(axis), max(axis)] for axis in zip(*position)]
    assert bounds == [[-18, 18], [-28, 34], [-19, 19]]
    return {"draw": VARIANT_ROOT, "bounds": bounds, "mouth": MOUTH_CENTER,
            "triangles": triangles, "texture": texture, "resources": sorted(visited)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("models_otr", type=Path)
    parser.add_argument("textures_otr", type=Path)
    parser.add_argument("libstorm", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    donors = (args.models_otr, args.textures_otr)
    if any(args.output.resolve() == donor.resolve() or (args.output.exists() and args.output.samefile(donor))
           for donor in donors):
        parser.error("Output must be separate from both input archives")
    hashes = tuple(digest(donor) for donor in donors)
    if hashes != INPUT_HASHES:
        parser.error("Input archives do not match the fitted TPObjects version")

    storm = open_storm(args.libstorm.resolve())
    with Mpq(args.models_otr, storm) as meshes, Mpq(args.textures_otr, storm) as textures:
        assets = build_assets(meshes, textures)
    report = verify(assets)
    with zipfile.ZipFile(args.output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(assets.items()):
            entry = zipfile.ZipInfo(name, (2026, 11, 21, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            entry.external_attr = 0o100644 << 16
            archive.writestr(entry, data, compresslevel=9)
    with zipfile.ZipFile(args.output) as archive:
        assert archive.testzip() is None
        assert verify({name: archive.read(name) for name in archive.namelist()}) == report
        report["texture"]["compressedBytes"] = archive.getinfo(VARIANT_PREFIX + "WalletTex").compress_size
    report["archiveBytes"] = args.output.stat().st_size
    report["sha256"] = digest(args.output)
    report["inputSha256"] = list(hashes)
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
