"""Exercise the production Gakki sampler, mixer, font repatch and song jingle.

The fixture uses real OoT audio types and the unmodified production function
bodies. Archive/resource services and ADPCM decoding are controlled boundaries;
the PCM is synthetic. This does not prove the donor pack or audible gameplay.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "tests/nei_item_stow"))
from run_ballchain_tests import flags

LOADER = "soh/mods/transformation_masks/assets/mm_asset_loader.cpp"
JINGLE = "soh/mods/transformation_masks/form_song_jingle.inc.c"
CASES = (
    "form-mapping", "native-custom-controls", "held-bend", "voice-ownership",
    "jingle-lifecycle", "explicit-sustain", "one-shot", "donor-origin",
    "missing-resources", "unrelated-audio",
)


def function(source, name):
    match = re.search(r"^[^\n{};]+\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    assert match, "Missing production function: " + name
    cursor, depth = match.end(), 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[match.start():cursor]


def source_text(path, baseline):
    if baseline:
        return subprocess.run(["git", "show", baseline + ":" + path], cwd=ROOT,
                              text=True, capture_output=True, check=True).stdout
    return (ROOT / path).read_text()


def production_fixture(source):
    chunks = []
    if re.search(r"^void MmGakki_UpdatePitch\(", source, re.M):
        chunks.append("#define FORM_AUDIO_HAS_UPDATE_PITCH 1\n")
    if re.search(r"^void MmGakki_StopJingleNote\(", source, re.M):
        chunks.append("#define FORM_AUDIO_HAS_JINGLE_OWNER 1\n")
    chunks.append(source[source.index("#define MM_DIRECT_MAX_SOUNDS"):
                         source.index("// Configure ADSR envelope for a playing sound.")])
    chunks.append(source[source.index("#define MM_ADSR_TICK_SAMPLES"):
                         source.index("// SFX → Instrument Mapping Table")])
    chunks.extend(function(source, name) for name in (
        "MmDirectAudio_SoundValid", "MmDirectAudio_GetInstrumentSoundDirect"))
    reader = source.index("typedef struct {\n    const u8* data;", source.index("// Minimal binary reader helpers"))
    chunks.append(source[reader:source.index("SoundFont* MmSfx_LoadFont(", reader)])
    chunks.append(function(source, "MmDirectAudio_PlaySingle"))
    table = source.rfind("typedef struct {", 0, source.index("} MmGakkiInstrument;"))
    chunks.append(source[table:source.index("void MmSfx_UpdateGameFrame(", table)])
    chunks.extend(function(source, name) for name in (
        "MmDirectAudio_MixInto", "MmDirectAudio_StopById", "MmDirectAudio_StopAll"))
    return "\n".join(chunks)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", help="Read the production bodies at this git ref")
    parser.add_argument("--report", type=Path)
    parser.add_argument("cases", nargs="*", choices=CASES)
    args = parser.parse_args()
    cases = args.cases or CASES
    loader = source_text(LOADER, args.baseline)
    jingle = source_text(JINGLE, args.baseline)
    report = {
        "boundary": "production Gakki/sample selection/repatch/mixer/jingle; synthetic PCM and archive services",
        "sanitizers": "ASan and UBSan; leak scanning disabled for traced runners",
        "baseline": args.baseline,
        "source_sha256": {
            LOADER: hashlib.sha256(loader.encode()).hexdigest(),
            JINGLE: hashlib.sha256(jingle.encode()).hexdigest(),
        },
        "hosts": [],
    }
    failures = []
    with tempfile.TemporaryDirectory(prefix="form-instrument-audio-") as temporary:
        directory = Path(temporary)
        (directory / "form_audio_production.inc").write_text(production_fixture(loader))
        (directory / "form_jingle_production.inc").write_text(jingle)
        for combined in (False, True):
            host = "ComboShip OoT" if combined else "solo SoH"
            binary = directory / ("combo" if combined else "solo")
            command = [os.environ.get("CXX", "c++"), "-std=gnu++20", "-O1", "-g",
                       "-Wall", "-Wextra", "-Wno-unused-function", "-Wno-unused-variable",
                       "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                       "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                       "-I" + str(ROOT / "tests/nei_leaf/stubs"), "-I" + temporary]
            command += [flag for flag in flags()
                        if not flag.startswith("-std=") and flag != "-Wno-incompatible-pointer-types"]
            if combined:
                command += ["-DCOMBO_BUILD", "-DSOH_BUILD_DLL"]
            command += [str(ROOT / "tests/form_instruments/audio_test.cpp"), "-lm", "-o", str(binary)]
            built = subprocess.run(command, text=True, capture_output=True, timeout=120)
            if built.returncode:
                sys.stderr.write(built.stderr)
                raise SystemExit(built.returncode)
            outcomes = []
            for case in cases:
                run = subprocess.run([str(binary), case], text=True, capture_output=True, timeout=20,
                                     env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
                outcome = {"case": case, "passed": run.returncode == 0}
                if run.returncode:
                    outcome["stderr"] = run.stderr[-4000:]
                    failures.append(host + ":" + case)
                outcomes.append(outcome)
                print(f"{'PASS' if outcome['passed'] else 'FAIL'} {host}: {case}", flush=True)
            report["hosts"].append({"host": host, "outcomes": outcomes})
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + "\n")
    if failures:
        print("FAIL form instrument audio: " + ", ".join(failures), file=sys.stderr)
        raise SystemExit(1)
    print(f"PASS form instrument audio: {len(cases) * 2} scenarios across solo SoH and ComboShip")


if __name__ == "__main__":
    main()
