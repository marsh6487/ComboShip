#include "soh/Enhancements/Graphics/SeasonFoliage.cpp"
#include "test_require.h"
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

int main() {
    Ship::Context context;
    context.rm = std::make_shared<Ship::ResourceManager>();
    Ship::Context::current = &context;
    // Authored field terrain and shuffled actor greenery must resolve through
    // both resource namespaces without affecting colored reward markers.
    for (const char* target : { "scenes/shared/spot00_scene/s00_yuka_01_78B00___Copie",
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
            for (int selected : { 2, 3 }) {
                season = selected;
                Update(nullptr);
                REQUIRE(context.rm->cache[path] != source && "target greenery did not receive the selected tint");
                REQUIRE(context.rm->cache[marker] == reward);
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
    REQUIRE(context.rm->cache.at(grass) == ground);
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
