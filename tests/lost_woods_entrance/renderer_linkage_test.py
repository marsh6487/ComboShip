#!/usr/bin/env python3
"""Verify the real MM passage object requests the native C interpolation ABI."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="lost-woods-renderer-linkage-") as temporary:
    obj = Path(temporary) / "passage.o"
    subprocess.run([
        os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-fmax-errors=5", "-fPIC",
        "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DFMT_HEADER_ONLY",
        *["-I" + str(ROOT / path) for path in
          ("mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets",
           "libultraship/include", "combo")],
        "-c", str(ROOT / "mm/2s2h/Enhancements/Graphics/ComboLostWoodsPassage.cpp"),
        "-o", str(obj),
    ], check=True)
    undefined = subprocess.check_output(["nm", "--undefined-only", str(obj)], text=True)
    symbols = {line.split()[-1] for line in undefined.splitlines() if line.split()}
    for function in ["FrameInterpolation_RecordOpenChild", "FrameInterpolation_RecordCloseChild"]:
        requested = [symbol for symbol in symbols if function in symbol]
        assert requested == [function], f"MM passage requests a non-native interpolation ABI: {requested}"
    print("PASS: production MM passage object uses both native C interpolation symbols")
