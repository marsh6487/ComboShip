#include "oot_adult_voice.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <vector>
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/archive/ArchiveManager.h>
#include <ship/resource/archive/Archive.h>

namespace {
using Clip = std::vector<int16_t>;
using Action = std::vector<Clip>;
using Recipe = std::vector<std::vector<unsigned>>;
// Derived from native OoT Sequence_0 voice dyn table 0x5FF2, font 0 SFX entries.
// Sequences contain their donor recordings in order; see tests/oot_adult_voice/README.md.
const std::array<Recipe, 32> recipes = { {
    { { 0 }, { 1 }, { 2 }, { 3 } },
    { { 4 }, { 5 } },
    { { 21 }, { 22 } },
    { { 6 }, { 25 } },
    { { 7 }, { 8 } },
    { { 9 }, { 10 }, { 11 } },
    { { 12 }, { 13 }, { 14 } },
    { { 17 }, { 18 } },
    { { 15 }, { 16 } },
    { { 19 }, { 23 } },
    { { 56 } },
    { { 77, 78, 79 } },
    { { 15 }, { 16 } },
    { { 9 }, { 10 }, { 11 } },
    { { 80, 81, 82 } },
    { { 83, 84, 85 } },
    { { 55 } },
    { { 86, 87, 88 } },
    { { 0 } },
    { { 67 } },
    { { 26 }, { 27 } },
    { { 5 } },
    { { 134 } },
    { { 4 } },
    { { 7 } },
    { { 6 } },
    { { 24 } },
    { { 60 } },
    { { 61 } },
    { { 15 }, { 16 } },
    { { 13 } },
    { { 0 }, { 1 }, { 2 }, { 3 } },
} };
const char* SamplePath(unsigned id) {
    switch (id) {
        case 0:
            return "audio/samples/Adult Link - Attack 1_META";
        case 1:
            return "audio/samples/Adult Link - Attack 2_META";
        case 2:
            return "audio/samples/Adult Link - Attack 3_META";
        case 3:
            return "audio/samples/Adult Link - Attack 4_META";
        case 4:
            return "audio/samples/Adult Link - Strong Attack 1_META";
        case 5:
            return "audio/samples/Adult Link - Strong Attack 2_META";
        case 6:
            return "audio/samples/Adult Link - Dangling Grunt_META";
        case 7:
            return "audio/samples/Adult Link - Climb Edge_META";
        case 8:
            return "audio/samples/Adult Link - Dangling Gasp 2_META";
        case 9:
            return "audio/samples/Adult Link - Hurt 1_META";
        case 10:
            return "audio/samples/Adult Link - Hurt 2_META";
        case 11:
            return "audio/samples/Adult Link - Hurt 3_META";
        case 12:
            return "audio/samples/Adult Link - Hurt 4_META";
        case 13:
            return "audio/samples/Adult Link - Knocked Back_META";
        case 14:
            return "audio/samples/Adult Link - Hurt 5_META";
        case 15:
            return "audio/samples/Adult Link - Falling 1_META";
        case 16:
            return "audio/samples/Adult Link - Falling 2_META";
        case 17:
            return "audio/samples/Adult Link - Gasp 2_META";
        case 18:
            return "audio/samples/Adult Link - Gasp 3_META";
        case 19:
            return "audio/samples/Adult Link - Pant 1_META";
        case 21:
            return "audio/samples/Adult Link - Spur Horse 1_META";
        case 22:
            return "audio/samples/Adult Link - Spur Horse 2_META";
        case 23:
            return "audio/samples/Adult Link - Pant 2_META";
        case 24:
            return "audio/samples/Adult Link - Painful Landing_META";
        case 25:
            return "audio/samples/Adult Link - Dangling Gasp 1_META";
        case 26:
            return "audio/samples/Adult Link - Hup_META";
        case 27:
            return "audio/samples/Adult Link - Gasp 1_META";
        case 55:
            return "audio/samples/Adult Link - Glug_META";
        case 56:
            return "audio/samples/Adult Link - Refreshed_META";
        case 60:
            return "audio/samples/Adult Link - Lift (Unused)_META";
        case 61:
            return "audio/samples/Adult Link - Cast Attacking Magic_META";
        case 67:
            return "audio/samples/Child Link - Shivers_META";
        case 77:
            return "audio/samples/Adult Link - Hurt (Unused)_META";
        case 78:
            return "audio/samples/Adult Link - Choking_META";
        case 79:
            return "audio/samples/Adult Link - Gasping (Unused)_META";
        case 80:
            return "audio/samples/Adult Link - Small Gasp (Unused)_META";
        case 81:
            return "audio/samples/Adult Link - Unsettled Moan_META";
        case 82:
            return "audio/samples/Adult Link - Sneezes_META";
        case 83:
            return "audio/samples/Adult Link - Sigh 1_META";
        case 84:
            return "audio/samples/Adult Link - Sigh 2_META";
        case 85:
            return "audio/samples/Adult Link - Sigh 3_META";
        case 86:
            return "audio/samples/Adult Link - Stretch Start_META";
        case 87:
            return "audio/samples/Adult Link - Stretching_META";
        case 88:
            return "audio/samples/Adult Link - Finished Stretching_META";
        case 134:
            return "audio/samples/Adult Link - Dramatic Gasp_META";
        default:
            return nullptr;
    }
}
struct Reader {
    const std::vector<char>& bytes;
    size_t pos = 64;
    bool valid = true;
    uint32_t U(unsigned n) {
        if (!valid || pos > bytes.size() || n > bytes.size() - pos) {
            valid = false;
            return 0;
        }
        uint32_t v = 0;
        for (unsigned i = 0; i < n; ++i)
            v |= uint32_t(uint8_t(bytes[pos++])) << (i * 8);
        return v;
    }
};
// Native sample V2 layout: AudioSampleFactory.cpp. Decode is the scalar N64
// predictor from soh/soh/mixer.c aADPCMdecImpl, using defined signed arithmetic.
Clip Decode(const std::vector<char>& bytes) {
    Reader r{ bytes };
    if (bytes.size() < 72 || uint8_t(bytes[0]) != 0 || std::string(bytes.data() + 4, 4) != "PMSO" ||
        uint8_t(bytes[8]) != 2)
        return {};
    unsigned codec = r.U(1);
    r.U(1);
    r.U(1);
    r.U(1);
    uint32_t size = r.U(4);
    if (codec != 0 || size == 0 || size > 16 * 1024 * 1024 || size > bytes.size() - r.pos)
        return {};
    size_t data = r.pos;
    r.pos += size;
    r.U(4);
    unsigned end = r.U(4);
    r.U(4);
    unsigned state = r.U(4);
    if (state > 16)
        return {};
    for (unsigned i = 0; i < state; ++i)
        r.U(2);
    unsigned order = r.U(4), predictors = r.U(4), count = r.U(4);
    if (order != 2 || predictors == 0 || predictors > 16 || count != predictors * 16)
        return {};
    std::vector<int16_t> book;
    for (unsigned i = 0; i < count; ++i)
        book.push_back(static_cast<int16_t>(r.U(2)));
    if (!r.valid)
        return {};
    const size_t samples = ((size + 8) / 9) * 16;
    if (!end || end > samples)
        return {};
    Clip raw;
    raw.reserve(samples);
    int16_t prev2 = 0, prev1 = 0;
    for (size_t frame = 0; frame < (size + 8) / 9; ++frame) {
        const size_t p = data + frame * 9;
        unsigned shift = uint8_t(bytes[p]) >> 4, pred = uint8_t(bytes[p]) & 15;
        if (shift > 12 || pred >= predictors)
            return {};
        const auto* b = book.data() + pred * 16;
        for (unsigned half = 0; half < 2; ++half) {
            int32_t residual[8];
            for (unsigned j = 0; j < 8; ++j) {
                size_t offset = p + 1 + half * 4 + j / 2;
                unsigned bits = offset < data + size ? uint8_t(bytes[offset]) : 0;
                int value = ((j & 1) ? bits : bits >> 4) & 15;
                residual[j] = (value >= 8 ? value - 16 : value) * (1 << shift);
            }
            int16_t out[8];
            for (unsigned j = 0; j < 8; ++j) {
                int64_t acc = int64_t(b[j]) * prev2 + int64_t(b[8 + j]) * prev1 + int64_t(residual[j]) * 2048;
                for (unsigned k = 0; k < j; ++k)
                    acc += int64_t(b[8 + j - k - 1]) * residual[k];
                // Arithmetic floor, matching the native arithmetic right shift.
                int64_t value = acc >= 0 ? acc / 2048 : -((-acc + 2047) / 2048);
                out[j] = static_cast<int16_t>(std::clamp<int64_t>(value, -32768, 32767));
                raw.push_back(out[j]);
            }
            prev2 = out[6];
            prev1 = out[7];
        }
    }
    raw.resize(end);
    // Every referenced font-0 voice donor has native tuning 0.625 (20 kHz).
    // Resample once to the 32 kHz PCM mixer domain without an adult pitch shift.
    Clip pcm((raw.size() * 8 + 4) / 5);
    for (size_t i = 0; i < pcm.size(); ++i) {
        size_t numerator = i * 5, lo = numerator / 8;
        if (lo >= raw.size())
            lo = raw.size() - 1;
        size_t hi = std::min(lo + 1, raw.size() - 1);
        pcm[i] = static_cast<int16_t>(
            (int32_t(raw[lo]) * int32_t(8 - numerator % 8) + int32_t(raw[hi]) * int32_t(numerator % 8)) / 8);
    }
    return pcm;
}
std::array<Action, 32> cache;
bool ready = false;
bool Load() {
    if (ready)
        return true;
#ifdef COMBO_BUILD
    // Combined boot always initializes OoT before either game's entry point.
    auto owner = Ship::CrossRMRegistry::Get("oot");
#else
    // Solo MM can mount the native OoT companion archive too.
    auto* context = Ship::Context::GetRawInstance();
    auto owner = context ? context->GetResourceManager() : nullptr;
#endif
    if (!owner || !owner->GetArchiveManager())
        return false;
    auto archives = owner->GetArchiveManager()->GetArchives();
    if (!archives)
        return false;
    std::shared_ptr<Ship::Archive> native;
    // Select the actual base archive, bypassing mod/Alt path shadowing.
    // Normal OoT takes precedence over MQ, matching the existing companion loader.
    for (const char* wanted : { "oot.o2r", "oot-mq.o2r" }) {
        for (const auto& archive : *archives) {
            if (!archive)
                continue;
            const auto& path = archive->GetPath();
            const auto split = path.find_last_of("/\\");
            if (path.substr(split == std::string::npos ? 0 : split + 1) == wanted) {
                native = archive;
                break;
            }
        }
        if (native)
            break;
    }
    if (!native)
        return false;
    std::array<Clip, 135> samples;
    std::array<Action, 32> loaded;
    for (unsigned action = 0; action < recipes.size(); ++action) {
        for (const auto& sequence : recipes[action]) {
            Clip clip;
            for (unsigned id : sequence) {
                if (samples[id].empty()) {
                    auto file = native->LoadFile(SamplePath(id));
                    if (!file || !file->Buffer)
                        return false;
                    samples[id] = Decode(*file->Buffer);
                    if (samples[id].empty())
                        return false;
                }
                clip.insert(clip.end(), samples[id].begin(), samples[id].end());
            }
            loaded[action].push_back(std::move(clip));
        }
    }
    cache = std::move(loaded);
    ready = true;
    return ready;
}
} // namespace
extern "C" const int16_t* OotAdultVoice_GetClip(unsigned action, unsigned variant, uint32_t* frames) {
    if (frames)
        *frames = 0;
    if (action >= cache.size() || !frames || !Load())
        return nullptr;
    const auto& clip = cache[action][variant % cache[action].size()];
    *frames = static_cast<uint32_t>(clip.size());
    return clip.data();
}
