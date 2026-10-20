#!/usr/bin/env python3
"""Compile both production forest routes with their actual native engine types."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from
signatures = ["ForestOrigin DefaultForestOrigin(", "bool ForestVisit(", "bool OriginScene(",
              "bool ForestSafe(", "void ForestTransition(", "void StageForestOrigin(",
              "void ForestPlayerUpdate(", "void LoadForestState(", "void SaveForestState(",
              "void NormalizeForestEntry("]
with tempfile.TemporaryDirectory(prefix="solo-forest-native-") as temp:
    for engine, path in (("mm", "mm/2s2h/Enhancements/Gameplay/ComboSoloForest.cpp"),
                         ("soh", "soh/soh/Enhancements/Gameplay/ComboSoloForest.cpp")):
        assert (ROOT / path).exists(), f"native {engine} forest route is missing"
        source = (ROOT / path).read_text()
        selected = list(signatures)
        if engine == "soh":
            selected += [signature for signature in ("void ResetForestFile(", "void BindForestFile(") if signature in source]
            selected += ["void RegisterSoloForest("]
        else:
            selected += ['extern "C" int ComboSoloForestMm_PassageAvailable(']
        bodies = "\n".join(block_from(source, source.index(signature)) for signature in selected)
        fixture = (ROOT / "tests/black_market/solo_forest_native_test.cpp").read_text().replace("/* PRODUCTION */", bodies)
        target = Path(temp) / (engine + ".cpp"); target.write_text(fixture)
        binary = Path(temp) / engine
        includes = [engine, engine + "/include", engine + "/include/PR", engine + "/src", engine + "/assets",
                    engine + ("/soh" if engine == "soh" else "/2s2h"), "libultraship/include", "combo", "combo/menu"]
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                        *(["-DOOT_FOREST_TEST"] if engine == "soh" else []), "-g", "-fsanitize=address,undefined",
                        "-fno-sanitize-recover=all", "-no-pie", *["-I" + str(ROOT / p) for p in includes],
                        str(target), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
