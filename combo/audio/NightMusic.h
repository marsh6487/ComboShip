#pragma once
#include <cstdint>
#include <vector>

// Graph-thread policy only; native adapters own players, ambience and replacements.
namespace ComboNightMusic {
enum class Action { None, Start, Stop };
struct Input {
    int scene;
    uint16_t source;
    bool enabled, night, eligible;
    bool blocked = false, canStart = false, playing = false;
    bool suspendWhenBlocked = false, stopOnDay = false;
};
class Lifecycle {
    int scene = -1;
    uint16_t source = 0xffff, playback = 0xffff;
    bool owns = false, refresh = false;

  public:
    void Reset() {
        scene = -1;
        source = playback = 0xffff;
        owns = refresh = false;
    }
    void Refresh() {
        refresh = owns;
    }
    bool Owns() const {
        return owns;
    }
    uint16_t Source() const {
        return source;
    }
    uint16_t Playback() const {
        return playback;
    }
    void DidStart(uint16_t sequence) {
        playback = sequence;
        owns = true;
        refresh = false;
    }
    Action Update(const Input& input) {
        if (scene != input.scene) {
            Reset(); // Never stop the destination's player after a scene boundary.
            scene = input.scene;
        }
        if (source != input.source)
            refresh = owns;
        source = input.source;
        if (input.blocked) {
            if (input.suspendWhenBlocked && owns && input.playing) {
                owns = refresh = false;
                return Action::Stop;
            }
            return Action::None;
        }
        if (!input.enabled || !input.eligible || !input.night) {
            const bool stop = owns && input.playing && (input.night || input.stopOnDay);
            owns = refresh = false;
            return stop ? Action::Stop : Action::None;
        }
        return input.canStart && (!owns || !input.playing || refresh) ? Action::Start : Action::None;
    }
};

// Presentation order only: IDs/CVars and the sequence collection remain untouched.
template <class Map> std::vector<uint16_t> OrderedRows(const Map& map, uint16_t anchor, uint16_t night) {
    const bool move = map.contains(anchor) && map.contains(night);
    std::vector<uint16_t> rows;
    rows.reserve(map.size());
    for (const auto& [key, value] : map) {
        (void)value;
        if (move && key == night)
            continue;
        rows.push_back(key);
        if (move && key == anchor)
            rows.push_back(night);
    }
    return rows;
}
} // namespace ComboNightMusic

// Each host provides its own lifecycle; these never cross the DLL boundary.
uint16_t SceneNightMusic_CurrentEntry();
bool SceneNightMusic_RequestRefresh();
