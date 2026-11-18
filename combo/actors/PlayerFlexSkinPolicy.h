#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string_view>
#include <vector>

namespace PlayerFlexSkin {
using Matrix = std::array<float, 16>; // native MtxF column order
struct Influence {
    uint16_t a, b, weight;
};
struct Plan {
    std::vector<Matrix> inverse;
    std::vector<Influence> blends;
};
inline bool Finite(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return (bits & 0x7f800000u) != 0x7f800000u;
}
inline std::optional<Plan> Decode(const std::vector<uint16_t>& words, unsigned nativeCount) {
    if (nativeCount < 1 || nativeCount > 32 || words.size() < 4 || words[0] != 0x4e53 || words[1] != 1 ||
        words[2] != nativeCount || words[3] == 0 || nativeCount + words[3] > 256 ||
        words.size() != 4 + nativeCount * 24 + size_t(words[3]) * 3)
        return {};
    Plan plan;
    plan.inverse.resize(nativeCount);
    for (unsigned bone = 0; bone < nativeCount; ++bone) {
        float row[12];
        for (int j = 0; j < 12; ++j) {
            const size_t at = 4 + bone * 24 + j * 2;
            uint32_t bits = words[at] | (uint32_t(words[at + 1]) << 16);
            std::memcpy(&row[j], &bits, sizeof(bits));
            if (!Finite(row[j]) || std::abs(row[j]) > 1e7f)
                return {};
        }
        const float determinant = row[0] * (row[5] * row[10] - row[6] * row[9]) -
                                  row[1] * (row[4] * row[10] - row[6] * row[8]) +
                                  row[2] * (row[4] * row[9] - row[5] * row[8]);
        if (!Finite(determinant) || std::abs(determinant) < 1e-9f)
            return {};
        auto& inverse = plan.inverse[bone];
        for (int col = 0; col < 4; ++col)
            for (int r = 0; r < 4; ++r)
                inverse[col * 4 + r] = r == 3 ? (col == 3 ? 1.f : 0.f) : row[r * 4 + col];
    }
    for (size_t at = 4 + nativeCount * 24; at < words.size(); at += 3) {
        Influence blend{ words[at], words[at + 1], words[at + 2] };
        if (blend.a >= blend.b || blend.b >= nativeCount || blend.weight == 0 || blend.weight >= 16)
            return {};
        plan.blends.push_back(blend);
    }
    return plan;
}
inline Matrix Blend(const Plan& plan, size_t index, const Matrix* native) {
    Matrix result{};
    const auto& blend = plan.blends[index];
    for (int influence = 0; influence < 2; ++influence) {
        const unsigned bone = influence ? blend.b : blend.a;
        const float weight = (influence ? blend.weight : 16 - blend.weight) / 16.f;
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row)
                for (int k = 0; k < 4; ++k)
                    result[col * 4 + row] += native[bone][k * 4 + row] * plan.inverse[bone][col * 4 + k] * weight;
    }
    return result;
}
inline std::string_view Canonical(std::string_view path) {
    if (path.starts_with("__OTR__"))
        path.remove_prefix(7);
    if (path.starts_with("alt/"))
        path.remove_prefix(4);
    return path;
}
inline bool PlayerSkeleton(std::string_view path) {
    path = Canonical(path);
    return path == "objects/object_link_boy/gLinkAdultSkel" || path == "objects/object_link_child/gLinkChildSkel" ||
           path == "objects/object_link_child/gLinkHumanSkel" ||
           path == "objects/object_link_boy/gLinkFierceDeitySkel" ||
           path == "objects/object_link_goron/gLinkGoronSkel" || path == "objects/object_link_zora/gLinkZoraSkel" ||
           path == "objects/object_link_nuts/gLinkDekuSkel";
}
} // namespace PlayerFlexSkin
