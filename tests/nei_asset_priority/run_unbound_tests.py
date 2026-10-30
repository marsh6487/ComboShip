"""Unbound stock archives must not suppress authored held swords or GIs."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/nei_held"))
from run_articulated_tests import flags as oot_flags
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_nei_tests import flags as mm_flags


def function(source, name):
    start = source.index("inline ", source.index(name) - 24)
    opening = source.index("{", start)
    end, depth = opening + 1, 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


with tempfile.TemporaryDirectory(prefix="unbound-equipment-priority-") as temporary:
    directory = Path(temporary)
    source = (ROOT / "combo/NeiAssetPriorityResource.h").read_text()
    (directory / "unbound_asset_priority.inc").write_text("\n".join(
        function(source, name) for name in ("FindCompanion", "IsModAsset")))
    for host, flags in [("oot", oot_flags()), ("mm", mm_flags())]:
        binary = directory / host
        definitions = ["-DCOMBO_BUILD"] + (["-DNEI_EQUIPMENT_MM"] if host == "mm" else [])
        sanitize = (["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer",
                     "-fno-pie", "-no-pie"] if "--sanitize" in sys.argv else [])
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *flags, *definitions, *sanitize,
                        "-I" + str(directory), str(ROOT / "tests/nei_asset_priority/unbound_priority_test.cpp"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        print(f"PASS: {host} Unbound stock equipment, held-sword ownership and live mod precedence")
