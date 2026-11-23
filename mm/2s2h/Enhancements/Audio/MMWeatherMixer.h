#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include "../../../../combo/audio/AdpcmSample.h"

namespace MMWeather {
using ComboAdpcm::Decode;
using ComboAdpcm::DecodedSample;
using ComboAdpcm::EncodedSample;

// The owner serializes access. Voices index owned PCM, never game resources.
class Mixer {
  public:
    void SetSamples(DecodedSample rain, DecodedSample rumble, DecodedSample lightning);
    void SetRain(float gain);
    void Thunder(float gain);
    void Mix(int16_t* interleavedStereo, size_t frames, float gain, float thunderGain = 1.0f);
    void Reset();

  private:
    struct Voice {
        bool active = false;
        double position = 0.0;
        float gain = 0.0f;
    };
    double Next(Voice& voice, const DecodedSample& sample, bool loop);
    std::array<DecodedSample, 3> samples;
    std::array<Voice, 3> voices;
};
} // namespace MMWeather
