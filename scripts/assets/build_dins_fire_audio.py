#!/usr/bin/env python3
"""Author deterministic Din's Fire PCM; no game recordings or external sources.

Only NumPy is needed for offline synthesis. The application consumes the checked
in int16 arrays, so it does not need Python, NumPy, a WAV loader, or sound packs.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys
import wave

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
INC = ROOT / "combo/assets/dins_fire_sfx_pcm.inc"
MANIFEST = ROOT / "tools/dins_fire_audio/manifest.json"
RATE = 32000
SEED = 0xD175F1AE
CLIPS = (
    ("charge", "kCwDinsChargePcm", 1.0, 0.22),
    ("contact", "kCwDinsContactPcm", 0.35, 0.14),
    ("release", "kCwDinsReleasePcm", 1.3, 0.30),
    ("loop", "kCwDinsLoopPcm", 3.2, 0.18),
    ("burnout", "kCwDinsBurnoutPcm", 0.6, 0.19),
)
# Held unchanged for this focused revision from recipe version 1.
# Keep all preceding PRNG call sizes/counts stable; tuning filters, weights and
# envelopes changes no random draws. Explicit hashes catch accidental drift.
PRESERVED_PCM_SHA256 = {
    "loop": "9df85156713a3a91675b3db7749986cbdf785f3a3a9565e94f0fe2c66add3961",
    "burnout": "1b1d718d3bcdac02701d560f6bdb8b8ef0e3c9fbe0e146ace9d451c4937a08b7",
}


def noise(n: int, rng: np.random.Generator, low: float, high: float,
          slope: float = 0.0) -> np.ndarray:
    """Periodic band-shaped noise with smooth rolloffs and no DC component."""
    f = np.fft.rfftfreq(n, 1.0 / RATE)
    bins = np.fft.rfft(rng.normal(size=n))
    safe = np.maximum(f, 1.0)
    shape = (1.0 - np.exp(-(f / low) ** 4)) * np.exp(-(f / high) ** 4)
    shape *= (safe / 400.0) ** slope
    shape[0] = 0.0
    y = np.fft.irfft(bins * shape, n=n)
    return y / np.sqrt(np.mean(y * y))


def envelope(t: np.ndarray, attack: float, tail: float) -> np.ndarray:
    """Smooth start/end, preserving silence at both edges of one-shot clips."""
    end = (len(t) - 1) / RATE
    a = np.clip(t / attack, 0, 1)
    b = np.clip((end - t) / tail, 0, 1)
    return np.sin(a * np.pi / 2) ** 2 * np.sin(b * np.pi / 2) ** 2


def crackles(n: int, rng: np.random.Generator, count: int,
             periodic: bool = False) -> np.ndarray:
    """Soft short filtered-noise embers rather than sharp white-noise clicks."""
    t = np.arange(n) / RATE
    duration = n / RATE
    bed = noise(n, rng, 1000, 6100, -0.2)
    env = np.zeros(n)
    for _ in range(count):
        center = rng.uniform(0, duration)
        width = rng.uniform(0.0025, 0.009)
        d = np.abs(t - center)
        if periodic:
            d = np.minimum(d, duration - d)
        env += rng.uniform(0.15, 0.65) * np.exp(-0.5 * (d / width) ** 2)
    return env * bed


def synthesize() -> dict[str, np.ndarray]:
    rng = np.random.Generator(np.random.PCG64(SEED))
    result = {}
    for name, _, duration, peak in CLIPS:
        n = round(duration * RATE)
        t = np.arange(n) / RATE
        u = t / duration
        if name == "charge":
            # A gentle gathering breath with stationary spectral bands. No
            # oscillator, pitch ramp, filter sweep, or accelerating tremolo.
            soft = noise(n, rng, 650, 2200, -0.25)
            haze = noise(n, rng, 1500, 4500, -0.25)
            air = noise(n, rng, 3300, 7200, -0.3)
            gather = 0.55 + 0.45 * (u * u * (3 - 2 * u))
            y = (0.58 * soft + 0.26 * haze + 0.07 * air) * gather
            y *= envelope(t, 0.22, 0.14)
        elif name == "contact":
            # Quiet flame catch rather than a ground thud: no bass oscillator.
            flame = noise(n, rng, 550, 2800, -0.25)
            whisper = noise(n, rng, 1800, 4500, -0.3)
            y = (0.7 * flame + 0.16 * whisper) * np.exp(-t * 5.8)
            y *= envelope(t, 0.045, 0.13)
        elif name == "release":
            # Flame opens over roughly a quarter-second without sub-bass or an
            # impact transient. Preserve the v1 noise/crackle random draw count.
            warmth = noise(n, rng, 250, 1100, -0.25)
            flame = noise(n, rng, 650, 3000, -0.3)
            air = noise(n, rng, 2000, 6300, -0.4)
            bloom = (1 - np.exp(-(t / 0.19) ** 2)) * np.exp(-t * 1.8)
            y = (0.20 * warmth + 0.65 * flame + 0.12 * air) * bloom
            y += 0.035 * crackles(n, rng, 22) * bloom
            y *= envelope(t, 0.12, 0.30)
        elif name == "loop":
            # Every constituent is periodic across the whole 3.2-second region.
            # The slow modulation uses integer cycle counts, as does FFT noise.
            warmth = noise(n, rng, 52, 680, -0.45)
            flame = noise(n, rng, 300, 2600, -0.4)
            air = noise(n, rng, 1600, 5500, -0.45)
            flutter = (0.83 + 0.10 * np.sin(2 * np.pi * 3 * u)
                       + 0.06 * np.sin(2 * np.pi * 7 * u + 1.1))
            y = (0.67 * warmth + 0.42 * flame + 0.09 * air) * flutter
            y += 0.27 * crackles(n, rng, 43, periodic=True)
        else:
            # Breath of embers settling out; the engine separately fades the bed.
            y = (0.55 * noise(n, rng, 100, 1200, -0.45)
                 + 0.20 * noise(n, rng, 900, 4800, -0.4)
                 + 0.18 * crackles(n, rng, 10))
            y *= np.exp(-t * 4.4) * envelope(t, 0.023, 0.20)

        assert np.all(np.isfinite(y)), f"nonfinite synthesis: {name}"
        # Remove any envelope-induced DC before applying final one-shot taper.
        y -= np.mean(y)
        if name != "loop":
            y *= envelope(t, 0.002, 0.005)
        y *= peak / np.max(np.abs(y))
        pcm = np.rint(y * 32767).astype("<i2")
        if name == "loop":
            # Rotate a periodic signal to a near-zero, low-derivative crossing.
            # Rotation changes neither its spectrum nor its periodicity. This
            # also makes the initial loop start gentle before engine smoothing.
            v = pcm.astype(np.int32)
            cost = np.abs(v) + 2 * np.abs(v - np.roll(v, 1))
            pcm = np.roll(pcm, -int(np.argmin(cost)))
        result[name] = pcm
    validate(result)
    return result


def metrics(pcm: np.ndarray) -> dict:
    v = pcm.astype(np.float64)
    delta = np.abs(np.diff(v))
    peak = float(np.max(np.abs(v))) / 32768
    rms = float(np.sqrt(np.mean((v / 32768) ** 2)))
    return {
        "samples": len(pcm), "duration_seconds": len(pcm) / RATE,
        "pcm_s16le_sha256": hashlib.sha256(pcm.tobytes()).hexdigest(),
        "peak_dbfs": round(20 * np.log10(peak), 6),
        "rms_dbfs": round(20 * np.log10(rms), 6),
        "dc_lsb": round(float(np.mean(v)), 6),
        "first_sample": int(pcm[0]), "last_sample": int(pcm[-1]),
        "boundary_step_lsb": int(abs(int(pcm[-1]) - int(pcm[0]))),
        "adjacent_step_rms_lsb": round(float(np.sqrt(np.mean(delta ** 2))), 6),
        "adjacent_step_p95_lsb": round(float(np.percentile(delta, 95)), 6),
        "adjacent_step_max_lsb": int(np.max(delta)),
    }


def validate(clips: dict[str, np.ndarray]) -> None:
    assert sum(pcm.nbytes for pcm in clips.values()) < 512 * 1024
    for name, _, duration, _ in CLIPS:
        pcm = clips[name]
        assert len(pcm) == round(duration * RATE)
        assert pcm.dtype == np.dtype("<i2")
        assert np.all(np.isfinite(pcm))
        m = metrics(pcm)
        assert m["peak_dbfs"] < -2.8, (name, m)
        assert abs(m["dc_lsb"]) < 8, (name, m)
        assert np.max(np.abs(pcm.astype(np.int32))) < 32767
        if name == "loop":
            assert m["boundary_step_lsb"] < m["adjacent_step_p95_lsb"]
            assert abs(m["first_sample"]) < 32
            assert abs(m["last_sample"]) < 32
        else:
            assert pcm[0] == pcm[-1] == 0
    # The quiet catch stays subordinate to the release bloom in this revision.
    assert metrics(clips["contact"])["peak_dbfs"] < metrics(clips["release"])["peak_dbfs"] - 5
    for name, expected in PRESERVED_PCM_SHA256.items():
        assert metrics(clips[name])["pcm_s16le_sha256"] == expected, name


def render_include(clips: dict[str, np.ndarray]) -> str:
    lines = ["// Original procedural Din's Fire samples. No imported recordings.",
             "// Regenerate: python3 scripts/assets/build_dins_fire_audio.py",
             "// See tools/dins_fire_audio/README.md for recipe and listening limits.",
             "#ifndef CW_DINS_FIRE_SFX_PCM_INC", "#define CW_DINS_FIRE_SFX_PCM_INC",
             "#include <stdint.h>", "#define CW_DINS_SFX_SAMPLE_RATE 32000", ""]
    for name, symbol, _, _ in CLIPS:
        pcm = clips[name]
        lines.append(f"static const int16_t {symbol}[] = {{")
        for i in range(0, len(pcm), 16):
            lines.append("    " + ", ".join(str(int(v)) for v in pcm[i:i + 16]) + ",")
        lines.extend(["};", ""])
    lines.extend(["#endif // CW_DINS_FIRE_SFX_PCM_INC", ""])
    return "\n".join(lines)


def render_manifest(clips: dict[str, np.ndarray], include: str) -> str:
    data = {
        "schema": 1, "provenance": "Original procedural synthesis; no imported recordings, vanilla samples, speech, or music.",
        "recipe": "scripts/assets/build_dins_fire_audio.py", "recipe_version": 2,
        "revision_reason": "User heard the first buildup as a racecar and ignition as too explosive; remove charge pitch/modulation and bass impact, soften contact and bloom release.",
        "preserved_v1_pcm_sha256": PRESERVED_PCM_SHA256,
        "generator": "NumPy PCG64 and FFT; authored with NumPy 2.3.5",
        "random_seed": SEED, "sample_rate": RATE, "channels": 1,
        "format": "signed 16-bit mono PCM; hashes use little-endian sample bytes",
        "decoded_bytes_per_host": sum(v.nbytes for v in clips.values()),
        "include_sha256": hashlib.sha256(include.encode()).hexdigest(),
        "loop_region": "The entire kCwDinsLoopPcm array repeats; no embedded attack or release.",
        "clips": {name: {"symbol": symbol, **metrics(clips[name])}
                  for name, symbol, _, _ in CLIPS},
        "evidence_limit": "Numerical sample validation only. Sound quality, in-game balance, spatialization and acceptance require listening.",
    }
    return json.dumps(data, indent=2) + "\n"


def write_wav(path: Path, pcm: np.ndarray) -> None:
    with wave.open(str(path), "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(pcm.astype("<i2").tobytes())


def audition(directory: Path, clips: dict[str, np.ndarray]) -> None:
    directory.mkdir(parents=True, exist_ok=True)
    for name, pcm in clips.items():
        write_wav(directory / f"dins_fire_{name}.wav", pcm)
    # Offline recipe audition only: actual engine adapter controls the final mix.
    out = np.zeros(round(8.2 * RATE), dtype=np.float64)
    def add(name: str, start: float) -> None:
        at = round(start * RATE)
        out[at:at + len(clips[name])] += clips[name].astype(np.float64)
    add("charge", 0.0)
    add("contact", 1.0)
    add("release", 1.05)
    start, fade, end = (round(x * RATE) for x in (1.05, 7.35, 7.95))
    bed = np.resize(clips["loop"], end - start).astype(np.float64)
    bed[:round(0.18 * RATE)] *= np.linspace(0, 1, round(0.18 * RATE))
    bed[fade - start:] *= np.linspace(1, 0, end - fade)
    out[start:end] += bed
    add("burnout", 7.35)
    assert np.max(np.abs(out)) < 32767, "audition sum clips"
    write_wav(directory / "dins_fire_offline_recipe_audition.wav", np.rint(out).astype("<i2"))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="validate and compare generated files without changing them")
    parser.add_argument("--audition-dir", type=Path, help="optional destination for untracked source-clip and offline-mix WAVs")
    args = parser.parse_args()
    clips = synthesize()
    include = render_include(clips)
    manifest = render_manifest(clips, include)
    if args.check:
        for path, expected in ((INC, include), (MANIFEST, manifest)):
            if not path.exists() or path.read_bytes() != expected.encode():
                print(f"Stale/missing generated asset: {path.relative_to(ROOT)}", file=sys.stderr)
                return 1
        print("PASS: PCM finite/bounded; durations/DC/dynamics/loop/budget valid; include and manifest match deterministic recipe")
    else:
        INC.parent.mkdir(parents=True, exist_ok=True)
        MANIFEST.parent.mkdir(parents=True, exist_ok=True)
        INC.write_text(include)
        MANIFEST.write_text(manifest)
        print(f"Generated {sum(v.nbytes for v in clips.values())} decoded PCM bytes per host")
    if args.audition_dir:
        audition(args.audition_dir, clips)
        print(f"WAV audition written to {args.audition_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
