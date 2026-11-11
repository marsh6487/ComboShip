#!/usr/bin/env python3
"""Compile complete native player units, retaining the production unity include order.

Catches early item state macro leakage into common initializers and object code.
No extracted bodies, player aliases, generated headers or gameplay mocks are used.
"""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def run_native_compile():
    failures = []
    with tempfile.TemporaryDirectory(prefix="whip-native-player-") as temporary:
        for host in ("soh", "mm"):
            includes = (".", host + "/include", host + "/include/PR", host + "/src", host,
                        host + "/assets", host + ("/soh" if host == "soh" else "/2s2h"),
                        host + "/mods", "libultraship/include", "libultraship/src",
                        "combo", "combo/menu", "combo/gui", "torch/lib/n64graphics")
            flags = ["-std=gnu2x" if host == "soh" else "-std=gnu17", "-DF3DEX_GBI_2",
                     "-DGBI_FLOAT_MTX", "-DGBI_S32_VTX", "-DLOG_LEVEL_GAME_PRINTS=6",
                     "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING",
                     "-D" + ("SOH_BUILD_DLL" if host == "soh" else "MM_BUILD_DLL"),
                     "-Wno-incompatible-pointer-types", "-Wno-int-conversion", "-fmax-errors=5"]
            for name in ("CMake/lus-cvars.cmake", "CMake/soh-cvars.cmake", "CMake/2ship-cvars.cmake"):
                flags += [f'-D{key}="{value}"' for key, value in
                          re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / name).read_text())]
            flags += ["-I" + str(ROOT / path) for path in includes]
            # CMake supplies these MM C precompiled headers in this order.
            if host == "mm":
                for name in ("ultra64.h", "global.h", "z64.h", "macros.h", "functions.h", "variables.h"):
                    flags += ["-include", str(ROOT / host / "include" / name)]
            source = ROOT / host / "src/overlays/actors/ovl_player_actor/z_player.c"
            for combo in (False, True):
                mode = "combo" if combo else "original"
                command = [os.environ.get("CC", "cc"), *flags,
                           *(["-DCOMBO_BUILD"] if combo else []), "-c", str(source),
                           "-o", str(Path(temporary) / f"{host}-{mode}.o")]
                result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
                if result.returncode:
                    print(f"FAIL complete native player: {host} {mode}", flush=True)
                    print(result.stdout + result.stderr, flush=True)
                    failures.append(f"{host} {mode}")
                    continue
                print(f"PASS complete native player: {host} {mode} "
                      f"(existing warnings={result.stderr.count('warning:')})", flush=True)

    if failures:
        raise RuntimeError("Native player compilation failed: " + ", ".join(failures))


if __name__ == "__main__":
    run_native_compile()
