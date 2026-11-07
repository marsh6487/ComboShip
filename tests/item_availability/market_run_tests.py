#!/usr/bin/env python3
"""Execute production Black Market admission and frozen-save compatibility."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="item-availability-market-") as temporary:
    binary = Path(temporary) / "market"
    command = [os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-Werror", "-O1",
               "-g", "-fsanitize=undefined", "-fno-sanitize-recover=undefined", "-I" + str(ROOT / "combo")]
    if os.environ.get("NLOHMANN_JSON_INCLUDE"):
        command += ["-I" + os.environ["NLOHMANN_JSON_INCLUDE"]]
    command += [str(ROOT / "tests/item_availability/market_test.cpp"), "-o", str(binary)]
    subprocess.run(command, check=True)
    failed = []
    for case in ("generation", "replacement", "filter", "legacy"):
        result = subprocess.run([str(binary), case])
        if result.returncode:
            failed.append(case)
    if failed:
        raise SystemExit("FAIL Black Market cases: " + ", ".join(failed))
