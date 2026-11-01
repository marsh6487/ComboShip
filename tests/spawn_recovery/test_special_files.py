#!/usr/bin/env python3
"""Replay OoT's special-file command through the production resource loader.

The production Blob, Scene, SetSpecialObjects and PlayState types are used.
Archive IO and the active resource-manager context are deterministic adapters;
the loader and special-file command bodies are extracted without alteration.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=ROOT)
    args = parser.parse_args()
    play_source = (args.source / "soh/soh/z_play_otr.cpp").read_text()
    scene_source = (args.source / "soh/soh/z_scene_otr.cpp").read_text()
    body = function(play_source, "Ship::IResource* OTRPlay_LoadFile(") + "\n"
    body += function(scene_source, "bool Scene_CommandSpecialFiles(") + "\n"
    fixture = (ROOT / "tests/spawn_recovery/special_files_test.cpp").read_text()
    fixture = fixture.replace("/* PRODUCTION */", body)
    includes = ("soh", "soh/include", "libultraship/include", "combo")
    sources = ("libultraship/src/ship/resource/Resource.cpp",
               "libultraship/src/ship/resource/type/Blob.cpp",
               "soh/soh/resource/type/Scene.cpp",
               "soh/soh/resource/type/scenecommand/SetSpecialObjects.cpp")
    with tempfile.TemporaryDirectory(prefix="oot-special-files-") as temporary:
        directory = Path(temporary)
        # Resource.cpp only uses SPDLOG_TRACE here. Keep its real lifetime and
        # metadata behavior without installing the full logger for a source check.
        logger = directory / "spdlog/spdlog.h"
        logger.parent.mkdir()
        logger.write_text("#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n")
        source = directory / "test.cpp"
        source.write_text(fixture)
        failed = False
        for combo in (True, False):
            variant = "combo" if combo else "standalone"
            binary = directory / variant
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-g",
                            "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=6", "-DSPDLOG_ACTIVE_LEVEL=6",
                            *(["-DCOMBO_BUILD"] if combo else []),
                            "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                            "-I" + str(directory),
                            *["-I" + str(ROOT / include) for include in includes],
                            str(source), *[str(ROOT / path) for path in sources], "-o", str(binary)], check=True)
            for case in ("field", "dungeon", "missing", "wrong-type", "throws", "none"):
                for alt in ("0", "1"):
                    result = subprocess.run([str(binary), case, alt], capture_output=True, text=True,
                                            env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
                    print(f"{variant} {case} alt={alt}: {'PASS' if result.returncode == 0 else 'FAIL'}", flush=True)
                    if result.returncode:
                        print(result.stderr.strip(), flush=True)
                        failed = True
        return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
