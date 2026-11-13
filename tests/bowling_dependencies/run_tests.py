#!/usr/bin/env python3
"""Run native Bowling Alley dependency, hit, fall and next-round controls."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/ci"))
from native_test_abi import ABI_FLAGS


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--include", action="append", type=Path, default=[])
    args = parser.parse_args()
    flags = ["-std=c11", "-O0", *ABI_FLAGS, "-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t",
             "-DLOG_LEVEL_GAME_PRINTS=6", '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"',
             "-ffunction-sections", "-fdata-sections",
             # Native resource handles intentionally cross legacy pointer APIs.
             "-Wno-int-conversion", "-Wno-incompatible-pointer-types",
             "-Wno-discarded-qualifiers", "-Wno-discarded-array-qualifiers"]
    for path in (ROOT, ROOT / "soh", ROOT / "soh/include", ROOT / "soh/src", ROOT / "soh/assets",
                 ROOT / "libultraship/include", ROOT / "combo", *args.include):
        flags.append("-I" + str(path))
    if args.sanitize:
        flags += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie"]
    sources = [ROOT / "tests/bowling_dependencies/native_test.c",
               ROOT / "soh/src/overlays/actors/ovl_En_Wall_Tubo/z_en_wall_tubo.c",
               ROOT / "soh/src/overlays/actors/ovl_Bg_Bowl_Wall/z_bg_bowl_wall.c",
               ROOT / "soh/src/code/z_lib.c", ROOT / "soh/src/code/z_bg_item.c"]
    failures = 0
    with tempfile.TemporaryDirectory(prefix="bowling-dependencies-") as temp:
        for mode in ("original", "combo"):
            binary = Path(temp) / mode
            definitions = ["-DCOMBO_BUILD"] if mode == "combo" else []
            subprocess.run([*shlex.split(os.environ.get("CC", "cc")), *flags, *definitions,
                            *map(str, sources), "-Wl,--gc-sections", "-lm", "-o", str(binary)], check=True)
            for case in ("bullseye-wait", "wall-wait", "round"):
                result = subprocess.run([str(binary), case], timeout=30,
                                        env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
                print(f"{mode} {case}: {'PASS' if result.returncode == 0 else 'FAIL'}", flush=True)
                failures += result.returncode != 0
    return int(bool(failures))


if __name__ == "__main__":
    raise SystemExit(main())
