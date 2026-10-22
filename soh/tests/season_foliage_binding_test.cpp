#include "soh/Enhancements/Graphics/SeasonFoliage.cpp"
#include "test_require.h"
#include <cstring>
#include <iostream>

Ship::Context* Ship::Context::current = nullptr;
GameInteractor* GameInteractor::Instance = nullptr;
// z_play.c owns this C symbol. A C++ declaration in the included production
// source must fail here instead of passing an ELF-only fixture.
extern "C" {
PlayState* gPlayState = nullptr;
}
static int season = -1;
static size_t evicted = 0;
extern "C" int32_t SeasonWeather_SeasonForPlay(const PlayState*) {
    return season;
}
extern "C" void gfx_texture_cache_clear() {
    ++evicted;
}

static std::shared_ptr<Fast::Texture> Make(const std::string& path) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = path;
    auto source = std::make_shared<Fast::Texture>(init);
    source->Type = Fast::TextureType::RGBA16bpp;
    source->Flags = TEX_FLAG_LOAD_AS_RAW;
    source->Width = source->Height = 1;
    source->ImageDataSize = 4;
    source->mImageBuffer = std::make_shared<std::vector<char>>(std::initializer_list<char>{ 60, 110, 70, 83 });
    source->ImageData = reinterpret_cast<uint8_t*>(source->mImageBuffer->data());
    return source;
}

static void CheckPavingSeams(Ship::Context& context) {
    const char* roads[] = { "scenes/shared/spot00_scene/s00_road_01_41180",
                            "scenes/shared/spot00_scene/s00_road_02_46580" };
    const char* ground = "scenes/shared/spot00_scene/s00_yuka_01_78B00___Copie";
    for (bool packed : { false, true }) {
        for (bool alt : { false, true }) {
            context.rm->alt = alt;
            for (const char* prefix : { "", "alt/" }) {
                const auto source = Make("fixture/shared-paving");
                source->Width = 3;
                source->Flags = packed ? 0 : TEX_FLAG_LOAD_AS_RAW;
                // Grass, slightly green stone, and warm stone. Sharing this
                // allocation with ordinary grass also checks copy-cache isolation.
                const std::vector<uint8_t> pixels = packed
                    ? std::vector<uint8_t>{ 0x2B, 0x87, 0x7C, 0x1C, 0x5A, 0x91 }
                    : std::vector<uint8_t>{ 40, 110, 25, 83, 120, 130, 118, 255, 95, 85, 70, 0 };
                source->mImageBuffer = std::make_shared<std::vector<char>>(pixels.begin(), pixels.end());
                source->ImageData = reinterpret_cast<uint8_t*>(source->mImageBuffer->data());
                source->ImageDataSize = pixels.size();
                const auto original = *source->mImageBuffer;
                const size_t stride = packed ? 2 : 4;
                for (const char* road : roads)
                    context.rm->cache[std::string(prefix) + road] = source;
                context.rm->cache[std::string(prefix) + ground] = source;
                for (int selected : { 0, 2, 3, 1, 2, -1 }) {
                    season = selected;
                    Update(nullptr);
                    const bool tinted = (!*prefix || alt) && (selected == 0 || selected == 2 || selected == 3);
                    for (const char* road : roads) {
                        const auto shown = std::dynamic_pointer_cast<Fast::Texture>(
                            context.rm->cache.at(std::string(prefix) + road));
                        REQUIRE((shown != source) == tinted && "paving seam grass missed the seasonal binding");
                        REQUIRE((std::memcmp(shown->ImageData, source->ImageData, stride) != 0) == tinted);
                        REQUIRE(std::memcmp(shown->ImageData + stride, source->ImageData + stride, stride * 2) == 0 &&
                                "paving stones must retain their exact source pixels");
                        REQUIRE(shown->Type == source->Type && shown->Flags == source->Flags &&
                                shown->Width == source->Width && shown->Height == source->Height &&
                                shown->HByteScale == source->HByteScale && shown->VPixelScale == source->VPixelScale);
                        for (size_t i = 0; i < pixels.size(); i += stride)
                            REQUIRE(packed ? (shown->ImageData[i + 1] & 1) == (source->ImageData[i + 1] & 1)
                                           : shown->ImageData[i + 3] == source->ImageData[i + 3]);
                    }
                    if (tinted) {
                        const auto ordinary = std::dynamic_pointer_cast<Fast::Texture>(
                            context.rm->cache.at(std::string(prefix) + ground));
                        REQUIRE(std::memcmp(ordinary->ImageData + stride, source->ImageData + stride, stride) != 0 &&
                                "grass trim must not narrow ordinary terrain's existing selection");
                    }
                    REQUIRE(*source->mImageBuffer == original);
                }
                Retire();
                context.rm->cache.clear();
            }
        }
    }
}

int main() {
    Ship::Context context;
    context.rm = std::make_shared<Ship::ResourceManager>();
    Ship::Context::current = &context;
    CheckPavingSeams(context);
    // The private forest is shared scenery: its eight greenery identities must
    // keep MM's darker autumn palette and blush spring grass.
    const char* forestLeaves[] = { "lw_enkei_01_0", "lw_kusa_01_12600", "lw_wood_02_2CA00", "lw_wood_03_31E00",
                                   "lw_wood_04_37200" };
    const char* forestGround[] = { "lw_kabe_01_5400", "lw_yuka_02_39C00", "lw_yuka_03_44400" };
    for (bool alt : { false, true }) {
        context.rm->alt = alt;
        for (const char* prefix : { "", "alt/" }) {
            for (bool ground : { false, true }) {
                const auto check = [&](const char* name) {
                    const std::string path = std::string(prefix) + "custom/skiejer_forest/textures/" + name;
                    const auto original = Make(path);
                    const auto bytes = *original->mImageBuffer;
                    context.rm->cache[path] = original;
                    for (int selected : { 2, 3, 0, 1, 2, -1 }) {
                        season = selected;
                        Update(nullptr);
                        const bool tinted = (!*prefix || alt) && (selected == 2 || selected == 3 || selected == 0);
                        const auto shown = std::dynamic_pointer_cast<Fast::Texture>(context.rm->cache.at(path));
                        REQUIRE((shown != original) == tinted);
                        if (tinted) {
                            const uint32_t color = selected == 2   ? (ground ? 0xD99C45FFu : 0xB96848FFu)
                                                   : selected == 3 ? (ground ? 0xE5EDF3FFu : 0xCCDCEEFFu)
                                                   : ground        ? 0xF6C1D9FFu
                                                                   : 0xF6C1D9E0u;
                            const auto expected = BuildTexture(original, { color, selected == 2 ? 256u : 461u });
                            REQUIRE(expected && *shown->mImageBuffer == *expected->mImageBuffer);
                            REQUIRE(shown->ImageData[3] == original->ImageData[3]);
                        }
                        REQUIRE(*original->mImageBuffer == bytes);
                    }
                    Retire();
                    context.rm->cache.clear();
                };
                if (ground)
                    for (const char* name : forestGround)
                        check(name);
                else
                    for (const char* name : forestLeaves)
                        check(name);
            }
            for (const char* name : { "lw_kinoko_01_FC00", "lw_nenrin_01_27600", "lw_tutu_01_2A000", "portal_bark" }) {
                const std::string path = std::string(prefix) + "custom/skiejer_forest/textures/" + name;
                const auto original = Make(path);
                context.rm->cache[path] = original;
                for (int selected : { 0, 2, 3 }) {
                    season = selected;
                    Update(nullptr);
                    REQUIRE(context.rm->cache.at(path) == original);
                }
                Retire();
                context.rm->cache.clear();
            }
        }
    }
    context.rm->alt = true;
    // The same cached image can be reached by different authored atlas paths.
    // Recolor only each path's foliage region, even when stone/bark is greenish.
    const auto atlas = Make("fixture/shared-atlas");
    atlas->Width = atlas->Height = 16;
    atlas->ImageDataSize = 16 * 16 * 4;
    atlas->mImageBuffer = std::make_shared<std::vector<char>>(atlas->ImageDataSize);
    for (size_t at = 0; at < atlas->ImageDataSize; at += 4) {
        (*atlas->mImageBuffer)[at] = 60;
        (*atlas->mImageBuffer)[at + 1] = 110;
        (*atlas->mImageBuffer)[at + 2] = 70;
        (*atlas->mImageBuffer)[at + 3] = 83;
    }
    atlas->ImageData = reinterpret_cast<uint8_t*>(atlas->mImageBuffer->data());
    const auto atlasOriginal = *atlas->mImageBuffer;
    struct AtlasCase {
        const char* path;
        unsigned left, top, right, bottom;
    };
    const AtlasCase atlasCases[] = {
        { "scenes/shared/spot00_scene/s00_yama_01_76B00", 0, 8, 16, 16 },
        { "custom/prelude/spot00_scene/recipe252_tex0", 0, 8, 8, 16 },
        { "scenes/shared/spot00_scene/s00_ki_02_22000", 0, 0, 16, 12 },
        { "scenes/shared/spot00_scene/s00_ki_03_2C800", 0, 0, 16, 15 },
    };
    for (const char* prefix : { "", "alt/" }) {
        for (const auto& item : atlasCases)
            context.rm->cache[std::string(prefix) + item.path] = atlas;
        season = 3;
        Update(nullptr);
        for (const auto& item : atlasCases) {
            const auto copy =
                std::dynamic_pointer_cast<Fast::Texture>(context.rm->cache.at(std::string(prefix) + item.path));
            REQUIRE(copy && copy != atlas && "authored foliage region was not tinted");
            for (unsigned y = 0; y < 16; ++y)
                for (unsigned x = 0; x < 16; ++x) {
                    const size_t at = (y * 16 + x) * 4;
                    const bool foliage = x >= item.left && x < item.right && y >= item.top && y < item.bottom;
                    REQUIRE((std::memcmp(atlas->ImageData + at, copy->ImageData + at, 3) != 0) == foliage);
                    REQUIRE(copy->ImageData[at + 3] == 83);
                }
        }
        REQUIRE(*atlas->mImageBuffer == atlasOriginal);
        season = 1;
        Update(nullptr);
        for (const auto& item : atlasCases)
            REQUIRE(context.rm->cache.at(std::string(prefix) + item.path) == atlas);
        Retire();
        context.rm->cache.clear();
    }
    // Authored field terrain and shuffled actor greenery must resolve through
    // both resource namespaces without affecting colored reward markers.
    for (const char* target : { "scenes/shared/spot00_scene/s00_yuka_01_78B00___Copie",
                                "custom/prelude/spot00_scene/s00_yuka_04_85D00",
                                "scenes/shared/spot00_scene/s00_kabe_01_11300",
                                "scenes/shared/spot00_scene/s00_kabe_05_1C700",
                                "custom/prelude/spot00_scene/recipe222_tex0",
                                "custom/prelude/spot00_scene/recipe232_tex0",
                                "custom/prelude/spot00_scene/recipe242_tex0",
                                "custom/prelude/spot00_scene/recipe282_tex0",
                                "custom/prelude/spot00_scene/recipe292_tex0",
                                "custom/prelude/spot00_scene/recipe302_tex0",
                                "custom/prelude/spot00_scene/recipe312_tex0",
                                "custom/prelude/spot00_scene/recipe322_tex0",
                                "scenes/shared/spot00_scene/s00_yuka_04_85D00",
                                "custom/prelude/spot00_scene/recipe223_tex0",
                                "custom/prelude/spot00_scene/recipe233_tex0",
                                "custom/prelude/spot00_scene/recipe243_tex0",
                                "custom/prelude/spot00_scene/recipe283_tex0",
                                "custom/prelude/spot00_scene/recipe293_tex0",
                                "custom/prelude/spot00_scene/recipe303_tex0",
                                "custom/prelude/spot00_scene/recipe313_tex0",
                                "custom/prelude/spot00_scene/recipe323_tex0",
                                "custom/prelude/spot00_scene/recipe229_tex0",
                                "custom/prelude/spot00_scene/recipe239_tex0",
                                "custom/prelude/spot00_scene/recipe249_tex0",
                                "custom/prelude/spot00_scene/recipe281_tex0",
                                "custom/prelude/spot00_scene/recipe289_tex0",
                                "custom/prelude/spot00_scene/recipe299_tex0",
                                "custom/prelude/spot00_scene/recipe309_tex0",
                                "custom/prelude/spot00_scene/recipe319_tex0",
                                "custom/prelude/spot00_scene/recipe329_tex0",
                                "custom/prelude/spot00_scene/recipe221_tex0",
                                "custom/prelude/spot00_scene/recipe225_tex0",
                                "custom/prelude/spot00_scene/recipe235_tex0",
                                "custom/prelude/spot00_scene/recipe245_tex0",
                                "custom/prelude/spot00_scene/recipe285_tex0",
                                "custom/prelude/spot00_scene/recipe295_tex0",
                                "custom/prelude/spot00_scene/recipe305_tex0",
                                "custom/prelude/spot00_scene/recipe315_tex0",
                                "custom/prelude/spot00_scene/recipe325_tex0",
                                "custom/prelude/spot00_scene/recipe226_tex0",
                                "custom/prelude/spot00_scene/recipe236_tex0",
                                "custom/prelude/spot00_scene/recipe246_tex0",
                                "custom/prelude/spot00_scene/recipe286_tex0",
                                "custom/prelude/spot00_scene/recipe296_tex0",
                                "custom/prelude/spot00_scene/recipe306_tex0",
                                "custom/prelude/spot00_scene/recipe316_tex0",
                                "custom/prelude/spot00_scene/recipe326_tex0",
                                "scenes/shared/spot00_scene/s00_hana_02_5500",
                                "scenes/shared/spot04_scene/s04_02_en_01_0",
                                "scenes/shared/spot04_scene/s04_ie_01r_D100",
                                "scenes/shared/spot04_scene/s04_ie_01x_17900",
                                "scenes/shared/spot04_scene/s04_ie_01x_17900___Copie",
                                "scenes/shared/spot04_scene/s04_tuta_02_41800",
                                "scenes/shared/spot04_scene/s04_kusa_05_44900",
                                "scenes/shared/spot04_scene/s04_yuka_01r_46800___Copie",
                                "scenes/shared/spot04_scene/spot04_room_0Tex_011F08_rgba16_png",
                                "scenes/shared/spot04_scene/spot04_room_0Tex_01B090_rgba16_png",
                                "scenes/shared/spot04_scene/spot04_room_1Tex_0056A8_rgba16_png",
                                "scenes/shared/spot04_scene/spot04_sceneTex_00FE18_rgba16_png",
                                "scenes/shared/spot04_scene/spot04_sceneTex_010618_rgba16_png",
                                "scenes/shared/spot10_scene/s04_kusa_05_2A00",
                                "scenes/shared/spot10_scene/sp10_ki_02_2CF00",
                                "scenes/shared/spot10_scene/sp10_ki_02c_4E100",
                                "scenes/shared/spot10_scene/sp10_mon_01_3CB00",
                                "scenes/shared/spot10_scene/sp10_mon_02_3F500",
                                "objects/gameplay_field_keep/BushTex.rgba16",
                                "objects/gameplay_field_keep/LeafTex.rgba16",
                                "objects/gameplay_keep/gGrassBunchTex.rgba16",
                                "objects/gameplay_keep/LeafTex.rgba16" }) {
        for (const char* prefix : { "", "alt/" }) {
            const std::string path = std::string(prefix) + target;
            const auto source = Make(path);
            const auto original = *source->mImageBuffer;
            const std::string marker = "objects/gameplay_keep/reward_marker";
            const auto reward = Make(marker);
            context.rm->cache[path] = source;
            context.rm->cache[marker] = reward;
            const std::string pavers = "alt/scenes/shared/spot00_scene/s00_road_01_41180";
            const auto stone = Make(pavers);
            stone->ImageData[0] = 120;
            stone->ImageData[1] = 130;
            stone->ImageData[2] = 118;
            context.rm->cache[pavers] = stone;
            for (int selected : { 0, 2, 3 }) {
                season = selected;
                Update(nullptr);
                REQUIRE(context.rm->cache[path] != source && "target greenery did not receive the selected tint");
                REQUIRE(context.rm->cache[marker] == reward);
                REQUIRE(context.rm->cache[pavers] == stone && "greenish stone paving must stay native");
                REQUIRE(*source->mImageBuffer == original);
            }
            season = 1;
            Update(nullptr);
            REQUIRE(context.rm->cache[path] == source);
            Retire();
            context.rm->cache.clear();
        }
    }
    const std::string canopy = "alt/scenes/shared/spot04_scene/s04_ha_03_38800";
    const std::string grass = "alt/scenes/shared/spot04_scene/s04_yuka_01r_46800";
    const auto tree = Make(canopy), ground = Make(grass);
    context.rm->cache[canopy] = tree;
    context.rm->cache[grass] = ground;
    const auto originalTree = *tree->mImageBuffer, originalGround = *ground->mImageBuffer;
    season = 0;
    Update(nullptr);
    const auto pink = context.rm->cache.at(canopy);
    REQUIRE(pink != tree);
    REQUIRE(context.rm->cache.at(grass) != ground && "spring terrain must no longer match summer");
    const auto populated = context.rm->cache.size();
    season = 2;
    Update(nullptr);
    const auto amber = context.rm->cache.at(canopy);
    REQUIRE(amber != tree && amber != pink && context.rm->cache.at(grass) != ground);
    season = 3;
    Update(nullptr);
    REQUIRE(context.rm->cache.at(canopy) != amber);
    season = 2;
    Update(nullptr);
    REQUIRE(context.rm->cache.at(canopy) == amber && "re-entering autumn must reuse original-source copy");
    REQUIRE(context.rm->cache.size() == populated && "cold assets must not be loaded by tint");
    season = 1;
    Update(nullptr);
    REQUIRE(context.rm->cache.at(canopy) == tree && context.rm->cache.at(grass) == ground);
    season = 0;
    Update(nullptr);
    REQUIRE(context.rm->cache.at(canopy) == pink);
    context.rm->alt = false;
    Update(nullptr);
    REQUIRE(context.rm->cache.at(canopy) == tree && "Alt-off must restore its bindings");
    context.rm->alt = true;
    Update(nullptr);
    const auto replacement = Make(canopy);
    context.rm->cache[canopy] = replacement;
    Reset();
    REQUIRE(context.rm->cache.at(canopy) == replacement && "cleanup must respect external reload");
    Update(nullptr);
    REQUIRE(context.rm->cache.at(canopy) != replacement);
    season = -1;
    Update(nullptr);
    REQUIRE(context.rm->cache.at(canopy) == replacement);
    REQUIRE(*tree->mImageBuffer == originalTree && *ground->mImageBuffer == originalGround);
    // Scene travel unloads Alt allocations. Retirement must release the old
    // source/copy cohort after draw/texture-cache teardown, before the next scene.
    Retire();
    REQUIRE(sCopies.empty() && evicted > 0);
    for (int visit = 0; visit < 50; ++visit) {
        context.rm->cache.clear();
        context.rm->cache[canopy] = Make(canopy);
        season = 2;
        Update(nullptr);
        REQUIRE(sCopies.size() == 1);
        Retire();
        REQUIRE(sCopies.empty());
    }
    Ship::Context::current = nullptr;
    Update(nullptr);
    std::cout << "PASS seasonal foliage bindings: spring/ground, native/Alt, season transitions, cold assets, reload "
                 "guards, cleanup\n";
}
