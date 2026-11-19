#!/usr/bin/env python3
"""Exercise floating pickup state without a game, ROM, GUI or optional pack."""
from pathlib import Path
import argparse
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sanitize", action="store_true")
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix="pickup-counter-") as directory:
    binary = Path(directory) / "test"
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT)]
    if args.sanitize:
        flags += ["-g", "-O1", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie"]
    subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), *flags,
                    str(ROOT / "tests/pickup_counter/state_test.cpp"), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
