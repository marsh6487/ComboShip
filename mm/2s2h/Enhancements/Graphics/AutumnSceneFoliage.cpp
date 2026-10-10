#include "AutumnSceneFoliage.h"
#include "2s2h/Enhancements/Audio/MMWeather.h"
#include <fast/resource/type/DisplayList.h>
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/Archive.h>
#include <ship/utils/StrHash64.h>
#include <spdlog/spdlog.h>
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
const char* SceneResourceName(int sceneId) {
    switch (sceneId) {
#define DEFINE_SCENE(name, enumId, ...) \
    case enumId:                        \
        return #name;
#define DEFINE_SCENE_UNSET(...)
#include "tables/scene_table.h"
#undef DEFINE_SCENE
#undef DEFINE_SCENE_UNSET
        default:
            return nullptr;
    }
}

// Texture identity, rather than command offsets or an old scene export, owns
// the palette. Both generated scene namespaces and explicit Alt hashes occur
// in edited packs. No source texture, UV, vertex or alpha command is changed.
struct Material {
    const char* path;
    uint32_t color;
};
constexpr Material kMaterials[] = {
#include "AutumnSceneMaterials.inc"
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
                std::string path = material.path;
                if (path.starts_with("scenes/nonmq/")) {
                    path.insert(path.find('/', 13), suffix);
                } else if (*suffix != '\0') {
                    continue; // Shared misc texture paths have no generated scene namespace.
                }
                result[CRC64(path.c_str())] = material.color;
                result[CRC64(("alt/" + path).c_str())] = material.color;
            }
        }
        return result;
    }();
    const auto found = colors.find(hash);
    return found == colors.end() ? 0 : found->second;
}

size_t MaterialCommandLength(uint8_t opcode) {
    switch (opcode) {
        case G_SETTIMG_OTR_HASH:
        case G_VTX_OTR_HASH:
        case G_MARKER:
        case G_DL_OTR_HASH:
        case G_MTX_OTR:
            return 2;
        case G_VTX:
        case G_MTX:
        case G_POPMTX:
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
        case G_LOADTILE:
        case G_SETTILE:
        case G_SETPRIMCOLOR:
        case G_SETENVCOLOR:
        case G_SETCOMBINE:
        case G_SETTIMG:
            return 1;
        default:
            // Unknown/custom extensions and existing grayscale treatments
            // retain their mod's original list. Never guess their word count.
            return 0;
    }
}

struct RendererPointerSlot {
    size_t index;
    uintptr_t observed;
    size_t copyIndex;
};

std::vector<RendererPointerSlot> RendererPointerSlots(const std::vector<Gfx>& source, bool seasonalCopy = false) {
    std::vector<RendererPointerSlot> slots;
    for (size_t i = 0; i < source.size();) {
        const auto opcode = static_cast<uint8_t>(source[i].words.w0 >> 24);
        const auto length =
            seasonalCopy && (opcode == G_SETGRAYSCALE || opcode == G_SETINTENSITY) ? 1 : MaterialCommandLength(opcode);
        if (length == 0 || i + length > source.size()) {
            // Whole-list wrappers may preserve unknown extensions. Leave their
            // words strictly compared rather than treating payloads as opcodes.
            return {};
        }
        if (opcode == G_SETTIMG_OTR_HASH || opcode == G_VTX_OTR_HASH) {
            slots.push_back({ i, source[i].words.w1, i });
        }
        i += length;
    }
    return slots;
}

std::vector<Gfx> BuildVariant(const std::vector<Gfx>& source) {
    std::vector<Gfx> result;
    uint32_t imageColor = 0;
    bool tileSlot[8]{};
    uint32_t loadedColors[2]{};
    unsigned renderTile = G_TX_RENDERTILE;
    bool explicitLoads = false;
    bool changed = false;
    for (size_t i = 0; i < source.size();) {
        const auto opcode = static_cast<uint8_t>(source[i].words.w0 >> 24);
        const auto length = MaterialCommandLength(opcode);
        if (length == 0 || i + length > source.size()) {
            return {};
        }
        if (opcode == G_SETTIMG_OTR_HASH) {
            imageColor = MaterialColor((uint64_t(source[i + 1].words.w0) << 32) | source[i + 1].words.w1);
        } else if (opcode == G_SETTIMG) {
            imageColor = 0;
        } else if (opcode == G_SETTILE) {
            // Match Interpreter::GfxDpSetTile: address 0 owns texture slot 0;
            // every nonzero TMEM address owns texture slot 1.
            tileSlot[(source[i].words.w1 >> 24) & 7] = (source[i].words.w0 & 0x1FF) != 0;
        } else if (opcode == G_LOADBLOCK || opcode == G_LOADTILE) {
            // A second image may populate a detail tile without replacing the
            // grass in the primary render tile. Follow TMEM ownership, not the
            // last image command (which may also be a palette load).
            loadedColors[tileSlot[(source[i].words.w1 >> 24) & 7]] = imageColor;
            explicitLoads = true;
        } else if (opcode == G_TEXTURE) {
            renderTile = (source[i].words.w0 >> 8) & 7;
        } else if (opcode == G_DL || opcode == G_DL_OTR_HASH) {
            // A child list can change texture state. Require a subsequent
            // explicit selected texture load before recoloring more triangles.
            imageColor = 0;
            loadedColors[0] = loadedColors[1] = 0;
            explicitLoads = false;
        }
        if (opcode == G_TRI1 || opcode == G_TRI2) {
            const uint32_t color = explicitLoads ? loadedColors[tileSlot[renderTile]] : imageColor;
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
    std::vector<RendererPointerSlot> pointerSlots = RendererPointerSlots(original);
    std::string forestPath;
    bool reportedPointerWrites = false;
    bool reportedConflict = false;

    Gfx Branch() const {
        return gsSPBranchList(commands.data());
    }
    void Restore() {
        if (!commands.empty() && !source->Instructions.empty() && SameCommand(source->Instructions[0], Branch())) {
            source->Instructions[0] = original[0];
            if (!pointerSlots.empty() && pointerSlots[0].index == 0) {
                // Our branch covered this cache field. Restoring its original
                // offset means the next renderer resolution is a fresh write.
                pointerSlots[0].observed = original[0].words.w1;
            }
        }
    }
    bool Apply() {
        if (commands.empty() || original.empty() || source->Instructions.size() != original.size()) {
            return false;
        }
        // Off/scene-ineligible draws execute the original list. Its hash
        // handlers cache image and vertex addresses in w1; these are not mod
        // edits. Compare every opcode and hash payload, and tolerate only the
        // renderer's pointer fields. Vertex offsets/pointer replacements and
        // all other material/geometry patches still invalidate the variant.
        size_t nextSlot = 0;
        unsigned pointerWrites = 0;
        const bool redirected = SameCommand(source->Instructions[0], Branch());
        for (size_t i = 0; i < original.size(); ++i) {
            auto* pointerSlot =
                nextSlot < pointerSlots.size() && pointerSlots[nextSlot].index == i ? &pointerSlots[nextSlot] : nullptr;
            if (pointerSlot != nullptr) {
                ++nextSlot;
            }
            if (i == 0 && SameCommand(source->Instructions[i], Branch())) {
                continue;
            }
            if (!SameCommand(source->Instructions[i], original[i])) {
                const auto& current = source->Instructions[i];
                const auto opcode = static_cast<uint8_t>(original[i].words.w0 >> 24);
                if (pointerSlot != nullptr && current.words.w0 == original[i].words.w0 && current.words.w1 > 0xFFFFF &&
                    (opcode == G_SETTIMG_OTR_HASH || pointerSlot->observed <= 0xFFFFF ||
                     current.words.w1 == pointerSlot->observed)) {
                    // Vertex hash resolution happens once. Unlike image hash
                    // reloads, a later change to that resolved address is an
                    // outside edit and must not be hidden by our copied draw.
                    ++pointerWrites;
                    continue;
                }
                if (!forestPath.empty() && !reportedConflict) {
                    SPDLOG_WARN("[AutumnForest] refusing changed draw path={} word={} old=({:X},{:X}) new=({:X},{:X})",
                                forestPath, i, original[i].words.w0, original[i].words.w1, current.words.w0,
                                current.words.w1);
                    reportedConflict = true;
                }
                Restore();
                return false;
            }
        }
        auto& copied = draw.empty() ? commands : draw;
        for (auto& slot : pointerSlots) {
            if (slot.index == 0 && redirected) {
                continue;
            }
            const auto current = source->Instructions[slot.index].words.w1;
            if ((original[slot.index].words.w0 >> 24) == G_VTX_OTR_HASH && (!redirected || current != slot.observed)) {
                // The source may resolve a newer resource while Off, or still
                // hold an offset if no Off frame rendered. Keep that exact
                // address/offset in the copy instead of replaying an old cache.
                copied[slot.copyIndex].words.w1 = current;
            }
            slot.observed = current;
        }
        if (!forestPath.empty() && pointerWrites != 0 && !reportedPointerWrites) {
            SPDLOG_INFO("[AutumnForest] accepted {} renderer pointer writes; tint remains active path={}",
                        pointerWrites, forestPath);
            reportedPointerWrites = true;
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
    const char* scene = play ? SceneResourceName(play->sceneId) : nullptr;
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
            if (forestColor != 0) {
                variant.forestPath = resolved;
                const auto initData = resource->GetInitData();
                const std::string loadedPath = initData != nullptr ? initData->Path : "";
                const auto lookupPath = loadedPath.empty() ? resolved : loadedPath;
                const auto ownerPath = archives->HasFile(lookupPath) ? lookupPath : lookupPath + ".meta";
                const auto archive = archives->HasFile(ownerPath) ? archives->GetArchiveFromFile(ownerPath) : nullptr;
                SPDLOG_INFO("[AutumnForest] bound path={} resource_path={} path_owner_archive={} words={} source={}",
                            resolved, loadedPath.empty() ? "<unknown>" : loadedPath,
                            archive ? archive->GetPath() : "<unknown>", resource->Instructions.size(),
                            static_cast<const void*>(resource.get()));
            }
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
            if (!variant.pointerSlots.empty()) {
                const auto& copied = variant.draw.empty() ? variant.commands : variant.draw;
                const auto copySlots = RendererPointerSlots(copied, true);
                if (copySlots.size() != variant.pointerSlots.size()) {
                    variant.pointerSlots.clear();
                } else {
                    for (size_t i = 0; i < copySlots.size(); ++i) {
                        variant.pointerSlots[i].copyIndex = copySlots[i].index;
                    }
                }
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
