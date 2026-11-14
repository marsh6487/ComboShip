#!/usr/bin/env python3
"""Run the production FleetNet sender and rescan against controlled IO boundaries.

Both hosts' function bodies, packet limit and NetDelta type are extracted without
rewriting. The fixture supplies live extraction, active-host status and packet IO;
native inventory/capacity behavior is covered separately by tests/ammo_sync.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "tests/shuffle_parity/boss_keys"))
from run_tests import function


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--source-ref", help="Use committed production bodies for a red control")
    parser.add_argument("--json-include", type=Path)
    args = parser.parse_args()
    failed_modes = []
    with tempfile.TemporaryDirectory(prefix="ammo-sync-network-") as directory:
        build = Path(directory)
        for host, path in (
            ("oot", "soh/soh/FleetShipCombo/FleetSync.cpp"),
            ("mm", "mm/2s2h/FleetShipCombo/FleetSync.cpp"),
        ):
            source = subprocess.check_output(
                ["git", "show", args.source_ref + ":" + path], cwd=ROOT, text=True
            ) if args.source_ref else (ROOT / path).read_text()
            packet_limit = re.search(r"^constexpr size_t kNetBatchBytes = [^;]+;", source, re.M)[0]
            delta_type = re.search(r"^struct NetDelta \{.*?^\};", source, re.M | re.S)[0]
            bodies = "\n".join(function(source, name) for name in (
                "NetHash", "NetIsVolatileLeaf", "NetArrayRootOf", "NetSendDelta",
                "NetSplitAll", "NetRescan",
            ))
            (build / "production.inc").write_text(packet_limit + "\n" + delta_type + "\n" + bodies)
            for combo in (False, True):
                mode = host + ("-combo" if combo else "-original")
                flags = ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-I" + str(build)]
                if combo:
                    flags += ["-DCOMBO_BUILD"]
                if args.json_include:
                    flags += ["-isystem", str(args.json_include)]
                if args.sanitize:
                    flags += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                              "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
                binary = build / mode
                subprocess.run([os.environ.get("CXX", "c++"), *flags, str(HERE / "network_test.cpp"),
                                "-o", str(binary)], check=True)
                print("FleetNet production functions:", mode, flush=True)
                result = subprocess.run([str(binary)],
                                        env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
                if result.returncode:
                    failed_modes.append(mode)
    if failed_modes:
        raise SystemExit("Failed production configurations: " + ", ".join(failed_modes))


if __name__ == "__main__":
    main()
