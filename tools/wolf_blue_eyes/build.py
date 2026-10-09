#!/usr/bin/env python3
"""Palette-only Wolf eye POC. Geometry, rig, clips and audio remain exact.

Texture alpha is an inverse emission mask for the matching native material
patch. Existing opaque Wolf materials still draw these texels, with lighting.
No image generation, reskinning, rasterized mesh or animation export is used.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

import numpy as np
from PIL import Image

RESOURCE = "objects/forms/wolf_link/gWolfLinkData"
BASE_SHA256 = "d6447975f22bcb1e62a6fac6009f5aa7ab7f78f65f499f19c9b4b7ce3216416e"
EYE_TILES = ((532, 4), (612, 4))


def read(path):
    with zipfile.ZipFile(path) as archive:
        resource = archive.read(RESOURCE)
        if archive.read("alt/" + RESOURCE) != resource:
            raise ValueError("baseline normal/Alt payloads differ")
    payload = resource[68:]
    if resource[4:8] != b"BLBO" or struct.unpack_from("<I", resource, 64)[0] != len(payload):
        raise ValueError("invalid native Blob envelope")
    if hashlib.sha256(payload).hexdigest() != BASE_SHA256:
        raise ValueError("expected the recovered TP POC1 payload; do not recolor an unknown atlas")
    return resource[:68], payload


def rgba5551(words):
    return np.stack([(words >> 11) & 31, (words >> 6) & 31, (words >> 1) & 31], axis=-1).astype(float) * 255 / 31


def eye_colors(old, style="soft"):
    yy, xx = np.mgrid[:64, :64]
    # Select the original blue iris/pupil by color inside its existing eye tile.
    # The baked fur, white sclera and dark eyelid pixels are protected.
    iris = (old[:, :, 2] - old[:, :, 0] >= 8) & (old[:, :, 2] >= old[:, :, 1])
    iris &= (xx >= 9) & (xx <= 44) & (yy >= 19) & (yy <= 48)
    radius = np.sqrt(((xx - 26.5) / 15.5) ** 2 + ((yy - 33.5) / 14.0) ** 2)
    if style == "soft":
        # Keep a wider pupil and the original dark outer iris. A muted, textured
        # blue crescent remains readable without turning the whole eye neon.
        pupil = (((xx - 26.0) / 8.0) ** 2 + ((yy - 32.5) / 8.5) ** 2) <= 1
        luminous = iris & ~pupil & (radius < 0.87)
        inner = np.exp(-((radius - 0.65) / 0.23) ** 2)
        glint = np.exp(-(((xx - 20) / 2.0) ** 2 + ((yy - 27) / 2.0) ** 2))
        blue = np.stack([18 + 12 * inner + 28 * glint,
                         62 + 26 * inner + 32 * glint,
                         104 + 36 * inner + 36 * glint], axis=-1)
        result = old.copy()
        result[luminous] = old[luminous] * 0.3 + blue[luminous] * 0.7
        return np.rint(result).astype(np.uint16), luminous
    pupil = (((xx - 26.0) / 5.4) ** 2 + ((yy - 32.5) / 6.0) ** 2) <= 1
    luminous = iris & ~pupil
    inner = np.exp(-((radius - 0.43) / 0.3) ** 2)
    rim = np.exp(-((radius - 0.8) / 0.23) ** 2)
    glint = np.exp(-(((xx - 20) / 2.8) ** 2 + ((yy - 27) / 2.7) ** 2))
    blue = np.empty_like(old)
    blue[:, :, 0] = 24 + 40 * inner + 12 * rim + 125 * glint
    blue[:, :, 1] = 128 + 56 * inner + 14 * rim + 64 * glint
    blue[:, :, 2] = 248 + 7 * np.maximum(inner, glint)
    result = old.copy()
    result[luminous] = np.minimum(blue[luminous], 255)
    # Keep dark, recognizable pupils. Only the iris is self-lit.
    return np.rint(result).astype(np.uint16), luminous


def build(source, output, style="soft"):
    envelope, baseline = read(source)
    header = struct.unpack_from("<20I", baseline, 12)
    if header[4:6] != (1024, 512):
        raise ValueError("unexpected atlas layout")
    words = np.frombuffer(baseline, "<u2", header[4] * header[5], header[16]).reshape(header[5], header[4]).copy()
    original = words.copy()
    masks = np.zeros(words.shape, bool)
    counts = []
    for x, y in EYE_TILES:
        old = rgba5551(words[y:y + 64, x:x + 64])
        rgb, luminous = eye_colors(old, style)
        changed = ((rgb[:, :, 0] >> 3) << 11) | ((rgb[:, :, 1] >> 3) << 6) | ((rgb[:, :, 2] >> 3) << 1) | 1
        changed[luminous] &= np.uint16(0xFFFE)
        tile = words[y:y + 64, x:x + 64]
        tile[luminous] = changed[luminous]
        masks[y:y + 64, x:x + 64] = luminous
        counts.append(int(luminous.sum()))
    assert all(100 < count < 900 for count in counts)
    assert np.array_equal(words[~masks], original[~masks])
    assert np.all((words[~masks] & 1) == 1)
    candidate = bytearray(baseline)
    candidate[header[16]:header[16] + header[17]] = words.astype("<u2").tobytes()
    # All nontexture bytes are identical, including the 144 clips and PCM sounds.
    assert candidate[:header[16]] == baseline[:header[16]]
    assert candidate[header[16] + header[17]:] == baseline[header[16] + header[17]:]
    output.mkdir(parents=True, exist_ok=True)
    (output / "wolf_link.bin").write_bytes(candidate)
    serialized = envelope + candidate
    pack = output / ("zz_Wolf_Link_Blue_Eyes_POC2.o2r" if style == "soft" else "zz_Wolf_Link_Blue_Eyes_POC1.o2r")
    with zipfile.ZipFile(pack, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name in (RESOURCE, "alt/" + RESOURCE):
            info = zipfile.ZipInfo(name, (2026, 10, 9, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, serialized, compresslevel=9)
    with zipfile.ZipFile(pack) as archive:
        assert archive.testzip() is None
        assert archive.namelist() == [RESOURCE, "alt/" + RESOURCE]
        assert archive.read(RESOURCE) == serialized == archive.read("alt/" + RESOURCE)
    Image.fromarray(np.rint(rgba5551(words)).astype(np.uint8)).save(output / "atlas_rgb.png")
    Image.fromarray((masks * 255).astype(np.uint8)).save(output / "emission_mask.png")
    report = {
        "baseline_payload_sha256": hashlib.sha256(baseline).hexdigest(),
        "candidate_payload_sha256": hashlib.sha256(candidate).hexdigest(),
        "candidate_archive_sha256": hashlib.sha256(pack.read_bytes()).hexdigest(),
        "changed_texels_per_eye": counts,
        "eye_style": style,
        "atlas_dimensions": [1024, 512],
        "geometry_weights_uvs_normals_rig_animation_audio_bytes": "identical",
        "unmasked_texture_words": "identical",
        "vertices": header[0], "triangles": header[1], "bones": header[2], "animation_clips": header[3],
        "normal_alt_payloads": "identical",
        "glow_requires_matching_native_material_patch": True,
        "current_binary_result": "brighter blue irises with normal scene lighting",
        "runtime_status": "untested"
    }
    (output / "verification.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--style", choices=("soft", "bright"), default="soft")
    args = parser.parse_args()
    build(args.source, args.output, args.style)
