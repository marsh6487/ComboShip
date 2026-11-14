// Controlled raw-archive boundary using real LUS class definitions and registry.
#include "archive_fixture.h"
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/archive/ArchiveManager.h>
#include <array>
#include <cassert>
#include <map>
#include <memory>

namespace {
struct Owner {
    std::shared_ptr<Ship::ResourceManager> manager;
    std::shared_ptr<Ship::ArchiveManager> archives;
    std::map<std::string, std::vector<uint8_t>> files;
    size_t reads = 0;
    bool available = true;
};
std::array<Owner, 2> owners;
std::shared_ptr<Ship::ResourceManager> active;
thread_local bool audioThread = false;
const char* Key(bool mm) { return mm ? "mm" : "oot"; }
void OnGameThread() { assert(!audioThread && "resource access from audio callback"); }
}

namespace Ship {
class DinsFixtureContext final : public Context {};
Context::~Context() = default;
Context* Context::GetRawInstance() {
    OnGameThread();
    static DinsFixtureContext context;
    return &context;
}
std::shared_ptr<ResourceManager> Context::GetResourceManager() const {
    OnGameThread();
    return active;
}
void Context::SetResourceManager(std::shared_ptr<ResourceManager> manager) {
    OnGameThread();
    active = std::move(manager);
}
ResourceManager::ResourceManager() = default;
ResourceManager::~ResourceManager() = default;
std::shared_ptr<ArchiveManager> ResourceManager::GetArchiveManager() {
    OnGameThread();
    for (auto& owner : owners) {
        if (owner.manager.get() == this) {
            return owner.archives;
        }
    }
    return nullptr;
}
ArchiveManager::ArchiveManager() = default;
ArchiveManager::~ArchiveManager() = default;
bool ArchiveManager::HasFile(const std::string& path) {
    OnGameThread();
    for (auto& owner : owners) {
        if (owner.archives.get() == this) {
            return owner.files.contains(path);
        }
    }
    return false;
}
std::shared_ptr<File> ArchiveManager::LoadFile(const std::string& path) {
    OnGameThread();
    for (auto& owner : owners) {
        if (owner.archives.get() != this) {
            continue;
        }
        ++owner.reads;
        const auto found = owner.files.find(path);
        if (found == owner.files.end()) {
            return nullptr;
        }
        auto file = std::make_shared<File>();
        file->Buffer = std::make_shared<std::vector<char>>(found->second.begin(), found->second.end());
        file->IsLoaded = true;
        return file;
    }
    return nullptr;
}
}

namespace DinsAudioFixture {
void Reset() {
    OnGameThread();
    Ship::CrossRMRegistry::Unregister("oot");
    Ship::CrossRMRegistry::Unregister("mm");
    active.reset();
    for (bool mm : {false, true}) {
        auto& owner = owners[mm];
        owner = {};
        owner.manager = std::make_shared<Ship::ResourceManager>();
        owner.archives = std::make_shared<Ship::ArchiveManager>();
        Ship::CrossRMRegistry::Register(Key(mm), owner.manager);
    }
    active = owners[0].manager;
}
void Mount(bool mm, const std::string& path, const std::vector<uint8_t>& bytes) {
    OnGameThread();
    owners[mm].files[path] = bytes;
}
void Remove(bool mm, const std::string& path) {
    OnGameThread();
    owners[mm].files.erase(path);
}
void SetAvailable(bool mm, bool available) {
    OnGameThread();
    auto& owner = owners[mm];
    const bool wasActive = active == owner.manager;
    owner.available = available;
    if (available) {
        Ship::CrossRMRegistry::Register(Key(mm), owner.manager);
        // Reset's default active host is OoT. SetActive selects MM in solo tests.
        if (!active) active = owner.manager;
    } else {
        Ship::CrossRMRegistry::Unregister(Key(mm));
        if (wasActive) active.reset();
    }
}
void SetActive(bool mm) {
    OnGameThread();
    active = owners[mm].available ? owners[mm].manager : nullptr;
}
void ReplaceOwner(bool mm) {
    OnGameThread();
    auto& owner = owners[mm];
    const bool wasActive = active == owner.manager;
    Ship::CrossRMRegistry::Unregister(Key(mm));
    owner.manager = std::make_shared<Ship::ResourceManager>();
    owner.archives = std::make_shared<Ship::ArchiveManager>();
    owner.reads = 0;
    if (owner.available) Ship::CrossRMRegistry::Register(Key(mm), owner.manager);
    if (wasActive) active = owner.available ? owner.manager : nullptr;
}
size_t ReadCount(bool mm) { return owners[mm].reads; }
void SetAudioThread(bool value) { audioThread = value; }
}
