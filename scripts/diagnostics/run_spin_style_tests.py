#!/usr/bin/env python3
"""Run production spin style policy/resource/host tests without launching a game."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def compile_run(source, sanitize=False, extra=()):
    with tempfile.TemporaryDirectory(prefix="spin-style-") as directory:
        binary = Path(directory) / "test"
        flags = ["-std=gnu++20", "-O1", "-g", "-I" + str(ROOT)]
        if sanitize:
            flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                      "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
        subprocess.run([os.environ.get("CXX", "c++"), *flags, *extra,
                        str(ROOT / source), "-o", str(binary)], cwd=ROOT, check=True)
        env = os.environ.copy()
        env["ASAN_OPTIONS"] = env.get("ASAN_OPTIONS", "") + ":detect_leaks=0"
        subprocess.run([str(binary)], cwd=ROOT, env=env, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for gate in ("policy", "resources", "render", "native", "combat"):
        parser.add_argument("--" + gate, action="store_true")
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    gates = [name for name in ("policy", "resources", "render", "native", "combat")
             if getattr(args, name)] or ["policy"]
    if "policy" in gates:
        compile_run("tests/spin_style/policy_test.cpp", args.sanitize)


if __name__ == "__main__":
    main()
