#!/usr/bin/env python3
"""Prepare geometry-only market resources for both ordinary port archives."""
import argparse
import hashlib
import os
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/black_market"))
import native_market

SOURCE = ROOT / "tools/native_market/bundled"


def prepare(output_root=ROOT, bundle_directory=SOURCE):
    resources = native_market.read_bundle(bundle_directory)
    written = 0
    for game in ("soh", "mm"):
        for name, data in sorted(resources.items()):
            target = Path(output_root) / game / "assets/custom" / name
            if target.is_file() and hashlib.sha256(target.read_bytes()).digest() == hashlib.sha256(data).digest():
                continue
            target.parent.mkdir(parents=True, exist_ok=True)
            temporary = None
            try:
                with tempfile.NamedTemporaryFile(dir=target.parent, prefix=".native-market-", delete=False) as stream:
                    temporary = Path(stream.name)
                    stream.write(data)
                os.replace(temporary, target)
                written += 1
            finally:
                if temporary is not None:
                    temporary.unlink(missing_ok=True)
    print(f"PASS: {len(resources)} native market resources prepared for soh.o2r and 2ship.o2r ({written} files written)")
    return written


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-root", type=Path, default=ROOT)
    parser.add_argument("--bundle-dir", type=Path, default=SOURCE)
    args = parser.parse_args()
    prepare(args.output_root, args.bundle_dir)
