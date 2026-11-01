#include "combo/NeiAssetPriority.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/Archive.h>
#include <ship/utils/StrHash64.h>
#include <ship/utils/binarytools/MemoryStream.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace Ship {
// The engine's global Context and disk IO are outside this archive-layer test.
// The production class headers and methods below retain real private state.
class Context {
public:
  inline static std::shared_ptr<ResourceManager> active;
  static Context *GetRawInstance() {
    static Context context;
    return &context;
  }
  auto GetResourceManager() { return active; }
};
static std::shared_ptr<ResourceManager> oot;
std::shared_ptr<ResourceManager>
CrossRMRegistry::GetOrActive(const std::string &) {
  return oot;
}
std::vector<std::string>
ArchiveManager::GetArchiveListInPaths(const std::vector<std::string> &paths) {
  assert(paths.empty());
  return {};
}
std::shared_ptr<Archive> ArchiveManager::AddArchive(const std::string &) {
  throw std::runtime_error("disk backend must not be needed by the layer");
}
} // namespace Ship

namespace SOH::Unbound::Schema {
inline constexpr char kReplace[] = "$replace";
}

/* PRODUCTION ARCHIVES */

#if __has_include("soh/unbound/BuiltinTempleSages.h")
#include "soh/unbound/BuiltinTempleSages.h"
#else
namespace SOH::Unbound {
bool MountBuiltinTempleSages(const std::shared_ptr<Ship::ResourceManager> &) {
  return false;
}
} // namespace SOH::Unbound
#endif

class MemoryArchive final : public Ship::Archive {
public:
  std::map<std::string, std::string> files;
  int closes = 0, writes = 0;
  std::string unreadable;
  MemoryArchive(const std::string &path) : Archive(path) {}
  bool Open() override {
    for (const auto &[path, bytes] : files)
      IndexFile(path);
    return true;
  }
  bool Close() override {
    ++closes;
    return true;
  }
  bool WriteFile(const std::string &, const std::vector<uint8_t> &) override {
    ++writes;
    return true;
  }
  std::shared_ptr<Ship::File> LoadFile(const std::string &path) override {
    auto found = files.find(path);
    if (found == files.end() || path == unreadable)
      return nullptr;
    auto file = std::make_shared<Ship::File>();
    file->Buffer = std::make_shared<std::vector<char>>(found->second.begin(),
                                                       found->second.end());
    file->IsLoaded = true;
    return file;
  }
  std::shared_ptr<Ship::File> LoadFile(uint64_t hash) override {
    auto names = ListFiles();
    auto found = names->find(hash);
    return found == names->end() ? nullptr : LoadFile(found->second);
  }
};

const std::string indexPath = "custom/temple_sages/builtin_index.json";
const std::string scenePath = "scenes/tokinoma/scene.json";
const std::string probePath = "custom/temple_sages/probe";
const std::string nativePath = "scenes/shared/tokinoma_scene/tokinoma_room_0";
const std::string unrelatedPath = "objects/native/unrelated";

std::vector<std::string> corePaths() {
  std::vector<std::string> paths;
  for (const char *prefix : {"", "alt/"})
    for (const char *suffix :
         {"scene.json", "rooms/0.json", "collision.json", "collision.bin"})
      paths.push_back(std::string(prefix) + "scenes/tokinoma/" + suffix);
  paths.insert(paths.end(), {probePath, "alt/" + probePath, nativePath,
                             "alt/" + nativePath});
  return paths;
}

std::string read(const std::shared_ptr<Ship::File> &file) {
  assert(file && file->IsLoaded && file->Buffer);
  return {file->Buffer->begin(), file->Buffer->end()};
}

int main(int argc, char **argv) {
  assert(argc == 2 || argc == 3);
  std::string test = argv[1];
  auto owner = std::make_shared<Ship::ResourceManager>();
  auto mm = std::make_shared<Ship::ResourceManager>();
  Ship::oot = owner;
  Ship::Context::active = mm;
  auto archives = owner->GetArchiveManager();
  if (test == "null-owner") {
    assert(!SOH::Unbound::MountBuiltinTempleSages(nullptr));
    return 0;
  }
  auto port = std::make_shared<MemoryArchive>("/game/soh.o2r");
  auto base = std::make_shared<MemoryArchive>("/game/oot-unbound.o2r");
  auto paths = corePaths();
  for (const auto &path : paths) {
    port->files[path] = path.ends_with(".json")
                            ? R"({"source":"builtin","retained":7})"
                            : "builtin";
    base->files[path] =
        path.ends_with(".json") ? R"({"source":"base","retained":8})" : "base";
  }
  port->files[indexPath] =
      nlohmann::json({{"version", 1}, {"resources", paths}}).dump();
  port->files[unrelatedPath] = "port";
  base->files[unrelatedPath] = "base";
  port->files["custom/temple_sages/unlisted"] = "unlisted";
  port->files["portVersion"] = "9.3.0";
  base->files["version"] = std::string("\0\x78\x56\x34\x12", 5);
  if (test == "prepared") {
    assert(argc == 3);
    port->files.clear();
    const std::filesystem::path directory = argv[2];
    for (const auto &entry :
         std::filesystem::recursive_directory_iterator(directory)) {
      if (!entry.is_regular_file())
        continue;
      std::ifstream input(entry.path(), std::ios::binary);
      assert(input);
      port->files[std::filesystem::relative(entry.path(), directory)
                      .generic_string()] =
          std::string(std::istreambuf_iterator<char>(input),
                      std::istreambuf_iterator<char>());
    }
    paths = nlohmann::json::parse(port->files.at(indexPath))["resources"]
                .get<std::vector<std::string>>();
    // The clean exporter has a canonical scene only. Story/spawn controls
    // must survive in both asset modes without a synthetic Alt base scene.
    base->files.erase("alt/" + scenePath);
    base->files[scenePath] =
        R"({"$schema":"unbound/scene/1","collision":"base-collision",
            "rooms":{"0":"base-room","1":"scenes/tokinoma/rooms/1.json"},
            "setups":{"0":{"spawns":{"0":{"pos":[0,-40,2453]}}},"4":{"story":11}}})";
  }
  if (test == "foreign-index")
    base->files[indexPath] = "{bad highest-priority index";
  if (test == "old-port")
    port->files.erase(indexPath);
  else if (test == "malformed")
    port->files[indexPath] = "{";
  else {
    auto index = nlohmann::json::parse(port->files[indexPath]);
    if (test == "traversal")
      index["resources"].push_back("custom/temple_sages/../escape");
    if (test == "backslash")
      index["resources"].push_back("custom/temple_sages/..\\escape");
    if (test == "outside")
      index["resources"].push_back(unrelatedPath);
    if (test == "duplicate")
      index["resources"].push_back(probePath);
    if (test == "incomplete")
      index["resources"].erase(0);
    if (test == "missing")
      index["resources"].push_back("custom/temple_sages/missing");
    if (test == "unreadable")
      port->unreadable = nativePath;
    if (test == "version")
      index["version"] = true;
    if (test == "index-self")
      index["resources"].push_back(indexPath);
    port->files[indexPath] = index.dump();
  }
  port->Load();
  base->Load();
  assert(archives->AddArchive(port) && archives->AddArchive(base));
  // The manager can restrict accepted ROM versions. The virtual layer must
  // pass admission without pretending it contains a version/portVersion file.
  archives->Init({}, {0x12345678});
  const auto versions = archives->GetGameVersions();
  bool valid = test == "precedence" || test == "lifetime" ||
               test == "foreign-index" || test == "prepared";
  const auto mounted = SOH::Unbound::MountBuiltinTempleSages(owner);
  assert(mounted == valid &&
         "valid builtin layer must mount; invalid index must fail atomically");
  assert(Ship::Context::active == mm &&
         archives->GetGameVersions() == versions);
  if (!valid) {
    assert(archives->GetArchives()->size() == 2);
    assert(read(archives->LoadFile(probePath)) == "base");
    return 0;
  }
  auto mountedArchives = archives->GetArchives();
  assert(mountedArchives->size() == 3);
  auto layer = mountedArchives->back();
  assert(!NeiAssetPriority::UsesModAsset(
             nativePath.c_str(), false, {port->GetPath()},
             [&](const std::string &path) {
               auto selected = archives->GetArchiveFromFile(path);
               return selected ? selected->GetPath() : std::string{};
             }) &&
         "built-in chamber must retain the shipped port's provenance");
  assert(layer->GetPath() == port->GetPath() && layer->GetPriority() == 2);
  assert(!layer->HasGameVersion());
  if (test == "prepared") {
    assert(paths.size() > 4000 && layer->ListFiles()->size() == paths.size());
    assert(port->files.size() == paths.size() + 1 &&
           "prepared port must contain only payloads plus index");
    for (const auto &path : paths) {
      assert(read(archives->LoadFile(path)) == port->files.at(path));
      assert(read(archives->LoadFile(CRC64(path.c_str()))) ==
             port->files.at(path));
    }
    for (const char *prefix : {"", "alt/"}) {
      const std::string directory = std::string(prefix) + "scenes/tokinoma/";
      // Resource selection adapter: probe Alt, then canonical. Archive
      // lookup and the selected document's layered merge are production.
      const std::string selected =
          prefix[0] && archives->GetArchiveFromFile(directory + "scene.json")
              ? directory + "scene.json"
              : scenePath;
      auto scene = SOH::Unbound::LoadMergedJson(selected);
      assert(scene["collision"] == "scenes/tokinoma/collision.json");
      assert(scene["rooms"]["0"] == "scenes/tokinoma/rooms/0.json");
      assert(scene["rooms"]["1"] == "scenes/tokinoma/rooms/1.json");
      assert(scene["setups"]["0"]["spawns"]["0"]["pos"] ==
             nlohmann::json({0, -40, 2453}));
      assert(scene["setups"]["4"]["story"] == 11);
      auto room = SOH::Unbound::LoadMergedJson(directory + "rooms/0.json");
      for (const char *setup : {"0", "2"}) {
        const auto &header = room["setups"][setup];
        int markers = 0;
        for (const auto &actor : header["actors"].items())
          if (actor.value()["id"] == 302 &&
              (actor.value()["params"].get<int>() & 0xFFF8) == 0xFC00)
            ++markers;
        assert(markers == 5);
        assert(header["mesh"]["entries"]["0"]["xlu"] ==
               "custom/temple_sages/annex_xlu");
      }
      auto collision =
          SOH::Unbound::LoadMergedJson(directory + "collision.json");
      assert(collision["bounds"]["min"][2] == -3473);
    }
    assert(read(archives->LoadFile(unrelatedPath)) == "base");
    assert(!layer->LoadFile(indexPath));
    assert(SOH::Unbound::MountBuiltinTempleSages(owner) &&
           archives->GetArchives()->size() == 3);
    puts("PASS actual prepared resources: every path/hash, both JSON modes, "
         "collision and five actor markers");
    return 0;
  }
  for (const auto &path :
       {probePath, "alt/" + probePath, nativePath, "alt/" + nativePath}) {
    assert(read(archives->LoadFile(path)) == "builtin");
    assert(read(archives->LoadFile(CRC64(path.c_str()))) == "builtin");
    assert(read(layer->LoadFile(CRC64(path.c_str()))) == "builtin");
  }
  assert(read(archives->LoadFile(unrelatedPath)) == "base");
  assert(!layer->LoadFile(unrelatedPath) && !layer->LoadFile("version") &&
         !layer->LoadFile("portVersion"));
  assert(!layer->LoadFile(indexPath) &&
         !layer->LoadFile("custom/temple_sages/unlisted"));
  assert(!layer->LoadFile(uint64_t{0}));
  assert(!layer->WriteFile(probePath, {1, 2, 3}) && port->writes == 0);
  auto merged = SOH::Unbound::LoadMergedJson(scenePath);
  assert(merged["source"] == "builtin" && merged["retained"] == 7);
  assert(SOH::Unbound::MountBuiltinTempleSages(owner) &&
         archives->GetArchives()->size() == 3);
  if (test == "lifetime") {
    std::weak_ptr<MemoryArchive> source = port;
    layer->Unload();
    assert(port->IsLoaded() && port->closes == 0 &&
           !layer->LoadFile(probePath));
    layer->Load();
    assert(read(layer->LoadFile(probePath)) == "builtin");
    mountedArchives.reset();
    port.reset();
    base.reset();
    archives.reset();
    Ship::oot.reset();
    owner.reset();
    assert(!source.expired() && read(layer->LoadFile(probePath)) == "builtin");
    layer.reset();
    assert(source.expired());
  } else {
    auto mod = std::make_shared<MemoryArchive>("/game/mods/soh/optional.o2r");
    mod->files[probePath] = "optional";
    mod->files[scenePath] = R"({"source":"optional"})";
    mod->files["version"] = std::string("\0\x78\x56\x34\x12", 5);
    mod->Load();
    assert(archives->AddArchive(mod));
    assert(read(archives->LoadFile(probePath)) == "optional");
    assert(SOH::Unbound::LoadMergedJson(scenePath)["source"] == "optional");
    assert(SOH::Unbound::LoadMergedJson(scenePath)["retained"] == 7);
    assert(SOH::Unbound::MountBuiltinTempleSages(owner) &&
           archives->GetArchives()->size() == 4);
    assert(read(archives->LoadFile(probePath)) == "optional");
  }
  puts("PASS real archive priority, owner isolation and scoped built-in "
       "resources");
}
