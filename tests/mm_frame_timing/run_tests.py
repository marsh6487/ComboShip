#!/usr/bin/env python3
"""ROM-free tests using the real MM-local recorder and native collection guard."""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CXX = os.environ.get("CXX", "g++")
FLAGS = ["-I", str(ROOT / "tests/mm_frame_timing/stubs"), "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic",
         *shlex.split(os.environ.get("FRAME_TIMING_CXXFLAGS", ""))]
with tempfile.TemporaryDirectory(prefix="mm-frame-timing-") as directory:
    directory = Path(directory)
    helper = directory / "helper"
    subprocess.run([CXX, *FLAGS, "-I", str(ROOT / "mm"),
                    str(ROOT / "tests/mm_frame_timing/helper_test.cpp"), "-o", str(helper)], check=True)
    subprocess.run([str(helper)], check=True)
    print("MM eligibility and collection RAII tests passed", flush=True)
    subprocess.run([sys.executable, str(ROOT / "tests/mm_frame_timing/test_native_integration.py")], check=True)
    spdlog_flags = os.environ.get("FRAME_TIMING_SPDLOG_FLAGS")
    if spdlog_flags is None:
        spdlog_flags = subprocess.check_output(["pkg-config", "--cflags", "--libs", "spdlog"], text=True)
    output = directory / "output_test"
    subprocess.run([CXX, *FLAGS, "-I", str(ROOT / "soh"), "-I", str(ROOT / "libultraship/include"),
                    str(ROOT / "tests/mm_frame_timing/output_test.cpp"),
                    str(ROOT / "soh/tests/frame_timing_context_fixture.cpp"),
                    str(ROOT / "mm/2s2h/Enhancements/Debug/FrameTimingProbe.cpp"),
                    str(ROOT / "libultraship/src/ship/diagnostics/PerformanceTrace.cpp"),
                    *shlex.split(spdlog_flags), "-pthread", "-o", str(output)], check=True)
    subprocess.run([str(output), str(directory / "logs")], check=True)
