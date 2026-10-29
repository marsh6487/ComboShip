"""Compile the production MM graph patcher and its vertex-address regressions."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    with tempfile.TemporaryDirectory(prefix="native-mm-vertices-") as tmp:
        binary = Path(tmp) / "test"
        command = ["c++", "-std=c++20", "-Wall", "-Wextra", "-Werror"]
        if args.sanitize:
            command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        command += [str(root / "soh/tests/mm_display_list_patch_test.cpp"),
                    str(root / "soh/mods/transformation_masks/assets/mm_display_list_patch.cpp"),
                    "-o", str(binary)]
        subprocess.run(command, check=True)
        environment = dict(os.environ)
        if args.sanitize:
            environment["ASAN_OPTIONS"] = "detect_leaks=0"
        subprocess.run([str(binary)], env=environment, check=True)
        source = (root / "soh/mods/transformation_masks/assets/mm_asset_loader.cpp").read_text()
        start = source.index("MmDisplayList_SelectVertexResource(", source.index("static uintptr_t MmAssets_ResolveDisplayListReference("))
        end, depth = source.index("(", start) + 1, 1
        while depth:
            depth += (source[end] == "(") - (source[end] == ")")
            end += 1
        fixture = (Path(tmp) / "loader.cpp")
        fixture.write_text((root / "tests/native_mm_actor_vertices/loader_resource_test.cpp").read_text().replace(
            "/* PRODUCTION_SELECTION */", source[start:end]))
        # Only destructor trace logging is suppressed; resource storage/methods are production code.
        (Path(tmp) / "spdlog").mkdir()
        (Path(tmp) / "spdlog/spdlog.h").write_text("#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n")
        for wide in (False, True):
            loader = Path(tmp) / ("loader-wide" if wide else "loader-legacy")
            flags = ["c++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-DF3DEX_GBI_2",
                     "-I" + str(root), "-I" + str(root / "libultraship/include"), "-I" + tmp]
            if wide:
                flags.append("-DGBI_S32_VTX=1")
            if args.sanitize:
                flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                          "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
            subprocess.run([*flags, str(fixture),
                            str(root / "libultraship/src/ship/resource/Resource.cpp"),
                            str(root / "libultraship/src/fast/resource/type/Vertex.cpp"),
                            str(root / "soh/soh/resource/type/Array.cpp"),
                            str(root / "soh/mods/transformation_masks/assets/mm_display_list_patch.cpp"),
                            "-o", str(loader)], check=True)
            subprocess.run([str(loader)], env=environment, check=True)
    print("PASS shared native MM graph: legacy/wide/mixed vertex records, final ranges, malformed metadata and overflow")


if __name__ == "__main__":
    main()
