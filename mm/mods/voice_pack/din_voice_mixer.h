#pragma once
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

// PCM storage belongs to an immutable process-lifetime cache. The audio thread owns
// each cursor; the game thread writes parameters only after reserving an idle slot.
class DinVoiceMixer {
    struct Slot {
        std::atomic<unsigned> state{ 0 }; // idle, reserved, ready
        std::atomic<bool> cancelled{ false };
        uint16_t tag = 0;
        bool renewable = false, refreshed = false;
        const int16_t* pcm = nullptr;
        size_t length = 0;
        double cursor = 0;
        float step = 1, gain = 1;
    };
    std::array<Slot, 4> slots;
    std::atomic<float> volume{ 1 };

  public:
    bool Play(const int16_t* pcm, size_t length, float gain, float step, uint16_t tag = 0, bool renewable = false) {
        if (!pcm || !length || !std::isfinite(step) || step <= 0 || !std::isfinite(gain))
            return false;
        for (auto& slot : slots) {
            if (tag && slot.state.load(std::memory_order_acquire) == 2 && slot.tag == tag &&
                !slot.cancelled.load(std::memory_order_relaxed)) {
                if (renewable && slot.renewable) {
                    slot.refreshed = true;
                    return true;
                }
                slot.cancelled.store(true, std::memory_order_release);
            }
        }
        for (auto& slot : slots) {
            unsigned idle = 0;
            if (!slot.state.compare_exchange_strong(idle, 1, std::memory_order_acquire))
                continue;
            slot.pcm = pcm;
            slot.tag = tag;
            slot.renewable = renewable;
            slot.refreshed = true;
            slot.cancelled.store(false, std::memory_order_relaxed);
            slot.length = length;
            slot.cursor = 0;
            slot.step = step;
            slot.gain = gain;
            slot.state.store(2, std::memory_order_release);
            return true;
        }
        return false;
    }
    void SetVolume(float gain) {
        volume.store(gain, std::memory_order_relaxed);
    }
    void EndFrame() {
        for (auto& slot : slots) {
            if (slot.state.load(std::memory_order_acquire) == 2 && slot.renewable) {
                if (!slot.refreshed)
                    slot.cancelled.store(true, std::memory_order_release);
                slot.refreshed = false;
            }
        }
    }
    void Stop(uint16_t tag = 0) {
        for (auto& slot : slots)
            if (slot.state.load(std::memory_order_acquire) == 2 && (!tag || slot.tag == tag))
                slot.cancelled.store(true, std::memory_order_release);
    }
    bool IsPlaying(uint16_t tag) const {
        for (const auto& slot : slots)
            if (slot.state.load(std::memory_order_acquire) == 2 && slot.tag == tag &&
                !slot.cancelled.load(std::memory_order_acquire))
                return true;
        return false;
    }
    void Mix(int16_t* output, uint32_t frames) {
        if (!output)
            return;
        for (auto& slot : slots) {
            if (slot.state.load(std::memory_order_acquire) != 2)
                continue;
            if (slot.cancelled.load(std::memory_order_acquire)) {
                slot.state.store(0, std::memory_order_release);
                continue;
            }
            for (uint32_t frame = 0; frame < frames; ++frame) {
                if (slot.cursor >= slot.length) {
                    slot.state.store(0, std::memory_order_release);
                    break;
                }
                size_t index = static_cast<size_t>(slot.cursor);
                double fraction = slot.cursor - index;
                float sample =
                    slot.pcm[index] + fraction * (slot.pcm[std::min(index + 1, slot.length - 1)] - slot.pcm[index]);
                int value = static_cast<int>(sample * slot.gain * volume.load(std::memory_order_relaxed));
                for (unsigned channel = 0; channel < 2; ++channel) {
                    auto& target = output[frame * 2 + channel];
                    target = static_cast<int16_t>(std::clamp(static_cast<int>(target) + value, -32768, 32767));
                }
                slot.cursor += slot.step;
            }
        }
    }
};
