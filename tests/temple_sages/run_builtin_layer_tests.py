#!/usr/bin/env python3
"""Exercise the built-in layer with real archive classes, selection and JSON merging.

Production archive method bodies are compiled unchanged against their own class
headers. Disk backends and the top-level Context/ResourceManager are adapters;
binary readers, hashes, layer priority, version admission and JSON merging are real.
Supply nlohmann headers through CPATH or UNBOUND_JSON_INCLUDE.
"""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--assets", type=Path, help="prepared port resource directory or ZIP (includes builtin_index.json)")
    args = parser.parse_args()
    archive = (ROOT / "libultraship/src/ship/resource/archive/Archive.cpp").read_text()
    manager = (ROOT / "libultraship/src/ship/resource/archive/ArchiveManager.cpp").read_text()
    json = (ROOT / "soh/soh/unbound/UnboundJson.cpp").read_text()
    archive_methods = ["Archive::Archive(", "Archive::~Archive(", "void Archive::Load(",
        "void Archive::Unload(", "void Archive::Validate(",
        "std::shared_ptr<std::unordered_map<uint64_t, std::string>> Archive::ListFiles()",
        "bool Archive::HasFile(const std::string&", "bool Archive::HasFile(uint64_t",
        "bool Archive::HasGameVersion(", "uint32_t Archive::GetGameVersion(",
        "const std::string& Archive::GetPath(", "bool Archive::IsLoaded(",
        "void Archive::SetLoaded(", "int32_t Archive::GetPriority(",
        "void Archive::SetPriority(", "void Archive::SetGameVersion(", "void Archive::IndexFile("]
    manager_methods = ["ArchiveManager::ArchiveManager(", "ArchiveManager::~ArchiveManager(",
        "void ArchiveManager::Init(const std::vector<std::string>& archivePaths,",
        "std::shared_ptr<Archive> ArchiveManager::AddArchive(std::shared_ptr<Archive>",
        "std::shared_ptr<File> ArchiveManager::LoadFile(const std::string&",
        "std::shared_ptr<File> ArchiveManager::LoadFile(uint64_t",
        "std::vector<std::shared_ptr<File>> ArchiveManager::LoadFileFromAllLayers(",
        "int32_t ArchiveManager::GetFilePriority(", "std::shared_ptr<Archive> ArchiveManager::GetArchiveFromFile(",
        "std::shared_ptr<std::vector<std::shared_ptr<Archive>>> ArchiveManager::GetArchives(",
        "std::vector<uint32_t> ArchiveManager::GetGameVersions(",
        "bool ArchiveManager::IsGameVersionValid(",
        "const std::string* ArchiveManager::HashToString(",
        "void ArchiveManager::SetArchives(", "void ArchiveManager::ResetVirtualFileSystem("]
    body = "namespace Ship {\n" + "\n".join(function(archive, s) for s in archive_methods)
    body += "\n" + "\n".join(function(manager, s) for s in manager_methods) + "\n}\n"
    body += "namespace SOH::Unbound {\nusing Json = nlohmann::json;\n"
    body += "\n".join(function(json, s) for s in ("static bool IsReplaceDirective(",
        "void MergeJson(", "static void StripDirectives(", "Json LoadMergedJson(")) + "\n}\n"
    fixture = (ROOT / "tests/temple_sages/builtin_layer_test.cpp").read_text().replace("/* PRODUCTION ARCHIVES */", body)
    with tempfile.TemporaryDirectory(prefix="builtin-temple-layer-") as temporary:
        out = Path(temporary)
        header = out / "ship/resource/ResourceManager.h"
        header.parent.mkdir(parents=True)
        header.write_text('''#pragma once
#include <ship/resource/archive/ArchiveManager.h>
namespace Ship {
class ResourceManager {
 public:
  std::shared_ptr<ArchiveManager> archives = std::make_shared<ArchiveManager>();
  auto GetArchiveManager() { return archives; }
};
}
''')
        log = out / "spdlog/spdlog.h"
        log.parent.mkdir()
        log.write_text("#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n#define SPDLOG_DEBUG(...) ((void)0)\n#define SPDLOG_INFO(...) ((void)0)\n#define SPDLOG_WARN(...) ((void)0)\n#define SPDLOG_ERROR(...) ((void)0)\n")
        source, binary = out / "fixture.cpp", out / "fixture"
        source.write_text(fixture)
        includes = [out, ROOT, ROOT / "soh", ROOT / "libultraship/include"]
        if os.environ.get("UNBOUND_JSON_INCLUDE"):
            includes.append(Path(os.environ["UNBOUND_JSON_INCLUDE"]))
        implementations = [ROOT / "libultraship/src/ship/utils" / path for path in (
            "StrHash64.cpp", "binarytools/BinaryReader.cpp", "binarytools/MemoryStream.cpp", "binarytools/Stream.cpp")]
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-g", "-DCOMBO_BUILD",
            "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
            *["-I" + str(path) for path in includes], str(source), *map(str, implementations),
            "-o", str(binary)], check=True)
        failed = False
        cases = ("precedence", "lifetime", "old-port", "foreign-index", "malformed", "traversal",
                 "backslash", "outside", "duplicate", "incomplete", "missing", "unreadable", "version",
                 "index-self", "null-owner")
        for case in cases:
            result = subprocess.run([str(binary), case], text=True, capture_output=True,
                                    env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
            print(case, "PASS" if result.returncode == 0 else "FAIL", flush=True)
            if result.returncode:
                print(result.stderr.strip(), flush=True)
                failed = True
        if args.assets:
            directory = args.assets.resolve()
            if not directory.is_dir():
                directory = out / "assets"
                directory.mkdir()
                with zipfile.ZipFile(args.assets) as archive:
                    for name in archive.namelist():
                        path = Path(name)
                        if path.is_absolute() or ".." in path.parts or "\\" in name:
                            raise ValueError("invalid prepared asset path: " + name)
                    archive.extractall(directory)
            result = subprocess.run([str(binary), "prepared", str(directory)], text=True, capture_output=True,
                                    env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
            print("prepared resources", "PASS" if result.returncode == 0 else "FAIL", flush=True)
            if result.returncode:
                print(result.stderr.strip(), flush=True)
                failed = True
        return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
