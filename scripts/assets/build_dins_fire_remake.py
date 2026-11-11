#!/usr/bin/env python3
"""Encode the accepted Din's Fire art for both native host archives.

The accepted RGBA atlas is preserved byte-for-byte. Its three rows are cropped
and resampled for bounded native textures; RGB and alpha remain independent.
The fixed-function material uses baked 20 Hz burnout samples, including the
accepted unwrapped horizontal UV scroll and alpha-driven erosion. The old
flameTex/sparkTex recovery resources are reproduced without any byte changes.
Neutral RGB companions retain exact source alpha for the native two-color
cosmetic material. Timing-independent resources retain their pre-linger bytes;
burnout resources are rebaked at the shared contract's actual later fire age.
--check compares every generated host resource, manifest and name declaration.
--check-native validates the committed outputs without the optional source PNGs.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import zipfile

from PIL import Image


ROOT = Path(__file__).resolve().parents[2]
ART = ROOT / "tools/dins_fire_remake"
PREFIX = "custom/dins_fire_remake/"
TEXTURE_TYPE = 0x4F544558
RGBA32_TYPE = 1
INTENSITY8_TYPE = 6
IMAGE_FLAGS = 3
RGBA_LOGICAL_SIZE = 32
ATLAS_NAME = "flame_ribbons_luminous_rgba.png"
ATLAS_SHA256 = "9705407ffa7226f6d4f704a7b20575d66e4aa704759ef3a0670309de78fc186e"
LEGACY_SOURCE_SHA256 = "f65fb5a8355103131a6a6536ab86879fdce3d17de0b52aaf8a7f4ee8a14d17b9"
PRE_LINGER_PROVENANCE_SHA256 = "b50d500f9322ee023b9ed1e978a40b54471fd6a612dd43c4afb3cd4bafb41769"
TIMING_INDEPENDENT_RESOURCE_SET_SHA256 = "67e95a75d182d584c4102a2c4563dff360c68efd3b56be18d9782c0d129b4d3d"
COMMITTED_MANIFEST_SHA256 = "4fccacef519654f3ac5d2aed6f4004ee7c332b82cfdd2dc2d865510cc79b2874"
LEGACY_SHA256 = {
    "flameTex": "dc2d51de60b2ba9bd55b9f3497a312b44ae99654ecf9730ad140f06f82158ff4",
    "sparkTex": "45c863532059d5dd7a3469217ae408b0938b9e9ad26e9b052e163fd40a200d4f",
}
ATLAS_ROWS = ((0, 343), (343, 668), (668, 1024))
ROW_SIZES = ((768, 172), (768, 163), (768, 178))
BAND_ROWS = (0, 1, 2, 0, 1, 2)
TIMING_HEADER_PATH = "combo/render/DinsFireTiming.h"
TIMING_HEADER_BYTES = (ROOT / TIMING_HEADER_PATH).read_bytes()
TIMING_BASE_NAMES = (
    "CW_DINS_NATIVE_RATE", "CW_DINS_NATIVE_CHARGE_TICKS", "CW_DINS_NATIVE_STRIKE_TICKS",
    "CW_DINS_NATIVE_RECOVERY_TICKS", "CW_DINS_NATIVE_GROW_TICKS", "CW_DINS_NATIVE_INTERACTION_TICKS",
    "CW_DINS_NATIVE_FADE_TICKS", "CW_DINS_NATIVE_DAMAGE_HOLD_TICKS",
)


def literal_timing_values(data: bytes) -> dict[str, int]:
    """Read only the shared header's named decimal integer bases; never eval C."""
    text = data.decode("utf-8")
    values = {}
    for name in TIMING_BASE_NAMES:
        definitions = re.findall(rf"(?m)^#define[ \t]+{re.escape(name)}\b.*$", text)
        value = re.fullmatch(rf"#define[ \t]+{re.escape(name)}[ \t]+([0-9]+)[ \t]*", definitions[0]) if len(definitions) == 1 else None
        if value is None:
            raise ValueError(f"Timing base must be one literal integer definition: {name}")
        values[name] = int(value.group(1))
    if any(value <= 0 for value in values.values()):
        raise ValueError("Native timing bases must be positive")
    return values


TIMING_BASES = literal_timing_values(TIMING_HEADER_BYTES)
NATIVE_RATE = TIMING_BASES["CW_DINS_NATIVE_RATE"]
FIRE_START_SECONDS = (TIMING_BASES["CW_DINS_NATIVE_CHARGE_TICKS"] + TIMING_BASES["CW_DINS_NATIVE_STRIKE_TICKS"]) / NATIVE_RATE
CONTROL_RETURN_SECONDS = FIRE_START_SECONDS + TIMING_BASES["CW_DINS_NATIVE_RECOVERY_TICKS"] / NATIVE_RATE
FADE_START_FRAME = TIMING_BASES["CW_DINS_NATIVE_RECOVERY_TICKS"] + TIMING_BASES["CW_DINS_NATIVE_INTERACTION_TICKS"]
HOLD_TICKS = FADE_START_FRAME - TIMING_BASES["CW_DINS_NATIVE_GROW_TICKS"]
FIRE_TICKS = FADE_START_FRAME + TIMING_BASES["CW_DINS_NATIVE_FADE_TICKS"]
FADE_START_SECONDS = round(FIRE_START_SECONDS + FADE_START_FRAME / NATIVE_RATE, 8)
END_SECONDS = round(FIRE_START_SECONDS + FIRE_TICKS / NATIVE_RATE, 8)
REFERENCE_FADE_START_SECONDS = 3.05
BURN_SHIFT_SECONDS = round(FADE_START_SECONDS - REFERENCE_FADE_START_SECONDS, 8)
BURN_START_OFFSETS = (.08, .06, .04, .02, .0, .05)
BURN_STARTS = tuple(round(FADE_START_SECONDS + offset, 8) for offset in BURN_START_OFFSETS)
BURN_FIRST_FRAME = FADE_START_FRAME + 1
BURN_FRAME_COUNT = TIMING_BASES["CW_DINS_NATIVE_FADE_TICKS"] - 1
ARCHIVE_DATE = (2026, 11, 10, 0, 0, 0)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def canonical_resource_hash(resources: dict[str, bytes]) -> str:
    """Stable set identity: sorted UTF-8 name + NUL + resource SHA256 + LF."""
    return sha("".join(name + "\0" + sha(data) + "\n" for name, data in sorted(resources.items())).encode())


def palette_source_names() -> list[str]:
    return ([f"ribbon{row}Tex" for row in range(3)] + ["haloTex"] +
            [f"ribbonBurn{band}_{frame}Tex" for band in range(6)
             for frame in range(BURN_FIRST_FRAME, BURN_FIRST_FRAME + BURN_FRAME_COUNT)])


def encode_palette_companion(colored: bytes) -> bytes:
    """Convert only encoded RGB to luminance; copy every alpha/header byte.

    Integer coefficients implement .299 R + .587 G + .114 B with nearest-byte
    rounding (half upward). This consumes the already encoded RGBA image,
    including its exact native resampling, scroll and burned alpha.
    """
    source = memoryview(colored)[92:]
    result = bytearray(len(source))
    for offset in range(0, len(source), 4):
        value = (299 * source[offset] + 587 * source[offset + 1] + 114 * source[offset + 2] + 500) // 1000
        result[offset] = result[offset + 1] = result[offset + 2] = value
        result[offset + 3] = source[offset + 3]
    return colored[:92] + result


def encode_intensity(image: Image.Image) -> bytes:
    intensity = image.convert("L")
    return encode_texture(Image.merge("RGBA", (intensity,) * 4), INTENSITY8_TYPE)


def encode_texture(image: Image.Image, texture_type: int = RGBA32_TYPE) -> bytes:
    if image.mode != "RGBA":
        raise ValueError("Native texture input must have independent RGBA channels")
    rgba = image.tobytes()
    header = bytearray(64)
    header[1] = 1
    struct.pack_into("<IIQ", header, 4, TEXTURE_TYPE, 1, 0xDEADBEEFDEADBEEF)
    return bytes(header) + struct.pack("<IIIIffI", texture_type, *image.size, IMAGE_FLAGS,
                                       1.0, 1.0, len(rgba)) + rgba


def spark_image() -> Image.Image:
    size = 64
    pixels = []
    for y in range(size):
        for x in range(size):
            u, v = (x - 31.5) / 31.5, (y - 31.5) / 31.5
            wide, narrow = max(abs(u), abs(v)), min(abs(u), abs(v))
            radius2 = u * u + v * v
            core = math.exp(-radius2 / 0.012)
            halo = 0.16 * math.exp(-radius2 / 0.12)
            cross = 0.65 * math.exp(-((narrow / 0.035) ** 2)) * max(0.0, 1.0 - wide) ** 1.5
            diagonal = (0.28 * math.exp(-(((abs(u) - abs(v)) / 0.035) ** 2))
                        * max(0.0, 1.0 - wide / 0.66) ** 2)
            feather = min(1.0, max(0.0, (1.0 - wide) / 0.08))
            pixels.append(round(255 * min(1.0, core + halo + cross + diagonal) * feather))
    image = Image.new("L", (size, size))
    image.putdata(pixels)
    return image


def smooth(value: float) -> float:
    value = min(1.0, max(0.0, value))
    return value * value * (3.0 - 2.0 * value)


def byte(value: float) -> int:
    return round(min(1.0, max(0.0, value)) * 255)


def halo_image(neutral: bool = False) -> Image.Image:
    """128px accepted soft halo; neutral variant supplies the white hand flare."""
    pixels = []
    for y in range(128):
        v = -1.0 + y * (2.0 / 127)
        for x in range(128):
            u = -1.0 + x * (2.0 / 127)
            profile = max(0.0, 1.0 - math.hypot(u, v))
            rgb = (1.0, 1.0, 1.0) if neutral else (1.0, .70 + profile**2 * .27,
                                                       .17 + profile**2 * .67)
            pixels.append((*map(byte, rgb), byte(profile**1.8)))
    result = Image.new("RGBA", (128, 128))
    result.putdata(pixels)
    return result


def trail_image() -> Image.Image:
    """Accepted mint trail, constant along U and softly feathered across V."""
    pixels = []
    for y in range(128):
        profile = max(0.0, 1.0 - abs(-1.0 + y * (2.0 / 127)))
        rgba = (byte(.30 + .70 * profile**3), 255, byte(.66 + .34 * profile**3),
                byte(profile**1.2))
        pixels.extend([rgba] * 16)
    result = Image.new("RGBA", (16, 128))
    result.putdata(pixels)
    return result


def ribbon_rows(atlas: Image.Image) -> list[Image.Image]:
    # Resize channels independently, retaining the accepted RGB and independent
    # alpha rather than converting the luminous art into an intensity mask.
    return [Image.merge("RGBA", tuple(channel.resize(size, Image.Resampling.LANCZOS)
                                     for channel in atlas.crop((0, top, 1536, bottom)).split()))
            for (top, bottom), size in zip(ATLAS_ROWS, ROW_SIZES)]


def burn_image(row: Image.Image, band: int, frame: int) -> tuple[Image.Image, dict]:
    """Bake scroll plus source-alpha erosion at an actual native fire update.

    A repeating erosion mask cannot reproduce the accepted shader: its fine
    detail uses unwrapped U, which changes across the texture wrap. These images
    instead span local ribbon U=0..1. The native fade mesh keeps the accepted
    geometry and samples that local U; its original scroll coefficient is baked
    here. Crop V maps back to the exact full-atlas row bounds for the detail.

    This samples the half-resolution native row, with bilinear horizontal
    interpolation. Raster quantization/resampling is an encoding approximation,
    not a claim of pixel-identical in-game output against the offline renderer.
    """
    width, height = row.size
    age = frame / NATIVE_RATE
    time = FIRE_START_SECONDS + age
    burn = smooth((time - BURN_STARTS[band]) / (END_SECONDS - BURN_STARTS[band])) ** 1.35
    threshold = burn**.80 * .68 - .08
    top, bottom = ATLAS_ROWS[BAND_ROWS[band]]
    scroll = -age * (.27 + band * .025)
    columns = []
    for x in range(width):
        u = x / width + scroll
        tx = u * width
        ix = math.floor(tx)
        columns.append((u, ix % width, (ix + 1) % width, tx - ix))
    source = row.load()
    pixels = []
    for y in range(height):
        v = (top + y / height * (bottom - top)) / 1024
        for u, x0, x1, fraction in columns:
            a, b = source[x0, y], source[x1, y]
            rgba = [(aa + (bb - aa) * fraction) / 255 for aa, bb in zip(a, b)]
            if burn > 0:
                detail = .025 * burn * (math.sin(u * 41.9 + v * 123.4 + band * 1.1)
                                       + .5 * math.sin(u * 103.7 - v * 271.1))
                rgba[3] *= smooth((rgba[3] + detail - threshold) / .20)
            pixels.append(tuple(map(byte, rgba)))
    result = Image.new("RGBA", row.size)
    result.putdata(pixels)
    return result, {"band": band, "atlas_row": BAND_ROWS[band], "fire_frame": frame,
                    "time_seconds": round(time, 2), "burn": burn,
                    "horizontal_scroll_cycles": scroll, "sampling_u": "local ribbon U=0..1; scroll baked"}


def validate_resource(data: bytes, expected_size: tuple[int, int], texture_type: int) -> dict:
    if len(data) < 92:
        raise ValueError("Truncated texture header")
    kind, version = struct.unpack_from("<II", data, 4)
    fmt, width, height, flags, hscale, vscale, payload_size = struct.unpack_from("<IIIIffI", data, 64)
    if (kind, version, fmt, width, height, flags, hscale, vscale) != (
        TEXTURE_TYPE, 1, texture_type, *expected_size, IMAGE_FLAGS, 1.0, 1.0
    ):
        raise ValueError("Unexpected whole-image texture metadata")
    payload = data[92:]
    if len(payload) != payload_size or payload_size != width * height * 4:
        raise ValueError("Texture dimensions/payload length disagree")
    equal = payload[0::4] == payload[1::4] == payload[2::4] == payload[3::4]
    if (texture_type == INTENSITY8_TYPE) != equal:
        raise ValueError("Protected intensity/new independent-alpha resource mismatch")
    alpha = payload[3::4]
    rgb_equal = payload[0::4] == payload[1::4] == payload[2::4]
    alpha_matches_rgb = any(alpha == payload[channel::4] for channel in range(3))
    if texture_type == RGBA32_TYPE and alpha_matches_rgb:
        raise ValueError("RGBA resource alpha was derived from a color channel")
    logical = 64 if texture_type == INTENSITY8_TYPE else RGBA_LOGICAL_SIZE
    return {"sha256": sha(data), "pixel_sha256": sha(payload), "bytes": len(data),
            "payload_bytes": len(payload), "physical_size": [width, height],
            "logical_size": [logical, logical], "version": version, "type": fmt,
            "type_name": "Grayscale8bpp" if texture_type == INTENSITY8_TYPE else "RGBA32bpp",
            "flags": flags, "flag_names": ["LOAD_AS_RAW", "LOAD_AS_IMG"],
            "scales": [hscale, vscale], "rgba_intensity_equal": equal,
            "rgb_channels_equal": rgb_equal, "alpha_matches_any_rgb_channel": alpha_matches_rgb,
            "alpha_range": [min(alpha), max(alpha)], "alpha_sum": sum(alpha), "alpha_sha256": sha(alpha),
            "alpha_zero_pixels": alpha.count(0),
            "alpha_encoding": "intensity replicated" if equal else "independent straight alpha"}


def resource_declarations() -> bytes:
    """One generated name contract, shared by the native renderer and assets."""
    names = [(f"sCwDinsRibbon{row}Tex", f"ribbon{row}Tex") for row in range(3)]
    names += [("sCwDinsHaloTex", "haloTex"), ("sCwDinsWhiteHaloTex", "whiteHaloTex"),
              ("sCwDinsTrailTex", "trailTex")]
    names += [(f"sCwDinsRibbonBurn{band}_{frame}Tex", f"ribbonBurn{band}_{frame}Tex")
              for band in range(6) for frame in range(BURN_FIRST_FRAME, BURN_FIRST_FRAME + BURN_FRAME_COUNT)]
    names += [(f"sCwDinsRibbon{row}PaletteTex", f"ribbon{row}PaletteTex") for row in range(3)]
    names += [("sCwDinsHaloPaletteTex", "haloPaletteTex")]
    names += [(f"sCwDinsRibbonBurn{band}_{frame}PaletteTex", f"ribbonBurn{band}_{frame}PaletteTex")
              for band in range(6) for frame in range(BURN_FIRST_FRAME, BURN_FIRST_FRAME + BURN_FRAME_COUNT)]
    lines = ["/* Generated by scripts/assets/build_dins_fire_remake.py; do not edit. */",
             f"/* Timing: {TIMING_HEADER_PATH}; SHA256 {sha(TIMING_HEADER_BYTES)} */",
             "#ifndef COMBO_DINS_FIRE_NATIVE_RESOURCES_H", "#define COMBO_DINS_FIRE_NATIVE_RESOURCES_H",
             '#include "align_asset_macro.h"', "",
             f"#define CW_DINS_RGBA_LOGICAL_SIZE {RGBA_LOGICAL_SIZE}",
             f"#define CW_DINS_BURN_FIRST_FRAME {BURN_FIRST_FRAME}",
             f"#define CW_DINS_BURN_FRAME_COUNT {BURN_FRAME_COUNT}", ""]
    lines += [f'static const ALIGN_ASSET(2) char {symbol}[] = "__OTR__{PREFIX}{name}";'
              for symbol, name in names]
    lines += ["", "static const char* const sCwDinsRibbonTextures[3] = {",
              "    sCwDinsRibbon0Tex, sCwDinsRibbon1Tex, sCwDinsRibbon2Tex,", "};", "",
              "static const char* const sCwDinsRibbonBurnTextures[6][CW_DINS_BURN_FRAME_COUNT] = {"]
    for band in range(6):
        lines += ["    {", *[f"        sCwDinsRibbonBurn{band}_{frame}Tex,"
                              for frame in range(BURN_FIRST_FRAME, BURN_FIRST_FRAME + BURN_FRAME_COUNT)], "    },"]
    lines += ["};", "", "static const char* const sCwDinsRibbonPaletteTextures[3] = {",
              "    sCwDinsRibbon0PaletteTex, sCwDinsRibbon1PaletteTex, sCwDinsRibbon2PaletteTex,", "};", "",
              "static const char* const sCwDinsRibbonBurnPaletteTextures[6][CW_DINS_BURN_FRAME_COUNT] = {"]
    for band in range(6):
        lines += ["    {", *[f"        sCwDinsRibbonBurn{band}_{frame}PaletteTex,"
                              for frame in range(BURN_FIRST_FRAME, BURN_FIRST_FRAME + BURN_FRAME_COUNT)], "    },"]
    lines += ["};", "", "#endif", ""]
    return "\n".join(lines).encode()


def generated_resources() -> tuple[dict[str, bytes], dict]:
    provenance_path = ART / "pre_linger_resource_provenance.json"
    provenance_bytes = provenance_path.read_bytes()
    if sha(provenance_bytes) != PRE_LINGER_PROVENANCE_SHA256:
        raise ValueError("Immutable pre-linger resource provenance changed")
    previous = json.loads(provenance_bytes)
    previous_first, previous_end = previous["burnout_frames_half_open"]
    if BURN_FRAME_COUNT != previous_end - previous_first or HOLD_TICKS < 0:
        raise ValueError("Burnout sample count or nonnegative native hold contract changed")
    source_path = ART / "flame_source.png"
    source_bytes = source_path.read_bytes()
    if sha(source_bytes) != LEGACY_SOURCE_SHA256:
        raise ValueError("Protected POC source changed")
    with Image.open(source_path) as source:
        source_size = list(source.size)
        flame = source.convert("L").resize((256, 256), Image.Resampling.LANCZOS)
    textures, metadata = {}, {}

    def add(name: str, image: Image.Image, texture_type: int = RGBA32_TYPE, **extra) -> None:
        if max(image.size) > 768:
            raise ValueError(f"Resource exceeds the native dimension budget: {name}")
        data = encode_intensity(image) if texture_type == INTENSITY8_TYPE else encode_texture(image)
        if name in LEGACY_SHA256 and sha(data) != LEGACY_SHA256[name]:
            raise ValueError(f"Protected recovery texture changed: {name}")
        textures[PREFIX + name] = data
        metadata[PREFIX + name] = {**validate_resource(data, image.size, texture_type), **extra}
        if name.startswith("ribbon") and metadata[PREFIX + name]["rgb_channels_equal"]:
            raise ValueError(f"Luminous RGB color was lost: {name}")

    add("flameTex", flame, INTENSITY8_TYPE, protected_recovery=True)
    add("sparkTex", spark_image(), INTENSITY8_TYPE, protected_recovery=True)
    atlas_path = ART / ATLAS_NAME
    atlas_bytes = atlas_path.read_bytes()
    if sha(atlas_bytes) != ATLAS_SHA256:
        raise ValueError("Accepted luminous atlas changed; never regenerate or retouch this input")
    with Image.open(atlas_path) as atlas:
        if atlas.size != (1536, 1024) or atlas.mode != "RGBA":
            raise ValueError("Accepted atlas dimensions or full RGBA channels changed")
        rows = ribbon_rows(atlas)
    for index, row in enumerate(rows):
        add(f"ribbon{index}Tex", row, atlas_row=index, source_rows_half_open=list(ATLAS_ROWS[index]))
    add("haloTex", halo_image())
    add("whiteHaloTex", halo_image(neutral=True))
    add("trailTex", trail_image())
    for band in range(6):
        for frame in range(BURN_FIRST_FRAME, BURN_FIRST_FRAME + BURN_FRAME_COUNT):
            image, details = burn_image(rows[BAND_ROWS[band]], band, frame)
            old_name = PREFIX + f"ribbonBurn{band}_{previous_first + frame - BURN_FIRST_FRAME}Tex"
            details.update({"previous_phase_resource": old_name, "previous_phase_sha256": previous["resources"][old_name],
                            "shift_from_previous_frames": BURN_FIRST_FRAME - previous_first})
            add(f"ribbonBurn{band}_{frame}Tex", image, **details)
    colored_textures = dict(textures)
    if len(colored_textures) != 74:
        raise ValueError("Default colored/recovery resource count changed")
    colored_decoded = sum(item["payload_bytes"] for item in metadata.values())
    palette_textures = {}
    for source_name in palette_source_names():
        source_path = PREFIX + source_name
        source = colored_textures[source_path]
        name = PREFIX + source_name.removesuffix("Tex") + "PaletteTex"
        data = encode_palette_companion(source)
        details = validate_resource(data, tuple(metadata[source_path]["physical_size"]), RGBA32_TYPE)
        if not details["rgb_channels_equal"] or data[:92] != source[:92] or data[95::4] != source[95::4]:
            raise ValueError(f"Neutral RGB, exact alpha or metadata preservation failed: {name}")
        provenance = {key: value for key, value in metadata[source_path].items() if key in
                      ("atlas_row", "source_rows_half_open", "band", "fire_frame", "time_seconds", "burn",
                       "horizontal_scroll_cycles", "sampling_u", "shift_from_previous_frames")}
        if "previous_phase_resource" in metadata[source_path]:
            old_name = metadata[source_path]["previous_phase_resource"].removesuffix("Tex") + "PaletteTex"
            provenance.update({"previous_phase_resource": old_name, "previous_phase_sha256": previous["resources"][old_name]})
        palette_textures[name] = data
        metadata[name] = {**details, **provenance, "palette_for": source_path,
                          "source_rgba_sha256": sha(source), "alpha_byte_identical_to_source": True,
                          "header_byte_identical_to_source": True,
                          "rgb_encoding": "round_half_up(0.299*R + 0.587*G + 0.114*B) replicated into RGB"}
    textures.update(palette_textures)
    timing_independent = {name: data for name, data in textures.items() if not Path(name).name.startswith("ribbonBurn")}
    if len(timing_independent) != 12 or canonical_resource_hash(timing_independent) != TIMING_INDEPENDENT_RESOURCE_SET_SHA256:
        raise ValueError("Timing-independent row/halo/trail/legacy resource bytes changed")
    if any(sha(data) != previous["timing_independent_resources"][name] for name, data in timing_independent.items()):
        raise ValueError("Timing-independent resource differs from immutable pre-linger checkpoint")
    palette_decoded = sum(metadata[name]["payload_bytes"] for name in palette_textures)
    decoded = sum(item["payload_bytes"] for item in metadata.values())
    if len(palette_textures) != 70 or len(textures) != 144 or decoded > 72 * 1024 * 1024:
        raise ValueError("Native resource count or bounded decoded-memory budget changed")
    manifest = {
        "candidate": "Din's Fire six-second usable-linger candidate",
        "baseline_commit": "cd12567fe1c0d64a205e1d360165cb000166f159",
        "baseline_tree": "97da3eb62dbcab604927d2af3f902a46a6f681b8",
        "timing": {"header": TIMING_HEADER_PATH, "header_sha256": sha(TIMING_HEADER_BYTES),
                   "literal_bases": TIMING_BASES, "native_hz": NATIVE_RATE,
                   "release_seconds": FIRE_START_SECONDS, "control_return_seconds": CONTROL_RETURN_SECONDS,
                   "grow_ticks": TIMING_BASES["CW_DINS_NATIVE_GROW_TICKS"], "hold_ticks": HOLD_TICKS,
                   "fade_ticks": TIMING_BASES["CW_DINS_NATIVE_FADE_TICKS"], "fire_ticks": FIRE_TICKS,
                   "steady_seconds_after_control": TIMING_BASES["CW_DINS_NATIVE_INTERACTION_TICKS"] / NATIVE_RATE,
                   "fade_start_frame": FADE_START_FRAME, "fade_start_seconds": FADE_START_SECONDS,
                   "end_seconds": END_SECONDS, "burnout_shift_seconds": BURN_SHIFT_SECONDS,
                   "motion_age": "Continuous actual fire_frame/native_hz; only the fade envelope is delayed"},
        "pre_linger_checkpoint": {"commit": previous["checkpoint_commit"], "tree": previous["checkpoint_tree"],
                   "provenance": "tools/dins_fire_remake/pre_linger_resource_provenance.json",
                   "provenance_sha256": PRE_LINGER_PROVENANCE_SHA256,
                   "canonical_resource_sets": previous["canonical_resource_sets"],
                   "preserved_resource_count": len(timing_independent),
                   "timing_independent_sha256": TIMING_INDEPENDENT_RESOURCE_SET_SHA256,
                   "derived_burnout_replaced": 2 * 6 * BURN_FRAME_COUNT,
                   "previous_frames_half_open": previous["burnout_frames_half_open"],
                   "replacement_frames_half_open": [BURN_FIRST_FRAME, BURN_FIRST_FRAME + BURN_FRAME_COUNT],
                   "scope": "Only derived colored/neutral burnout changes; source art and all non-burn resource bytes remain exact"},
        "source": {"path": "tools/dins_fire_remake/" + ATLAS_NAME, "sha256": ATLAS_SHA256,
                   "size": [1536, 1024], "mode": "RGBA", "bytes": len(atlas_bytes),
                   "origin": "Accepted Dins_Fire_Burnout_and_Light_Study_Recipe.zip atlas; bytes preserved unchanged",
                   "encoding": "Three exact row crops; independent RGBA channels resized with Lanczos",
                   "rows_half_open": [list(bounds) for bounds in ATLAS_ROWS],
                   "native_row_sizes": [list(size) for size in ROW_SIZES]},
        "protected_source": {"path": "tools/dins_fire_remake/flame_source.png", "sha256": sha(source_bytes),
                   "size": source_size, "origin": "New image-generation artwork; preserved unchanged",
                   "encoding": "Grayscale luminance, Lanczos 256px; intensity replicated into RGBA"},
        "spark": {"origin": "Exact analytical core/halo/cross/diagonal glint in the encoder", "black_border": True},
        "palette": {"resource_count": len(palette_textures), "base_resources": 4, "burnout_resources": 66,
                    "source": "Already encoded native colored resources; no new source-image conversion",
                    "rgb_formula": "floor((299*R + 587*G + 114*B + 500) / 1000), replicated into R/G/B",
                    "alpha": "Each alpha byte is identical to the corresponding colored texture",
                    "metadata": "Each 92-byte resource header is identical to its colored source",
                    "cosmetic_entries": ["Magic.DinsPrimary", "Magic.DinsSecondary"],
                    "material_contract": "secondary + (primary-secondary)*neutral_RGB; original texture alpha * vertex alpha",
                    "selection_contract": "Use only when native Primary/Secondary differ from their defaults; default path retains original RGBA",
                    "missing_burnout_contract": "Use the selected neutral moving base; never fall back to colored RGB"},
        "canonical_resource_sets": {
            "format": "SHA256 of sorted UTF-8 name + NUL + resource SHA256 + LF records",
            "colored_and_recovery": {"count": len(colored_textures), "sha256": canonical_resource_hash(colored_textures)},
            "palette": {"count": len(palette_textures), "sha256": canonical_resource_hash(palette_textures)},
            "all": {"count": len(textures), "sha256": canonical_resource_hash(textures)},
            "timing_independent": {"count": len(timing_independent), "sha256": TIMING_INDEPENDENT_RESOURCE_SET_SHA256},
        },
        "burnout": {"band_rows": list(BAND_ROWS), "band_start_seconds": list(BURN_STARTS),
                    "end_seconds": END_SECONDS, "release_seconds": FIRE_START_SECONDS, "native_hz": NATIVE_RATE,
                    "fire_frames_half_open": [BURN_FIRST_FRAME, BURN_FIRST_FRAME + BURN_FRAME_COUNT],
                    "variants": 6 * BURN_FRAME_COUNT,
                    "burn_formula": "smoothstep(clamp((time-band_start)/(end_seconds-band_start),0,1)) ** 1.35",
                    "source_u": "local_u - fire_frame/native_hz * (0.27 + band*0.025)",
                    "source_v": "(atlas_row_start + local_v*atlas_row_height) / 1024",
                    "detail_formula": "0.025*burn*(sin(u*41.9+v*123.4+band*1.1)+0.5*sin(u*103.7-v*271.1))",
                    "alpha_formula": "alpha * smoothstep((alpha+detail-(burn**0.80*0.68-0.08))/0.20), if burn>0",
                    "runtime_sampling": "local U on baked frames; original unwrapped scroll is baked",
                    "color": "original RGB sampled with the accepted scroll; vertex tint remains native",
                    "limit": "Half-resolution and 8-bit encoding; visual/runtime parity requires gameplay evidence"},
        "loader_contract": {
            "texture_type_source": "libultraship/include/fast/resource/type/Texture.h",
            "parser_source": "libultraship/src/fast/resource/factory/TextureFactory.cpp",
            "upload_source": "libultraship/src/fast/interpreter.cpp: ImportTextureImg",
            "native_format": "RGBA32bpp=1; raw=1 and image=2 flags; image path uploads entire RGBA payload",
            "logical_size": [RGBA_LOGICAL_SIZE, RGBA_LOGICAL_SIZE],
            "gbi_load": "RGBA32 32x32 (4096 bytes); accepted 64-unit UVs scaled by 0.5 on submission",
            "scales": [1.0, 1.0],
        },
        "memory_budget": {"resource_count_per_host": len(textures), "decoded_bytes_per_host": decoded,
                          "resource_bytes_per_host": sum(map(len, textures.values())),
                          "default_set_decoded_bytes_per_host": colored_decoded,
                          "palette_companion_decoded_bytes_per_host": palette_decoded,
                          "decoded_limit_bytes_per_host": 72 * 1024 * 1024,
                          "maximum_texture_dimension": 768,
                          "loading": "Default colors use only original RGBA; resolve neutral companions only for custom colors and selected fade frames on demand"},
        "resources": metadata,
        "hosts": ["soh", "mm"],
        "cvar": "gEnhancements.DinsFireRemake",
        "cvar_default": 0,
        "requires_new_source_build": True,
        "runtime_tested": False,
        "scope": "Private resources only; original flame/spark bytes and native/medallion/weapon resources preserved",
    }
    return textures, manifest


def verify_committed_resources(root: Path) -> dict:
    """Check the reviewed native outputs, independently of source-art access.

    Pin the manifest as well as each resource: editing both host copies and
    their recorded hashes must not turn changed artwork into a passing gate.
    Source-recipe reproduction remains the separate, explicit --check mode.
    """
    art = root / "tools/dins_fire_remake"
    manifest_bytes = (art / "manifest.json").read_bytes()
    if sha(manifest_bytes) != COMMITTED_MANIFEST_SHA256:
        raise ValueError("Committed Din resource manifest differs from the reviewed hash")
    manifest = json.loads(manifest_bytes)
    if sha((art / "pre_linger_resource_provenance.json").read_bytes()) != PRE_LINGER_PROVENANCE_SHA256:
        raise ValueError("Immutable pre-linger resource provenance changed")
    timing_bytes = (root / TIMING_HEADER_PATH).read_bytes()
    if (sha(timing_bytes) != manifest["timing"]["header_sha256"] or
            literal_timing_values(timing_bytes) != manifest["timing"]["literal_bases"]):
        raise ValueError("Native gameplay timing differs from the committed resources")
    if (art / "native_resources.h").read_bytes() != resource_declarations():
        raise ValueError("Native resource declarations differ from the timing/name contract")

    resources = manifest["resources"]
    budget = manifest["memory_budget"]
    if len(resources) != 144 or budget["resource_count_per_host"] != len(resources):
        raise ValueError("Native resource count differs from the reviewed set")
    host_hashes = []
    for host in ("soh", "mm"):
        base = root / host / "assets/custom"
        actual = {path.relative_to(base).as_posix() for path in (base / PREFIX).rglob("*") if path.is_file()}
        if actual != set(resources):
            raise ValueError(f"Native resource closure differs for {host}: "
                             f"unexpected={sorted(actual - set(resources))}, missing={sorted(set(resources) - actual)}")
        native = {}
        decoded = total = 0
        for name, expected in resources.items():
            data = (base / name).read_bytes()
            if sha(data) != expected["sha256"]:
                raise ValueError(f"Native resource hash differs: {host}/{name}")
            metadata = validate_resource(data, tuple(expected["physical_size"]), expected["type"])
            if any(expected[key] != value for key, value in metadata.items()):
                raise ValueError(f"Native resource metadata differs: {host}/{name}")
            if max(metadata["physical_size"]) > 768:
                raise ValueError(f"Native texture exceeds the dimension budget: {host}/{name}")
            decoded += metadata["payload_bytes"]
            total += len(data)
            native[name] = data
        if (decoded != budget["decoded_bytes_per_host"] or decoded > 72 * 1024 * 1024 or
                total != budget["resource_bytes_per_host"]):
            raise ValueError(f"Native resource memory budget differs for {host}")
        sets = {
            "all": native,
            "palette": {name: data for name, data in native.items() if name.endswith("PaletteTex")},
            "colored_and_recovery": {name: data for name, data in native.items() if not name.endswith("PaletteTex")},
            "timing_independent": {name: data for name, data in native.items()
                                   if not name.removeprefix(PREFIX).startswith("ribbonBurn")},
        }
        for name, members in sets.items():
            expected = manifest["canonical_resource_sets"][name]
            if len(members) != expected["count"] or canonical_resource_hash(members) != expected["sha256"]:
                raise ValueError(f"Native canonical resource set differs: {host}/{name}")
        host_hashes.append(canonical_resource_hash(native))
    if host_hashes[0] != host_hashes[1]:
        raise ValueError("Native host resource parity failed")
    return {"result": "PASS", "operation": "check-native", "host_resources": 288,
            "host_parity": True, "source_regenerated": False,
            "manifest_sha256": COMMITTED_MANIFEST_SHA256,
            "canonical_resource_sets": manifest["canonical_resource_sets"],
            "memory_budget": budget, "runtime_tested": False}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--check", action="store_true", help="Compare regenerated resources and manifest to committed files; requires source PNGs")
    modes.add_argument("--check-native", action="store_true", help="Validate committed native resources without source PNGs")
    parser.add_argument("--archive", type=Path, help="Write an optional deterministic private-resource O2R (requires new source)")
    args = parser.parse_args()
    if args.check_native:
        if args.archive:
            parser.error("--archive requires source-art regeneration; use it with --check or encoding mode")
        print(json.dumps(verify_committed_resources(ROOT), indent=2))
        return
    missing_sources = [name for name in (ATLAS_NAME, "flame_source.png") if not (ART / name).is_file()]
    if missing_sources:
        parser.error("Source-art regeneration requires separately supplied files in tools/dins_fire_remake/: "
                     + ", ".join(missing_sources) + "; use --check-native to validate the committed outputs")
    textures, manifest = generated_resources()
    outputs = {ROOT / host / "assets/custom" / name: data for host in ("soh", "mm") for name, data in textures.items()}
    outputs[ART / "manifest.json"] = (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode()
    outputs[ART / "native_resources.h"] = resource_declarations()
    for path, data in outputs.items():
        if args.check:
            if not path.is_file() or path.read_bytes() != data:
                raise SystemExit(f"Resource differs from deterministic recipe: {path.relative_to(ROOT)}")
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
    # Retire only the explicit old generated burnout names. Never remove an
    # unknown private file or a timing-independent input as directory cleanup.
    previous_first, previous_end = manifest["pre_linger_checkpoint"]["previous_frames_half_open"]
    retired = {PREFIX + f"ribbonBurn{band}_{frame}{suffix}Tex"
               for band in range(6) for frame in range(previous_first, previous_end)
               for suffix in ("", "Palette")} - set(textures)
    for host in ("soh", "mm"):
        base = ROOT / host / "assets/custom"
        if not args.check:
            for name in sorted(retired):
                path = base / name
                if path.exists():
                    if not path.is_file():
                        raise SystemExit(f"Retired burnout path is not a regular file: {path.relative_to(ROOT)}")
                    path.unlink()
        actual = {str(path.relative_to(base)) for path in (base / PREFIX).rglob("*") if path.is_file()}
        if actual != set(textures):
            raise SystemExit(f"Private resource closure differs for {host}: unexpected={sorted(actual - set(textures))}, missing={sorted(set(textures) - actual)}")
    # Compare actual host bytes after the deterministic-recipe check/write.
    for name in textures:
        if (ROOT / "soh/assets/custom" / name).read_bytes() != (ROOT / "mm/assets/custom" / name).read_bytes():
            raise SystemExit(f"Host resource parity failed: {name}")
    archive_result = None
    if args.archive:
        args.archive.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(args.archive, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            for name, data in sorted(textures.items()):
                info = zipfile.ZipInfo(name, ARCHIVE_DATE)
                info.create_system = 3
                info.external_attr = 0o100644 << 16
                info.compress_type = zipfile.ZIP_DEFLATED
                archive.writestr(info, data, compresslevel=9)
        with zipfile.ZipFile(args.archive) as archive:
            if archive.testzip() is not None or archive.namelist() != sorted(textures):
                raise SystemExit("Archive verification failed")
            if any(archive.read(name) != data for name, data in textures.items()):
                raise SystemExit("Archive payload verification failed")
            if any(info.date_time != ARCHIVE_DATE or info.compress_type != zipfile.ZIP_DEFLATED
                   or info.external_attr != 0o100644 << 16 for info in archive.infolist()):
                raise SystemExit("Archive deterministic metadata verification failed")
        archive_bytes = args.archive.read_bytes()
        archive_result = {"path": str(args.archive), "bytes": len(archive_bytes),
                          "sha256": sha(archive_bytes), "entries": len(textures)}
    print(json.dumps({"result": "PASS", "operation": "check" if args.check else "encode",
                      "host_resources": len(textures) * 2, "host_parity": True,
                      "accepted_atlas_sha256": ATLAS_SHA256, "protected_resource_sha256": LEGACY_SHA256,
                      "memory_budget": manifest["memory_budget"], "archive": archive_result,
                      "canonical_resource_sets": manifest["canonical_resource_sets"],
                      "timing": manifest["timing"], "pre_linger_checkpoint": manifest["pre_linger_checkpoint"],
                      "runtime_tested": False}, indent=2))


if __name__ == "__main__":
    main()
