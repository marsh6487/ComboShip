"""Capture both hosts' production Grace particles at the native effect boundary."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
sys.path.insert(0, str(ROOT / "tests/nei_held"))
from run_time_pedestal_tests import functions
from run_mm_nei_tests import flags as mm_flags
from run_articulated_tests import flags as oot_flags

for host, flags in (("soh", oot_flags()), ("mm", mm_flags())):
    source = (ROOT / host / "mods/items/logic/item_hylias_grace.c").read_text()
    parsed = functions(source)
    with tempfile.TemporaryDirectory(prefix=f"{host}-grace-particles-") as temporary:
        build = Path(temporary)
        (build / "grace_particles.inc").write_text("\n".join(
            parsed[name] for name in ("HGrace_SpawnFairySparkles", "HGrace_SpawnTrailSparkles")))
        binary = build / "particles"
        host_flags = ["-DGRACE_PARTICLE_MM"] if host == "mm" else []
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", *flags, *host_flags,
                        "-I" + str(build), str(ROOT / "tests/mm_grace/particle_test.cpp"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
