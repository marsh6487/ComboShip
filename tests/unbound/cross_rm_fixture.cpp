#include <algorithm>
#include <cassert>
#include <map>
#include <memory>
#include <string_view>
#include "soh/soh/unbound/UnboundJson.h"
#include "soh/soh/unbound/UnboundSchema.h"

#define SPDLOG_DEBUG(...) ((void)0)
#define SPDLOG_ERROR(...) ((void)0)

namespace Ship {
struct File { std::shared_ptr<std::vector<char>> Buffer; };
struct IResource { std::string owner; };
struct Archives {
    std::map<std::string, std::vector<std::shared_ptr<File>>> files;
    std::shared_ptr<std::vector<std::string>> names = std::make_shared<std::vector<std::string>>();
    unsigned queries = 0;
    void Add(const std::string& path, const std::string& text) {
        files[path].push_back(std::make_shared<File>(File{
            std::make_shared<std::vector<char>>(text.begin(), text.end()) }));
    }
    auto LoadFileFromAllLayers(const std::string& path) { ++queries; return files[path]; }
    auto LoadFile(const std::string& path) { ++queries; return files[path].back(); }
    auto ListFiles(const std::string& mask) {
        assert(mask == "unbound/actors/*.json"); ++queries; return names;
    }
};
struct ResourceManager {
    std::string name;
    std::shared_ptr<Archives> archives = std::make_shared<Archives>();
    explicit ResourceManager(std::string name) : name(name) {}
    auto GetArchiveManager() { return archives; }
    std::shared_ptr<IResource> LoadResourceProcess(const char* path) {
        assert(std::string(path) == "scenes/custom/collision.json");
        return std::make_shared<IResource>(IResource{ name });
    }
};
struct Context {
    std::shared_ptr<ResourceManager> active;
    static Context* GetRawInstance() { static Context context; return &context; }
    auto GetResourceManager() { return active; }
};
struct CrossRMRegistry {
    inline static std::shared_ptr<ResourceManager> oot;
    static auto GetOrActive(const std::string& name) {
        assert(name == "oot"); return oot ? oot : Context::GetRawInstance()->active;
    }
};
}

namespace SOH::Unbound {
/* ACTUAL JSON LOADERS */
}
namespace SOH {
/* ACTUAL NESTED RESOURCE LOADER */
}

int main() {
    auto oot = std::make_shared<Ship::ResourceManager>("oot");
    auto mm = std::make_shared<Ship::ResourceManager>("mm");
    Ship::CrossRMRegistry::oot = oot;
    Ship::Context::GetRawInstance()->active = mm;
    const std::string path = "scenes/custom/scene.json";
    oot->archives->Add(path, R"({"actors":{"0":{"id":1},"1":{"id":2}},"base":true})");
    oot->archives->Add(path, "broken JSON");
    oot->archives->Add(path, R"({"actors":{"0":{"id":77},"1":null},"mod":true})");
    mm->archives->Add(path, R"({"wrongGame":true})");
    auto doc = SOH::Unbound::LoadMergedJson(path);
    assert(doc["base"] == true && doc["mod"] == true && doc["actors"]["0"]["id"] == 77);
    assert(!doc["actors"].contains("1") && !doc.contains("wrongGame"));
    const std::string bulk = "scenes/custom/collision.bin";
    oot->archives->Add(bulk, "oot-base"); oot->archives->Add(bulk, "oot-mod");
    mm->archives->Add(bulk, "mm-bulk");
    auto bytes = SOH::Unbound::LoadBulk(bulk);
    assert(std::string(bytes.begin(), bytes.end()) == "oot-mod");
    *oot->archives->names = { "unbound/actors/npc-old.json", "unbound/actors/npc.json" };
    *mm->archives->names = { "unbound/actors/mm-actor.json" };
    auto actors = SOH::Unbound::ListRegistryFiles("unbound/actors/");
    assert(actors.size() == 2 && actors[0].name == "npc" && actors[1].name == "npc-old");
    assert(SOH::LoadSub("scenes/custom/collision.json")->owner == "oot");
    assert(mm->archives->queries == 0 && Ship::Context::GetRawInstance()->active == mm);
    // A standalone SoH process has no cross-game registry; its active manager is its owner.
    Ship::CrossRMRegistry::oot.reset(); Ship::Context::GetRawInstance()->active = oot;
    assert(SOH::Unbound::LoadMergedJson(path)["base"] == true);
}
