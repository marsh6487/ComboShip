#!/usr/bin/env python3
"""Exercise the complete production Config implementation on a pristine file."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = r'''
#include "ship/config/Config.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
using nlohmann::json;
int main(int argc, char** argv) {
    const auto path = std::filesystem::path(argv[1]);
    assert(!std::filesystem::exists(path));
    Ship::Config config(path.string());
    config.SetBlock("CVars.gRando.StartingItems", json::array());
    auto first = config.GetNestedJson();
    assert(first.is_object());
    // nlohmann flatten/unflatten represents an authored empty array as a null
    // leaf. MM's real starting-item reader explicitly clears that leaf; a
    // missing path instead selects its four-item default kit.
    assert(first.at("CVars").at("gRando").at("StartingItems").is_null());
    config.SetBlock("CVars.gRando.ExcludedChecks", json::array({"RC_STONE_TOWER_TEMPLE_INVERTED_BOSS_KEY"}));
    Ship::Config reopened(path.string());
    auto saved = reopened.GetNestedJson();
    assert(saved.at("CVars").at("gRando").at("StartingItems").is_null());
    assert(saved.at("CVars").at("gRando").at("ExcludedChecks") == json::array({"RC_STONE_TOWER_TEMPLE_INVERTED_BOSS_KEY"}));
    reopened.SetBlock("CVars.gRando.StartingItems", json::array({"RI_OCARINA"}));
    assert(reopened.GetNestedJson().at("CVars").at("gRando").at("StartingItems") == json::array({"RI_OCARINA"}));
    reopened.SetBlock("CVars.gRando.StartingItems", json::array());
    assert(reopened.GetNestedJson().at("CVars").at("gRando").at("StartingItems").is_null());
    // Existing explicit scalar intermediates retain the native no-overwrite policy.
    auto collision = path; collision += ".collision";
    { std::ofstream stream(collision); stream << R"({"CVars":17,"unrelated":42})"; }
    Ship::Config clashing(collision.string());
    clashing.SetBlock("CVars.gRando.StartingItems", json::array());
    assert(clashing.GetNestedJson().at("CVars") == 17);
    assert(clashing.GetNestedJson().at("unrelated") == 42);
    std::cout << "PASS actual native Config: pristine authoritative empty/nonempty arrays, save/reload, scalar collision preservation\n";
}
'''
with tempfile.TemporaryDirectory(prefix='native-empty-config-') as directory:
    work = Path(directory)
    source = work / 'test.cpp'; source.write_text(SOURCE)
    binary = work / 'test'
    flags = ['-std=c++20', '-DFMT_HEADER_ONLY', '-ffunction-sections', '-fdata-sections',
             '-I' + str(ROOT / 'libultraship/include'), '-Wl,--gc-sections']
    subprocess.run([os.environ.get('CXX', 'c++'), *flags, str(source),
                    str(ROOT / 'libultraship/src/ship/config/Config.cpp'),
                    str(ROOT / 'libultraship/src/ship/utils/StringHelper.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(work / 'pristine.json')], check=True)
