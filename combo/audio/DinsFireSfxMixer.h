/* Din-only immutable PCM mixer. Game thread publishes context/ownership; the
 * audio callback consumes copied values and never follows an engine pointer. */
#ifndef COMBO_AUDIO_DINS_FIRE_SFX_MIXER_H
#define COMBO_AUDIO_DINS_FIRE_SFX_MIXER_H
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <mutex>
#include "DinsFireSfxSamples.h"
#include "../render/DinsFireTiming.h"

namespace CwDinsAudio {
#include "../assets/dins_fire_sfx_pcm.inc"

struct Point {
    float x = 0, y = 0, z = 0;
};
struct Context {
    const void* play = nullptr; // identity only; never dereferenced
    int scene = 0;
    bool live = false, paused = false;
    float gain = 0;
    Point eye{}, at{};
};

inline float ClampGain(float value) {
    return std::isfinite(value) ? std::clamp(value, 0.0f, 1.0f) : 0.0f;
}
inline bool Finite(Point p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}
// A bounded numerical source model: camera-relative pan plus distance rolloff.
// No native room reverb, occlusion or sound-bank priority is claimed.
inline std::array<float, 2> SpatialGains(Point anchor, const Context& context) {
    if (!Finite(anchor) || !Finite(context.eye) || !Finite(context.at))
        return {};
    const double dx = double(anchor.x) - context.eye.x, dy = double(anchor.y) - context.eye.y,
                 dz = double(anchor.z) - context.eye.z;
    const double fx = double(context.at.x) - context.eye.x, fz = double(context.at.z) - context.eye.z;
    const double horizontal = std::hypot(fx, fz);
    const double side = horizontal > 0.001 ? (-fz * dx + fx * dz) / horizontal : dx;
    const double depth = horizontal > 0.001 ? (fx * dx + fz * dz) / horizontal : -dz;
    const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    const float attenuation = static_cast<float>(std::clamp((6000.0 - distance) / 5500.0, 0.0, 1.0));
    const float pan = static_cast<float>(std::clamp(side / (std::abs(depth) + 200.0), -1.0, 1.0));
    const float gain = ClampGain(context.gain) * attenuation;
    return { gain * (1.0f - std::max(pan, 0.0f)), gain * (1.0f + std::min(pan, 0.0f)) };
}

class Mixer {
    struct Clip {
        const int16_t* pcm = nullptr;
        size_t size = 0;
    };
    struct Voice {
        const int16_t* pcm = nullptr;
        size_t size = 0, cursor = 0;
    };
    std::mutex mutex;
    Context context{};
    Point anchor{};
    std::array<Voice, 4> voices{}; // charge, contact, release, burnout
    std::array<Clip, kCueCount> clips{};
    std::shared_ptr<const SampleBank> samples;
    uint32_t serial = 0, handle = 0;
    const void* actor = nullptr; // identity only
    bool contacted = false, released = false, burnout = false;
    int fireFrame = -1;
    size_t loopCursor = 0;
    uint32_t aliveFrames = 0;
    std::array<float, 2> target{}, current{};
    float loopGain = 0, loopTarget = 0;
    static constexpr uint32_t kWatchdog = 32000 / 2;
    static constexpr float kGainStep = 1.0f / 320; // 10 ms control ramp

    void Clear() {
        // A callback may clear a cast. Keep its immutable bank pinned here;
        // the next valid game-thread Begin retires it after releasing the lock.
        handle = 0;
        actor = nullptr;
        voices = {};
        contacted = released = burnout = false;
        fireFrame = -1;
        loopCursor = 0;
        aliveFrames = 0;
        target = current = {};
        loopGain = loopTarget = 0;
    }
    bool Own(uint32_t id) const {
        return id != 0 && id == handle;
    }
    void Gains() {
        target = SpatialGains(anchor, context);
    }
    void SelectClips() {
        clips = { { { kCwDinsChargePcm, std::size(kCwDinsChargePcm) },
                    { kCwDinsContactPcm, std::size(kCwDinsContactPcm) },
                    { kCwDinsReleasePcm, std::size(kCwDinsReleasePcm) },
                    { kCwDinsLoopPcm, std::size(kCwDinsLoopPcm) },
                    { kCwDinsBurnoutPcm, std::size(kCwDinsBurnoutPcm) } } };
        if (samples)
            for (size_t i = 0; i < clips.size(); ++i) {
                const auto& pcm = samples->clips[i];
                if (!pcm.empty())
                    clips[i] = { pcm.data(), pcm.size() };
            }
    }
    void Start(size_t voice, Cue cue) {
        const auto& clip = clips[static_cast<size_t>(cue)];
        voices[voice] = { clip.pcm, clip.size, 0 };
    }
    static float Approach(float value, float goal) {
        return value < goal ? std::min(value + kGainStep, goal) : std::max(value - kGainStep, goal);
    }

  public:
    void Publish(const Context& next) {
        std::lock_guard lock(mutex);
        if (!next.live || !next.play || (context.play && (context.play != next.play || context.scene != next.scene)))
            Clear();
        context = next;
        if (handle) {
            aliveFrames = kWatchdog;
            Gains();
        }
    }
    uint32_t Begin(const void* play, Point position, std::shared_ptr<const SampleBank> bank = {}) {
        std::shared_ptr<const SampleBank> retired;
        uint32_t result;
        {
            std::lock_guard lock(mutex);
            if (!context.live || context.paused || !play || play != context.play || !Finite(position))
                return 0;
            retired = std::move(samples);
            samples = std::move(bank);
            SelectClips();
            Clear();
            if (++serial == 0)
                ++serial;
            handle = serial;
            anchor = position;
            aliveFrames = kWatchdog;
            Gains();
            Start(0, Cue::Charge);
            result = handle;
        }
        // The prior bank's final reference may free several vectors. The caller
        // is the game thread, and any destructor runs outside the mixer mutex.
        return result;
    }
    uint32_t Attach(const void* play, const void* owner) {
        std::lock_guard lock(mutex);
        if (!handle || actor || released || !owner || play != context.play || !context.live)
            return 0;
        actor = owner;
        return handle;
    }
    void Contact(uint32_t id) {
        std::lock_guard lock(mutex);
        if (!Own(id) || contacted || released)
            return;
        contacted = true;
        Start(1, Cue::Contact);
    }
    void Release(uint32_t id) {
        std::lock_guard lock(mutex);
        if (!Own(id) || released)
            return;
        released = true;
        voices[0] = {};
        Start(2, Cue::Release);
        loopTarget = 1;
    }
    void Update(uint32_t id, Point position, int frame) {
        std::lock_guard lock(mutex);
        if (!Own(id))
            return;
        if (!Finite(position) || frame >= CW_DINS_NATIVE_FIRE_TICKS) {
            Clear();
            return;
        }
        anchor = position;
        fireFrame = frame;
        Gains();
        if (!released || frame < 0)
            return;
        if (frame >= CW_DINS_NATIVE_FADE_START_FRAME) {
            loopTarget = std::clamp(1.0f - (frame - CW_DINS_NATIVE_FADE_START_FRAME) / float(CW_DINS_NATIVE_FADE_TICKS),
                                    0.0f, 1.0f);
            if (!burnout) {
                burnout = true;
                Start(3, Cue::Burnout);
            }
        }
    }
    void Cancel(uint32_t id) {
        std::lock_guard lock(mutex);
        if (Own(id) && !released)
            Clear();
    }
    void Stop(uint32_t id) {
        std::lock_guard lock(mutex);
        if (Own(id))
            Clear();
    }
    void Mix(int16_t* output, uint32_t frames) {
        if (!output || !frames)
            return;
        std::lock_guard lock(mutex);
        if (!handle)
            return;
        const auto allowed = std::min(frames, aliveFrames);
        aliveFrames -= allowed;
        if (context.paused) {
            if (allowed < frames)
                Clear();
            return;
        }
        for (uint32_t i = 0; i < allowed; ++i) {
            float sample = 0;
            for (auto& voice : voices)
                if (voice.pcm) {
                    sample += voice.pcm[voice.cursor++];
                    if (voice.cursor == voice.size)
                        voice = {};
                }
            loopGain = Approach(loopGain, loopTarget);
            if (released) {
                const auto& loop = clips[static_cast<size_t>(Cue::Loop)];
                sample += loop.pcm[loopCursor] * loopGain;
                loopCursor = (loopCursor + 1) % loop.size;
            }
            for (size_t ch = 0; ch < 2; ++ch) {
                current[ch] = Approach(current[ch], target[ch]);
                const int mixed = output[i * 2 + ch] + static_cast<int>(sample * current[ch]);
                output[i * 2 + ch] = static_cast<int16_t>(std::clamp(mixed, -32768, 32767));
            }
        }
        if (allowed < frames)
            Clear();
    }
};
} // namespace CwDinsAudio
#endif
