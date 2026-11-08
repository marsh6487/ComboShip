#!/usr/bin/env python3
"""Deterministic checks for the real MM native integration boundaries, no ROM."""
import os
import shlex
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "mm/2s2h/BenPort.cpp").read_text()

def body(signature):
    begin = SOURCE.index(signature)
    opening = SOURCE.index("{", begin)
    depth = 1
    cursor = opening + 1
    while depth:
        if SOURCE[cursor] == "{":
            depth += 1
        elif SOURCE[cursor] == "}":
            depth -= 1
        cursor += 1
    return SOURCE[opening + 1:cursor - 1]

class NativeIntegration(unittest.TestCase):
    def test_executed_live_shop_and_active_host_gating(self):
        fixture = (ROOT / "tests/mm_frame_timing/native_gating_fixture.cpp").read_text()
        production = "static bool MMFrameTiming_Eligible() {" + body("static bool MMFrameTiming_Eligible()") + "}"
        with tempfile.TemporaryDirectory(prefix="mm-native-gating-") as temporary:
            source = Path(temporary) / "gating.cpp"
            source.write_text(fixture.replace("/* PRODUCTION_ELIGIBLE */", production))
            for combined in [False, True]:
                binary = Path(temporary) / ("combo" if combined else "standalone")
                subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic",
                                *shlex.split(os.environ.get("FRAME_TIMING_CXXFLAGS", "")),
                                *(["-DCOMBO_BUILD"] if combined else []),
                                "-I", str(ROOT / "mm"), str(source), "-o", str(binary)], check=True)
                subprocess.run([str(binary)], check=True)

if __name__ == "__main__":
    unittest.main()
