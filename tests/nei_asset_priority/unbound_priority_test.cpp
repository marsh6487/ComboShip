// The archive lookup and application paths are controlled boundaries. The
// production provenance decision and held-sword ownership gate run unchanged.
#include <cassert>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "../../combo/NeiAssetPriority.h"

namespace Ship {
struct Archive {
    std::string path;
    std::string GetPath() const { return path; }
};
struct ArchiveManager {
    std::unordered_map<std::string, std::string> files;
    bool HasFile(const std::string& path) const { return files.contains(path); }
    std::shared_ptr<Archive> GetArchiveFromFile(const std::string& path) const {
        const auto found = files.find(path);
        return found == files.end() ? nullptr : std::make_shared<Archive>(Archive{found->second});
    }
};
struct ResourceManager {
    bool alt = false;
    std::shared_ptr<ArchiveManager> archives = std::make_shared<ArchiveManager>();
    bool IsAltAssetsEnabled() const { return alt; }
    std::shared_ptr<ArchiveManager> GetArchiveManager() const { return archives; }
};
struct Context {
    static std::string LocateFileAcrossAppDirs(const std::string& file, const char* app = nullptr) {
        if (file == "oot-unbound.o2r")
            assert(app && std::strcmp(app, "soh") == 0);
        return "/game/" + file;
    }
};
namespace CrossRMRegistry {
std::unordered_map<std::string, std::shared_ptr<ResourceManager>> owners;
std::shared_ptr<ResourceManager> Get(const std::string& name) {
    const auto found = owners.find(name);
    return found == owners.end() ? nullptr : found->second;
}
}
}

namespace NeiAssetPriority {
#include "unbound_asset_priority.inc"
}

#include "../../combo/NeiHeldSword.h"
extern "C" int ResourceMgr_IsModAsset(const char* path) {
#ifdef NEI_EQUIPMENT_MM
    return NeiAssetPriority::IsModAsset("mm", "mm", path);
#else
    return NeiAssetPriority::IsModAsset("oot", "oot", path);
#endif
}
extern "C" int ResourceMgr_IsModAssetForGame(const char* game, const char* path) {
    return NeiAssetPriority::IsModAsset("mm", game, path);
}

int main() {
    const auto oot = std::make_shared<Ship::ResourceManager>();
    const auto mm = std::make_shared<Ship::ResourceManager>();
    Ship::CrossRMRegistry::owners = {{"oot", oot}, {"mm", mm}};
    const char* hand = "objects/object_link_child/gLinkChildLeftFistAndKokiriSwordNearDL";
    const char* gi = "objects/object_gi_sword_1/gGiKokiriSwordDL";
    const char* boots = "objects/object_gi_boots_2/gGiIronBootsDL";
    for (const char* path : {hand, gi, boots})
        oot->archives->files[path] = "/game/oot-unbound.o2r";
    for (bool alt : {false, true}) {
        oot->alt = alt;
        mm->alt = !alt; // The resource owner's state must control selection.
        assert(!NeiHeldSword_EquipmentSelected(NEI_HELD_SWORD_MM_KOKIRI, NEI_HELD_SWORD_MM_HUMAN) &&
               "Unbound stock hands incorrectly suppress the authored MM Kokiri Sword");
        for (const char* path : {hand, gi, boots})
            assert(!NeiAssetPriority::IsModAsset("mm", "oot", path) &&
                   "The generated Unbound base must not own a selected mod replacement");
    }
    // A similarly named archive inside mods remains an intentional replacement.
    oot->archives->files[hand] = "/game/mods/oot-unbound.o2r";
    assert(NeiHeldSword_EquipmentSelected(NEI_HELD_SWORD_MM_KOKIRI, NEI_HELD_SWORD_MM_HUMAN));
    oot->archives->files[hand] = "/game/./oot-unbound.o2r";
    assert(!NeiHeldSword_EquipmentSelected(NEI_HELD_SWORD_MM_KOKIRI, NEI_HELD_SWORD_MM_HUMAN));
    // Alt and metadata-only mod overrides still win, and change live.
    const std::string alternate = std::string("alt/") + hand + ".meta";
    oot->archives->files[alternate] = "/game/mods/equipment.o2r";
    oot->alt = true;
    assert(NeiHeldSword_EquipmentSelected(NEI_HELD_SWORD_MM_KOKIRI, NEI_HELD_SWORD_MM_HUMAN));
    oot->alt = false;
    assert(!NeiHeldSword_EquipmentSelected(NEI_HELD_SWORD_MM_KOKIRI, NEI_HELD_SWORD_MM_HUMAN));
    oot->archives->files.erase(alternate);
    // Missing owners never borrow the active host's resources.
    Ship::CrossRMRegistry::owners.erase("oot");
    assert(!NeiAssetPriority::IsModAsset("mm", "oot", gi));
}
