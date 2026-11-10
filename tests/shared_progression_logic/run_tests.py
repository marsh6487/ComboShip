#!/usr/bin/env python3
"""Compile and run the production combined-fill regression against a chosen source tree."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--json-include", type=Path, help="Directory containing nlohmann/json.hpp")
    parser.add_argument("--output-dir", type=Path, help="Keep compiler and execution evidence here")
    args = parser.parse_args()
    source = args.source_root.resolve()
    temporary = tempfile.TemporaryDirectory(prefix="combo-shared-progression-") if not args.output_dir else None
    output = args.output_dir.resolve() if args.output_dir else Path(temporary.name)
    output.mkdir(parents=True, exist_ok=True)
    binary = output / "native_test"
    command = [os.environ.get("CXX", "g++"), "-std=c++20", "-O2", "-I", str(source / "combo")]
    if args.json_include:
        command += ["-I", str(args.json_include.resolve())]
    command += [str(Path(__file__).with_name("native_test.cpp").resolve()), "-o", str(binary)]
    compiled = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (output / "compile.log").write_text(compiled.stdout)
    result = {"sourceRoot": str(source), "compileCommand": command, "compileExitCode": compiled.returncode}
    if compiled.returncode:
        print(compiled.stdout)
        (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
        return compiled.returncode
    # Production forward replay writes sphere evidence under its working directory.
    # Keep those synthetic saves beside the test evidence, outside any user runtime.
    tested = subprocess.run([str(binary)], cwd=output, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (output / "run.log").write_text(tested.stdout)
    result["executionCwd"] = str(output)
    result["testExitCode"] = tested.returncode
    result["assertions"] = [line for line in tested.stdout.splitlines() if line.startswith(("FAIL:", "PASS:"))]
    (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print("\n".join(result["assertions"]))
    if args.output_dir:
        print("Evidence:", output)
    return tested.returncode


if __name__ == "__main__":
    raise SystemExit(main())
