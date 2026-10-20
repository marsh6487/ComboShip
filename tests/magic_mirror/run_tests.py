#!/usr/bin/env python3
"""Exercise the actual shared mirror placement and both native travel adapters."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def main():
    assert (ROOT / "combo/gameplay/MagicMirrorAnchor.h").exists(), "Mirror cannot follow the raised stump yet"
    with tempfile.TemporaryDirectory(prefix="magic-mirror-") as directory:
        binary = Path(directory) / "anchor"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie", "-I" + str(ROOT),
                        str(ROOT / "tests/magic_mirror/anchor_test.cpp"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
        assert (ROOT / "combo/gameplay/MagicMirrorRuntime.inc.h").exists(), "Mirror recall is not implemented yet"
        for host in ("soh", "mm"):
            # Run each engine's real once-animation advancement and dispatcher.
            native = (ROOT / host / "src/code/z_skelanime.c").read_text()
            prefix = "LinkAnimation" if host == "soh" else "PlayerAnimation"
            bodies = []
            for suffix in ("Once", "Update"):
                match = re.search(r"s32\s+" + prefix + "_" + suffix + r"\([^;{}]*\)\s*\{", native)
                assert match, prefix + "_" + suffix
                start = match.start()
                end = match.end()
                depth = 1
                while depth:
                    depth += (native[end] == "{") - (native[end] == "}")
                    end += 1
                bodies.append(native[start:end])
            (Path(directory) / "native_animation.inc.h").write_text("\n".join(bodies))
            includes = [ROOT, ROOT / host, ROOT / host / "include", ROOT / host / "include/PR",
                        ROOT / host / "src", ROOT / host / "assets", ROOT / host / "mods",
                        ROOT / host / ("soh" if host == "soh" else "2s2h"), ROOT / "libultraship/include",
                        ROOT / "combo", ROOT / "combo/menu"]
            target = Path(directory) / host
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD",
                            *(["-DMIRROR_OOT"] if host == "soh" else []), "-ffunction-sections", "-fdata-sections",
                            "-Wl,--gc-sections", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                            "-I" + directory, *["-I" + str(p) for p in includes],
                            str(ROOT / "tests/magic_mirror/native_test.cpp"),
                            "-o", str(target)], check=True)
            subprocess.run([str(target)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})

if __name__ == "__main__":
    main()
