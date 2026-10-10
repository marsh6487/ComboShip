// Exercise production transformation/lifecycle and extracted renderer handlers
// with real GBI/MM headers. Archive/cache services and GPU submission are seams.
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
struct ResourceInitData {
    std::string Path;
};
struct IResource {
    virtual ~IResource() = default;
    std::shared_ptr<ResourceInitData> initData;
    std::shared_ptr<ResourceInitData> GetInitData() {
        return initData;
    }
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
struct Archive {
    std::string GetPath() {
        return "fixture.o2r";
    }
};
struct ArchiveManager {
    std::map<std::string, std::shared_ptr<IResource>> files;
    std::string archiveLookup;
    bool HasFile(const std::string& path) {
        return files.contains(path);
    }
    std::shared_ptr<Archive> GetArchiveFromFile(const std::string& path) {
        archiveLookup = path;
        return HasFile(path) ? std::make_shared<Archive>() : nullptr;
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
    bool OtrSignatureCheck(const char* path);
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

static std::map<std::string, unsigned> sForestReports;
template <class... Args> static void ForestReport(const char* format, Args&&...) {
    ++sForestReports[format];
}
#define SPDLOG_INFO(...) ForestReport(__VA_ARGS__)
#define SPDLOG_WARN(...) ForestReport(__VA_ARGS__)
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
        if (op == G_SETTIMG_OTR_HASH || op == G_VTX_OTR_HASH || op == G_DL_OTR_HASH || op == G_MARKER ||
            op == G_MTX_OTR) {
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
        if (op == G_SETTIMG_OTR_HASH || op == G_VTX_OTR_HASH || op == G_DL_OTR_HASH || op == G_MARKER ||
            op == G_MTX_OTR)
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
#include "autumn_scene_renderer_fixture.h"
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
static void CheckActualGrassRenderer(const char* path, int sceneId, const std::vector<Gfx>& original,
                                     const std::vector<uint32_t>& expected,
                                     const std::vector<std::string>& texturePaths) {
    for (const auto& texture : texturePaths)
        ForestRendererFixture::manager.paths[CRC64(texture.c_str())] = texture;
    for (const bool alt : { false, true }) {
        ClearCache();
        auto rm = std::make_shared<Ship::ResourceManager>();
        Ship::Context::GetRawInstance()->rm = rm;
        auto resource = std::make_shared<Fast::DisplayList>();
        resource->Instructions = original;
        const std::string resourcePath = (alt ? "alt/" : "") + std::string(path);
        rm->archive->files[resourcePath] = resource;
        rm->resources[resourcePath] = resource;
        rm->alt = alt;
        ForestRendererFixture::manager.alt = alt;
        ForestRendererFixture::manager.refreshed = false;
        PlayState play{};
        play.sceneId = sceneId;
        for (const int phase : { SEASON_AUTUMN, SEASON_OFF, SEASON_AUTUMN, SEASON_OFF, SEASON_AUTUMN }) {
            season = phase;
            MMAutumnSceneFoliage_Update(&play);
            const auto stats = ForestRendererFixture::Draw(resource->Instructions);
            assert(stats.palettes == (phase == SEASON_AUTUMN ? expected : std::vector<uint32_t>(expected.size(), 0)));
        }
    }
    ClearCache();
    std::printf("PASS actual grass renderer %s: native/Alt tint survives rendered Off frames and pointer writebacks\n",
                path);
}

static std::vector<uint32_t> TrianglePalettes(const std::vector<Gfx>& commands) {
    std::vector<uint32_t> palettes;
    bool tint = false;
    uint32_t color = 0;
    for (size_t i = 0; i < commands.size(); ++i) {
        const auto op = commands[i].words.w0 >> 24;
        if (op == G_SETGRAYSCALE)
            tint = commands[i].words.w1 != 0;
        else if (op == G_SETINTENSITY)
            color = commands[i].words.w1;
        else if (op == G_TRI1 || op == G_TRI2)
            palettes.insert(palettes.end(), op == G_TRI2 ? 2 : 1, tint ? color : 0);
        if (op == G_SETTIMG_OTR_HASH || op == G_VTX_OTR_HASH || op == G_DL_OTR_HASH || op == G_MARKER ||
            op == G_MTX_OTR)
            ++i;
    }
    assert(!tint);
    return palettes;
}

static void CheckActualSceneMaterial(const char* path, int sceneId, const std::vector<Gfx>& original,
                                     const std::vector<uint32_t>& expected) {
    const bool selected = std::any_of(expected.begin(), expected.end(), [](uint32_t color) { return color != 0; });
    const auto transformed = BuildVariant(original);
    if (selected && transformed.empty())
        std::fprintf(stderr, "FAIL actual scene material %s: selected grass/forest remains untinted\n", path);
    assert(selected == !transformed.empty());
    if (selected) {
        assert(Equal(RemoveTint(transformed), original));
        assert(TrianglePalettes(transformed) == expected);
    }
    for (const auto* suffix : { "", "_scene" }) {
        for (const bool alt : { false, true }) {
            ClearCache();
            auto rm = std::make_shared<Ship::ResourceManager>();
            Ship::Context::GetRawInstance()->rm = rm;
            std::string resourcePath = path;
            resourcePath.insert(resourcePath.find('/', 13), suffix);
            if (alt)
                resourcePath.insert(0, "alt/");
            auto resource = std::make_shared<Fast::DisplayList>();
            resource->Instructions = original;
            rm->archive->files[resourcePath] = resource;
            rm->resources[resourcePath] = resource;
            rm->alt = alt;
            PlayState play{};
            play.sceneId = sceneId;
            season = SEASON_AUTUMN;
            MMAutumnSceneFoliage_Update(&play);
            if (selected) {
                assert(resource->Instructions[0].words.w0 >> 24 == G_DL);
                const auto& variant = sVariants.at(resource.get());
                if (ForestListColor(path))
                    CheckScopedForestCommands(variant, original);
                else
                    assert(TrianglePalettes(variant.commands) == expected);
            } else {
                assert(Equal(resource->Instructions, original) && sVariants.empty());
            }
            season = SEASON_OFF;
            MMAutumnSceneFoliage_Update(&play);
            assert(Equal(resource->Instructions, original));
            season = SEASON_AUTUMN;
            MMAutumnSceneFoliage_Update(&play);
            if (selected)
                assert(resource->Instructions[0].words.w0 >> 24 == G_DL);
            else
                assert(Equal(resource->Instructions, original));
        }
    }
    ClearCache();
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

static void CheckPrimaryGrassTile() {
    const auto grass = MakeMaterial("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01C650");
    const auto detail = MakeMaterial("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01F650");
    for (const bool grassPrimary : { true, false }) {
        std::vector<Gfx> original = { gsSPTexture(0xFFFF, 0xFFFF, 0, 0, G_ON) };
        const auto& primary = grassPrimary ? grass : detail;
        const auto& secondary = grassPrimary ? detail : grass;
        original.insert(original.end(), primary.begin(), primary.begin() + 2);
        original.push_back(gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0));
        original.push_back(gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 1023, 0));
        original.push_back(gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0));
        original.insert(original.end(), secondary.begin(), secondary.begin() + 2);
        original.push_back(gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 256, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0));
        original.push_back(gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 1023, 0));
        original.push_back(gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 256, 1, 0, 0, 0, 0, 0, 0, 0));
        original.push_back(gsSP1Triangle(0, 1, 2, 0));
        original.push_back(gsSPEndDisplayList());
        const auto variant = BuildVariant(original);
        if (grassPrimary) {
            assert(!variant.empty()); // The detail image cannot hide primary ground grass.
            assert(Equal(RemoveTint(variant), original));
            assert(std::count_if(variant.begin(), variant.end(), [](const Gfx& c) {
                       return c.words.w0 >> 24 == G_SETINTENSITY && c.words.w1 == 0xD99C45FF;
                   }) == 1);
        } else {
            assert(variant.empty()); // Grass in an unused secondary tile cannot tint primary dirt.
        }
    }
    std::puts("PASS primary grass render tile survives an unrelated detail load; secondary grass does not tint dirt");
}

static void CheckOtherSceneGrass() {
    for (const auto& [sceneId, name] :
         { std::pair{ SCENE_BACKTOWN, "Z2_BACKTOWN" }, std::pair{ SCENE_ALLEY, "Z2_ALLEY" },
           std::pair{ SCENE_ROMANYMAE, "Z2_ROMANYMAE" } }) {
        for (const auto* prefix : { "", "alt/" }) {
            ClearCache();
            auto rm = std::make_shared<Ship::ResourceManager>();
            Ship::Context::GetRawInstance()->rm = rm;
            const auto path = std::string(prefix) + "scenes/nonmq/" + name + "/" + name + "_room_00DL_000100";
            // An edited scene may reuse an already-registered texture from another scene.
            auto resource = List("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01C650");
            const auto original = resource->Instructions;
            rm->archive->files[path] = resource;
            rm->resources[path] = resource;
            rm->alt = std::string(prefix) == "alt/";
            PlayState play{};
            play.sceneId = sceneId;
            season = SEASON_AUTUMN;
            MMAutumnSceneFoliage_Update(&play);
            assert(resource->Instructions[0].words.w0 >> 24 == G_DL);
            season = SEASON_OFF;
            MMAutumnSceneFoliage_Update(&play);
            assert(Equal(resource->Instructions, original));
            season = SEASON_AUTUMN;
            MMAutumnSceneFoliage_Update(&play);
            assert(resource->Instructions[0].words.w0 >> 24 == G_DL);
            for (const int invalid : { -1, int(SCENE_UNSET_01), 32767 }) {
                play.sceneId = invalid;
                MMAutumnSceneFoliage_Update(&play);
                assert(Equal(resource->Instructions, original));
            }
        }
    }
    ClearCache();
    std::puts("PASS shared grass in North Town, Laundry Pool and Milk Road; native/Alt, Off and invalid scenes");
}

static void CheckRendererTextureSlots() {
    const auto grass = MakeMaterial("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01C650");
    const auto dirt = MakeMaterial("scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01F650");
    std::vector<Gfx> original = { gsSPTexture(0xFFFF, 0xFFFF, 0, 3, G_ON) };
    original.insert(original.end(), grass.begin(), grass.begin() + 2);
    original.push_back(gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 64, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0));
    original.push_back(gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 1023, 0));
    // The production renderer puts every nonzero TMEM address in texture slot 1.
    original.push_back(gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 128, 3, 0, 0, 0, 0, 0, 0, 0));
    original.push_back(gsSP1Triangle(0, 1, 2, 0));
    original.insert(original.end(), dirt.begin(), dirt.begin() + 2);
    original.push_back(gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 256, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0));
    original.push_back(gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 1023, 0));
    original.push_back(gsSP1Triangle(0, 1, 2, 0));
    original.push_back(gsSPEndDisplayList());
    const auto variant = BuildVariant(original);
    assert(!variant.empty() && Equal(RemoveTint(variant), original));
    assert(CountMaterialTriangles(variant, "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01C650", 0xD99C45FF) == 1);
    assert(CountMaterialTriangles(variant, "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_01F650", 0xD99C45FF) == 0);
    std::puts("PASS renderer texture slots, nonzero render tile and detail-slot replacement");
}

int main() {
    CheckPrimaryGrassTile();
    CheckOtherSceneGrass();
    CheckRendererTextureSlots();
    for (const auto* path :
         { "scenes/nonmq/Z2_BACKTOWN/Z2_BACKTOWNTex_005A40", "scenes/nonmq/Z2_ALLEY/Z2_ALLEYTex_0050E0",
           "scenes/nonmq/Z2_ROMANYMAE/Z2_ROMANYMAETex_001EB0", "scenes/nonmq/Z2_F01/Z2_F01Tex_01BA00",
           "misc/scene_texture_08/scene_texture_08_Tex_004800" }) {
        assert(MaterialColor(CRC64(path)) == 0xD99C45FF);
        assert(MaterialColor(CRC64((std::string("alt/") + path).c_str())) == 0xD99C45FF);
    }
    assert(MaterialColor(CRC64("scenes/nonmq/Z2_BACKTOWN/Z2_BACKTOWNTex_007A40")) == 0xB96848FF);
    for (const auto* path :
         { "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_02B050", "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_02C850",
           "scenes/nonmq/Z2_BACKTOWN/Z2_BACKTOWNTex_00C5E0" })
        assert(MaterialColor(CRC64(path)) == 0); // Water and the accepted matching dirt stay unchanged.
    // These exact decoded duplicates are cracked masonry with moss, not grass.
    // Keep them unselected even when an eligible scene reuses the material.
    for (const auto* path :
         { "scenes/nonmq/Z2_DANPEI/Z2_DANPEI_room_00Tex_009708", "scenes/nonmq/Z2_DANPEI/Z2_DANPEI_room_03Tex_008AB8",
           "scenes/nonmq/Z2_DANPEI/Z2_DANPEI_room_04Tex_004CB0", "scenes/nonmq/Z2_DANPEI/Z2_DANPEI_room_05Tex_009670",
           "scenes/nonmq/Z2_DANPEI2TEST/Z2_DANPEI2TESTTex_005210" }) {
        std::string alias(path);
        alias.insert(alias.find('/', 13), "_scene");
        for (const auto& name : { std::string(path), "alt/" + std::string(path), alias, "alt/" + alias })
            assert(MaterialColor(CRC64(name.c_str())) == 0);
    }
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
    CheckRenderedForestTransitions();
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
