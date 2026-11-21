#include <array>
#include <cstring>
#include <fstream>
#include <vector>

#include "../../../combo/gameplay/CrossoverAssetPath.h"

extern "C" {
#include "expansions/ssbb/characters/pikachu_ssbb_all_anims.h"
#include "expansions/ssbb/ssbb_spawn.h"
}

namespace {
struct AnimationClip {
    PikachuAnimId index;
    SSBBAnim animation{};
    std::vector<SSBBBoneFrame> frames;
};

std::array<AnimationClip, 6> sClips{ {
    { PIKA_ANIM_WAIT1 },
    { PIKA_ANIM_WALKMIDDLE },
    { PIKA_ANIM_RUN },
    { PIKA_ANIM_JUMPF },
    { PIKA_ANIM_FALL },
    { PIKA_ANIM_SWIMF },
} };

template <typename T, std::size_t Size>
T ReadField(const std::array<char, Size>& bytes, std::size_t offset) {
    T field;
    std::memcpy(&field, bytes.data() + offset, sizeof(T));
    return field;
}

void LoadClips() {
    const char* path = Nei_FindCrossoverAsset("pikachu_anims.bin");
    if (path == nullptr) {
        return;
    }
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    const std::streamoff size = file.tellg();
    std::array<char, 32> header;
    if (size < (std::streamoff)header.size() || !file.seekg(0).read(header.data(), header.size()) ||
        std::memcmp(header.data(), "NEIPKANM", 8) != 0 || ReadField<u32>(header, 8) != 1 ||
        ReadField<u32>(header, 12) != PIKA_ANIM_MAX) {
        return;
    }

    // Both games' exports use the same animation indices and little-endian TRS records.
    constexpr std::streamoff kEntrySize = 20;
    const std::streamoff table = ReadField<u32>(header, 16);
    const std::streamoff framesOffset = ReadField<u32>(header, 24);
    if (table < (std::streamoff)header.size() || table + PIKA_ANIM_MAX * kEntrySize > size ||
        framesOffset < table + PIKA_ANIM_MAX * kEntrySize || framesOffset > size) {
        return;
    }

    for (auto& clip : sClips) {
        std::array<char, kEntrySize> entry;
        if (!file.seekg(table + clip.index * kEntrySize).read(entry.data(), entry.size())) {
            return;
        }
        const u16 frames = ReadField<u16>(entry, 4);
        const u16 bones = ReadField<u16>(entry, 6);
        const f32 rate = ReadField<f32>(entry, 8);
        const std::streamoff offset = framesOffset + ReadField<u32>(entry, 12);
        const std::streamoff span = (std::streamoff)frames * bones * sizeof(SSBBBoneFrame);
        if (frames == 0 || (bones != 47 && bones != 48) || !(rate > 0.0f && rate <= 120.0f) ||
            offset > size || span > size - offset) {
            continue;
        }

        clip.frames.resize((size_t)frames * bones);
        if (!file.seekg(offset).read(reinterpret_cast<char*>(clip.frames.data()), span)) {
            clip.frames.clear();
            return;
        }
        clip.animation = { "Pikachu", frames, bones, rate, clip.frames.data() };
    }
}
} // namespace

extern "C" const SSBBAnim* PikachuForm_GetAnimation(s32 index) {
    static const bool loaded = [] {
        LoadClips();
        return true;
    }();
    (void)loaded;
    for (const auto& clip : sClips) {
        if (clip.index == index && clip.animation.frames != nullptr) {
            return &clip.animation;
        }
    }
    return nullptr;
}
