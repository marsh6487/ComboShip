#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#define COMBO_BUILD 1
#define SPDLOG_INFO(...) ((void)0)
const std::string appShortName = "soh";
static std::filesystem::path root;
static std::vector<std::string> events;
static bool cachedVanillaScenes = false, cachedVanillaText = false;
std::string Nei_AssetDir() { return "nei"; }
namespace StringHelper {
bool IEquals(std::string a, std::string b) {
    std::transform(a.begin(), a.end(), a.begin(), ::tolower);
    std::transform(b.begin(), b.end(), b.begin(), ::tolower);
    return a == b;
}
}
namespace Ship {
struct Archives {
    std::shared_ptr<int> AddArchive(const std::string& path) {
        auto name = std::filesystem::path(path).filename().string();
        if (name == "model.ZIP" || name == "scene.o2r") {
            assert(!cachedVanillaScenes && !cachedVanillaText);
        }
        events.push_back(name);
        return std::make_shared<int>(1);
    }
};
struct ResourceManager {
    bool altAssets = true;
    std::shared_ptr<Archives> archives = std::make_shared<Archives>();
    auto GetArchiveManager() { return archives; }
    bool IsAltAssetsEnabled() { return altAssets; }
    void SetAltAssetsEnabled(bool value) { altAssets = value; }
    void UnloadResources(const std::string& mask) {
        if (mask == "scenes/*") cachedVanillaScenes = false;
        else if (mask == "text/*") cachedVanillaText = false;
        else assert(false);
    }
};
struct Context {
    inline static std::shared_ptr<ResourceManager> active;
    static std::string LocateFileAcrossAppDirs(const std::string& path, const std::string&) {
        return (root / path).string();
    }
    static std::string GetPathRelativeToAppDirectory(const std::string& path, const std::string&) {
        return (root / path).string();
    }
};
struct CrossRMRegistry {
    inline static std::shared_ptr<ResourceManager> oot;
    static auto GetOrActive(const std::string& name) {
        assert(name == "oot"); return oot ? oot : Context::active;
    }
};
}
namespace SOH::Unbound {
inline constexpr const char* kBaseArchiveName = "oot-unbound.o2r";
static std::string standaloneProblem;
enum class BaseArchiveState { None, Mounted, Converted };
BaseArchiveState EnsureBaseArchive(const std::string& dir) {
    // Clean conversion must not see authored extras or the user's Alt Assets selection.
    assert(!Ship::CrossRMRegistry::oot->altAssets && events.empty());
    assert(std::filesystem::path(dir) == root);
    cachedVanillaScenes = cachedVanillaText = true;
    events.push_back(kBaseArchiveName);
    return BaseArchiveState::Converted;
}
std::string CheckStandaloneBaseArchive() { events.push_back("validate"); return standaloneProblem; }
}
static void AbortUnboundStartup(const std::string& problem) { throw std::runtime_error(problem); }

/* ACTUAL ARCHIVE STARTUP */

int main() {
    root = std::filesystem::temp_directory_path() /
        ("unbound-archive-startup-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup { ~Cleanup() { std::filesystem::remove_all(root); } } cleanup;
    std::filesystem::create_directories(root / "nei" / "directory.o2r");
    std::filesystem::create_directories(root / "mods/soh");
    for (const char* path : { "oot.o2r", "nei/oot.o2r", "nei/oot-mq.o2r", "nei/model.ZIP", "nei/ignored.pak", "mods/soh/scene.o2r" }) {
        std::ofstream(root / path) << "fixture";
    }
    auto owner = std::make_shared<Ship::ResourceManager>();
    auto mm = std::make_shared<Ship::ResourceManager>();
    Ship::CrossRMRegistry::oot = owner;
    Ship::Context::active = mm;
    std::string oot = (root / "oot.o2r").string(), mq = (root / "oot-mq.o2r").string();
    assert(!MountStandaloneUnboundBase(oot, mq));
    FinishUnboundArchiveSetup(false, oot, mq);
    assert((events == std::vector<std::string>{ "oot-unbound.o2r", "model.ZIP", "scene.o2r" }));
    assert(owner->altAssets && Ship::Context::active == mm);
    events.clear(); std::filesystem::remove(oot); std::ofstream(root / "oot-unbound.o2r") << "base";
    assert(MountStandaloneUnboundBase(oot, mq));
    assert(events == std::vector<std::string>{ "oot-unbound.o2r" });
    events.clear(); FinishUnboundArchiveSetup(true, oot, mq);
    assert((events == std::vector<std::string>{ "validate", "model.ZIP", "scene.o2r" }));
    events.clear(); SOH::Unbound::standaloneProblem = "incompatible format";
    bool rejected = false;
    try { FinishUnboundArchiveSetup(true, oot, mq); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected && events == std::vector<std::string>{ "validate" });
}
