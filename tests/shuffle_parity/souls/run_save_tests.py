"""Exercise the real native MM save serializer with current and historical soul flags."""
import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
with tempfile.TemporaryDirectory(prefix="soul-save-") as temporary:
    for mode in ("combo", "original"):
        binary = Path(temporary) / mode
        result = subprocess.run([
            os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-w", "-DF3DEX_GBI_2",
            *(["-DCOMBO_BUILD"] if mode == "combo" else []),
            "-DCONTROLLERBUTTONS_T=uint32_t", "-DGBI_FLOAT_MTX", "-DGBI_S32_VTX", "-DFMT_HEADER_ONLY",
            *["-I" + str(ROOT / path) for path in (".", "mm", "mm/2s2h", "mm/include", "mm/include/PR",
                                                 "mm/src", "mm/assets", "libultraship/include", "combo")],
            str(ROOT / "tests/shuffle_parity/souls/save_test.cpp"),
            str(ROOT / "mm/2s2h/Rando/SoloEntrances.cpp"), "-o", str(binary)
        ], capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        subprocess.run([str(binary)], check=True)
