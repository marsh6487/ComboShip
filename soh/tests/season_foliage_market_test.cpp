#include "soh/Enhancements/Graphics/SeasonFoliage.cpp"
#include "test_require.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include <iterator>

Ship::Context* Ship::Context::current = nullptr;
GameInteractor* GameInteractor::Instance = nullptr;
extern "C" { PlayState* gPlayState = nullptr; }
static int selectedSeason = 2;
extern "C" int32_t SeasonWeather_FoliageSeasonForPlay(const PlayState*) { return selectedSeason; }
extern "C" void gfx_texture_cache_clear() {}

static std::shared_ptr<Fast::Texture> Texture(const std::string& path) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = path;
    auto result = std::make_shared<Fast::Texture>(init);
    result->Type = Fast::TextureType::RGBA32bpp;
    result->Flags = TEX_FLAG_LOAD_AS_RAW;
    result->Width = result->Height = 16;
    result->ImageDataSize = 16 * 16 * 4;
    result->mImageBuffer = std::make_shared<std::vector<char>>(result->ImageDataSize);
    result->ImageData = reinterpret_cast<uint8_t*>(result->mImageBuffer->data());
    for (size_t i = 0; i < result->ImageDataSize; i += 4) {
        result->ImageData[i] = 40;
        result->ImageData[i + 1] = 140;
        result->ImageData[i + 2] = 20;
        result->ImageData[i + 3] = 83;
    }
    return result;
}

static uint32_t U32(const std::vector<char>& bytes, size_t offset) {
    REQUIRE(offset + 4 <= bytes.size());
    return uint8_t(bytes[offset]) | uint32_t(uint8_t(bytes[offset + 1])) << 8 |
           uint32_t(uint8_t(bytes[offset + 2])) << 16 | uint32_t(uint8_t(bytes[offset + 3])) << 24;
}

static void CheckArchiveTexture(const char* path, const char* file) {
    const Material* material = nullptr;
    for (const auto& entry : kMaterials) if (!std::strcmp(entry.path, path)) { material = &entry; break; }
    REQUIRE(material);
    std::ifstream input(file, std::ios::binary);
    REQUIRE(input.good());
    const std::vector<char> bytes{ std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
    REQUIRE(bytes.size() >= 80 && U32(bytes, 4) == 0x4F544558);
    const unsigned version = U32(bytes, 8);
    REQUIRE(version <= 1);
    const size_t offset = version ? 92 : 80;
    auto source = Texture(path);
    source->Type = static_cast<Fast::TextureType>(U32(bytes, 64));
    source->Width = U32(bytes, 68); source->Height = U32(bytes, 72);
    source->Flags = version ? U32(bytes, 76) : 0;
    source->ImageDataSize = U32(bytes, offset - 4);
    REQUIRE(bytes.size() == offset + source->ImageDataSize);
    source->mImageBuffer = std::make_shared<std::vector<char>>(bytes.begin() + offset, bytes.end());
    source->ImageData = reinterpret_cast<uint8_t*>(source->mImageBuffer->data());
    const auto original = *source->mImageBuffer;
    const bool packed = source->Type == Fast::TextureType::RGBA16bpp && source->Flags == 0;
    const size_t stride = packed ? 2 : 4;
    bool tinted = false;
    for (int season : { 0, 1, 2, 3 }) {
        const auto copy = BuildTexture(source, Palette(season, material->surface), material->region, material->pixelMask);
        REQUIRE(*source->mImageBuffer == original);
        if (season == 1) { REQUIRE(!copy); continue; }
        if (!copy) continue;
        tinted = true;
        for (size_t i = 0; i < source->ImageDataSize; i += stride) {
            const unsigned pixel = packed ? unsigned(source->ImageData[i]) << 8 | source->ImageData[i + 1] : 0;
            const unsigned r = packed ? pixel >> 11 : source->ImageData[i];
            const unsigned g = packed ? (pixel >> 6) & 31 : source->ImageData[i + 1];
            const unsigned b = packed ? (pixel >> 1) & 31 : source->ImageData[i + 2];
            if (g <= r || g <= b) REQUIRE(!std::memcmp(source->ImageData + i, copy->ImageData + i, stride));
            REQUIRE(packed ? (copy->ImageData[i + 1] & 1) == (source->ImageData[i + 1] & 1)
                           : copy->ImageData[i + 3] == source->ImageData[i + 3]);
        }
    }
    REQUIRE(tinted && "reviewed foliage region must affect its actual archive texture");
}

int main(int argc, char** argv) {
    struct Target { const char* path; int16_t scene; Surface surface; Region region; PixelMask mask; };
    const Target targets[] = { /* MARKET_TARGETS */ };
    Ship::Context context;
    context.rm = std::make_shared<Ship::ResourceManager>();
    Ship::Context::current = &context;
    size_t cases = 0;
    for (const auto& target : targets) {
        const Material* reviewed = nullptr;
        for (const auto& material : kMaterials)
            if (material.scene == target.scene && !std::strcmp(material.path, target.path)) { reviewed = &material; break; }
        REQUIRE(reviewed && reviewed->surface == target.surface && reviewed->pixelMask == target.mask);
        REQUIRE(reviewed->region.left == target.region.left && reviewed->region.top == target.region.top &&
                reviewed->region.right == target.region.right && reviewed->region.bottom == target.region.bottom);
        for (bool alt : { false, true }) {
            for (const char* prefix : { "", "alt/" }) {
                if (*prefix && !alt) continue;
                Retire();
                context.rm->cache.clear(); context.rm->alt = alt;
                std::string path = std::string(prefix) + target.path;
                auto source = Texture(path);
                const auto original = *source->mImageBuffer;
                context.rm->cache[path] = source;
                PlayState play{ target.scene };
                for (int season : { 0, 2, 3 }) {
                    selectedSeason = season;
                    Update(&play);
                    const auto copy = std::dynamic_pointer_cast<Fast::Texture>(context.rm->cache[path]);
                    REQUIRE(copy && copy != source);
                    REQUIRE(*source->mImageBuffer == original);
                    for (size_t i = 3; i < copy->ImageDataSize; i += 4) REQUIRE(copy->ImageData[i] == 83);
                    selectedSeason = 1; Update(&play);
                    REQUIRE(context.rm->cache[path] == source);
                }
                selectedSeason = 2;
                play.sceneNum = SCENE_HYRULE_FIELD; Update(&play);
                REQUIRE(context.rm->cache[path] == source && "suite materials must not leak to another scene");
                ++cases;
            }
        }
    }
    // Paint every atlas texel green so protected poster/masonry rectangles
    // cannot pass accidentally because their fixture colors were already gray.
    auto source = Texture("fixture/market-atlas");
    const auto props = BuildTexture(source, Palette(2, Surface::Understory), {}, PixelMask::MarketPropsFoliage);
    const auto bands = BuildTexture(source, Palette(3, Surface::Ground), {}, PixelMask::MarketGrassBands);
    REQUIRE(props && bands);
    for (size_t y = 0; y < 16; ++y) for (size_t x = 0; x < 16; ++x) {
        const size_t i = (y * 16 + x) * 4;
        const bool plant = (x < 8 && y < 6) || (x >= 2 && x < 14 && y >= 12);
        const bool grass = y == 2 || y == 15;
        REQUIRE((std::memcmp(source->ImageData + i, props->ImageData + i, 4) != 0) == plant);
        REQUIRE((std::memcmp(source->ImageData + i, bands->ImageData + i, 4) != 0) == grass);
    }
    Retire();
    std::cout << "PASS " << cases << " Market native/Alt cache bindings, all tint seasons, Summer restore, scene scope and atlas protection\n";
    REQUIRE(argc % 2 == 1);
    for (int i = 1; i < argc; i += 2) CheckArchiveTexture(argv[i], argv[i + 1]);
    if (argc > 1) std::cout << "PASS " << (argc - 1) / 2 << " actual Market archive textures: tint, source/alpha/petal preservation and Summer restoration\n";
}
