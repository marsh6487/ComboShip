#!/usr/bin/env python3
"""Execute native production transaction and recovery bodies with real MM types."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from
source = (ROOT / "mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp").read_text()
signatures = ["bool SoloExterior(", "bool Exterior(", "Vec3f Door(", "u16 ReturnEntrance(",
              "void StageSoloReturn(", "bool Interior(",
              "bool Safe(", "void Transition(", "void PlayerUpdate(", "void SceneInit(",
              "const Item* Offer(", "PurchaseResult Status(", "void Purchase(", "void MarketText(",
              "void ComboBlackMarket_SaveState(", "void ComboBlackMarket_LoadState(",
              'extern "C" void ComboBlackMarket_ApplyResume(',
              'extern "C" void ComboBlackMarket_NormalizeEntry(', "void LoadSolo("]
if "bool MatchesTravel(" in source:
    signatures.insert(0, "bool MatchesTravel(")
if "void RememberTravel(" in source:
    signatures.insert(0, "void RememberTravel(")
body = "\n".join(block_from(source, source.index(s)) for s in signatures)
fixture = (ROOT / "tests/black_market/native_test.cpp").read_text().replace("/* PRODUCTION */", body)
with tempfile.TemporaryDirectory(prefix="market-native-") as temp:
    path = Path(temp) / "test.cpp"
    path.write_text(fixture)
    binary = Path(temp) / "test"
    includes = ("mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets",
                "libultraship/include", "combo")
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD",
                    "-DFMT_HEADER_ONLY", "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                    *["-I" + str(ROOT / p) for p in includes], str(path), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
