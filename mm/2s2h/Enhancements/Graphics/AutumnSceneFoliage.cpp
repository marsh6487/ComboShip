#include "AutumnSceneFoliage.h"
#include "2s2h/Enhancements/Audio/MMWeather.h"
#include <fast/resource/type/DisplayList.h>
#include <fast/resource/type/Texture.h>
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

uint32_t SeasonPalette(uint32_t autumnColor, int selectedSeason) {
    if (selectedSeason != SEASON_WINTER)
        return autumnColor;
    switch (autumnColor) {
        case 0xD99C45FF:
        case 0xFFD078FF:
            return 0xBED2C2B0; // Soft frost; retain 31% of the original color variation.
        case 0xB96848FF:
        case 0xF4B783FF:
            return 0xB9CDD9B0; // Cool gray-blue upper/distant foliage.
        case 0xFF9F91FF:
            return 0xC4D2C9B0; // Neutral gray-green middle trees stay distinct.
        default:
            return autumnColor;
    }
}

uint32_t ForestListColor(std::string path, int selectedSeason = SEASON_AUTUMN) {
    if (path.starts_with("alt/"))
        path.erase(0, 4);
    // This exact draw owns the distant forest wallpaper and upper canopy.
    // The middle trees use shared textures in DL_003928, selected by MaterialColor.
    // Scope both together even when a replacement exposes private or mixed
    // texture identities. Do not recolor adjacent terrain or architecture.
    for (const auto* suffix : { "", "_scene" }) {
        const std::string prefix = "scenes/nonmq/Z2_00KEIKOKU" + std::string(suffix) + "/Z2_00KEIKOKU_room_00";
        for (const auto* setup : { "", "Set_001200", "Set_001340", "Set_0014C0", "Set_0015D0", "Set_001750",
                                   "Set_0019F0", "Set_001B30", "Set_001C90", "Set_001E60" }) {
            if (path == prefix + setup + "DL_0241B0")
                return SeasonPalette(0xF4B783FF, selectedSeason);
        }
    }
    return 0;
}

uint32_t WholeListColor(std::string path, int selectedSeason = SEASON_AUTUMN) {
    if (path.starts_with("alt/"))
        path.erase(0, 4);
    // The accepted POC3 foliage roots call private child materials. Follow
    // these exact foliage-only roots without guessing their private texture names.
    for (const auto* suffix : { "", "_scene" }) {
        const std::string prefix = "scenes/nonmq/Z2_00KEIKOKU" + std::string(suffix) + "/Z2_00KEIKOKU_room_00DL_";
        for (const auto* top : { "0153A8", "015DF0", "016598" }) {
            if (path == prefix + top)
                return SeasonPalette(0xFFD078FF, selectedSeason);
        }
        for (const auto* side : { "0158E8", "016180", "016A38" }) {
            if (path == prefix + side)
                return SeasonPalette(0xF4B783FF, selectedSeason);
        }
    }
    return 0;
}

uint32_t MaterialColor(uint64_t hash, int selectedSeason = SEASON_AUTUMN) {
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
    return found == colors.end() ? 0 : SeasonPalette(found->second, selectedSeason);
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

std::vector<Gfx> BuildVariant(const std::vector<Gfx>& source, int selectedSeason = SEASON_AUTUMN) {
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
            imageColor =
                MaterialColor((uint64_t(source[i + 1].words.w0) << 32) | source[i + 1].words.w1, selectedSeason);
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
                result.push_back(gsSPGrayscale(2)); // Preserve the texture's mean intensity.
                result.push_back(
                    gsDPSetGrayscaleColor(color >> 24, (color >> 16) & 255, (color >> 8) & 255, color & 255));
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
    // Observations belong to the source, not its season's immutable draw copy.
    std::shared_ptr<std::map<size_t, uintptr_t>> observedPointers = std::make_shared<std::map<size_t, uintptr_t>>();
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
                (*observedPointers)[0] = original[0].words.w1;
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
        for (auto& slot : pointerSlots) {
            const auto observed = observedPointers->find(slot.index);
            if (observed != observedPointers->end()) {
                slot.observed = observed->second;
            }
        }
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
            (*observedPointers)[slot.index] = current;
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
std::map<const Fast::DisplayList*, Variant> sWinterVariants;
std::map<const Fast::DisplayList*, std::weak_ptr<Fast::DisplayList>> sIgnored;
std::set<const Fast::DisplayList*> sActive;
std::vector<std::string> sPaths;
const Ship::ArchiveManager* sArchives = nullptr;
int sScene = -1;
int sFoliageSeason = -1;

auto& CurrentVariants() {
    return sFoliageSeason == SEASON_WINTER ? sWinterVariants : sVariants;
}

struct SurfaceTextureVariant {
    std::shared_ptr<Fast::Texture> source;
    std::shared_ptr<Fast::Texture> copy;
};
struct SurfaceTextureBinding {
    std::weak_ptr<Ship::ResourceManager> owner;
    std::string path;
    SurfaceTextureVariant texture;
};
// Retain immutable originals and copies until renderer teardown, as with the
// display-list variants. A new pixel address also gives the GPU a new cache key.
std::map<std::pair<const Fast::Texture*, uint32_t>, SurfaceTextureVariant> sSurfaceTextures;
std::vector<SurfaceTextureBinding> sSurfaceBindings;

std::shared_ptr<Fast::Texture> BuildSurfaceTexture(const std::shared_ptr<Fast::Texture>& source,
                                                   uint32_t color = 0xFFD078FF) {
    const unsigned red = color >> 24, green = (color >> 16) & 255, blue = (color >> 8) & 255;
    const unsigned colorSum = red + green + blue;
    const unsigned blend = color & 255;
    if (source->Type != Fast::TextureType::RGBA32bpp || source->ImageData == nullptr || source->Width == 0 ||
        source->Height == 0 || size_t(source->Width) * source->Height * 4 != source->ImageDataSize || colorSum == 0) {
        return nullptr;
    }
    auto copy = std::make_shared<Fast::Texture>(source->GetInitData());
    copy->Type = source->Type;
    copy->Width = source->Width;
    copy->Height = source->Height;
    copy->Flags = source->Flags;
    copy->HByteScale = source->HByteScale;
    copy->VPixelScale = source->VPixelScale;
    copy->ImageDataSize = source->ImageDataSize;
    const auto* first = reinterpret_cast<const char*>(source->ImageData);
    copy->mImageBuffer = std::make_shared<std::vector<char>>(first, first + source->ImageDataSize);
    copy->ImageData = reinterpret_cast<uint8_t*>(copy->mImageBuffer->data());
    bool changed = false;
    for (size_t i = 0; i < copy->ImageDataSize; i += 4) {
        auto* pixel = copy->ImageData + i;
        // This exact mixed atlas has green grass beside brown bark. Preserve
        // wood, neutral pixels and every alpha byte rather than tinting its draw.
        if (pixel[1] > pixel[0] && pixel[1] > pixel[2]) {
            // Match the brightness-preserving grass treatment used by the
            // display-list variants, with only byte rounding/highlight clipping.
            const unsigned sum = unsigned(pixel[0]) + pixel[1] + pixel[2];
            pixel[0] = (unsigned(pixel[0]) * (255 - blend) + std::min(255u, sum * red / colorSum) * blend) / 255;
            pixel[1] = (unsigned(pixel[1]) * (255 - blend) + std::min(255u, sum * green / colorSum) * blend) / 255;
            pixel[2] = (unsigned(pixel[2]) * (255 - blend) + std::min(255u, sum * blue / colorSum) * blend) / 255;
            changed = true;
        }
    }
    return changed ? copy : nullptr;
}

void RestoreSurfaceBinding(const SurfaceTextureBinding& binding) {
#ifdef COMBO_BUILD
    Ship::OwnRMScope rmScope("mm");
#endif
    if (auto rm = binding.owner.lock(); rm && rm->GetCachedResource(binding.path, true) == binding.texture.copy) {
        rm->CacheExternalResource(binding.path, binding.texture.source);
    }
}

void RestoreSurfaceTextures() {
    for (const auto& binding : sSurfaceBindings) {
        RestoreSurfaceBinding(binding);
    }
    sSurfaceBindings.clear();
}

void UpdateSurfaceTextures(const PlayState* play, const std::shared_ptr<Ship::ResourceManager>& rm,
                           int selectedSeason) {
    if (play->sceneId != SCENE_00KEIKOKU || !rm->IsAltAssetsEnabled()) {
        RestoreSurfaceTextures();
        return;
    }
    std::vector<SurfaceTextureBinding> active;
    for (const auto* suffix : { "", "_scene" }) {
        for (const auto* prefix : { "", "alt/" }) {
            const std::string path =
                std::string(prefix) + "scenes/nonmq/Z2_00KEIKOKU" + suffix + "/mm3d_field_poc4/textures/tl_ki_02";
            auto source = std::dynamic_pointer_cast<Fast::Texture>(rm->GetCachedResource(path, true));
            if (source == nullptr) {
                continue; // Leave cold assets to the normal renderer load path.
            }
            auto current = std::find_if(sSurfaceBindings.begin(), sSurfaceBindings.end(), [&](const auto& binding) {
                return binding.owner.lock() == rm && binding.path == path && binding.texture.copy == source;
            });
            if (current != sSurfaceBindings.end()) {
                active.push_back(*current);
                continue;
            }
            const auto color = SeasonPalette(0xFFD078FF, selectedSeason);
            const auto key = std::pair{ source.get(), color };
            auto it = sSurfaceTextures.find(key);
            if (it == sSurfaceTextures.end()) {
                auto copy = BuildSurfaceTexture(source, color);
                if (copy == nullptr) {
                    continue;
                }
                it = sSurfaceTextures.emplace(key, SurfaceTextureVariant{ source, std::move(copy) }).first;
            }
            rm->CacheExternalResource(path, it->second.copy);
            active.push_back({ rm, path, it->second });
        }
    }
    for (const auto& binding : sSurfaceBindings) {
        if (std::none_of(active.begin(), active.end(), [&](const auto& next) {
                return next.owner.lock() == binding.owner.lock() && next.path == binding.path &&
                       next.texture.copy == binding.texture.copy;
            })) {
            RestoreSurfaceBinding(binding); // Respect outside cache edits and reloads.
        }
    }
    sSurfaceBindings = std::move(active);
}

void RestoreActive() {
    for (const auto* source : sActive) {
        CurrentVariants().at(source).Restore();
    }
    sActive.clear();
}

void ClearCache() {
    // Registered teardown runs after rendering stops and before game DLLs are
    // unloaded, so queued copies can be released at this boundary.
    MMAutumnSceneFoliage_Reset();
    sVariants.clear();
    sWinterVariants.clear();
    sSurfaceTextures.clear();
}
} // namespace

void MMAutumnSceneFoliage_Update(const PlayState* play) {
    const char* scene = play ? SceneResourceName(play->sceneId) : nullptr;
    const int selectedSeason = MMWeather_SeasonForPlay(play);
    if (scene == nullptr || (selectedSeason != SEASON_AUTUMN && selectedSeason != SEASON_WINTER)) {
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
    if (sFoliageSeason != selectedSeason) {
        // Restore before selecting another immutable cache. Both seasons retain
        // their queued command/pixel buffers, without tinting a tinted copy.
        RestoreActive();
        RestoreSurfaceTextures();
        sFoliageSeason = selectedSeason;
    }
    UpdateSurfaceTextures(play, rm, selectedSeason);
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
    auto& variants = CurrentVariants();
    for (const auto& path : sPaths) {
        std::string resolved = path;
        if (rm->IsAltAssetsEnabled()) {
            const std::string altPath = "alt/" + path;
            // A failed renderer Alt lookup leaves a null ArchiveManager entry.
            // HasFile alone sees that key and would abandon the cached native
            // draw. Only a real archive can supply a replacement or .meta alias.
            const auto hasArchive = [&](const std::string& candidate) {
                return archives->HasFile(candidate) && archives->GetArchiveFromFile(candidate) != nullptr;
            };
            if (hasArchive(altPath) || hasArchive(altPath + ".meta")) {
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
        auto it = variants.find(resource.get());
        if (it == variants.end()) {
            const auto ignored = sIgnored.find(resource.get());
            if (ignored != sIgnored.end() && ignored->second.lock() == resource) {
                continue;
            }
            // Both seasons compare against the same intact source snapshot.
            // An Off draw may have resolved a first-command vertex offset since
            // the first season was cached; that season still restores its known
            // offset when removing the branch. Preserve that identity and the
            // observed pointer guards rather than snapshotting a different base.
            const auto& otherVariants = selectedSeason == SEASON_WINTER ? sVariants : sWinterVariants;
            const auto other = otherVariants.find(resource.get());
            const auto& original = other == otherVariants.end() ? resource->Instructions : other->second.original;
            const auto forestColor = ForestListColor(path, selectedSeason);
            auto commands = forestColor != 0 ? std::vector<Gfx>{} : BuildVariant(original, selectedSeason);
            const auto wholeColor = forestColor != 0 ? forestColor : WholeListColor(path, selectedSeason);
            if (commands.empty() && wholeColor == 0) {
                // Ordinary scene materials stay owned by the resource manager;
                // don't retain or duplicate every non-foliage list in the scene.
                sIgnored[resource.get()] = resource;
                continue;
            }
            Variant variant{ resource, original, std::move(commands), {} };
            if (other != otherVariants.end()) {
                variant.observedPointers = other->second.observedPointers;
            }
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
                variant.commands = { gsSPGrayscale(2), // Preserve intensity; keep legacy effects unchanged.
                                     gsDPSetGrayscaleColor(wholeColor >> 24, (wholeColor >> 16) & 255,
                                                           (wholeColor >> 8) & 255, wholeColor & 255),
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
            it = variants.emplace(resource.get(), std::move(variant)).first;
        }
        if (it->second.Apply()) {
            active.insert(resource.get());
        }
    }
    for (const auto* source : sActive) {
        if (!active.contains(source)) {
            variants.at(source).Restore();
        }
    }
    sActive = std::move(active);
}

void MMAutumnSceneFoliage_Reset() {
    RestoreActive();
    RestoreSurfaceTextures();
    sPaths.clear();
    sIgnored.clear();
    sArchives = nullptr;
    sScene = -1;
    sFoliageSeason = -1;
}
