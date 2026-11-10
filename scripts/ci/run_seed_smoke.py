#!/usr/bin/env python3
"""Run the real seed smoke, collecting a bounded backtrace only after failure."""
import shutil
import subprocess
import sys


def main(argv):
    if not argv:
        print("usage: run_seed_smoke.py PROGRAM [ARGS...]", file=sys.stderr)
        return 2
    result = subprocess.run(argv)
    if result.returncode == 0:
        return 0
    status = result.returncode if result.returncode > 0 else 128 - result.returncode
    print(f"Seed generation failed (exit {status}); collecting a diagnostic backtrace.",
          file=sys.stderr, flush=True)
    debugger = shutil.which("gdb")
    if debugger:
        try:
            subprocess.run([debugger, "--batch", "-ex", "set pagination off", "-ex", "run",
                            "-ex", "thread apply all bt 24", "--args", *argv], timeout=120)
        except (OSError, subprocess.TimeoutExpired) as error:
            print(f"Backtrace unavailable: {error}", file=sys.stderr)
    else:
        print("Backtrace unavailable: gdb is not installed.", file=sys.stderr)
    return status


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
