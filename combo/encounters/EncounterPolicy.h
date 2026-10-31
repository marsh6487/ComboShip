#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace CustomEncounters {
enum class Enemy : uint8_t { None, Miniblin, Molmauk, Beetle, Lynel };
inline Enemy Choose(uint32_t seed, unsigned mask, bool timed) {
    if (timed)
        mask &= 5; // Long guarded fights do not belong in timed rooms.
    std::array<Enemy, 4> pool{};
    unsigned count = 0;
    for (unsigned i = 0; i < 4; ++i)
        if (mask & (1u << i))
            pool[count++] = static_cast<Enemy>(i + 1);
    if (!count)
        return Enemy::None;
    seed ^= seed >> 16;
    seed *= 0x7feb352d;
    seed ^= seed >> 15;
    seed *= 0x846ca68b;
    seed ^= seed >> 16;
    return pool[seed % count];
}
class Visit {
    enum class State : uint8_t { Ready, Alive, Defeated, Skipped };
    std::array<State, 64> slots{};
    int scene = -1, age = -1, cycle = -1;

  public:
    bool Enter(int newScene, int newAge, int newCycle) {
        if (scene == newScene && age == newAge && cycle == newCycle)
            return false;
        scene = newScene;
        age = newAge;
        cycle = newCycle;
        slots.fill(State::Ready);
        return true;
    }
    bool Available(size_t i) const {
        return i < slots.size() && slots[i] == State::Ready;
    }
    bool Request(size_t i, bool enabled, bool validFloor, bool assets) {
        if (i >= slots.size() || !enabled || !assets || slots[i] != State::Ready)
            return false;
        if (!validFloor) {
            slots[i] = State::Skipped;
            return false;
        }
        return true;
    }
    void Spawned(size_t i) {
        if (i < slots.size())
            slots[i] = State::Alive;
    }
    void Defeated(size_t i) {
        if (i < slots.size())
            slots[i] = State::Defeated;
    }
    void Disable() {
        for (auto& s : slots)
            if (s == State::Alive)
                s = State::Ready;
    }
};
} // namespace CustomEncounters
