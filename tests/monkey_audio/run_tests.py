#!/usr/bin/env python3
"""Exercise real monkey actor updates and the MM sound-bank/Sequence_0 bridge."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/nei_item_stow"))
from run_ballchain_tests import flags


def function(source, name):
    match = re.search(r"^[^\n{};]+\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    assert match, name
    cursor, depth = match.end(), 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[match.start():cursor]


def main():
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
    env["C_INCLUDE_PATH"] = os.pathsep.join(
        [str(ROOT / "combo"), str(ROOT / "combo/menu"), env.get("C_INCLUDE_PATH", "")])
    subprocess.run([sys.executable, "-B", "scripts/diagnostics/run_mm_rendering_regression.py", "--viewer-only"],
                   cwd=ROOT, check=True, env=env)
    assets = (ROOT / "soh/mods/transformation_masks/assets/mm_asset_loader.cpp").read_text()
    lock = re.search(r"struct MmAudioScopedLock \{.*?\n\};", assets, re.S).group(0)
    fixture = (ROOT / "tests/monkey_audio/bank_test.cpp").read_text().replace(
        "/* PRODUCTION_EMITTER_STOP */", lock + "\n" + function(assets, "MmSfx_StopAtPos"))
    with tempfile.TemporaryDirectory(prefix="monkey-audio-") as temp:
        directory = Path(temp)
        test = directory / "bank.cpp"
        test.write_text(fixture)
        engine = directory / "engine.cpp"
        engine.write_text('''#include "mods/sound_translator/mm_audio_sfx.h"
#undef SFX_FLAG
#undef SFX_BANK_SHIFT
#undef SFX_BANK_MASK
#undef SFX_INDEX
#undef SFX_BANK
#include "mods/sound_translator/mm_audio_sfx.cpp"
''')
        includes = [flag for flag in flags()
                    if not flag.startswith("-std=") and flag != "-Wno-incompatible-pointer-types"]
        binary = directory / "bank"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=gnu++20",
                        "-I" + str(ROOT / "tests/nei_leaf/stubs"), *includes,
                        "-O1", "-g", "-fno-pie", "-no-pie", "-fsanitize=address,undefined",
                        "-fno-sanitize-recover=all", "-ffunction-sections", "-fdata-sections",
                        str(test), str(engine),
                        str(ROOT / "soh/mods/sound_translator/mm_audio_sfx_dispatch.cpp"),
                        str(ROOT / "soh/mods/sound_translator/mm_audio_sfx_params.cpp"),
                        "-Wl,--gc-sections", "-lm", "-o", str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], check=True, env=env, timeout=10)


if __name__ == "__main__":
    main()
