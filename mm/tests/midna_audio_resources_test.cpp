// The real adapter runs against archive and resource boundaries. An active OoT
// manager deliberately contains different data from MM to catch ownership leaks.
#include "test_require.h"
#include "2s2h/Enhancements/Companion/MidnaAudioResources.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <limits>

static std::map<std::string, int> settings;
static std::map<std::string, float> volumes;
int CVarGetInteger(const char* key, int fallback) {
    return settings.count(key) ? settings.at(key) : fallback;
}
float CVarGetFloat(const char* key, float fallback) {
    return volumes.count(key) ? volumes.at(key) : fallback;
}
namespace Ship {
struct File {
    std::shared_ptr<std::vector<char>> Buffer;
};
struct ArchiveManager {
    std::map<std::string, std::shared_ptr<File>> files;
    int loads = 0;
    bool HasFile(const std::string& path) {
        return files.count(path) != 0;
    }
    std::shared_ptr<File> LoadFile(const std::string& path) {
        ++loads;
        return files.at(path);
    }
};
struct Resource {
    int payload = 42;
    size_t size = sizeof(payload);
    size_t GetPointerSize() {
        return size;
    }
    void* GetRawPointer() {
        return &payload;
    }
};
struct ResourceManager {
    std::shared_ptr<ArchiveManager> archive = std::make_shared<ArchiveManager>();
    std::map<std::string, std::shared_ptr<Resource>> resources;
    std::shared_ptr<ArchiveManager> GetArchiveManager() {
        return archive;
    }
    std::shared_ptr<Resource> LoadResource(const std::string& path) {
        return resources[path];
    }
};
static std::shared_ptr<ResourceManager> mm = std::make_shared<ResourceManager>();
static std::shared_ptr<ResourceManager> oot = std::make_shared<ResourceManager>();
struct CrossRMRegistry {
    static std::shared_ptr<ResourceManager> Get(const std::string& game) {
        REQUIRE(game == "mm");
        return mm;
    }
};
struct Context {
    static Context* GetRawInstance() {
        static Context context;
        return &context;
    }
    std::shared_ptr<ResourceManager> GetResourceManager() {
        return oot;
    }
};
} // namespace Ship

/* PRODUCTION_MIDNA_AUDIO_RESOURCES */

int main() {
    const char* model = "objects/midna_navi/poc1/MidnaFloatDL";
    const char* path = "objects/midna_navi/audio/dash.wav";
    auto file = std::make_shared<Ship::File>();
    file->Buffer = std::make_shared<std::vector<char>>(std::initializer_list<char>{ 'R', 'I', 'F', 'F', -1 });
    Ship::oot->archive->files[path] = file;
    Ship::oot->archive->files[model] = file;
    std::vector<uint8_t> bytes;
    REQUIRE(!MMMidnaAudioResources::Enabled());
    settings["gEnhancements.MidnaCompanion"] = 1;
    REQUIRE(!MMMidnaAudioResources::Enabled()); // OoT opt-in does not enable MM
    settings["gEnhancements.MidnaCompanionMM"] = 1;
    REQUIRE(MMMidnaAudioResources::Enabled());
    settings["gEnhancements.MidnaCompanion"] = 0;
    REQUIRE(MMMidnaAudioResources::Enabled());
    REQUIRE(!MMMidnaAudioResources::HasModel());
    REQUIRE(!MMMidnaAudioResources::ReadClip(path, bytes));
    REQUIRE(MMMidnaResources_Load(model) == nullptr);
    Ship::mm->archive->files[path] = file;
    Ship::mm->archive->files[model] = file;
    REQUIRE(MMMidnaAudioResources::HasModel());
    REQUIRE(MMMidnaAudioResources::ReadClip(path, bytes));
    REQUIRE(bytes.size() == 5 && bytes[4] == 255);
    REQUIRE(Ship::mm->archive->loads == 1 && Ship::oot->archive->loads == 0);
    REQUIRE(!MMMidnaAudioResources::ReadClip("objects/midna_navi/audio/absent.wav", bytes));
    REQUIRE(MMMidnaResources_Load(model) == nullptr); // advertised but unloadable
    auto first = std::make_shared<Ship::Resource>();
    Ship::mm->resources[model] = first;
    REQUIRE(MMMidnaResources_Load(model) == &first->payload);
    auto next = std::make_shared<Ship::Resource>();
    Ship::mm->resources[model] = next;
    REQUIRE(MMMidnaResources_Load(model) == &next->payload); // no stale raw-pointer cache
    next->size = 0;
    REQUIRE(MMMidnaResources_Load(model) == nullptr);
    Ship::mm->archive->files.erase(model);
    REQUIRE(MMMidnaResources_Load(model) == nullptr);
    file->Buffer->clear();
    REQUIRE(!MMMidnaAudioResources::ReadClip(path, bytes));
    file->Buffer->resize(32000 * 10 * 2 + 65537);
    REQUIRE(!MMMidnaAudioResources::ReadClip(path, bytes));
    file->Buffer.reset();
    REQUIRE(!MMMidnaAudioResources::ReadClip(path, bytes));
    Ship::mm->archive->files[path].reset();
    REQUIRE(!MMMidnaAudioResources::ReadClip(path, bytes));
    Ship::mm.reset();
    REQUIRE(!MMMidnaAudioResources::HasModel());
    REQUIRE(!MMMidnaAudioResources::ReadClip(path, bytes));
    REQUIRE(MMMidnaResources_Load(model) == nullptr); // absent MM never falls back to active OoT
    REQUIRE(MMMidnaAudioResources::Gain() == 0.4f);
    volumes["gSettings.Audio.SoundEffectsVolume"] = 0.5f;
    REQUIRE(MMMidnaAudioResources::Gain() == 0.2f);
    volumes["gSettings.Audio.MasterVolume"] = -1.0f;
    REQUIRE(MMMidnaAudioResources::Gain() == 0.0f);
    volumes["gSettings.Audio.MasterVolume"] = 2.0f;
    volumes["gSettings.Audio.SoundEffectsVolume"] = 2.0f;
    REQUIRE(MMMidnaAudioResources::Gain() == 1.0f);
    volumes["gSettings.Audio.MasterVolume"] = std::numeric_limits<float>::quiet_NaN();
    REQUIRE(MMMidnaAudioResources::Gain() == 0.0f);
    puts("PASS: MM manager isolation, exact WAV paths, independent opt-in, resource reloads and MM volumes");
}
