#!/usr/bin/env python3
"""Exercise production OoT item identity/ownership/exact grant bodies with real save types."""
from pathlib import Path
import os, subprocess, sys, tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from
source = (ROOT / "soh/soh/Enhancements/Gameplay/ComboBlackMarketItems.cpp").read_text()
body = "\n".join(block_from(source, source.index(signature)) for signature in
                 ("RandomizerGet Id(", "bool Owned(", "bool Available(", "void LatchToolOwnership(", "bool Grant("))
fixture = (ROOT / "tests/black_market/oot_items_test.cpp").read_text().replace("/* PRODUCTION */", body)
with tempfile.TemporaryDirectory(prefix="market-oot-items-") as temp:
    path = Path(temp) / "test.cpp"; path.write_text(fixture)
    binary = Path(temp) / "test"
    includes = ("soh/include", "soh/include/PR", "soh", "soh/soh", "soh/src", "soh/assets",
                "libultraship/include", "combo", "combo/menu")
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD",
                    "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                    *["-I" + str(ROOT / p) for p in includes], str(path), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
