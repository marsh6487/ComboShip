#!/usr/bin/env python3
"""Verify the rupee loop and native wallet rendering in both hosts."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import functions

with tempfile.TemporaryDirectory(prefix="wallet-gi-") as folder:
    binary = Path(folder) / "wallet"
    subprocess.run(
        [os.environ.get("CXX", "c++"), "-std=c++20", "-I" + str(ROOT),
         str(ROOT / "tests/wallet_gi_model/model_test.cpp"), "-o", str(binary)],
        check=True,
    )
    subprocess.run([str(binary)], check=True)
    for host in ("soh", "mm"):
        graph = functions((ROOT / host / "src/code/graph.c").read_text())
        (Path(folder) / "wallet_graph.inc").write_text(graph["Graph_OpenDisps"] + "\n" + graph["Graph_CloseDisps"])
        flags = ["-std=c++20", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0",
                 "-DGBI_FLOAT_MTX", "-DGBI_S32_VTX"]
        includes = ("", host, f"{host}/include", f"{host}/include/PR", f"{host}/src", f"{host}/assets",
                    f"{host}/mods", "mm/2s2h" if host == "mm" else "soh/soh", "libultraship/include", "combo",
                    "combo/menu")
        if host == "mm":
            flags += ["-DCOMBO_ZFG_WALLET_HOST_MM", "-DCONTROLLERBUTTONS_T=uint32_t"]
        subprocess.run(
            [os.environ.get("CXX", "c++"), *flags, *["-I" + str(ROOT / p) for p in includes], "-I" + folder,
             str(ROOT / "tests/wallet_gi_model/draw_test.cpp"), "-o", str(binary)],
            check=True,
        )
        subprocess.run([str(binary)], check=True)
