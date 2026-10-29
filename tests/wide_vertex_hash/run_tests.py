#!/usr/bin/env python3
"""Execute the production hash-vertex handler at archive/runtime ABI boundaries."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sanitize", action="store_true")
parser.add_argument("--case", choices=("offsets", "mm", "packet", "lifetime", "bounds"))
args = parser.parse_args()
source = (ROOT / "libultraship/src/fast/interpreter.cpp").read_text()
match = re.search(r"bool gfx_vtx_hash_handler_custom\([^;{]+\)\s*\{", source)
assert match
end, depth = match.end(), 1
while depth:
    depth += (source[end] == "{") - (source[end] == "}")
    end += 1
body = source[match.start():end]
with tempfile.TemporaryDirectory(prefix="wide-vertex-hash-") as directory:
    build = Path(directory)
    cpp = build / "native.cpp"
    cpp.write_text((ROOT / "tests/wide_vertex_hash/native_test.cpp").read_text().replace("/* PRODUCTION_HANDLER */", body))
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-DGBI_FLOAT_MTX=1", "-DGBI_S32_VTX=1",
             "-DF3DEX_GBI_2", "-DFMT_HEADER_ONLY", "-I" + str(ROOT),
             "-I" + str(ROOT / "libultraship/include")]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    binary = build / "test"
    subprocess.run([os.environ.get("CXX", "c++"), *flags, str(cpp),
                    str(ROOT / "libultraship/src/ship/resource/Resource.cpp"),
                    str(ROOT / "libultraship/src/fast/resource/type/Vertex.cpp"),
                    str(ROOT / "mm/2s2h/resource/type/Array.cpp"), "-o", str(binary)], check=True)
    subprocess.run([str(binary), *([args.case] if args.case else [])], check=True)
