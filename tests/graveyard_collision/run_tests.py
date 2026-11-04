#!/usr/bin/env python3
"""Exercise production graveyard patches and exit bounds across Alt teardown."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
CASES = ("mod32-retained-types", "custom39", "custom32", "vanilla31", "vanilla32", "teardown-reentry")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    source = ROOT / "soh/soh/Enhancements/Restorations/GraveHoleJumps.cpp"
    # Include the entire production body, not a separately implemented policy.
    production = re.sub(r"^#include[^\n]*$", "", source.read_text(), flags=re.M)
    fixture = (HERE / "native_test.cpp").read_text().replace("/* PRODUCTION */", production)
    results = []
    with tempfile.TemporaryDirectory(prefix="graveyard-collision-") as tmp:
        cpp = Path(tmp) / "test.cpp"
        cpp.write_text(fixture)
        binary = Path(tmp) / "test"
        for mode in ("solo", "combined"):
            command = [os.environ.get("CXX", "c++"), "-std=c++20", "-g", "-O1", "-w",
                       "-DGBI_FLOAT_MTX", "-DGBI_S32_VTX", "-DF3DEX_GBI_2",
                       "-DCONTROLLERBUTTONS_T=uint32_t", '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"',
                       "-fmax-errors=3", "-fsanitize=address,undefined",
                       "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
            if mode == "combined":
                command.append("-DCOMBO_BUILD")
            command += ["-I" + str(ROOT / path) for path in (
                "tests/oot_room_background/include", "soh", "soh/include", "soh/include/PR",
                "soh/src", "soh/assets", "soh/soh", "libultraship/include", "combo", "combo/menu")]
            subprocess.run([*command, str(cpp), "-o", str(binary)], check=True)
            for case in CASES:
                done = subprocess.run([str(binary), case], capture_output=True, text=True,
                                      env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0",
                                           "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"})
                results.append({"mode": mode, "case": case, "returncode": done.returncode})
                print(f"{'PASS' if done.returncode == 0 else 'FAIL'} {mode}/{case}")
                if done.returncode:
                    print("\n".join(done.stderr.splitlines()[:12]))
    # The shared guard is also used by Hylia's Grace's fairy-mode exit path.
    with tempfile.TemporaryDirectory(prefix="scene-exit-bounds-") as tmp:
        binary = Path(tmp) / "test"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-fsanitize=address,undefined",
                        "-fno-pie", "-no-pie", "-I" + str(ROOT / "soh/include"),
                        str(HERE / "exit_bounds_test.cpp"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
        print("PASS null/negative/high-bit/out-of-range exits and valid loaded bounds")
    for filename in ("soh/src/overlays/actors/ovl_player_actor/z_player.c", "soh/mods/items/logic/item_hylias_grace.c"):
        assert "SceneExit_IsValid(play->setupExitList, play->setupExitListCount, exitIndex)" in (ROOT / filename).read_text()
    if args.output:
        args.output.write_text(json.dumps(results, indent=2) + "\n")
    return int(any(x["returncode"] for x in results))

if __name__ == "__main__":
    raise SystemExit(main())
