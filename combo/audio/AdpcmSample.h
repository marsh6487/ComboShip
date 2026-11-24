#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ComboAdpcm {
struct EncodedSample {
    const uint8_t* data;
    size_t size;
    int codec;
    const int16_t* book;
    size_t bookSize;
    int order;
    int predictors;
    size_t loopStart;
    size_t loopEnd;
    double rate;
};

struct DecodedSample {
    std::vector<int16_t> pcm;
    size_t loopStart = 0;
    size_t loopEnd = 0;
    double rate = 1.0;
};

inline DecodedSample Decode(const EncodedSample& input) {
    const size_t frameSize = input.codec == 0 ? 9 : input.codec == 3 ? 5 : 0;
    if (frameSize == 0 || input.data == nullptr || input.book == nullptr || input.order != 2 || input.predictors < 1 ||
        input.predictors > 16 || input.bookSize < static_cast<size_t>(input.predictors * 16) ||
        input.size < frameSize || !std::isfinite(input.rate) || input.rate <= 0.0 || input.rate > 16.0) {
        return {};
    }
    // ROM sample sizes can include alignment bytes after the last complete frame.
    const size_t count = input.size / frameSize * 16;
    const size_t end = input.loopEnd == 0 ? count : input.loopEnd;
    if (end > count || input.loopStart >= end) {
        return {};
    }
    DecodedSample decoded{ {}, input.loopStart, end, input.rate };
    decoded.pcm.reserve(end);
    int16_t history[2] = {};
    for (size_t offset = 0; offset + frameSize <= input.size && decoded.pcm.size() < end; offset += frameSize) {
        const uint8_t* frame = input.data + offset;
        const int predictor = frame[0] & 15;
        const int shift = frame[0] >> 4;
        if (predictor >= input.predictors) {
            return {};
        }
        const int16_t* first = input.book + predictor * 16;
        const int16_t* second = first + 8;
        for (size_t half = 0; half < 2 && decoded.pcm.size() < end; ++half) {
            int16_t residual[8] = {};
            for (size_t i = 0; i < 8; ++i) {
                const size_t index = half * 8 + i;
                const int bits = input.codec == 0 ? 4 : 2;
                const int perByte = 8 / bits;
                const int mask = (1 << bits) - 1;
                int value = (frame[1 + index / perByte] >> (8 - bits * (index % perByte + 1))) & mask;
                if (value >= (1 << (bits - 1))) {
                    value -= 1 << bits;
                }
                // ADPCM residuals are signed 16-bit; multiplying avoids undefined
                // negative left shifts in the donor decoder.
                residual[i] = static_cast<int16_t>(value * (1 << std::min(shift, 16 - bits)));
            }
            int16_t output[8] = {};
            for (size_t i = 0; i < 8; ++i) {
                int64_t value = static_cast<int64_t>(first[i]) * history[0] +
                                static_cast<int64_t>(second[i]) * history[1] + residual[i] * 2048;
                for (size_t previous = 0; previous < i; ++previous) {
                    value += static_cast<int64_t>(second[i - previous - 1]) * residual[previous];
                }
                output[i] = static_cast<int16_t>(std::clamp<int64_t>(value >> 11, -32768, 32767));
                if (decoded.pcm.size() < end) {
                    decoded.pcm.push_back(output[i]);
                }
            }
            history[0] = output[6];
            history[1] = output[7];
        }
    }
    return decoded;
}
} // namespace ComboAdpcm
