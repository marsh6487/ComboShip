#!/usr/bin/env python3
"""Exercise the production fixed-camera controller across alt-resource teardown."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
CASES = (
    "reused-header-shop-entry", "unloaded-header-shop-entry", "same-scene-reentry",
    "bottle-camera-return", "option-off-restores-live-table", "enabling-waits-for-scene-change",
    "first-entry-market-day", "first-entry-market-night", "menu-enables-before-entry", "opening-cutscene-defers",
)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    source = ROOT / "soh/soh/Enhancements/Graphics/DisableFixedCamera.cpp"
    # Include the entire production body, not a separately implemented policy.
    production = re.sub(r"^#include[^\n]*$", "", source.read_text(), flags=re.M)
    fixture = (HERE / "native_test.cpp").read_text().replace("/* PRODUCTION */", production)
    results = []
    with tempfile.TemporaryDirectory(prefix="fixed-camera-lifetime-") as tmp:
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
    if args.output:
        args.output.write_text(json.dumps(results, indent=2) + "\n")
    return int(any(x["returncode"] for x in results))

if __name__ == "__main__":
    raise SystemExit(main())
