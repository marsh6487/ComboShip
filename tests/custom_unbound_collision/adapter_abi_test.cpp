#include "global.h"
#include "combo/BlackMarketGranny.h"
#include <cassert>
#include <cstddef>
#include <cstdio>

struct GrannyResourcesFixture {
    std::vector<std::array<uint8_t, 22>> lights;
    const auto& Lights() const { return lights; }
} sGrannyResources;
static std::vector<EnvLightSettings> sGrannyLights;
#include "granny_light_apply.inc"

static void TestLegacyLights() {
    static PlayState play{};
    sGrannyResources.lights.resize(2);
    for (size_t i = 0; i < sGrannyResources.lights.size(); ++i)
        for (size_t byte = 0; byte < 22; ++byte)
            sGrannyResources.lights[i][byte] = static_cast<uint8_t>(i * 53 + byte * 7);
    sGrannyLights.resize(2);
    for (auto& light : sGrannyLights) {
        light.worldFog = 1;
        light.fogStart = light.fogEnd = light.drawDistance = light.nearPlane = 99999.0f;
    }
    ApplyProductionGrannyLights(&play);
    assert(play.envCtx.numLightSettings == 2 && play.envCtx.lightSettingsList == sGrannyLights.data());
    for (size_t i = 0; i < 2; ++i) {
        const auto& light = sGrannyLights[i];
        assert(std::memcmp(&light, sGrannyResources.lights[i].data(), 22) == 0);
        assert(!light.worldFog && !light.fogStart && !light.fogEnd && !light.drawDistance && !light.nearPlane);
    }
    assert(std::memcmp(&play.envCtx.lightSettings, sGrannyLights.data(), sizeof(EnvLightSettings)) == 0);
}

static void TestVaporBounds() {
    using namespace BlackMarketGranny;
    std::array<Gfx, 5> root{};
    std::array<Gfx, 45> leaf{};
    std::array<Vtx, 56> vertices{};
    root[0].words.w0 = 0x33000000;
    root[2].words.w0 = 0x31000000;
    root[3].words = { 0x9db10eea, 0x0253d372 };
    root[4].words.w0 = 0xdf000000;
    leaf[4].words = { 0x32008010, 0x300 };
    leaf[28].words = { 0x32020040, 0 };
    leaf[38].words = { 0x32010020, 0x200 };
    leaf[44].words.w0 = 0xdf000000;
    for (int command : { 5, 29, 39 })
        leaf[command].words = { 0xfc4b43fe, 0xf8565a38 };
    for (int quad = 0; quad < 12; ++quad) {
        const int first = quad < 8 ? quad * 4 : (quad - 8) * 4;
        const int command = quad < 8 ? 30 + quad : 40 + quad - 8;
        leaf[command].words = { (6u << 24) | ((first * 2) << 16) | ((first + 1) * 2 << 8) | ((first + 2) * 2),
                                static_cast<uintptr_t>(((first * 2) << 16) | ((first + 2) * 2 << 8) | ((first + 3) * 2)) };
    }
    constexpr int bounds[6][6] = { { 71, 37, 42, 98, 200, 45 }, { 45, 37, 47, 73, 200, 48 },
                                    { 59, 37, 66, 86, 200, 67 }, { -104, 51, 43, -79, 218, 43 },
                                    { -117, 51, 24, -91, 218, 25 }, { -93, 51, 20, -68, 218, 22 } };
    for (int quad = 0; quad < 6; ++quad)
        for (int vertex = 0; vertex < 4; ++vertex)
            for (int axis = 0; axis < 3; ++axis)
                vertices[24 + quad * 4 + vertex].v.ob[axis] = bounds[quad][axis + (vertex & 1 ? 3 : 0)];
    const auto originalRoot = root;
    const auto originalLeaf = leaf;
    const auto originalVertices = vertices;
    const auto owner = std::make_shared<int>(1);
    Vapor vapor;
    assert(vapor.Load({ root.data(), root.size(), owner }, { leaf.data(), leaf.size(), owner },
                      { vertices.data(), vertices.size(), owner }));
    const auto* copy = reinterpret_cast<const Gfx*>(vapor.Root()[2].words.w1);
    for (int command = 0; command < 45; ++command) {
        if (command == 36 || command == 37 || (command >= 40 && command <= 43))
            assert(copy[command].words.w0 == static_cast<uintptr_t>(G_SPNOOP) << 24 && copy[command].words.w1 == 0);
        else
            assert(std::memcmp(&copy[command], &leaf[command], sizeof(Gfx)) == 0);
    }
    assert(std::memcmp(root.data(), originalRoot.data(), sizeof(root)) == 0);
    assert(std::memcmp(leaf.data(), originalLeaf.data(), sizeof(leaf)) == 0);
    assert(std::memcmp(vertices.data(), originalVertices.data(), sizeof(vertices)) == 0);
    vertices[24].v.ob[0] = 65607; // Would alias the authored 71 if narrowed back to s16.
    assert(!vapor.Load({ root.data(), root.size(), owner }, { leaf.data(), leaf.size(), owner },
                       { vertices.data(), vertices.size(), owner }));
    assert(!vapor.Root());
}

int main() {
    TestLegacyLights();
    TestVaporBounds();
    std::puts("PASS ABI adapters: legacy light bytes/defaults, native wide vapor bounds and source immutability");
}
