#pragma once

#include "AdpcmSample.h"

#include <cstring>
#include <utility>

namespace ComboOptionalSfx {

// The OoT and MM extractors write the same little-endian, version 2 SoundFont sample.
inline std::vector<int16_t> Decode(const std::vector<char>& raw) {
    constexpr size_t headerSize = 64;
    constexpr size_t maxSampleBytes = 32000 * 10;
    if (raw.size() < headerSize + 8 || raw[0] != 0 || std::memcmp(raw.data() + 4, "PMSO\x02\0\0\0", 8) != 0) {
        return {};
    }

    const auto* bytes = reinterpret_cast<const uint8_t*>(raw.data());
    const auto read16 = [bytes](size_t offset) -> uint16_t { return bytes[offset] | (bytes[offset + 1] << 8); };
    const auto read32 = [&read16](size_t offset) -> uint32_t {
        return read16(offset) | (static_cast<uint32_t>(read16(offset + 2)) << 16);
    };

    const size_t dataOffset = headerSize + 8;
    const size_t dataSize = read32(headerSize + 4);
    if (dataSize > maxSampleBytes || raw.size() - dataOffset < dataSize + 28) {
        return {};
    }
    const size_t loopOffset = dataOffset + dataSize;
    const size_t states = read32(loopOffset + 12);
    if (states > 16 || raw.size() - loopOffset < 28 + states * 2) {
        return {};
    }
    const size_t bookOffset = loopOffset + 16 + states * 2;
    const auto predictors = read32(bookOffset + 4);
    const size_t coefficients = read32(bookOffset + 8);
    if (read32(bookOffset) != 2 || predictors == 0 || predictors > 16 || coefficients != predictors * 16 ||
        raw.size() - bookOffset - 12 < coefficients * 2) {
        return {};
    }

    int16_t book[16 * 16];
    for (size_t i = 0; i < coefficients; ++i) {
        const int32_t coefficient = read16(bookOffset + 12 + i * 2);
        book[i] = static_cast<int16_t>(coefficient < 32768 ? coefficient : coefficient - 65536);
    }
    auto decoded = ComboAdpcm::Decode({ bytes + dataOffset, dataSize, bytes[headerSize], book, coefficients, 2,
                                        static_cast<int>(predictors), 0, 0, 1.0 });
    // Some one-shots have an end marker one sample past the last complete
    // frame. Ignore partial frames and trim padding when the marker is earlier.
    const size_t end = read32(loopOffset + 4);
    if (end != 0 && end < decoded.pcm.size()) {
        decoded.pcm.resize(end);
    }
    return std::move(decoded.pcm);
}

} // namespace ComboOptionalSfx
