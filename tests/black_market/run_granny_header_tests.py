#!/usr/bin/env python3
"""Compile the shared Granny adapter with UI and native engine GBI settings."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="granny-header-") as temporary:
    source = Path(temporary) / "header.cpp"
    source.write_text(
        '#include "combo/BlackMarketGranny.h"\n'
        'int main() { BlackMarketGranny::Room room; return room.Opaque() != nullptr; }\n'
    )
    for name, defines in [("shared UI", []), ("native engines", ["-DF3DEX_GBI_2"])]:
        subprocess.run(
            [os.environ.get("CXX", "c++"), "-std=c++20", *defines,
             "-I" + str(ROOT), "-I" + str(ROOT / "libultraship/include"),
             "-fsyntax-only", str(source)],
            check=True,
        )
        print("PASS: Granny header compiles for " + name, flush=True)
