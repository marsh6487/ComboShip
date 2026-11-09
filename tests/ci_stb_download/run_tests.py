#!/usr/bin/env python3
"""Run the real STB CMake acquisition block against local transport failures."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PIN = "0bc88af4de5fb022db643c2d8e549a0927749354"
SHA256 = "c54b15a689e6a1f32c75e2ec23afa442e3e0e37e894b73c1974d08679b20dd5c"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--header", type=Path,
                        help="actual pinned header; otherwise acquire it through the production CMake block")
    parser.add_argument("--source", type=Path,
                        default=ROOT / "libultraship/cmake/dependencies/common.cmake",
                        help="CMake source override for a historical negative control")
    args = parser.parse_args()
    source = args.source.read_text()
    block = source[source.index("#=================== STB"):
                   source.index('file(WRITE "${STB_DIR}/stb_impl.c"')]
    url = re.search(r'"(https://[^"\s]+/' + PIN + r'/stb_image.h)"', block).group(1)
    with tempfile.TemporaryDirectory(prefix="stb-download-policy-") as td:
        root = Path(td)
        if args.header is not None:
            header = args.header.read_bytes()
        else:
            # Exercise the unchanged production URL/TLS/status/hash checks once;
            # all remaining transport failures use local fixtures reproducibly.
            build = root / "official-download"
            script = root / "official-download.cmake"
            script.write_text('set(CMAKE_BINARY_DIR "' + build.as_posix() + '")\n' + block)
            subprocess.run(["cmake", "-P", str(script)], check=True)
            header = (build / "_deps/stb/stb_image.h").read_bytes()
            print("PASS actual pinned HTTPS acquisition through production CMake")
        assert hashlib.sha256(header).hexdigest() == SHA256, "input is not the pinned official header"
        real = root / "official.h"
        real.write_bytes(header)
        empty = root / "empty.h"
        empty.write_bytes(b"")
        malformed = root / "html.h"
        malformed.write_text("<html>upstream failure</html>\n")
        missing = root / "missing.h"

        def run(name, response, initial, success):
            build = root / name
            cache = build / "_deps/stb/stb_image.h"
            cache.parent.mkdir(parents=True)
            if initial is not None:
                cache.write_bytes(initial)
            script = root / (name + ".cmake")
            script.write_text('set(CMAKE_BINARY_DIR "' + build.as_posix() + '")\n' +
                              block.replace(url, response.as_uri()))
            result = subprocess.run(["cmake", "-P", str(script)], capture_output=True, text=True)
            assert (result.returncode == 0) == success, (name, result.stdout, result.stderr)
            if not success:
                reason = "SHA256 mismatch" if response.exists() else "download failed"
                assert "Pinned stb_image.h " + reason in result.stderr, (name, result.stderr)
            if success:
                assert cache.read_bytes() == header, name + ": cache is not the pinned header"
            elif initial is None:
                assert not cache.exists(), name + ": failed response became the cache"
            else:
                assert cache.read_bytes() == initial, name + ": failed response replaced the cache"
            assert not list(cache.parent.glob("*.download*")), name + ": rejected candidate retained"
            print("PASS " + name)
            return cache

        run("empty-response-rejected", empty, None, False)
        run("html-response-rejected", malformed, None, False)
        run("missing-response-rejected", missing, None, False)
        run("bad-cache-failed-download-preserved", missing, b"invalid previous header\n", False)
        run("good-cache-offline-reused", missing, header, True)
        cache = run("good-download-promoted", real, None, True)
        run("bad-cache-repaired", real, b"invalid previous header\n", True)
        probe = root / "declarations.cpp"
        probe.write_text('#include "stb_image.h"\n'
                         'stbi_uc* pixels = nullptr;\n'
                         'void (*release_image)(void*) = &stbi_image_free;\n'
                         'const char* (*failure_reason)() = &stbi_failure_reason;\n')
        subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Werror",
                        "-I" + str(cache.parent), "-fsyntax-only", str(probe)], check=True)
        print("PASS pinned native stbi_uc/stbi_image_free/stbi_failure_reason declarations")


if __name__ == "__main__":
    main()
