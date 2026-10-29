"""Compile the production MM graph patcher and its vertex-address regressions."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    with tempfile.TemporaryDirectory(prefix="native-mm-vertices-") as tmp:
        binary = Path(tmp) / "test"
        command = ["c++", "-std=c++20", "-Wall", "-Wextra", "-Werror"]
        if args.sanitize:
            command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        command += [str(root / "soh/tests/mm_display_list_patch_test.cpp"),
                    str(root / "soh/mods/transformation_masks/assets/mm_display_list_patch.cpp"),
                    "-o", str(binary)]
        subprocess.run(command, check=True)
        environment = dict(os.environ)
        if args.sanitize:
            environment["ASAN_OPTIONS"] = "detect_leaks=0"
        subprocess.run([str(binary)], env=environment, check=True)
    print("PASS shared native MM graph: legacy/wide/mixed vertex records, final ranges, malformed metadata and overflow")


if __name__ == "__main__":
    main()
