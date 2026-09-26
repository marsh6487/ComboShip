#!/usr/bin/env python3
"""Test the real timing accumulator and compile its C bridge header; no ROM needed."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="oot-frame-timing-") as temporary:
    temporary = Path(temporary)
    output = temporary / "frame_timing_test"
    subprocess.run([
        os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic",
        *shlex.split(os.environ.get("FRAME_TIMING_CXXFLAGS", "")),
        "-I", str(ROOT / "soh"), str(ROOT / "soh/tests/frame_timing_probe_test.cpp"), "-o", str(output),
    ], check=True)
    subprocess.run([str(output)], check=True)
    source = temporary / "bridge.c"
    source.write_text('#include "soh/Enhancements/debugger/FrameTimingProbe.h"\n'
                      'void sample(void) { FrameTimingSpan span = FrameTiming_BeginSpan();\n'
                      'FrameTiming_EndSpan(FRAME_TIMING_PLAY_UPDATE, span); }\n')
    subprocess.run([os.environ.get("CC", "gcc"), "-std=c11", "-Wall", "-Werror", "-pedantic",
                    "-I", str(ROOT / "soh"), "-c", str(source), "-o", str(temporary / "bridge.o")], check=True)
    print("C bridge header compiled")
