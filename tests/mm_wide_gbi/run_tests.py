#!/usr/bin/env python3
"""Run MM's real matrix/transition writers in legacy and float/s32 GBI modes.

--source-ref reads only the owned production C files at a Git ref so the same
tests can demonstrate pre-adapter failures without touching the worktree.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--mode", choices=("legacy", "wide", "both"), default="both")
parser.add_argument("--source-ref")
parser.add_argument("--sanitize", action="store_true")
args = parser.parse_args()

sources = (
    "mm/2s2h/gu_pc.c",
    "mm/src/boot/O2/mtxuty-cvt.c",
    "mm/src/code/sys_matrix.c",
    "mm/src/code/su_mtx.c",
    "mm/src/code/z_fbdemo.c",
    "mm/src/code/z_lib.c",
    "mm/src/code/stubs.c",
    "mm/src/code/z_skin_matrix.c",
)
owned = sources[:5]
includes = ("mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets",
            "mm/mods", "libultraship/include", "combo", "combo/menu")
base = [os.environ.get("CC", "cc"), "-std=gnu11", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0",
        *["-I" + str(ROOT / path) for path in includes],
        "-Wno-incompatible-pointer-types", "-Wno-int-conversion", "-Wno-implicit-function-declaration",
        "-Wno-pointer-to-int-cast", "-Wno-int-to-pointer-cast", "-Wno-discarded-qualifiers",
        "-ffunction-sections", "-fdata-sections", "-fno-strict-aliasing", "-g"]
if args.sanitize:
    # Unchanged legacy writers use signed shifts to construct packed N64 words.
    base += ["-fsanitize=address,undefined", "-fno-sanitize=shift", "-fno-sanitize-recover=all",
             "-fno-omit-frame-pointer", "-no-pie"]

with tempfile.TemporaryDirectory(prefix="mm-wide-gbi-") as temporary:
    directory = Path(temporary)
    paths = []
    for source in sources:
        path = ROOT / source
        if args.source_ref and source in owned:
            path = directory / (Path(source).name + ".baseline.c")
            path.write_bytes(subprocess.check_output(["git", "show", args.source_ref + ":" + source], cwd=ROOT))
        paths.append(path)
    modes = ("legacy", "wide") if args.mode == "both" else (args.mode,)
    for mode in modes:
        flags = [*base, *( ["-DGBI_FLOAT_MTX", "-DGBI_S32_VTX"] if mode == "wide" else [])]
        objects = []
        for index, source in enumerate(paths):
            obj = directory / f"{mode}-{index}.o"
            subprocess.run([*flags, "-c", str(source), "-o", str(obj)], check=True)
            objects.append(str(obj))
        binary = directory / mode
        subprocess.run([*flags, str(ROOT / "tests/mm_wide_gbi/native_test.c"), *objects,
                        "-Wl,--gc-sections", "-lm", "-o", str(binary)], check=True)
        # This managed runner disallows LeakSanitizer's /proc thread scan;
        # the native fixture accounts for each transition allocation/free.
        subprocess.run([str(binary)], check=True,
                       env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}, timeout=10)
