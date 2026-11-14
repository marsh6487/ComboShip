#!/usr/bin/env python3
"""Test real Din WAV decoder/provider and optional native O2R/libzip reader.

LUS class headers and CrossRMRegistry are real. ResourceManager/Context and raw
ArchiveManager lookup methods are controlled by archive_fixture.cpp; no library
headers are fabricated. Dependencies must already exist; nothing is downloaded.
"""
import argparse
import ctypes.util
import os
from pathlib import Path
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]


def run(command):
    result = subprocess.run(list(map(str, command)), cwd=ROOT, capture_output=True,
                            text=True, env={**os.environ, "ASAN_OPTIONS": os.environ.get("ASAN_OPTIONS", "detect_leaks=0")})
    if result.stdout:
        print(result.stdout, end="", flush=True)
    if result.returncode:
        if result.stderr:
            print(result.stderr, end="", flush=True)
        raise SystemExit(result.returncode)
    if result.stderr:
        print(result.stderr, end="", flush=True)


def main():
    started = time.monotonic()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", choices=("soh", "mm"))
    parser.add_argument("--parser-only", action="store_true")
    parser.add_argument("--archive-only", action="store_true", help="Run only native starter build/read stage after provider verification")
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--baseline-parser", action="store_true", help="RED diagnostic: missing decoder returns no PCM")
    parser.add_argument("--baseline-provider", action="store_true", help="RED diagnostic: missing provider supplies no overrides")
    parser.add_argument("--dependency-root", type=Path, action="append", default=[], help="Repeatable existing include roots")
    archive_group = parser.add_mutually_exclusive_group()
    archive_group.add_argument("--archive", type=Path, help="Also read each starter WAV through actual O2rArchive/libzip")
    archive_group.add_argument("--build-starter", action="store_true", help="Build accepted defaults in temp, then verify actual helper-to-O2R-reader PCM")
    parser.add_argument("--zip-include", type=Path, help="Existing directory containing real libzip zip.h/zipconf.h")
    args = parser.parse_args()
    if args.archive_only and (args.parser_only or args.baseline_parser or not (args.archive or args.build_starter)):
        parser.error("--archive-only requires --archive/--build-starter and cannot select parser-only")
    candidates = [*args.dependency_root]
    candidates += [Path(p) for p in os.environ.get("DINS_AUDIO_DEPENDENCY_ROOTS", "").split(os.pathsep) if p]
    # The shared CI setup prepares these real headers for regression fixtures.
    # Gate-origin tasks do not inherit the NEI umbrella's CPATH assignment.
    candidates += [Path(p) for variable in ("CPATH", "CPLUS_INCLUDE_PATH", "COMBO_NEI_FIXTURE_CPATH")
                   for p in os.environ.get(variable, "").split(os.pathsep) if p]
    candidates += [ROOT.parent / "dins_audio_native_dependencies", ROOT / "deps", Path("/usr/include"), Path("/usr/local/include")]
    candidates += [p.parents[1] for p in sorted(ROOT.parents[1].glob("*/deps/nlohmann/json.hpp"))]
    includes = [ROOT, ROOT / "libultraship/include", *[p for p in candidates if p.is_dir()]]
    if not args.parser_only and not args.baseline_parser:
        for name in ("spdlog/async.h", "BS_thread_pool.hpp", "nlohmann/json.hpp", "SDL2/SDL.h"):
            if not any((p / name).is_file() for p in includes):
                parser.error(f"real dependency missing: {name}; supply --dependency-root (no downloads performed)")
    flags = ["-std=c++20", "-g", "-O1", "-pthread", "-DFMT_HEADER_ONLY", "-fno-omit-frame-pointer", "-ffunction-sections", "-fdata-sections"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie"]
    flags += ["-I" + str(path) for path in includes]
    with tempfile.TemporaryDirectory(prefix="dins-audio-overrides-") as temporary:
        temporary = Path(temporary)
        source = ROOT / "tests/dins_fire_audio/override_resource_test.cpp"
        if args.parser_only or args.baseline_parser:
            binary = temporary / "parser"
            run([os.environ.get("CXX", "c++"), *flags, "-DTEST_PARSER_ONLY", *(["-DTEST_BASELINE_WAV"] if args.baseline_parser else []), source, "-o", binary])
            run([binary])
            return
        common_objects = []
        for implementation in (ROOT / "tests/dins_fire_audio/archive_fixture.cpp", ROOT / "libultraship/src/ship/resource/CrossRMRegistry.cpp"):
            obj = temporary / (implementation.stem + ".o")
            run([os.environ.get("CXX", "c++"), *flags, "-c", implementation, "-o", obj])
            common_objects.append(obj)
        implementations = [source, *common_objects]
        for host in (() if args.archive_only else ("soh", "mm")):
            if args.host and host != args.host:
                continue
            for combo in (False, True):
                binary = temporary / f"{host}-{combo}"
                defines = (["-DCW_DINS_AUDIO_MM"] if host == "mm" else []) + (["-DCOMBO_BUILD"] if combo else [])
                if args.baseline_provider:
                    defines += ["-DTEST_BASELINE_PROVIDER"]
                run([os.environ.get("CXX", "c++"), *flags, *defines, *implementations, "-Wl,--gc-sections", "-o", binary])
                run([binary])
                print(f"PASS Din override resources host={host} combo={combo}", flush=True)
        if args.build_starter:
            args.archive = temporary / "starter.o2r"
            run([os.environ.get("PYTHON", "python3"), "-B", ROOT / "scripts/assets/build_dins_fire_audio_mod.py", "--defaults", "--output", args.archive])
        if args.archive:
            native_o2r(args, parser, flags, temporary, common_objects)
        print(f"PASS native resource verification completed in {time.monotonic() - started:.2f}s", flush=True)


def method(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def native_o2r(args, parser, flags, temporary, common_objects):
    # Use real existing libzip declarations and native binary; never synthesize headers.
    zip_candidates = [args.zip_include, Path("/usr/include"), Path("/usr/local/include")]
    zip_candidates += list(Path("/opt/codex/runtimes").glob("*/dependencies/python/lib/python*/site-packages/casadi/include"))
    zip_include = next((p for p in zip_candidates if p and (p / "zip.h").is_file() and (p / "zipconf.h").is_file()), None)
    if not zip_include:
        parser.error("real libzip headers unavailable; supply --zip-include")
    library = ctypes.util.find_library("zip")
    if not library:
        parser.error("native libzip unavailable; resource-fixture tests passed, actual O2R reader unproven")
    archive = (ROOT / "libultraship/src/ship/resource/archive/Archive.cpp").read_text()
    o2r = (ROOT / "libultraship/src/ship/resource/archive/O2rArchive.cpp").read_text()
    methods = [method(archive, signature) for signature in (
        "Archive::Archive(", "Archive::~Archive(", "const std::string& Archive::GetPath(",
        "void Archive::IndexFile(", "std::shared_ptr<std::unordered_map<uint64_t, std::string>> Archive::ListFiles()",
        "bool Archive::HasFile(const std::string&", "bool Archive::HasFile(uint64_t")]
    methods += [method(o2r, signature) for signature in (
        "O2rArchive::O2rArchive(", "O2rArchive::~O2rArchive(", "zip_t* O2rArchive::GetZipHandle(",
        "void O2rArchive::ReleaseZipHandle(", "std::shared_ptr<File> O2rArchive::LoadFile(uint64_t",
        "std::shared_ptr<File> O2rArchive::LoadFile(const std::string&", "bool O2rArchive::Open(",
        "bool O2rArchive::Close(", "bool O2rArchive::WriteFile(")]
    (temporary / "native_o2r_methods.inc").write_text("\n\n".join(methods) + "\n")
    binary = temporary / "o2r-reader"
    implementations = [ROOT / "tests/dins_fire_audio/o2r_reader_test.cpp", *common_objects,
                       ROOT / "libultraship/src/ship/utils/StrHash64.cpp"]
    run([os.environ.get("CXX", "c++"), *flags, "-I" + str(temporary), "-I" + str(zip_include),
         *implementations, "-Wl,--gc-sections", "-l:" + library, "-o", binary])
    run([binary, args.archive.resolve()])


if __name__ == "__main__":
    main()
