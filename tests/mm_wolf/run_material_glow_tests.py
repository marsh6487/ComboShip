#!/usr/bin/env python3
"""Execute each host's actual Wolf material through real native GBI macros."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def body(source, signature):
    start = source.index(signature)
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main():
    with tempfile.TemporaryDirectory(prefix="wolf-material-glow-") as temporary:
        directory = Path(temporary)
        for host in ("mm", "soh"):
            source = (ROOT / host / "mods/transformation_masks/wolf_link_form.cpp").read_text()
            material = body(source, "static u32 Log2(") + "\n" + body(source, "static void BuildMaterialDisplayList(")
            (directory / "wolf-material.inc").write_text(material)
            binary = directory / host
            subprocess.run([
                os.environ.get("CXX", "c++"), "-std=c++20", "-DF3DEX_GBI_2",
                "-I" + str(ROOT / "libultraship/include"), "-I" + str(directory),
                "-O1", "-g", "-fsanitize=undefined", "-fno-sanitize-recover=all",
                str(ROOT / "tests/mm_wolf/material_glow_test.cpp"), "-o", str(binary)
            ], check=True)
            print(host + ":", flush=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
