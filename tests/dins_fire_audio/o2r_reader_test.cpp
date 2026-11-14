// Actual O2rArchive/libzip string+hash reads; Context/manager are fixture boundaries.
#include "archive_fixture.h"
#include "combo/audio/DinsFireSfxWav.h"
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/O2rArchive.h>
#include <ship/utils/StrHash64.h>
#include <spdlog/spdlog.h>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <span>

namespace {
#include "combo/assets/dins_fire_sfx_pcm.inc"
std::unordered_map<uint64_t, std::string> nativePaths;
const std::array<std::span<const int16_t>, CwDinsAudio::kCueCount> defaults = {
    kCwDinsChargePcm, kCwDinsContactPcm, kCwDinsReleasePcm, kCwDinsLoopPcm, kCwDinsBurnoutPcm
};
}
namespace Ship {
const std::string* ArchiveManager::HashToString(uint64_t hash) const {
    const auto found = nativePaths.find(hash);
    return found == nativePaths.end() ? nullptr : &found->second;
}
// Compiled unchanged from actual source bodies, against their actual class headers.
#include "native_o2r_methods.inc"
}

int main(int argc, char** argv) {
    assert(argc == 2);
    DinsAudioFixture::Reset();
    Ship::O2rArchive archive(argv[1]);
    assert(archive.Open());
    nativePaths = *archive.ListFiles();
    assert(nativePaths.size() == CwDinsAudio::kCueCount);
    std::vector<std::vector<int16_t>> decoded;
    for (size_t cue = 0; cue < CwDinsAudio::kCueCount; ++cue) {
        const std::string path = CwDinsAudio::kOverridePaths[cue];
        assert(archive.HasFile(path));
        auto file = archive.LoadFile(path);
        assert(file && file->IsLoaded && file->Buffer && !file->Buffer->empty());
        auto hashed = archive.LoadFile(CRC64(path.c_str()));
        assert(hashed && *hashed->Buffer == *file->Buffer);
        auto samples = CwDinsAudio::DecodeOverrideWav(std::span(
            reinterpret_cast<const uint8_t*>(file->Buffer->data()), file->Buffer->size()));
        assert(samples.size() == defaults[cue].size());
        assert(std::equal(samples.begin(), samples.end(), defaults[cue].begin()));
        std::printf("PASS native O2rArchive/libzip %s exact accepted PCM (%zu samples)\n", path.c_str(), samples.size());
        decoded.push_back(std::move(samples));
    }
    assert(!archive.LoadFile("audio/dins_fire_remake/missing.wav"));
    assert(archive.Close());
    for (size_t cue = 0; cue < decoded.size(); ++cue) {
        assert(std::equal(decoded[cue].begin(), decoded[cue].end(), defaults[cue].begin()));
    }
    std::puts("PASS actual native O2R open/index/path+hash/raw reads, missing file, close and copied PCM lifetime");
}
