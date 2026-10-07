// Execute the complete production OoT loader on the explicit real payload.
#include WOLF_IMPLEMENTATION
#include <cassert>
#include <iterator>
#include <sstream>
#include <spdlog/sinks/ostream_sink.h>

static std::string assetDirectory;
static std::vector<u8> resourceBlob;
namespace Ship {
std::string Context::LocateFileAcrossAppDirs(const std::string& path, const std::string&) {
    return std::filesystem::exists(path) ? path : std::string{};
}
} // namespace Ship
extern "C" {
const char* Nei_AssetDir(void) {
    return assetDirectory.c_str();
}
int32_t CVarGetInteger(const char*, int32_t fallback) {
    return fallback;
}
int OOT_CopyWolfLinkResource(uint8_t* destination, size_t capacity, size_t* size, const char** owner) {
    if (resourceBlob.empty())
        return 0;
    if (owner)
        *owner = "oot";
    *size = NeiWolfAsset::ResourcePayloadSize(resourceBlob.data(), resourceBlob.size());
    if (!*size || (destination && capacity < *size))
        return -1;
    if (destination)
        std::memcpy(destination, resourceBlob.data(), *size);
    return 1;
}
}

static bool load(const std::vector<u8>& blob) {
    sAssetsLoaded = 0;
    std::ofstream file(assetDirectory + "/wolf_link.bin", std::ios::binary);
    file.write((const char*)blob.data(), blob.size());
    file.close();
    return LoadAssets();
}
int main(int argc, char** argv) {
    assert(argc == 3);
    assetDirectory = argv[1];
    std::ostringstream logs;
    auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(logs);
    spdlog::set_default_logger(std::make_shared<spdlog::logger>("oot-real-wolf", sink));
    std::ifstream file(argv[2], std::ios::binary);
    const std::vector<u8> real{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    assert(load(real));
    const auto field = [&](u32 index) { return ReadU32(real.data() + 12 + index * 4); };
    assert(sSkin.vertexCount == field(0) && sSkin.boneCount == field(2) && sAnimations.size() == field(3));
    assert(sVertices[0].posX == ReadF32(real.data() + field(6)));
    assert(std::memcmp(sSkin.weights, real.data() + field(7), (size_t)field(0) * 8) == 0);
    assert(sAnimations[0].frames == (const SSBBBoneFrame*)(sBlob.data() + ReadU32(real.data() + field(11) + 12)));
    const u16 texture = ReadU16(real.data() + field(16));
    assert(sTexture[0] == (u16)((texture >> 8) | (texture << 8)));
    for (const char* name : kAnimNames)
        assert(FindAnim(name) >= 0);
    auto bad = real;
    for (u32 i = 0; i < field(3); ++i) {
        const u32 name = ReadU32(real.data() + field(11) + i * 16);
        if (std::strcmp((const char*)real.data() + name, kAnimNames[0]) == 0)
            bad[name] = 'X';
    }
    const s32 previousRegistration = sDefIndex;
    assert(!load(bad) && !sAssetsLoaded && sDefIndex == previousRegistration);
    assert(logs.str().find("reason=missing-animation") != std::string::npos);
    assert(load(real) && "the corrected OoT real asset must retry before cache poisoning");
    resourceBlob = real;
    resourceBlob.resize(real.size() + 16, 0);
    sAssetsLoaded = 0;
    assert(LoadAssets() && sBlob == real && sAssetPath == "@oot:objects/forms/wolf_link/gWolfLinkData");
    resourceBlob.back() = 1;
    sAssetsLoaded = 0;
    assert(!LoadAssets() && !sAssetsLoaded);
    resourceBlob.back() = 0;
    assert(LoadAssets());
    std::printf("PASS production OoT real Wolf v2 load: %u vertices, %u bones, %zu clips; texture/weights/clip mapping "
                "and rejected-asset retry\n",
                sSkin.vertexCount, sSkin.boneCount, sAnimations.size());
}
