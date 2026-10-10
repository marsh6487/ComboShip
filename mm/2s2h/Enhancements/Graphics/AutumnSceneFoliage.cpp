#include "AutumnSceneFoliage.h"
#include "2s2h/Enhancements/Audio/MMWeather.h"
#include <fast/resource/type/DisplayList.h>
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/utils/StrHash64.h>
#ifdef COMBO_BUILD
#include <ship/resource/CrossRMRegistry.h>
#endif
#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

extern "C" {
#include "global.h"
}

namespace {
// Texture identity, rather than command offsets or an old scene export, owns
// the palette. Both generated scene namespaces and explicit Alt hashes occur
// in edited packs. No source texture, UV, vertex or alpha command is changed.
struct Material {
    const char* scene;
    const char* texture;
    uint32_t color;
};
constexpr Material kMaterials[] = {
    { "Z2_00KEIKOKU", "Z2_00KEIKOKUTex_01BE50", 0xD99C45FF },
    { "Z2_00KEIKOKU", "Z2_00KEIKOKUTex_01C650", 0xD99C45FF },
    { "Z2_00KEIKOKU", "Z2_00KEIKOKUTex_01DE50", 0xD99C45FF },
    { "Z2_00KEIKOKU", "Z2_00KEIKOKUTex_02C050", 0xD99C45FF },
    { "Z2_00KEIKOKU", "Z2_00KEIKOKUTex_021650", 0xD99C45FF },
    { "Z2_00KEIKOKU", "Z2_00KEIKOKUTex_0216D0", 0xD99C45FF },
    { "Z2_00KEIKOKU", "Z2_00KEIKOKUTex_0218D0", 0xD99C45FF },
    { "Z2_00KEIKOKU", "Z2_00KEIKOKUTex_034898", 0xB96848FF },
    { "Z2_00KEIKOKU", "Z2_00KEIKOKUTex_034098", 0xB96848FF },
    { "Z2_00KEIKOKU", "Z2_00KEIKOKUTex_037098", 0xB96848FF },
    // Exported brush tops also draw terrain, so their nonempty per-material
    // variant bypasses the whole-list fallback. Match the private foliage loads.
    { "Z2_00KEIKOKU", "foliage_poc3/leaves_rgba", 0xB96848FF },
    { "Z2_00KEIKOKU", "foliage_poc3/stems_rgba", 0xB96848FF },
    { "Z2_21MITURINMAE", "Z2_21MITURINMAETex_0055D0", 0xB96848FF },
};

uint32_t ForestListColor(std::string path) {
    if (path.starts_with("alt/"))
        path.erase(0, 4);
    // This exact draw owns only the lower forest wallpaper and upper canopy.
    // Scope both together even when a replacement exposes private or mixed
    // texture identities. Do not recolor adjacent terrain or architecture.
    for (const auto* suffix : { "", "_scene" }) {
        const std::string prefix = "scenes/nonmq/Z2_00KEIKOKU" + std::string(suffix) + "/Z2_00KEIKOKU_room_00";
        for (const auto* setup : { "", "Set_001200", "Set_001340", "Set_0014C0", "Set_0015D0", "Set_001750",
                                   "Set_0019F0", "Set_001B30", "Set_001C90", "Set_001E60" }) {
            if (path == prefix + setup + "DL_0241B0")
                return 0xB96848FF;
        }
    }
    return 0;
}

uint32_t WholeListColor(std::string path) {
    if (path.starts_with("alt/"))
        path.erase(0, 4);
    // The accepted POC3 foliage roots call private child materials. Follow
    // these exact foliage-only roots without guessing their private texture names.
    for (const auto* suffix : { "", "_scene" }) {
        const std::string prefix = "scenes/nonmq/Z2_00KEIKOKU" + std::string(suffix) + "/Z2_00KEIKOKU_room_00DL_";
        for (const auto* top : { "0153A8", "015DF0", "016598" }) {
            if (path == prefix + top)
                return 0xD99C45FF;
        }
        for (const auto* side : { "0158E8", "016180", "016A38" }) {
            if (path == prefix + side)
                return 0xB96848FF;
        }
    }
    return 0;
}

uint32_t MaterialColor(uint64_t hash) {
    static const auto colors = [] {
        std::map<uint64_t, uint32_t> result;
        for (const auto& material : kMaterials) {
            for (const auto* suffix : { "", "_scene" }) {
                const std::string path =
                    "scenes/nonmq/" + std::string(material.scene) + suffix + "/" + material.texture;
                result[CRC64(path.c_str())] = material.color;
                result[CRC64(("alt/" + path).c_str())] = material.color;
            }
        }
        return result;
    }();
    const auto found = colors.find(hash);
    return found == colors.end() ? 0 : found->second;
}

std::vector<Gfx> BuildVariant(const std::vector<Gfx>& source) {
    std::vector<Gfx> result;
    uint32_t color = 0;
    bool changed = false;
    for (size_t i = 0; i < source.size();) {
        const auto opcode = static_cast<uint8_t>(source[i].words.w0 >> 24);
        size_t length = 1;
        switch (opcode) {
            case G_SETTIMG_OTR_HASH:
            case G_VTX_OTR_HASH:
            case G_MARKER:
            case G_DL_OTR_HASH:
                length = 2;
                break;
            case G_VTX:
            case G_CULLDL:
            case G_TRI1:
            case G_TRI2:
            case G_TEXTURE:
            case G_GEOMETRYMODE:
            case G_ENDDL:
            case G_DL:
            case G_SETOTHERMODE_L:
            case G_SETOTHERMODE_H:
            case G_RDPLOADSYNC:
            case G_RDPPIPESYNC:
            case G_RDPTILESYNC:
            case G_LOADTLUT:
            case G_SETTILESIZE:
            case G_LOADBLOCK:
            case G_SETTILE:
            case G_SETPRIMCOLOR:
            case G_SETENVCOLOR:
            case G_SETCOMBINE:
            case G_SETTIMG:
                break;
            default:
                // Unknown/custom extensions and existing grayscale treatments
                // retain their mod's original list. Never guess their word count.
                return {};
        }
        if (i + length > source.size()) {
            return {};
        }
        if (opcode == G_SETTIMG_OTR_HASH) {
            color = MaterialColor((uint64_t(source[i + 1].words.w0) << 32) | source[i + 1].words.w1);
        } else if (opcode == G_SETTIMG || opcode == G_DL || opcode == G_DL_OTR_HASH) {
            // A child list can change texture state. Require a subsequent
            // explicit selected texture load before recoloring more triangles.
            color = 0;
        }
        if (opcode == G_TRI1 || opcode == G_TRI2) {
            size_t end = i + 1;
            while (end < source.size()) {
                const auto next = source[end].words.w0 >> 24;
                if (next != G_TRI1 && next != G_TRI2) {
                    break;
                }
                ++end;
            }
            if (color != 0) {
                result.push_back(gsSPGrayscale(true));
                result.push_back(gsDPSetGrayscaleColor(color >> 24, (color >> 16) & 255, (color >> 8) & 255, 255));
                changed = true;
            }
            result.insert(result.end(), source.begin() + i, source.begin() + end);
            if (color != 0) {
                result.push_back(gsSPGrayscale(false));
            }
            i = end;
            continue;
        }
        result.insert(result.end(), source.begin() + i, source.begin() + i + length);
        if (opcode == G_ENDDL) {
            return changed && i + 1 == source.size() ? result : std::vector<Gfx>{};
        }
        i += length;
    }
    return {};
}

bool SameCommand(const Gfx& first, const Gfx& second) {
    return first.words.w0 == second.words.w0 && first.words.w1 == second.words.w1;
}

struct Variant {
    // As with the actor material caches, retain commands and their source for
    // queued raw display-list pointers, including after a season/Alt transition.
    std::shared_ptr<Fast::DisplayList> source;
    std::vector<Gfx> original;
    std::vector<Gfx> commands;
    // The interpreter writes resolved texture pointers into executed lists.
    // A scoped child must not mutate the immutable source comparison above.
    std::vector<Gfx> draw;

    Gfx Branch() const {
        return gsSPBranchList(commands.data());
    }
    void Restore() {
        if (!commands.empty() && !source->Instructions.empty() && SameCommand(source->Instructions[0], Branch())) {
            source->Instructions[0] = original[0];
        }
    }
    bool Apply() {
        if (commands.empty() || source->Instructions.size() != original.size()) {
            return false;
        }
        // Respect later patches to the same loaded list. Do not freeze another
        // feature's dynamic commands into this material-only variant.
        for (size_t i = 0; i < original.size(); ++i) {
            if (i == 0 && SameCommand(source->Instructions[i], Branch())) {
                continue;
            }
            if (!SameCommand(source->Instructions[i], original[i])) {
                Restore();
                return false;
            }
        }
        source->Instructions[0] = Branch();
        return true;
    }
};

std::map<const Fast::DisplayList*, Variant> sVariants;
std::map<const Fast::DisplayList*, std::weak_ptr<Fast::DisplayList>> sIgnored;
std::set<const Fast::DisplayList*> sActive;
std::vector<std::string> sPaths;
const Ship::ArchiveManager* sArchives = nullptr;
int sScene = -1;

void RestoreActive() {
    for (const auto* source : sActive) {
        sVariants.at(source).Restore();
    }
    sActive.clear();
}

void ClearCache() {
    // Registered teardown runs after rendering stops and before game DLLs are
    // unloaded, so queued copies can be released at this boundary.
    MMAutumnSceneFoliage_Reset();
    sVariants.clear();
}
} // namespace

void MMAutumnSceneFoliage_Update(const PlayState* play) {
    const char* scene = play && play->sceneId == SCENE_00KEIKOKU      ? "Z2_00KEIKOKU"
                        : play && play->sceneId == SCENE_21MITURINMAE ? "Z2_21MITURINMAE"
                                                                      : nullptr;
    if (scene == nullptr || MMWeather_SeasonForPlay(play) != SEASON_AUTUMN) {
        MMAutumnSceneFoliage_Reset();
        return;
    }
#ifdef COMBO_BUILD
    static const bool teardownRegistered = [] {
        Ship::CrossRMRegistry::RegisterTeardownListener(ClearCache);
        return true;
    }();
    (void)teardownRegistered;
    Ship::OwnRMScope rmScope("mm");
#endif
    auto* context = Ship::Context::GetRawInstance();
    auto rm = context ? context->GetResourceManager() : nullptr;
    auto archives = rm ? rm->GetArchiveManager() : nullptr;
    if (archives == nullptr) {
        MMAutumnSceneFoliage_Reset();
        return;
    }
    if (sScene != play->sceneId || sArchives != archives.get()) {
        RestoreActive();
        sScene = play->sceneId;
        sArchives = archives.get();
        std::set<std::string> paths;
        for (const auto* suffix : { "", "_scene" }) {
            const std::string prefix = "scenes/nonmq/" + std::string(scene) + suffix + "/";
            for (const auto* assetPrefix : { "", "alt/" }) {
                auto files = archives->ListFiles(assetPrefix + prefix + "*DL*");
                if (files == nullptr) {
                    continue;
                }
                for (auto path : *files) {
                    if (path.find("room_") == std::string::npos || path.find("DL_") == std::string::npos) {
                        continue;
                    }
                    if (path.starts_with("alt/")) {
                        path.erase(0, 4);
                    }
                    if (path.ends_with(".meta")) {
                        path.resize(path.size() - 5);
                    }
                    paths.insert(path);
                }
            }
        }
        sPaths.assign(paths.begin(), paths.end());
    }
    std::set<const Fast::DisplayList*> active;
    for (const auto& path : sPaths) {
        std::string resolved = path;
        if (rm->IsAltAssetsEnabled()) {
            const std::string altPath = "alt/" + path;
            if (archives->HasFile(altPath) || archives->HasFile(altPath + ".meta")) {
                resolved = altPath;
            }
        }
        auto loaded = rm->GetCachedResource(resolved, true);
        if (loaded == nullptr && resolved != path && rm->GetCachedResource(path, true) != nullptr) {
            // Preload a cold Alt only when replacing an already-loaded scene
            // list. Leave unrelated/unloaded rooms to the normal render path.
            loaded = rm->LoadResource(resolved, true);
        }
        auto resource = std::dynamic_pointer_cast<Fast::DisplayList>(loaded);
        if (resource == nullptr || resource->Instructions.empty()) {
            continue;
        }
        auto it = sVariants.find(resource.get());
        if (it == sVariants.end()) {
            const auto ignored = sIgnored.find(resource.get());
            if (ignored != sIgnored.end() && ignored->second.lock() == resource) {
                continue;
            }
            const auto forestColor = ForestListColor(path);
            auto commands = forestColor != 0 ? std::vector<Gfx>{} : BuildVariant(resource->Instructions);
            const auto wholeColor = forestColor != 0 ? forestColor : WholeListColor(path);
            if (commands.empty() && wholeColor == 0) {
                // Ordinary scene materials stay owned by the resource manager;
                // don't retain or duplicate every non-foliage list in the scene.
                sIgnored[resource.get()] = resource;
                continue;
            }
            Variant variant{ resource, resource->Instructions, std::move(commands), {} };
            if (variant.commands.empty()) {
                // Execute the intact root as a pushed child, then restore the
                // color even when that root ends with a tail branch. Copies
                // retain all dynamic child calls, geometry and alpha commands.
                variant.draw = variant.original;
                variant.commands = { gsSPGrayscale(true),
                                     gsDPSetGrayscaleColor(wholeColor >> 24, (wholeColor >> 16) & 255,
                                                           (wholeColor >> 8) & 255, 255),
                                     gsSPDisplayList(variant.draw.data()), gsSPGrayscale(false), gsSPEndDisplayList() };
            }
            it = sVariants.emplace(resource.get(), std::move(variant)).first;
        }
        if (it->second.Apply()) {
            active.insert(resource.get());
        }
    }
    for (const auto* source : sActive) {
        if (!active.contains(source)) {
            sVariants.at(source).Restore();
        }
    }
    sActive = std::move(active);
}

void MMAutumnSceneFoliage_Reset() {
    RestoreActive();
    sPaths.clear();
    sIgnored.clear();
    sArchives = nullptr;
    sScene = -1;
}
