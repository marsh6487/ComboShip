// Only the archive/cache services are test boundaries. Exercise the production
// transformation and lifecycle with real GBI instructions and MM PlayState.
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <ship/utils/StrHash64.h>
#include "2s2h/Enhancements/Graphics/AutumnSceneFoliage.h"
extern "C" {
#include "global.h"
}

namespace Ship {
struct IResource {
    virtual ~IResource() = default;
};
static std::string activeGame = "oot";
struct OwnRMScope {
    std::string previous = activeGame;
    explicit OwnRMScope(const char* game) {
        activeGame = game;
    }
    ~OwnRMScope() {
        activeGame = previous;
    }
};
struct CrossRMRegistry {
    inline static void (*teardown)() = nullptr;
    static void RegisterTeardownListener(void (*listener)()) {
        teardown = listener;
    }
};
struct ArchiveManager {
    std::map<std::string, std::shared_ptr<IResource>> files;
    bool HasFile(const std::string& path) {
        return files.contains(path);
    }
    std::shared_ptr<std::vector<std::string>> ListFiles(const std::string& mask) {
        auto result = std::make_shared<std::vector<std::string>>();
        const auto prefix = mask.substr(0, mask.find('*'));
        for (const auto& [path, resource] : files) {
            if (path.starts_with(prefix))
                result->push_back(path);
        }
        return result;
    }
};
struct ResourceManager {
    std::map<std::string, std::shared_ptr<IResource>> resources;
    std::shared_ptr<ArchiveManager> archive = std::make_shared<ArchiveManager>();
    bool alt = false;
    unsigned loads = 0;
    bool IsAltAssetsEnabled() {
        return alt;
    }
    auto GetArchiveManager() {
        return archive;
    }
    std::shared_ptr<IResource> GetCachedResource(const std::string& path, bool exact = false) {
        assert(activeGame == "mm" && exact);
        auto i = resources.find(path);
        return i == resources.end() ? nullptr : i->second;
    }
    std::shared_ptr<IResource> LoadResource(const std::string& path, bool exact = false) {
        ++loads;
        if (auto r = GetCachedResource(path, exact))
            return r;
        auto i = archive->files.find(path);
        if (i == archive->files.end())
            i = archive->files.find(path + ".meta");
        return i == archive->files.end() ? nullptr : resources[path] = i->second;
    }
};
struct Context {
    std::shared_ptr<ResourceManager> rm = std::make_shared<ResourceManager>();
    static Context* GetRawInstance() {
        static Context context;
        return &context;
    }
    auto GetResourceManager() {
        return rm;
    }
};
} // namespace Ship
namespace Fast {
struct DisplayList : Ship::IResource {
    std::vector<Gfx> Instructions;
};
} // namespace Fast
static int season = SEASON_AUTUMN;
extern "C" int MMWeather_SeasonForPlay(const PlayState*) {
    return season;
}

#include "autumn_scene_foliage_production.inc"

static Gfx Command(uintptr_t w0, uintptr_t w1) {
    Gfx command{};
    command.words.w0 = w0;
    command.words.w1 = w1;
    return command;
}
static std::vector<Gfx> MakeMaterial(const char* texture) {
    const uint64_t hash = CRC64(texture);
    return { Command(uintptr_t(G_SETTIMG_OTR_HASH) << 24, 0), Command(hash >> 32, hash & 0xFFFFFFFF),
             gsDPSetCombineMode(G_CC_MODULATEIA, G_CC_MODULATEIA), gsDPSetPrimColor(0, 0, 180, 200, 160, 192),
             gsSP1Triangle(0, 1, 2, 0) };
}
static std::shared_ptr<Fast::DisplayList> List(const char* texture) {
    auto list = std::make_shared<Fast::DisplayList>();
    list->Instructions = MakeMaterial(texture);
    auto unrelated = MakeMaterial("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_029090");
    list->Instructions.insert(list->Instructions.end(), unrelated.begin(), unrelated.end());
    list->Instructions.push_back(gsSPEndDisplayList());
    return list;
}
static bool Equal(const std::vector<Gfx>& a, const std::vector<Gfx>& b) {
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].words.w0 != b[i].words.w0 || a[i].words.w1 != b[i].words.w1)
            return false;
    }
    return true;
}
static std::vector<Gfx> RemoveTint(const std::vector<Gfx>& source) {
    std::vector<Gfx> result;
    for (size_t i = 0; i < source.size(); ++i) {
        const auto op = source[i].words.w0 >> 24;
        if (op == G_SETGRAYSCALE || op == G_SETINTENSITY)
            continue;
        result.push_back(source[i]);
        if (op == G_SETTIMG_OTR_HASH || op == G_VTX_OTR_HASH || op == G_DL_OTR_HASH || op == G_MARKER) {
            result.push_back(source.at(++i));
        }
    }
    return result;
}
static unsigned CountMaterialTriangles(const std::vector<Gfx>& commands, const char* texture, uint32_t color = 0) {
    const auto target = CRC64(texture);
    uint64_t currentTexture = 0;
    uint32_t currentColor = 0;
    bool tint = false;
    unsigned count = 0;
    for (size_t i = 0; i < commands.size(); ++i) {
        const auto op = commands[i].words.w0 >> 24;
        if (op == G_SETGRAYSCALE)
            tint = commands[i].words.w1 != 0;
        if (op == G_SETINTENSITY)
            currentColor = commands[i].words.w1;
        if (op == G_SETTIMG_OTR_HASH) {
            const auto payload = commands.at(i + 1);
            currentTexture = (uint64_t(payload.words.w0) << 32) | payload.words.w1;
        } else if (op == G_SETTIMG || op == G_DL || op == G_DL_OTR_HASH) {
            currentTexture = 0;
        }
        if ((op == G_TRI1 || op == G_TRI2) && currentTexture == target &&
            (color == 0 || (tint && currentColor == color)))
            count += op == G_TRI2 ? 2 : 1;
        if (op == G_SETTIMG_OTR_HASH || op == G_VTX_OTR_HASH || op == G_DL_OTR_HASH || op == G_MARKER)
            ++i;
    }
    assert(!tint); // Seasonal state must not leak past a tinted batch.
    return count;
}
static void CheckScopedForestCommands(const Variant& variant, const std::vector<Gfx>& original) {
    assert(Equal(variant.original, original));
    const auto& commands = variant.commands;
    assert(commands.size() == 5);
    assert(SameCommand(commands[0], gsSPGrayscale(true)));
    assert(SameCommand(commands[1], gsDPSetGrayscaleColor(185, 104, 72, 255)));
    const auto* draw = reinterpret_cast<const Gfx*>(commands[2].words.w1);
    assert(draw != variant.original.data()); // The renderer writes resolved texture pointers into executed lists.
    assert(Equal(std::vector<Gfx>(draw, draw + original.size()), original));
    assert(SameCommand(commands[2], gsSPDisplayList(draw)));
    assert(SameCommand(commands[3], gsSPGrayscale(false)));
    assert(SameCommand(commands[4], gsSPEndDisplayList()));
}

#include "autumn_scene_forest_fixture.h"
static void CheckForestDrawOwners() {
    constexpr const char* wallpaper = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_034098";
    constexpr const char* canopy = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_037098";
    assert(CountMaterialTriangles(kActualForestCommands, wallpaper) == 14);
    assert(CountMaterialTriangles(kActualForestCommands, canopy) == 8);
    const auto wallHash = CRC64(wallpaper);
    const auto canopyHash = CRC64(canopy);
    unsigned cases = 0;
    for (const auto* suffix : { "", "_scene" }) {
        for (const auto* prefix : { "", "alt/" }) {
            for (const auto* setup : { "", "Set_001200", "Set_001340", "Set_0014C0", "Set_0015D0", "Set_001750",
                                       "Set_0019F0", "Set_001B30", "Set_001C90", "Set_001E60" }) {
                // Start with both private, then mixed native/private, then native.
                for (const unsigned privateMask : { 3, 1, 2, 0 }) {
                    auto original = kActualForestCommands;
                    for (size_t i = 0; i < original.size(); ++i) {
                        const auto op = original[i].words.w0 >> 24;
                        if (op == G_SETTIMG_OTR_HASH) {
                            const auto hash = (uint64_t(original[i + 1].words.w0) << 32) | original[i + 1].words.w1;
                            const auto replacement =
                                hash == wallHash && (privateMask & 1) ? CRC64("custom/test_private_forest/wallpaper")
                                : hash == canopyHash && (privateMask & 2) ? CRC64("custom/test_private_forest/canopy")
                                                                          : hash;
                            original[i + 1] = Command(replacement >> 32, replacement & 0xFFFFFFFF);
                        }
                        if (op == G_SETTIMG_OTR_HASH || op == G_VTX_OTR_HASH || op == G_DL_OTR_HASH || op == G_MARKER)
                            ++i;
                    }
                    ClearCache();
                    auto rm = std::make_shared<Ship::ResourceManager>();
                    Ship::Context::GetRawInstance()->rm = rm;
                    const auto path = std::string(prefix) + "scenes/nonmq/Z2_00KEIKOKU" + suffix +
                                      "/Z2_00KEIKOKU_room_00" + setup + "DL_0241B0";
                    auto resource = std::make_shared<Fast::DisplayList>();
                    resource->Instructions = original;
                    rm->archive->files[path] = resource;
                    rm->resources[path] = resource;
                    rm->alt = std::string(prefix) == "alt/";
                    PlayState play{};
                    play.sceneId = SCENE_00KEIKOKU;
                    season = SEASON_AUTUMN;
                    MMAutumnSceneFoliage_Update(&play);
                    if ((resource->Instructions[0].words.w0 >> 24) != G_DL) {
                        std::fprintf(stderr, "FAIL %s: both private forest layers remain untinted\n", path.c_str());
                        assert(false);
                    }
                    const auto branch = resource->Instructions[0];
                    const auto& variant = sVariants.at(resource.get());
                    CheckScopedForestCommands(variant, original);
                    assert(branch.words.w1 == reinterpret_cast<uintptr_t>(variant.commands.data()));
                    for (size_t i = 1; i < original.size(); ++i)
                        assert(SameCommand(resource->Instructions[i], original[i]));
                    auto* draw = reinterpret_cast<Gfx*>(variant.commands[2].words.w1);
                    for (size_t i = 0; i < original.size(); ++i) {
                        const auto op = draw[i].words.w0 >> 24;
                        if (op == G_SETTIMG_OTR_HASH)
                            draw[i].words.w1 = uintptr_t(0x10000000) + i * 256;
                        if (op == G_SETTIMG_OTR_HASH || op == G_VTX_OTR_HASH || op == G_DL_OTR_HASH || op == G_MARKER)
                            ++i;
                    }
                    for (unsigned i = 0; i < 100; ++i)
                        MMAutumnSceneFoliage_Update(&play);
                    assert(SameCommand(resource->Instructions[0], branch) && rm->loads == 0 && sVariants.size() == 1);
                    season = SEASON_OFF;
                    MMAutumnSceneFoliage_Update(&play);
                    assert(Equal(resource->Instructions, original));
                    season = SEASON_AUTUMN;
                    MMAutumnSceneFoliage_Update(&play);
                    assert(SameCommand(resource->Instructions[0], branch) && sVariants.size() == 1 && rm->loads == 0);
                    ClearCache();
                    assert(Equal(resource->Instructions, original));
                    ++cases;
                }
            }
        }
    }
    std::printf("PASS %u actual forest draw cases: 14 wallpaper + 8 canopy triangles, native/private/mixed bindings, "
                "both namespaces, Alt, all ten setups, exact restoration and cache reuse\n",
                cases);
    for (const auto* name : { "Z2_00KEIKOKU_room_01DL_0241B0", "Z2_00KEIKOKU_room_00Set_999999DL_0241B0",
                              "Z2_00KEIKOKU_room_00DL_0241B1" }) {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        const auto path = std::string("scenes/nonmq/Z2_00KEIKOKU/") + name;
        auto resource = List("custom/test_private_forest/unrelated");
        const auto original = resource->Instructions;
        rm->archive->files[path] = resource;
        rm->resources[path] = resource;
        PlayState play{};
        play.sceneId = SCENE_00KEIKOKU;
        season = SEASON_AUTUMN;
        MMAutumnSceneFoliage_Update(&play);
        assert(Equal(resource->Instructions, original) && sVariants.empty());
    }
    ClearCache();
    std::puts("PASS forest owner matching excludes other rooms, unknown setups and neighboring draws");
}

static void CheckActualMaterial(const char* path, const std::vector<Gfx>& original, const char* texture,
                                unsigned expectedTriangles) {
    const auto variant = BuildVariant(original);
    assert(!variant.empty() && Equal(RemoveTint(variant), original));
    assert(CountMaterialTriangles(original, texture) == expectedTriangles);
    const auto tinted = CountMaterialTriangles(variant, texture, 0xB96848FF);
    if (tinted != expectedTriangles)
        std::fprintf(stderr, "%s: copper-red tint covers %u/%u triangles\n", path, tinted, expectedTriangles);
    assert(tinted == expectedTriangles);

    ClearCache();
    auto rm = std::make_shared<Ship::ResourceManager>();
    Ship::Context::GetRawInstance()->rm = rm;
    auto resource = std::make_shared<Fast::DisplayList>();
    resource->Instructions = original;
    rm->archive->files[path] = resource;
    rm->resources[path] = resource;
    rm->alt = std::string(path).starts_with("alt/");
    PlayState play{};
    play.sceneId = SCENE_00KEIKOKU;
    season = SEASON_AUTUMN;
    MMAutumnSceneFoliage_Update(&play);
    const auto branch = resource->Instructions[0];
    assert((branch.words.w0 >> 24) == G_DL);
    const auto& applied = sVariants.at(resource.get()).commands;
    assert(branch.words.w1 == reinterpret_cast<uintptr_t>(applied.data()));
    if (applied.size() == 5 && (applied[0].words.w0 >> 24) == G_SETGRAYSCALE) {
        CheckScopedForestCommands(sVariants.at(resource.get()), original);
    } else {
        assert(Equal(applied, variant));
    }
    for (unsigned i = 0; i < 100; ++i)
        MMAutumnSceneFoliage_Update(&play);
    assert(SameCommand(resource->Instructions[0], branch) && rm->loads == 0 && sVariants.size() == 1);
    season = SEASON_OFF;
    MMAutumnSceneFoliage_Update(&play);
    assert(Equal(resource->Instructions, original));
    season = SEASON_AUTUMN;
    MMAutumnSceneFoliage_Update(&play);
    assert(SameCommand(resource->Instructions[0], branch) && sVariants.size() == 1 && rm->loads == 0);
    ClearCache();
    assert(Equal(resource->Instructions, original));
    std::printf("PASS %s: %u/%u copper-red triangles; exact commands, cache reuse and Off restoration\n", path, tinted,
                expectedTriangles);
}
static void CheckPrivateBrushMaterials() {
    // A terrain match makes BuildVariant nonempty, so the root fallback cannot
    // cover later private leaves. Material identity must survive that boundary.
    for (const auto* suffix : { "", "_scene" }) {
        for (const auto* prefix : { "", "alt/" }) {
            const auto scene = std::string(prefix) + "scenes/nonmq/Z2_00KEIKOKU" + suffix + "/";
            const auto terrain = scene + "Z2_00KEIKOKUTex_01C650";
            const auto leaves = scene + "foliage_poc3/leaves_rgba";
            const auto stems = scene + "foliage_poc3/stems_rgba";
            const auto unrelated = scene + "foliage_poc3/unrelated_rgba";
            std::vector<Gfx> original;
            for (const auto& texture : { terrain, leaves, stems, unrelated }) {
                const auto material = MakeMaterial(texture.c_str());
                original.insert(original.end(), material.begin(), material.end());
            }
            original.push_back(gsSPEndDisplayList());
            const auto variant = BuildVariant(original);
            assert(!variant.empty() && Equal(RemoveTint(variant), original));
            assert(CountMaterialTriangles(variant, terrain.c_str(), 0xD99C45FF) == 1);
            assert(CountMaterialTriangles(variant, leaves.c_str(), 0xB96848FF) == 1);
            assert(CountMaterialTriangles(variant, stems.c_str(), 0xB96848FF) == 1);
            assert(CountMaterialTriangles(variant, unrelated.c_str()) == 1);
            assert(CountMaterialTriangles(variant, unrelated.c_str(), 0xB96848FF) == 0);
            assert(CountMaterialTriangles(variant, unrelated.c_str(), 0xD99C45FF) == 0);
        }
    }
    std::puts(
        "PASS mixed terrain/private brush materials in both namespaces and Alt hashes; unrelated draws preserved");
}
#include "autumn_scene_foliage_fixtures.inc"
static void CheckVariant(const std::shared_ptr<Fast::DisplayList>& list, const std::vector<Gfx>& original) {
    assert((list->Instructions[0].words.w0 >> 24) == G_DL);
    auto* commands = reinterpret_cast<const Gfx*>(list->Instructions[0].words.w1);
    std::vector<Gfx> stripped;
    unsigned tintedTriangles = 0;
    bool tint = false;
    for (size_t i = 0; i < original.size() + 3; ++i) {
        auto command = commands[i];
        const auto op = command.words.w0 >> 24;
        if (op == G_SETGRAYSCALE) {
            tint = command.words.w1 != 0;
            continue;
        }
        if (op == G_SETINTENSITY) {
            assert(command.words.w1 == 0xD99C45FF || command.words.w1 == 0xB96848FF);
            continue;
        }
        if (op == G_TRI1 && tint)
            ++tintedTriangles;
        stripped.push_back(command);
    }
    assert(tintedTriangles == 1 && !tint);
    assert(Equal(stripped, original)); // UVs, textures, alpha/combine/render and geometry remain exact.
}

int main() {
    // POC3 beds call private material lists and no longer load the old native
    // texture in their root. Seasonal color must follow this established root.
    {
        auto rm = Ship::Context::GetRawInstance()->rm;
        const char* path = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_015DF0";
        auto bed = std::make_shared<Fast::DisplayList>();
        const auto hash = CRC64("scenes/nonmq/Z2_00KEIKOKU/foliage_poc3/bed_top_material");
        bed->Instructions = { Command(uintptr_t(G_DL_OTR_HASH) << 24, 0), Command(hash >> 32, hash & 0xFFFFFFFF),
                              gsSPEndDisplayList() };
        const auto original = bed->Instructions;
        rm->archive->files[std::string("alt/") + path] = bed;
        rm->resources[std::string("alt/") + path] = bed;
        rm->alt = true;
        PlayState play{};
        play.sceneId = SCENE_00KEIKOKU;
        MMAutumnSceneFoliage_Update(&play);
        assert((bed->Instructions[0].words.w0 >> 24) == G_DL);
        const auto* wrapped = (const Gfx*)bed->Instructions[0].words.w1;
        assert((wrapped[0].words.w0 >> 24) == G_SETGRAYSCALE && wrapped[0].words.w1 == 1);
        assert(wrapped[1].words.w1 == 0xD99C45FF);
        assert((wrapped[2].words.w0 >> 24) == G_DL);
        const auto* preserved = (const Gfx*)wrapped[2].words.w1;
        for (size_t i = 0; i < original.size(); ++i)
            assert(SameCommand(preserved[i], original[i]));
        assert((wrapped[3].words.w0 >> 24) == G_SETGRAYSCALE && wrapped[3].words.w1 == 0);
        assert((wrapped[4].words.w0 >> 24) == G_ENDDL);
        MMAutumnSceneFoliage_Reset();
        assert(Equal(bed->Instructions, original));
        rm->alt = false;
        std::puts("PASS edited Alt bed keeps private child materials and exact geometry under a scoped seasonal color");
    }
    for (const auto* name : { "Z2_00KEIKOKUTex_034098", "Z2_00KEIKOKUTex_037098" }) {
        auto forest = List((std::string("alt/scenes/nonmq/Z2_00KEIKOKU/") + name).c_str());
        const auto original = forest->Instructions;
        const auto changed = BuildVariant(original);
        assert(!changed.empty() && Equal(RemoveTint(changed), original));
    }
    CheckForestDrawOwners();
    CheckActualMaterials();
    CheckPrivateBrushMaterials();
    auto rm = Ship::Context::GetRawInstance()->rm;
    constexpr const char* path = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_0153A8";
    auto native = List("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_021650");
    const auto original = native->Instructions;
    rm->archive->files[path] = native;
    rm->resources[path] = native; // Simulate a scene list already loaded for rendering.
    auto unused = List("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_021650");
    const auto unusedOriginal = unused->Instructions;
    rm->archive->files["scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_Unused"] = unused;
    PlayState play{};
    play.sceneId = SCENE_00KEIKOKU;
    MMAutumnSceneFoliage_Update(&play);
    CheckVariant(native, original);
    assert(rm->loads == 0 && Equal(unused->Instructions, unusedOriginal));
    // An unrelated feature patch made after activation must not be overwritten
    // or replayed from a stale copy. Only our first-command redirect is undone.
    native->Instructions[3].words.w1 = 0x80909055;
    MMAutumnSceneFoliage_Update(&play);
    assert(SameCommand(native->Instructions[0], original[0]));
    assert(native->Instructions[3].words.w1 == 0x80909055);
    native->Instructions[3] = original[3];
    MMAutumnSceneFoliage_Update(&play);
    CheckVariant(native, original);
    const unsigned firstLoads = rm->loads;
    for (unsigned i = 0; i < 100; ++i)
        MMAutumnSceneFoliage_Update(&play);
    assert(rm->loads == firstLoads);
    season = SEASON_OFF;
    MMAutumnSceneFoliage_Update(&play);
    assert(Equal(native->Instructions, original));
    std::puts("PASS selected foliage triangles only; exact source/alpha/geometry restoration; cached steady state");

    auto alt = List("alt/scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_021650");
    const auto altOriginal = alt->Instructions;
    // An unloaded Alt .meta must win over an already cached vanilla display list.
    rm->archive->files[std::string("alt/") + path + ".meta"] = alt;
    rm->alt = true;
    season = SEASON_AUTUMN;
    MMAutumnSceneFoliage_Update(&play);
    CheckVariant(alt, altOriginal);
    assert(Equal(native->Instructions, original));
    rm->alt = false;
    MMAutumnSceneFoliage_Update(&play);
    assert(Equal(alt->Instructions, altOriginal));
    CheckVariant(native, original);
    // Replacing a cache entry must restore the retained old resource instance.
    auto replacement = List("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_021650");
    const auto replacementOriginal = replacement->Instructions;
    rm->resources[path] = replacement;
    MMAutumnSceneFoliage_Update(&play);
    assert(Equal(native->Instructions, original));
    CheckVariant(replacement, replacementOriginal);
    play.sceneId = SCENE_TOWN;
    MMAutumnSceneFoliage_Update(&play);
    assert(Equal(replacement->Instructions, replacementOriginal));
    std::puts("PASS Alt cold load/toggle and resource-instance reload; excluded scene cleanup");

    auto wall = List("scenes/nonmq/Z2_21MITURINMAE/Z2_21MITURINMAETex_0055D0");
    const auto wallOriginal = wall->Instructions;
    rm->archive->files["scenes/nonmq/Z2_21MITURINMAE/Z2_21MITURINMAE_room_00DL_001C48"] = wall;
    rm->resources["scenes/nonmq/Z2_21MITURINMAE/Z2_21MITURINMAE_room_00DL_001C48"] = wall;
    play.sceneId = SCENE_21MITURINMAE;
    MMAutumnSceneFoliage_Update(&play);
    CheckVariant(wall, wallOriginal);
    MMAutumnSceneFoliage_Reset();
    assert(Equal(wall->Instructions, wallOriginal));
    season = -1; // Story, underwater and interior eligibility is provided by MMWeather.
    MMAutumnSceneFoliage_Update(&play);
    assert(Equal(wall->Instructions, wallOriginal));
    std::puts("PASS one Woodfall tree-wall material, reset and season-ineligible restoration");

    auto unsupported = wallOriginal;
    unsupported.insert(unsupported.begin(), Command(0x7F000000, 0));
    assert(BuildVariant(unsupported).empty());
    assert(BuildVariant({ Command(uintptr_t(G_SETTIMG_OTR_HASH) << 24, 0) }).empty());
    assert(BuildVariant({ gsSPEndDisplayList() }).empty());
    std::puts("PASS later material patches, unsupported extensions and truncated resources are preserved");
    season = SEASON_AUTUMN;
    MMAutumnSceneFoliage_Update(&play);
    assert(Ship::CrossRMRegistry::teardown != nullptr);
    Ship::CrossRMRegistry::teardown();
    assert(Equal(wall->Instructions, wallOriginal) && sVariants.empty());
    std::puts("PASS loaded-list scope and safe teardown cache release");
}
