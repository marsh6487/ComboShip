#!/usr/bin/env python3
"""Focused production-boundary gates for Solo MM entrance generation and play."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
deps = Path(os.environ.get("COMBO_TEST_DEPS", ROOT.parent / "build-deps"))
env = dict(os.environ, COMBO_TEST_DEPS=str(deps))
include = deps / "usr/include"
if include.exists():
    env["CPATH"] = str(include) + (os.pathsep + env["CPATH"] if env.get("CPATH") else "")

with tempfile.TemporaryDirectory(prefix="solo-mm-explicit-runtime-") as directory:
    binary = Path(directory) / "test"
    subprocess.run([env.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-I", str(ROOT), str(HERE / "explicit_runtime_test.cpp"), "-o", str(binary)],
                   env=env, check=True)
    subprocess.run([str(binary)], env=env, check=True)

for name in ("layout", "overworld", "logic", "codec", "save", "runtime", "spiritual_stone", "retry"):
    print("RUN Solo MM entrance " + name, flush=True)
    subprocess.run([sys.executable, "-B", str(HERE / (name + "_test.py"))],
                   cwd=ROOT, env=env, check=True)
print("PASS Solo MM entrances: 9 production-boundary suites", flush=True)
