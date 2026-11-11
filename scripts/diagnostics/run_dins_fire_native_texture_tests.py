#!/usr/bin/env python3
"""Parse every manifest-listed Din texture through both production native factories.

Each host runner compiles its real factory/reader once and receives all resource
pairs in one invocation. The existing runners retain their sanitizer, buffer
ownership and payload checks; the CI compiler/ABI environment is inherited.
Run the encoder's --check-native separately to verify committed resource hashes.
Its optional --check mode reproduces the source recipe when authoring PNGs are supplied.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
RUNNERS = {
    "soh": "scripts/diagnostics/run_spin_effect_texture_tests.py",
    "mm": "scripts/diagnostics/run_mm_spin_effect_texture_tests.py",
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", choices=tuple(RUNNERS), help="Restrict a focused diagnostic to one host")
    args = parser.parse_args()
    manifest = json.loads((ROOT / "tools/dins_fire_remake/manifest.json").read_text())
    resources = sorted(manifest["resources"])
    if not resources or len(resources) != manifest["memory_budget"]["resource_count_per_host"]:
        raise RuntimeError("Din texture manifest count does not match its resource budget")
    if set(manifest["hosts"]) != set(RUNNERS):
        raise RuntimeError("Din texture manifest hosts do not match the production factory runners")
    hosts = (args.host,) if args.host else tuple(RUNNERS)
    for host in hosts:
        command = [sys.executable, "-B", str(ROOT / RUNNERS[host])]
        for name in resources:
            command.extend(("--resource", name, str(ROOT / host / "assets/custom" / name)))
        print(f"Checking {len(resources)} Din textures with the {host} production factory", flush=True)
        subprocess.run(command, cwd=ROOT, check=True)
    print(f"PASS {len(resources) * len(hosts)} native Din texture imports "
          f"({len(resources)} resources per host)", flush=True)


if __name__ == "__main__":
    main()
