#!/usr/bin/env python3
"""Exercise actual native MM JSON conversion, including preview isolation."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="solo-mm-save-") as directory:
    for mode in ("original", "combo"):
        binary = Path(directory) / mode
        flags = ["-std=c++20", "-O0", "-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t",
                 "-DGBI_FLOAT_MTX", "-DGBI_S32_VTX", "-DFMT_HEADER_ONLY", "-I" + str(ROOT),
                 "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections"]
        if mode == "combo":
            flags += ["-DCOMBO_BUILD"]
        for path in ("mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets",
                     "libultraship/include", "combo"):
            flags += ["-isystem", str(ROOT / path)]
        source = ROOT / "mm/2s2h/Rando/SoloEntrances.cpp"
        sources = [str(Path(__file__).with_name("save_test.cpp"))]
        if source.exists():
            sources.append(str(source))
        subprocess.run([os.environ.get("CXX", "c++"), *flags, *sources, "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        print("PASS native Solo MM save serialization", mode, flush=True)
