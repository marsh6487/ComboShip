#include "soh/Enhancements/Graphics/SeasonFoliage.cpp"
#include "test_require.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include <iterator>

Ship::Context* Ship::Context::current = nullptr;
GameInteractor* GameInteractor::Instance = nullptr;
extern "C" {
PlayState* gPlayState = nullptr;
}
static int selectedSeason = -1;
extern "C" int32_t SeasonWeather_FoliageSeasonForPlay(const PlayState*) {
    return selectedSeason;
}
extern "C" void gfx_texture_cache_clear() {
}

static auto Texture(Fast::TextureType type = Fast::TextureType::RGBA32bpp, unsigned width = 32, unsigned height = 16,
                    unsigned flags = 0) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = "replacement/any-stone-color";
    auto image = std::make_shared<Fast::Texture>(init);
    image->Type = type;
    image->Flags = flags;
    image->Width = width;
    image->Height = height;
    image->HByteScale = 8;
    image->VPixelScale = 4;
    const bool indexed = type == Fast::TextureType::Palette4bpp || type == Fast::TextureType::Palette8bpp;
    const bool decoded = (flags & TEX_FLAG_LOAD_AS_IMG) || ((flags & TEX_FLAG_LOAD_AS_RAW) && !indexed);
    unsigned bits = decoded || type == Fast::TextureType::RGBA32bpp                                          ? 32
                    : type == Fast::TextureType::RGBA16bpp || type == Fast::TextureType::GrayscaleAlpha16bpp ? 16
                    : type == Fast::TextureType::Palette4bpp || type == Fast::TextureType::Grayscale4bpp ||
                            type == Fast::TextureType::GrayscaleAlpha4bpp
                        ? 4
                        : 8;
    image->ImageDataSize = (width * height * bits + 7) / 8;
    image->mImageBuffer = std::make_shared<std::vector<char>>(image->ImageDataSize);
    image->ImageData = reinterpret_cast<uint8_t*>(image->mImageBuffer->data());
    for (size_t i = 0; i < image->ImageDataSize; ++i)
        image->ImageData[i] = (i * 79 + 13) % 256;
    return image;
}

static void Formats() {
    for (auto type : { Fast::TextureType::RGBA32bpp, Fast::TextureType::RGBA16bpp, Fast::TextureType::Grayscale4bpp,
                       Fast::TextureType::Grayscale8bpp, Fast::TextureType::GrayscaleAlpha4bpp,
                       Fast::TextureType::GrayscaleAlpha8bpp, Fast::TextureType::GrayscaleAlpha16bpp,
                       Fast::TextureType::Palette4bpp, Fast::TextureType::Palette8bpp }) {
        for (unsigned flags : { 0u, unsigned(TEX_FLAG_LOAD_AS_RAW), unsigned(TEX_FLAG_LOAD_AS_IMG),
                                unsigned(TEX_FLAG_LOAD_AS_RAW | TEX_FLAG_LOAD_AS_IMG) }) {
            auto source = Texture(type, 64, 32, flags);
            auto before = *source->mImageBuffer;
            auto output = BuildWinterTerrainTexture(source);
            REQUIRE(output && output != source && output->NeutralizeShading && !source->NeutralizeShading);
            REQUIRE(output->GetInitData() == source->GetInitData());
            REQUIRE(output->Type == source->Type && output->Flags == flags && output->Width == 64 &&
                    output->Height == 32);
            REQUIRE(output->HByteScale == 8 && output->VPixelScale == 4 && *source->mImageBuffer == before);
            for (size_t i = 0; i < output->ImageDataSize; ++i) {
                const unsigned old = source->ImageData[i], now = output->ImageData[i];
                const bool indexed = type == Fast::TextureType::Palette4bpp || type == Fast::TextureType::Palette8bpp;
                if ((flags & TEX_FLAG_LOAD_AS_IMG) || ((flags & TEX_FLAG_LOAD_AS_RAW) && !indexed) ||
                    type == Fast::TextureType::RGBA32bpp) {
                    if (i % 4 == 3)
                        REQUIRE(old == now);
                    else
                        REQUIRE(now >= 168);
                } else if (type == Fast::TextureType::RGBA16bpp) {
                    if (i % 2)
                        REQUIRE((old & 1) == (now & 1));
                } else if (type == Fast::TextureType::GrayscaleAlpha16bpp) {
                    if (i % 2)
                        REQUIRE(old == now);
                    else
                        REQUIRE(now >= 168);
                } else if (type == Fast::TextureType::GrayscaleAlpha8bpp) {
                    REQUIRE((old & 15) == (now & 15));
                } else if (type == Fast::TextureType::GrayscaleAlpha4bpp) {
                    REQUIRE((old & 0x11) == (now & 0x11));
                } else if (type == Fast::TextureType::Palette4bpp || type == Fast::TextureType::Palette8bpp) {
                    REQUIRE(old == now); // Indices are not RGB: the dedicated TLUT supplies white.
                }
            }
        }
    }
    auto allPacked = Texture(Fast::TextureType::RGBA16bpp, 256, 256);
    for (unsigned value = 0; value < 65536; ++value) {
        allPacked->ImageData[value * 2] = value >> 8;
        allPacked->ImageData[value * 2 + 1] = value;
    }
    auto snow = BuildWinterTerrainTexture(allPacked);
    for (unsigned value = 0; value < 65536; ++value) {
        unsigned out = (unsigned(snow->ImageData[value * 2]) << 8) | snow->ImageData[value * 2 + 1];
        REQUIRE((out & 1) == (value & 1));
        REQUIRE((out >> 11) == ((out >> 6) & 31) && (out >> 11) == ((out >> 1) & 31));
        REQUIRE((out >> 11) >= 20);
    }
    REQUIRE(SnowBrightness(0, 255) == 168 && SnowBrightness(255, 255) == 255);
    for (unsigned value = 1; value <= 255; ++value)
        REQUIRE(SnowBrightness(value, 255) >= SnowBrightness(value - 1, 255));
    REQUIRE(SnowBrightness(40, 255) < SnowBrightness(160, 255));
    auto atlas = Texture();
    auto masked = BuildWinterTerrainTexture(atlas, { 0, 0, 8, 16 });
    REQUIRE(masked && !masked->NeutralizeShading);
    for (unsigned y = 0; y < atlas->Height; ++y)
        for (unsigned x = atlas->Width / 2; x < atlas->Width; ++x) {
            const size_t offset = (y * atlas->Width + x) * 4;
            REQUIRE(std::memcmp(atlas->ImageData + offset, masked->ImageData + offset, 4) == 0);
        }
    auto malformed = Texture();
    malformed->ImageDataSize--;
    REQUIRE(!BuildWinterTerrainTexture(malformed));
}

static void RawIndexed() {
    // ImportTextureRaw dispatches indexed types to CI importers. These remain
    // packed indices even though RAW non-indexed assets contain RGBA bytes.
    for (auto type : { Fast::TextureType::Palette4bpp, Fast::TextureType::Palette8bpp }) {
        const unsigned bits = type == Fast::TextureType::Palette4bpp ? 4 : 8;
        auto indices = Texture(type, 32, 32, TEX_FLAG_LOAD_AS_RAW);
        REQUIRE(indices->ImageDataSize == 32 * 32 * bits / 8);
        auto before = *indices->mImageBuffer;
        auto snow = BuildWinterTerrainTexture(indices);
        REQUIRE(snow && snow->NeutralizeShading && snow->Flags == TEX_FLAG_LOAD_AS_RAW);
        REQUIRE(snow->Type == type && snow->ImageDataSize == indices->ImageDataSize);
        REQUIRE(*snow->mImageBuffer == before && *indices->mImageBuffer == before);
        REQUIRE(snow->HByteScale == indices->HByteScale && snow->VPixelScale == indices->VPixelScale);
        // The dedicated palette, not the index buffer, owns snow's RGB values.
        auto palette = Texture(Fast::TextureType::RGBA16bpp, bits == 4 ? 16 : 256, 1);
        auto tintedPalette = BuildWinterTerrainTexture(palette);
        REQUIRE(tintedPalette && *tintedPalette->mImageBuffer != *palette->mImageBuffer);
        for (size_t at = 0; at < palette->ImageDataSize; at += 2) {
            const unsigned value = (unsigned(tintedPalette->ImageData[at]) << 8) | tintedPalette->ImageData[at + 1];
            REQUIRE((value >> 11) == ((value >> 6) & 31) && (value >> 11) == ((value >> 1) & 31));
            REQUIRE((value & 1) == (palette->ImageData[at + 1] & 1));
        }
        // IMG takes precedence over RAW in ImportTexture and is decoded RGBA,
        // including when an imported image retains an indexed source type tag.
        auto decoded = Texture(type, 32, 32, TEX_FLAG_LOAD_AS_RAW | TEX_FLAG_LOAD_AS_IMG);
        REQUIRE(decoded->ImageDataSize == 32 * 32 * 4);
        auto decodedSnow = BuildWinterTerrainTexture(decoded);
        REQUIRE(decodedSnow && *decodedSnow->mImageBuffer != *decoded->mImageBuffer);
        // A RAW indexed resource with an RGBA-sized payload is not a valid CI
        // resource; do not guess its encoding from the byte count.
        decoded->Flags = TEX_FLAG_LOAD_AS_RAW;
        REQUIRE(!BuildWinterTerrainTexture(decoded));
    }
}

static void Cohorts() {
    Ship::Context context;
    context.rm = std::make_shared<Ship::ResourceManager>();
    Ship::Context::current = &context;
    unsigned cases = 0;
    for (bool alt : { false, true }) {
        context.rm->alt = alt;
        for (const auto& material : kWinterTerrainMaterials) {
            for (const char* prefix : { "", "alt/" }) {
                if (*prefix && !alt)
                    continue;
                Retire();
                context.rm->cache.clear();
                const std::string path = std::string(prefix) + material.path;
                auto original = Texture();
                const auto pixels = *original->mImageBuffer;
                context.rm->cache[path] = original;
                const std::string protectedPaths[] = { "scenes/shared/spot09_scene/spot09_sceneTex_00E160", // river
                                                       "scenes/shared/spot09_scene/spot09_sceneTex_00F160", // waterfall
                                                       "scenes/shared/spot16_scene/s16_mon_01_3AB80",       // door
                                                       "scenes/shared/spot12_scene/s12_tent_01_1AE80",
                                                       "objects/object_link_child/green_tunic",
                                                       "textures/icon_item_static/ui" };
                for (const auto& protectedPath : protectedPaths)
                    context.rm->cache[protectedPath] = original;
                PlayState play{};
                play.sceneNum = material.scene;
                for (int season : { 3, 3, 1, -1, 2, 0, 3 }) {
                    selectedSeason = season;
                    Update(&play);
                    auto current = std::dynamic_pointer_cast<Fast::Texture>(context.rm->cache[path]);
                    REQUIRE(current && *original->mImageBuffer == pixels);
                    if (season == 3) {
                        REQUIRE(current != original);
                        REQUIRE(current->NeutralizeShading ==
                                (material.region.left == 0 && material.region.top == 0 && material.region.right == 16 &&
                                 material.region.bottom == 16));
                    } else if (std::string_view(material.path).ends_with("s16_yama_03_42480") &&
                               (season == 0 || season == 2)) {
                        const auto expected = BuildTexture(original, Palette(season, Surface::Ground));
                        REQUIRE(expected && *expected->mImageBuffer == *current->mImageBuffer &&
                                !current->NeutralizeShading);
                    } else
                        REQUIRE(current == original);
                    for (const auto& protectedPath : protectedPaths)
                        REQUIRE(context.rm->cache[protectedPath] == original);
                }
                auto replacement = Texture(Fast::TextureType::RGBA16bpp, 128, 64, TEX_FLAG_LOAD_AS_RAW);
                context.rm->cache[path] = replacement;
                selectedSeason = 3;
                Update(&play);
                auto current = std::dynamic_pointer_cast<Fast::Texture>(context.rm->cache[path]);
                REQUIRE(current != replacement && current->Width == 128 && current->Height == 64);
                play.sceneNum = 0x51;
                Update(&play);
                if (std::string_view(material.path).ends_with("s16_yama_03_42480")) {
                    const auto expected = BuildTexture(replacement, Palette(3, Surface::Ground));
                    const auto restored = std::dynamic_pointer_cast<Fast::Texture>(context.rm->cache[path]);
                    REQUIRE(expected && *expected->mImageBuffer == *restored->mImageBuffer &&
                            !restored->NeutralizeShading);
                } else
                    REQUIRE(context.rm->cache[path] ==
                            replacement); // Restore the latest source / accepted foliage role.
                ++cases;
            }
        }
    }
    Retire();
    Ship::Context::current = nullptr;
    std::cout << "PASS winter terrain " << cases
              << " native/Alt material cohorts, scene/season isolation, reload/"
                 "restoration, all formats, 65536 native colors and source/alpha/detail preservation\n";
}
static void Archive(const char* path) {
    std::ifstream file(path, std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(file)), {});
    REQUIRE(bytes.size() >= 80 && std::string_view(bytes.data() + 4, 4) == "XETO");
    const auto word = [&](size_t offset) {
        uint32_t value = 0;
        for (unsigned i = 0; i < 4; ++i)
            value |= uint32_t(uint8_t(bytes.at(offset + i))) << (i * 8);
        return value;
    };
    auto source = Texture();
    const bool v1 = word(8) == 1;
    source->Type = Fast::TextureType(word(64));
    source->Width = word(68);
    source->Height = word(72);
    source->Flags = v1 ? word(76) : 0;
    source->ImageDataSize = word(v1 ? 88 : 76);
    const size_t header = v1 ? 92 : 80;
    REQUIRE(bytes.size() == header + source->ImageDataSize);
    source->mImageBuffer = std::make_shared<std::vector<char>>(bytes.begin() + header, bytes.end());
    source->ImageData = reinterpret_cast<uint8_t*>(source->mImageBuffer->data());
    auto snow = BuildWinterTerrainTexture(source);
    REQUIRE(snow && snow->NeutralizeShading && !source->NeutralizeShading);
    REQUIRE(snow->Width == source->Width && snow->Height == source->Height && snow->Type == source->Type &&
            snow->Flags == source->Flags && snow->GetInitData() == source->GetInitData());
    REQUIRE(std::equal(bytes.begin() + header, bytes.end(), source->mImageBuffer->begin()));
}
int main(int argc, char** argv) {
    Formats();
    RawIndexed();
    Cohorts();
    for (int i = 1; i < argc; ++i)
        Archive(argv[i]);
    if (argc > 1)
        std::cout << "PASS " << argc - 1 << " actual native/Djipi serialized terrain textures\n";
}
