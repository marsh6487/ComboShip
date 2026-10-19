#!/usr/bin/env python3
"""Exercise the actual keeper interaction and camera adapters with native types."""
from pathlib import Path
import os, re, subprocess, sys, tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

with tempfile.TemporaryDirectory(prefix="market-browsing-") as temporary:
    for host in ("mm", "soh"):
        source = (ROOT / host / ("2s2h" if host == "mm" else "soh") /
                  "Enhancements/Gameplay" / ("BlackMarket.cpp" if host == "mm" else
                  "ComboBlackMarketNative.cpp")).read_text()
        declarations = re.findall(r'constexpr u16 kBrowseText[^;]+;', source)
        native = (ROOT / "combo/BlackMarketBrowsingNative.h").read_text()
        signatures = ["bool BrowsingMessage(", "void EndBrowsing(", "bool BeginBrowsing(",
                      "void UpdateBrowsingCamera(PlayState* play) {", "void MonitorBrowsing(", "void BrowseInput("]
        bodies = [block_from(native, native.index(s)) for s in signatures]
        bodies.append(block_from(source, source.index("void KeeperUpdate(")))
        fixture = (ROOT / "tests/black_market/browsing_native_test.cpp").read_text()
        fixture = fixture.replace("/* PRODUCTION */", "\n".join(declarations + bodies))
        cpp = Path(temporary) / (host + ".cpp"); cpp.write_text(fixture)
        binary = Path(temporary) / host
        paths = [host, host + "/include", host + "/include/PR", host + "/src", host + "/assets",
                 host + ("/2s2h" if host == "mm" else "/soh"), "libultraship/include", "combo"]
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                        *(["-DOOT_BROWSING_TEST", "-DBLACK_MARKET_BROWSING_OOT"] if host == "soh" else []), "-g",
                        "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                        "-I" + str(ROOT), *["-I" + str(ROOT / p) for p in paths], str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
