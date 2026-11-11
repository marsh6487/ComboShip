#!/usr/bin/env python3
"""Execute the production shaft state/geometry; no game or visual claim."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-I" + str(ROOT)]
    if "--sanitize" in sys.argv:
        flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer",
                  "-fno-pie", "-no-pie"]
    with tempfile.TemporaryDirectory(prefix="gi-shaft-") as directory:
        binary = Path(directory) / "state-test"
        subprocess.run([os.environ.get("CXX", "g++-14"), *flags,
                        str(Path(__file__).with_name("state_test.cpp")), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True,
                       env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()
