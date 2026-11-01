#include "z64.h"
#include <libultraship/luslog.h>
#include "ship/resource/type/Blob.h"
#include "soh/resource/type/Scene.h"
#include "soh/resource/type/scenecommand/SetSpecialObjects.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

namespace Ship {
// Cache and owner boundaries only; resource types and the caller's native
// layouts come from the engine, including Blob's Data vector.
struct TestArchive {
    bool HasFile(const std::string&) { return true; }
};
struct TestResourceManager {
    bool alt = false;
    bool fail = false;
    int calls = 0;
    std::map<std::string, std::shared_ptr<IResource>> cache;
    std::shared_ptr<IResource> LoadResource(const char* name) {
        ++calls;
        if (fail)
            throw std::runtime_error("fixture archive failure");
        std::string path = name;
        if (alt && cache.contains("alt/" + path))
            path = "alt/" + path;
        auto found = cache.find(path);
        return found == cache.end() ? nullptr : found->second;
    }
    auto GetArchiveManager() { return std::make_shared<TestArchive>(); }
    bool IsAltAssetsEnabled() { return alt; }
};
struct Context {
    std::shared_ptr<TestResourceManager> manager;
    static Context* GetRawInstance() {
        static Context instance;
        return &instance;
    }
    auto GetResourceManager() { return manager; }
};
struct CrossRMRegistry {
    inline static std::shared_ptr<TestResourceManager> oot;
    static auto GetOrActive(const char*) { return oot; }
};
struct OwnRMScope {
    std::shared_ptr<TestResourceManager> previous;
    OwnRMScope(const char*) {
        previous = Context::GetRawInstance()->manager;
        Context::GetRawInstance()->manager = CrossRMRegistry::oot;
    }
    ~OwnRMScope() { Context::GetRawInstance()->manager = previous; }
};
} // namespace Ship

// Logger output and allocation are external boundaries. Loading must complete
// without either changing native message bytes or falling through a bad cast.
void lusprintf(const char*, int32_t, int32_t, const char*, ...) {}
extern "C" s32 Object_Spawn(ObjectContext*, s16 objectId) { return objectId; }
extern "C" {
char fieldMessagePath[] = "text/elf_message_field/elf_message_field";
char dungeonMessagePath[] = "text/elf_message_ydan/elf_message_ydan";
RomFile sNaviMsgFiles[] = {
    { 0, 0, fieldMessagePath },
    { 0, 0, dungeonMessagePath },
    ROM_FILE_UNSET,
};
}

/* PRODUCTION */

template <class T> std::shared_ptr<T> resource(const std::string& path) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = path;
    return std::make_shared<T>(init);
}

static PlayState play;

int main(int argc, char** argv) {
    assert(argc == 3);
    std::string test = argv[1];
    const bool alt = std::string(argv[2]) == "1";
    auto oot = std::make_shared<Ship::TestResourceManager>();
    auto mm = std::make_shared<Ship::TestResourceManager>();
    oot->alt = alt;
    Ship::CrossRMRegistry::oot = oot;
#ifdef COMBO_BUILD
    Ship::Context::GetRawInstance()->manager = mm;
#else
    Ship::Context::GetRawInstance()->manager = oot;
#endif
    const char* path = test == "dungeon" ? "text/elf_message_ydan/elf_message_ydan"
                                         : "text/elf_message_field/elf_message_field";
    std::string selected = (alt ? "alt/" : "") + std::string(path);
    auto blob = resource<Ship::Blob>(selected);
    const uint8_t expected[4] = { 0xE0, 0x00, 0x40, 0x00 };
    blob->Data.assign(expected, expected + 4);
    oot->cache[selected] = blob;
    // A same-named MM resource must not supply an OoT scene's hint bytes.
    auto otherBlob = resource<Ship::Blob>(path);
    otherBlob->Data.assign(4, 0xFF);
    mm->cache[path] = otherBlob;
    auto command = resource<SOH::SetSpecialObjects>("special-file-command");
    command->specialObjects.elfMessage = test == "dungeon" ? 2 : 1;
    command->specialObjects.globalObject = 17;
    if (test == "none")
        command->specialObjects.elfMessage = 0;
    else if (test == "missing")
        oot->cache.clear();
    else if (test == "wrong-type")
        oot->cache[selected] = resource<SOH::Scene>(selected);
    else if (test == "throws")
        oot->fail = true;
    assert(!Scene_CommandSpecialFiles(&play, command.get()));
    assert(play.objectCtx.subKeepIndex == 17);
    if (test == "field" || test == "dungeon") {
        assert(play.cUpElfMsgs == reinterpret_cast<ElfMessage*>(blob->Data.data()));
        assert(std::memcmp(play.cUpElfMsgs, expected, sizeof(expected)) == 0);
    } else {
        assert(play.cUpElfMsgs == nullptr);
    }
    assert(oot->calls == (test == "none" ? 0 : 1));
    assert(mm->calls == 0);
#ifdef COMBO_BUILD
    assert(Ship::Context::GetRawInstance()->manager == mm);
#else
    assert(Ship::Context::GetRawInstance()->manager == oot);
#endif
    puts("PASS production special-file loader and command");
}
