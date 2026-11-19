#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// Presentation state only. Neither host writes inventory through this overlay.
namespace PickupCounter {
inline constexpr char kEnabled[] = "gEnhancements.PickupCounter";
inline constexpr char kScale[] = "gEnhancements.PickupCounterScale";
inline constexpr float kLifetime = 2.8f;
inline constexpr float kFadeTime = 0.45f;
inline constexpr size_t kMaxRows = 3;

enum class Kind : uint8_t { Rupees, Arrows, Bombs, Bombchus, Seeds, Sticks, Nuts, Beans, PowderKeg, Count };
inline constexpr size_t kResourceCount = static_cast<size_t>(Kind::Count);
inline constexpr size_t Index(Kind kind) {
    return static_cast<size_t>(kind);
}
inline constexpr uint32_t Mask(Kind kind) {
    return kind == Kind::Count ? 0 : 1u << Index(kind);
}

struct Resource {
    int current = 0;
    int capacity = 0;
    bool infinite = false;
};
using Snapshot = std::array<Resource, kResourceCount>;

inline Resource Sanitize(Resource value) {
    value.capacity = std::clamp(value.capacity, 0, 99999);
    value.current = std::clamp(value.current, 0, value.capacity);
    return value;
}

inline float Smooth(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

struct Row {
    Kind kind = Kind::Rupees;
    Resource value;
    float displayed = 0;
    float age = 0;
    float quiet = 0;
    float pulse = 0;
    int gained = 0;
    uint64_t order = 0;
    bool active = false;

    float Alpha() const {
        return Smooth(age / 0.16f) * (1.0f - Smooth((quiet - (kLifetime - kFadeTime)) / kFadeTime));
    }
    bool Full() const {
        return value.capacity > 0 && value.current >= value.capacity && !value.infinite;
    }
};

class State {
  public:
    void Reset() {
        *this = State{};
    }

    void Sample(const Snapshot& snapshot, uint32_t receipts = 0) {
        Snapshot next;
        std::transform(snapshot.begin(), snapshot.end(), next.begin(), Sanitize);
        if (!ready) {
            previous = next;
            ready = true;
            return; // A save load, game switch or newly enabled setting is not a pickup.
        }
        for (size_t i = 0; i < next.size(); ++i) {
            const Resource& value = next[i];
            const int delta = value.current - previous[i].current;
            auto row = std::find_if(rows.begin(), rows.end(),
                                    [i](const Row& entry) { return entry.active && Index(entry.kind) == i; });
            // Receipts also catch a pickup at capacity. Differences cover the
            // randomizer/foreign refill paths that bypass native item hooks.
            const bool received = (receipts & (1u << i)) != 0 || (delta > 0 && !value.infinite);
            if (received && value.capacity > 0) {
                if (row == rows.end()) {
                    row = std::min_element(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
                        if (a.active != b.active) {
                            return !a.active;
                        }
                        return a.order < b.order;
                    });
                    *row = Row{};
                    row->kind = static_cast<Kind>(i);
                    row->displayed = static_cast<float>(std::min(previous[i].current, value.current));
                    row->active = true;
                }
                row->quiet = 0;
                row->pulse = 0;
                row->order = ++serial;
                row->gained = std::min(99999, row->gained + std::max(0, delta));
            }
            if (row != rows.end()) {
                row->value = value;
                if (value.capacity == 0) {
                    row->active = false;
                }
                // Spending during a popup updates its count without replaying
                // the pickup. It must never briefly overstate remaining ammo.
                row->displayed = std::min(row->displayed, static_cast<float>(value.current));
            }
        }
        previous = next;
    }

    void Advance(float seconds) {
        if (!std::isfinite(seconds) || seconds <= 0) {
            return;
        }
        // Do not turn a stalled render into a large jump, or a burst of particles.
        const float dt = std::min(seconds, 0.1f);
        for (Row& row : rows) {
            if (!row.active) {
                continue;
            }
            row.age += dt;
            row.quiet += dt;
            row.pulse += dt;
            const float distance = static_cast<float>(row.value.current) - row.displayed;
            row.displayed += distance * (1.0f - std::exp(-18.0f * dt));
            if (std::abs(distance) < 0.08f) {
                row.displayed = static_cast<float>(row.value.current);
            }
            if (row.quiet >= kLifetime) {
                row.active = false;
            }
        }
    }

    std::array<Row, kMaxRows> VisibleRows() const {
        auto result = rows;
        std::sort(result.begin(), result.end(), [](const Row& a, const Row& b) {
            if (a.active != b.active) {
                return a.active;
            }
            return a.order > b.order;
        });
        return result;
    }

    bool Empty() const {
        return std::none_of(rows.begin(), rows.end(), [](const Row& row) { return row.active; });
    }

  private:
    Snapshot previous{};
    std::array<Row, kMaxRows> rows{};
    uint64_t serial = 0;
    bool ready = false;
};

struct Point {
    float x = 0, y = 0;
};
struct Rect {
    float x = 0, y = 0, width = 0, height = 0;
};

// Clip-space projection is shared by both hosts, including widescreen/mirrored
// rendering. A point behind the camera must not get clamped onto a screen edge.
inline bool Project(float x, float y, float w, const Rect& view, bool mirrored, Point& point) {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(w) || w <= 0.01f || view.width < 1 ||
        view.height < 1) {
        return false;
    }
    const float nx = x / w * (mirrored ? -1.0f : 1.0f);
    const float ny = y / w;
    if (std::abs(nx) > 1.0f || std::abs(ny) > 1.0f) {
        return false;
    }
    point = { view.x + (nx + 1) * view.width * 0.5f, view.y + (1 - ny) * view.height * 0.5f };
    return true;
}

// Switch sides near the right edge, then constrain the entire stack to the
// actual game viewport (not the application window or a detached settings tab).
inline Point Constrain(Point point, const Rect& view, float width, float height, float scale) {
    const float margin = 12 * scale;
    return { std::clamp(point.x, view.x + margin, std::max(view.x + margin, view.x + view.width - width - margin)),
             std::clamp(point.y, view.y + margin, std::max(view.y + margin, view.y + view.height - height - margin)) };
}
inline Point Place(Point anchor, const Rect& view, float width, float height, float scale) {
    const float margin = 12 * scale;
    float x = anchor.x + 34 * scale;
    if (x + width > view.x + view.width - margin) {
        x = anchor.x - width - 34 * scale;
    }
    return Constrain({ x, anchor.y - 16 * scale }, view, width, height, scale);
}
} // namespace PickupCounter
