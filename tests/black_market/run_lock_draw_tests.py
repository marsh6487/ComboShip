#!/usr/bin/env python3
"""Execute native shop draw bodies with real host vertices and GBI packets."""
from pathlib import Path
import argparse
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--host", choices=("mm", "soh", "all"), default="all")
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix="market-lock-draw-") as temporary:
    for host in (("mm", "soh") if args.host == "all" else (args.host,)):
        source = (ROOT / host / ("2s2h" if host == "mm" else "soh") /
                  "Enhancements/Gameplay" / ("BlackMarket.cpp" if host == "mm" else "ComboBlackMarketNative.cpp")).read_text()
        bodies = "\n".join(block_from(source, source.index(signature))
                           for signature in ("void DrawSeal(", "void DrawMarket("))
        fixture = (ROOT / "tests/black_market/lock_draw_test.cpp").read_text().replace("/* PRODUCTION */", bodies)
        cpp = Path(temporary) / (host + ".cpp")
        cpp.write_text(fixture)
        binary = Path(temporary) / host
        includes = [host, host + "/include", host + "/include/PR", host + "/src", host + "/assets",
                    host + ("/2s2h" if host == "mm" else "/soh"), "libultraship/include", "combo"]
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                        *(["-DOOT_LOCK_TEST"] if host == "soh" else []), "-g", "-fsanitize=address,undefined",
                        "-fno-sanitize-recover=all", "-no-pie", "-I" + str(ROOT),
                        *["-I" + str(ROOT / path) for path in includes], str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
