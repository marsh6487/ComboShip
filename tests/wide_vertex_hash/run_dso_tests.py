#!/usr/bin/env python3
"""Prove the engine/MM vertex-source cast across real ELF shared libraries."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sanitize", action="store_true")
args = parser.parse_args()
compiler = os.environ.get("CXX", "c++")
common = ["-std=c++20", "-O1", "-g", "-Wall", "-Wextra", "-Werror", "-DFMT_HEADER_ONLY",
          "-DGBI_FLOAT_MTX=1", "-DGBI_S32_VTX=1", "-DF3DEX_GBI_2",
          "-I" + str(ROOT), "-I" + str(ROOT / "libultraship/include")]
if args.sanitize:
    common += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
with tempfile.TemporaryDirectory(prefix="vertex-hidden-dso-") as directory:
    build = Path(directory)
    engine = build / "libvertex_engine.so"
    plugin = build / "libvertex_mm.so"
    subprocess.run([compiler, *common, "-fPIC", "-shared", "-Dlibultraship_EXPORTS",
                    str(HERE / "dso_engine.cpp"),
                    str(ROOT / "libultraship/src/ship/resource/Resource.cpp"),
                    str(ROOT / "libultraship/src/fast/resource/type/Vertex.cpp"),
                    "-Wl,-z,defs", "-o", str(engine)], check=True)
    subprocess.run([compiler, *common, "-fPIC", "-shared", "-DMM_BUILD_DLL", "-D_2ship_EXPORTS",
                    "-fvisibility=hidden", "-fvisibility-inlines-hidden",
                    str(HERE / "dso_mm_plugin.cpp"), str(ROOT / "mm/2s2h/resource/type/Array.cpp"),
                    "-L" + directory, "-lvertex_engine", "-Wl,-rpath,$ORIGIN", "-Wl,-z,defs",
                    "-o", str(plugin)], check=True)
    for library, should_export in ((engine, True), (plugin, False)):
        symbols = subprocess.check_output(["nm", "-D", "--defined-only", "-C", str(library)], text=True)
        assert ("typeinfo for Fast::IVertexSource" in symbols) == should_export, symbols
    binary = build / "host"
    subprocess.run([compiler, *common, "-fno-pie", "-no-pie", str(HERE / "dso_host.cpp"),
                    "-ldl", "-o", str(binary)], check=True)
    subprocess.run([str(binary), str(engine), str(plugin)], check=True,
                   env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
