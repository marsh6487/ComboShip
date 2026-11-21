#!/usr/bin/env python3
"""Exercise complete pause atlas selection and MM's additional song slots."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PAUSE_MM = ROOT / 'mm/src/overlays/kaleido_scope/ovl_kaleido_scope'
PAUSE_OOT = ROOT / 'soh/src/overlays/misc/ovl_kaleido_scope'

mm_scope = (PAUSE_MM / 'z_kaleido_scope_NES.c').read_text()
oot_collect = (PAUSE_OOT / 'z_kaleido_collect.c').read_text()
mm_positions = [
    int(value) for value in re.findall(
        r'(-?\d+),\s*// QUEST_SONG_', mm_scope.split('sQuestVtxRectLeft[] = {')[1].split('};')[0]
    )
]
oot_positions = [
    int(value) for value in re.findall(r'-?\d+', oot_collect.split('sMmQuestQuadX[31] = {')[1].split('};')[0])
][6:18]
assert len(mm_positions) == 12 and mm_positions == oot_positions
for row in (mm_positions[:6], mm_positions[6:]):
    assert all(right - left >= 16 for left, right in zip(row, row[1:]))
assert 'CURSOR_NONE, QUEST_SONG_ELEGY, QUEST_SONG_SARIA' in (PAUSE_MM / 'z_kaleido_collect.c').read_text()
assert 'QUEST_SONG_OATH, QUEST_SONG_SOARING, QUEST_SONG_SUN' in (PAUSE_MM / 'z_kaleido_collect.c').read_text()
assert '{ 16, -1, 9, 11 }' in oot_collect and '{ 22, 10, 15, 17 }' in oot_collect

bridge = (ROOT / 'libultraship/src/libultraship/bridge/resourcebridge.cpp').read_text()
start = bridge.index('namespace {')
end = bridge.index('uint64_t ResourceGetCrcByName', start)
production = bridge[start:end] + '}'

fixture = '''
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace Ship {
struct Archive {
    std::string name;
    bool loaded = true;
    std::unordered_set<std::string> files, invalid;
    bool HasFile(const std::string& path) { return files.contains(path); }
    bool IsLoaded() { return loaded; }
};
struct ArchiveManager {
    uint64_t revision = 0;
    std::shared_ptr<std::vector<std::shared_ptr<Archive>>> archives =
        std::make_shared<std::vector<std::shared_ptr<Archive>>>();
    uint64_t GetMountRevision() { return revision; }
    auto GetArchives() { return archives; }
    void Mount(std::shared_ptr<Archive> archive) { archives->push_back(archive); ++revision; }
};
struct IResource { std::string source, path; };
struct ResourceIdentifier { std::string path; uintptr_t owner; std::shared_ptr<Archive> archive; };
struct ResourceManager {
    std::shared_ptr<ArchiveManager> archives = std::make_shared<ArchiveManager>();
    bool alt = true;
    auto GetArchiveManager() { return archives; }
    bool IsAltAssetsEnabled() { return alt; }
    std::shared_ptr<IResource> LoadResourceProcess(const ResourceIdentifier& key, bool exact) {
        assert(exact);
        assert(key.archive->HasFile(key.path) || key.archive->HasFile(key.path + ".meta"));
        if (key.archive->invalid.contains(key.path)) return nullptr;
        return std::make_shared<IResource>(IResource{ key.archive->name, key.path });
    }
};
struct ResourceManagerScope {
    explicit ResourceManagerScope(const std::shared_ptr<ResourceManager>&) {}
};
struct Context {
    std::shared_ptr<ResourceManager> active = std::make_shared<ResourceManager>();
    auto GetResourceManager() { return active; }
    static Context* GetRawInstance() { static Context instance; return &instance; }
};
static auto peer = std::make_shared<ResourceManager>();
struct CrossRMRegistry {
    static auto Get(const std::string&) { return peer; }
};
}
namespace Fast { using Texture = Ship::IResource; }
'''
checks = '''
int main() {
    std::array<std::string, kPausePageTileCount> paths;
    std::array<void*, kPausePageTileCount> tiles;
    for (size_t i = 0; i < tiles.size(); ++i) {
        paths[i] = "__OTR__page/tile" + std::to_string(i);
        tiles[i] = paths[i].data();
    }
    auto makeAtlas = [&](const char* name, const char* prefix, size_t count, bool meta = false) {
        auto archive = std::make_shared<Ship::Archive>();
        archive->name = name;
        for (size_t i = 0; i < count; ++i) {
            archive->files.insert(prefix + paths[i].substr(7) + (meta ? ".meta" : ""));
        }
        return archive;
    };
    auto check = [&](void** resolved, std::string_view owner, std::string_view prefix) {
        assert(resolved);
        for (size_t i = 0; i < tiles.size(); ++i) {
            const auto resource = ResourceLoadPausePageTile(static_cast<const char*>(resolved[i]));
            assert(resource && resource->source == owner && resource->path == prefix + paths[i].substr(7));
        }
    };

    auto active = Ship::Context::GetRawInstance()->GetResourceManager();
    assert(!ResourceGetPausePageTiles(tiles.data(), nullptr));
    active->archives->Mount(makeAtlas("native", "", 15));
    active->archives->Mount(makeAtlas("mismatched-top", "alt/", 3));
    auto native = ResourceGetPausePageTiles(tiles.data(), nullptr);
    check(native, "native", "");
    assert(native == ResourceGetPausePageTiles(tiles.data(), nullptr));

    auto coherent = makeAtlas("complete-alt", "alt/", 15, true);
    active->archives->Mount(coherent);
    active->archives->Mount(makeAtlas("later-title-only", "alt/", 1));
    auto wrongFormat = makeAtlas("complete-but-invalid", "alt/", 15, true);
    wrongFormat->invalid.insert("alt/page/tile5");
    active->archives->Mount(wrongFormat);
    auto complete = ResourceGetPausePageTiles(tiles.data(), nullptr);
    check(complete, "complete-alt", "alt/");
    check(native, "native", "");
    check(ResourceGetPausePageTiles(tiles.data(), "peer"), "complete-alt", "alt/");

    active->alt = false;
    check(ResourceGetPausePageTiles(tiles.data(), nullptr), "native", "");
    Ship::peer->archives->Mount(makeAtlas("peer-native", "", 15));
    check(ResourceGetPausePageTiles(tiles.data(), "peer"), "peer-native", "");
    coherent->loaded = false;
    assert(!ResourceLoadPausePageTile(static_cast<const char*>(complete[0])));
    std::puts("PASS coherent pause atlases, alternate-asset fallback, replay and distinct MM song slots");
}
'''.replace('prefix + paths[i].substr(7)', 'std::string(prefix) + paths[i].substr(7)')

with tempfile.TemporaryDirectory(prefix='shared-pause-presentation-') as tmp:
    source = Path(tmp) / 'test.cpp'
    binary = Path(tmp) / 'test'
    source.write_text(fixture + production + checks)
    subprocess.run([
        os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
        str(source), '-o', str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
