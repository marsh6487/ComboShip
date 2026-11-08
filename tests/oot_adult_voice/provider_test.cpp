#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>
namespace Ship {
struct File { std::shared_ptr<std::vector<char>> Buffer; };
struct Archive {
    std::string path = "/native/oot.o2r";
    const std::string& GetPath() { return path; }
    std::string root;
    std::shared_ptr<File> LoadFile(const std::string& path) {
        std::ifstream input(root + "/" + path, std::ios::binary);
        if (!input) return nullptr;
        auto file = std::make_shared<File>();
        file->Buffer = std::make_shared<std::vector<char>>(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
        return file;
    }
};
struct ArchiveManager {
    std::shared_ptr<std::vector<std::shared_ptr<Archive>>> list = std::make_shared<std::vector<std::shared_ptr<Archive>>>();
    auto GetArchives() { return list; }
};
struct ResourceManager {
    std::shared_ptr<Archive> base = std::make_shared<Archive>();
    std::shared_ptr<ArchiveManager> archive = std::make_shared<ArchiveManager>();
    ResourceManager() { archive->list->push_back(base); }
    std::shared_ptr<ArchiveManager> GetArchiveManager() { return archive; }
};
struct Context {
    static inline Context* active = nullptr;
    std::shared_ptr<ResourceManager> owner;
    static Context* GetRawInstance() { return active; }
    auto GetResourceManager() { return owner; }
};
struct CrossRMRegistry {
    static inline std::shared_ptr<ResourceManager> oot;
    static std::shared_ptr<ResourceManager> Get(const std::string& name) { assert(name == "oot"); return oot; }
};
}
#include "provider.inc"
#include <cstring>
static std::vector<uint8_t> referenceEncoded;
static std::vector<int16_t> referenceDecoded;
struct RefState { int in = 0, out = 0, nbytes = 0; int16_t adpcm_table[16][2][8]; int16_t* adpcm_loop_state; } rspa;
using ADPCM_STATE = int16_t*;
#define BUF_U8(x) (referenceEncoded.data())
#define BUF_S16(x) (referenceDecoded.data())
#define ROUND_UP_32(x) (((x) + 31) & ~31)
#define A_INIT 1
#define A_LOOP 2
static int16_t clamp16(int32_t x) { return static_cast<int16_t>(std::clamp(x, -32768, 32767)); }
#include "native_decoder.inc"
static void CompareNative(const std::vector<char>& bytes) {
    Reader r{bytes}; r.U(4); unsigned size = r.U(4);
    referenceEncoded.assign(bytes.begin() + r.pos, bytes.begin() + r.pos + size);
    referenceEncoded.resize(((size + 8) / 9) * 9, 0);
    r.pos += size; r.U(4); unsigned end = r.U(4); r.U(4);
    unsigned state = r.U(4); for (unsigned i = 0; i < state; ++i) r.U(2);
    r.U(4); r.U(4); unsigned count = r.U(4);
    auto* table = &rspa.adpcm_table[0][0][0];
    for (unsigned i = 0; i < count; ++i) table[i] = static_cast<int16_t>(r.U(2));
    rspa.nbytes = ((size + 8) / 9) * 32;
    referenceDecoded.assign(rspa.nbytes / 2 + 16, 0);
    int16_t history[16] = {};
    aADPCMdecImpl(A_INIT, history);
    const auto pcm = Decode(bytes);
    assert(pcm.size() == (end * 8 + 4) / 5);
    for (size_t i = 0; i < pcm.size(); ++i) {
        size_t numerator = i * 5, lo = std::min(numerator / 8, size_t(end - 1));
        size_t hi = std::min(lo + 1, size_t(end - 1));
        int expected = (int32_t(referenceDecoded[16 + lo]) * int32_t(8 - numerator % 8) +
            int32_t(referenceDecoded[16 + hi]) * int32_t(numerator % 8)) / 8;
        assert(pcm[i] == expected);
    }
}
int main(int argc, char** argv) {
    uint32_t frames = 99;
    assert(!OotAdultVoice_GetClip(32, 0, &frames) && frames == 0);
    assert(!OotAdultVoice_GetClip(0, 0, nullptr));
    assert(!OotAdultVoice_GetClip(0, 0, &frames) && frames == 0);
    auto owner = std::make_shared<Ship::ResourceManager>();
    owner->base->root = argv[1];
#ifdef COMBO_BUILD
    Ship::CrossRMRegistry::oot = owner;
#else
    Ship::Context context;
    context.owner = owner;
    Ship::Context::active = &context;
#endif
    owner->base->path = "/mods/oot-voice-override.o2r";
    assert(!OotAdultVoice_GetClip(0, 0, &frames) && frames == 0);
    owner->base->path = "/native/oot.o2r";
    for (unsigned id = 0; id < 135; ++id) {
        if (!SamplePath(id)) continue;
        auto file = owner->base->LoadFile(SamplePath(id));
        if (!file || Decode(*file->Buffer).empty()) { std::cerr << "Decode failed " << id << " " << SamplePath(id) << "\n"; return 1; }
    }
    for (unsigned id = 0; id < 135; ++id) {
        if (SamplePath(id)) CompareNative(*owner->base->LoadFile(SamplePath(id))->Buffer);
    }
    const int16_t* first = nullptr;
    uint32_t firstFrames = 0;
    for (unsigned action = 0; action < 32; ++action) {
        for (unsigned variant = 0; variant < recipes[action].size(); ++variant) {
            auto* clip = OotAdultVoice_GetClip(action, variant, &frames);
            assert(clip && frames > 32 && frames < 32000 * 20);
            bool nonzero = false;
            for (unsigned i = 0; i < frames; ++i) nonzero |= clip[i] != 0;
            assert(nonzero);
            uint32_t wrapFrames = 0;
            assert(clip == OotAdultVoice_GetClip(action, variant + recipes[action].size(), &wrapFrames));
            assert(frames == wrapFrames);
            if (!action && !variant) { first = clip; firstFrames = frames; }
        }
    }
    // Loaded PCM must survive all registry/resource teardown and missing files.
    Ship::CrossRMRegistry::oot.reset();
    Ship::Context::active = nullptr;
    assert(first == OotAdultVoice_GetClip(0, 0, &frames) && frames == firstFrames);
    auto file = owner->base->LoadFile(SamplePath(0));
    auto bytes = *file->Buffer;
    assert(!Decode(bytes).empty());
    bytes[64] = 3; assert(Decode(bytes).empty());
    bytes.resize(70); assert(Decode(bytes).empty());
    std::cout << "PASS actual native OoT samples: 32 actions, variants, nonzero PCM, wrap, immutable lifetime, malformed decode\n";
}
