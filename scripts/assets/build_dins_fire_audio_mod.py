#!/usr/bin/env python3
"""Build a deterministic raw-WAV Din's Fire .o2r; no audio normalization.

Input mode uses Python's standard library only. --defaults also uses the existing
NumPy authoring recipe, without changing that recipe or its checked-in assets.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]
CUES = ("charge", "contact", "release", "loop", "burnout")
ARCHIVE_PREFIX = "audio/dins_fire_remake/"
RATE = 32000
MAX_SAMPLES = RATE * 10
MAX_FILE_BYTES = MAX_SAMPLES * 2 + 65536


def validate_wav(blob: bytes) -> bytes:
    """Return PCM bytes under the shared native DinsFireSfxSamples contract."""
    if not 12 <= len(blob) <= MAX_FILE_BYTES:
        raise ValueError("WAV file must be 12..705536 bytes")
    if blob[:4] != b"RIFF" or blob[8:12] != b"WAVE":
        raise ValueError("expected little-endian RIFF/WAVE")
    if struct.unpack_from("<I", blob, 4)[0] != len(blob) - 8:
        raise ValueError("RIFF size must exactly cover the file")
    fmt = None
    pcm = None
    cursor = 12
    while cursor < len(blob):
        if len(blob) - cursor < 8:
            raise ValueError("truncated RIFF chunk header")
        chunk_id = blob[cursor:cursor + 4]
        size = struct.unpack_from("<I", blob, cursor + 4)[0]
        start = cursor + 8
        end = start + size
        padded_end = end + (size & 1)
        if padded_end > len(blob):
            raise ValueError("RIFF chunk or odd-byte padding exceeds file")
        if chunk_id == b"fmt ":
            if fmt is not None or size < 16:
                raise ValueError("expected one fmt chunk of at least 16 bytes")
            fmt = struct.unpack_from("<HHIIHH", blob, start)
            if fmt != (1, 1, RATE, RATE * 2, 2, 16):
                raise ValueError("expected PCM format1, mono, 32000Hz, 16-bit, byteRate64000, blockAlign2")
            if size != 16 and (size < 18 or struct.unpack_from("<H", blob, start + 16)[0] != 0):
                raise ValueError("only an empty PCM fmt extension is supported")
        elif chunk_id == b"data":
            if pcm is not None:
                raise ValueError("duplicate data chunk")
            if size == 0 or size % 2 or size > MAX_SAMPLES * 2:
                raise ValueError("data must contain 1..320000 complete signed-16 samples")
            pcm = blob[start:end]
        # Unknown metadata chunks are allowed only within the same file bound.
        cursor = padded_end
    if fmt is None or pcm is None:
        raise ValueError("WAV must contain one fmt and one nonempty data chunk")
    return pcm


def wav_from_pcm(pcm: bytes) -> bytes:
    fmt = struct.pack("<HHIIHH", 1, 1, RATE, RATE * 2, 2, 16)
    chunks = b"fmt " + struct.pack("<I", len(fmt)) + fmt
    chunks += b"data" + struct.pack("<I", len(pcm)) + pcm
    return b"RIFF" + struct.pack("<I", len(chunks) + 4) + b"WAVE" + chunks


def default_payloads() -> dict[str, bytes]:
    recipe = ROOT / "scripts/assets/build_dins_fire_audio.py"
    spec = importlib.util.spec_from_file_location("dins_fire_existing_recipe", recipe)
    if spec is None or spec.loader is None:
        raise ValueError("cannot load existing default recipe")
    module = importlib.util.module_from_spec(spec)
    old_bytecode_setting = sys.dont_write_bytecode
    try:
        sys.dont_write_bytecode = True
        spec.loader.exec_module(module)
    finally:
        sys.dont_write_bytecode = old_bytecode_setting
    clips = module.synthesize()
    include = module.render_include(clips)
    manifest = module.render_manifest(clips, include)
    if module.INC.read_bytes() != include.encode() or module.MANIFEST.read_bytes() != manifest.encode():
        raise ValueError("default recipe does not match checked-in PCM/manifest; refusing to export different audio")
    return {cue: wav_from_pcm(clips[cue].astype("<i2").tobytes()) for cue in CUES}


def build_archive(payloads: dict[str, bytes], output: Path) -> dict:
    clips = {}
    for cue in CUES:
        if cue not in payloads:
            continue
        blob = payloads[cue]
        try:
            pcm = validate_wav(blob)
        except ValueError as error:
            raise ValueError(f"{cue}.wav: {error}") from error
        clips[cue] = {
            "archive_path": ARCHIVE_PREFIX + cue + ".wav",
            "wav_bytes": len(blob), "wav_sha256": hashlib.sha256(blob).hexdigest(),
            "samples": len(pcm) // 2, "duration_seconds": len(pcm) / (2 * RATE),
            "pcm_s16le_sha256": hashlib.sha256(pcm).hexdigest(),
        }
    if not clips:
        raise ValueError("no named cue WAVs found; expected at least one of " + ", ".join(c + ".wav" for c in CUES))
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_STORED) as archive:
        for cue, metadata in clips.items():
            info = zipfile.ZipInfo(metadata["archive_path"], date_time=(1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_STORED
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            info.extra = b""
            info.comment = b""
            archive.writestr(info, payloads[cue])
    blob = output.read_bytes()
    return {"format": "ZIP-format .o2r, raw WAV entries only", "sample_rate": RATE,
            "archive_path": str(output.resolve()), "archive_bytes": len(blob),
            "archive_sha256": hashlib.sha256(blob).hexdigest(), "clips": clips}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--input-dir", type=Path, help="directory containing any subset of charge/contact/release/loop/burnout.wav")
    source.add_argument("--defaults", action="store_true", help="export the unchanged current compiled softer defaults")
    parser.add_argument("--output", type=Path, required=True, help="destination .o2r (ordinary deterministic ZIP)")
    parser.add_argument("--export-dir", type=Path, help="also export named editable WAVs; only with --defaults")
    parser.add_argument("--report-json", type=Path, help="optional separate artifact/hash report, never placed inside the mod")
    args = parser.parse_args()
    try:
        if args.export_dir and not args.defaults:
            raise ValueError("--export-dir requires --defaults")
        if args.input_dir is not None and not args.input_dir.is_dir():
            raise ValueError("--input-dir must name an existing directory")
        if args.output.suffix.lower() != ".o2r":
            raise ValueError("--output must end with .o2r")
        payloads = default_payloads() if args.defaults else {
            cue: (args.input_dir / (cue + ".wav")).read_bytes()
            for cue in CUES if (args.input_dir / (cue + ".wav")).is_file()
        }
        report = build_archive(payloads, args.output)
        if args.export_dir:
            args.export_dir.mkdir(parents=True, exist_ok=True)
            for cue, blob in payloads.items():
                (args.export_dir / (cue + ".wav")).write_bytes(blob)
        if args.report_json:
            args.report_json.parent.mkdir(parents=True, exist_ok=True)
            args.report_json.write_text(json.dumps(report, indent=2) + "\n")
        print(f"Built {args.output}: {len(payloads)} cue(s), {report['archive_bytes']} bytes")
        print("SHA-256: " + report["archive_sha256"])
    except (OSError, ValueError) as error:
        parser.exit(1, str(error) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
