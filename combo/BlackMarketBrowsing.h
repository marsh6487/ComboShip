#pragma once
#include <cmath>

namespace BlackMarketBrowsing {
enum class Phase { Browse, Confirm, Result };
struct Point {
    float x, y, z;
};
struct View {
    Point eye, at;
    float fov;
};

// The room's eight offers have two flat 2x2 banks. Slots 0/4 are the
// inner bottom offers; odd slots are the upper row. -1 faces the keeper.
inline View Target(int slot) {
    if (slot < 0)
        return { { 0, 80, 240 }, { 0, 66, 86 }, 55 };
    // EnOssan turns 30 degrees to each shelf. These taller, compact flat
    // banks need the farther eye to keep both rows above the Y142 textbox;
    // translate it sideways so that the native turn still faces the actual
    // bank center rather than looking past it. Geometry/slot order stay fixed.
    constexpr float bankX = 52.5f, eyeZ = 300, shelfZ = 110;
    constexpr float eyeX = bankX - (eyeZ - shelfZ) * .57735026919f; // tan(30 degrees)
    const float side = slot < 4 ? 1.0f : -1.0f;
    return { { side * eyeX, 80, eyeZ }, { side * bankX, 52, shelfZ }, 64 };
}
inline float Approach(float current, float target) {
    return std::abs(target - current) < .05f ? target : current + (target - current) * .4f;
}
struct Axis {
    int direction = 0, delay = 0;
    int Step(int value) {
        const int next = value > 30 ? 1 : value < -30 ? -1 : 0;
        if (!next) {
            direction = delay = 0;
            return 0;
        }
        if (next != direction) {
            direction = next;
            delay = 10;
            return next;
        }
        if (--delay <= 0) {
            delay = 4;
            return next;
        }
        return 0;
    }
};
struct State {
    int slot = -1, row = 0;
    Phase phase = Phase::Browse;
    View view = Target(-1);
    Axis horizontal, vertical;
    void ResetInput() {
        horizontal = {};
        vertical = {};
    }
    bool Move(int x, int y) {
        const int previous = slot;
        if (slot < 0) {
            if (x)
                slot = (x > 0 ? 0 : 4) + row;
        } else if (x) {
            const bool right = slot < 4, outer = (slot % 4) >= 2;
            if ((x > 0) == right) {
                if (!outer)
                    slot += 2;
            } else if (outer)
                slot -= 2;
            else {
                row = slot & 1;
                slot = -1;
            }
        } else if (y) {
            slot = (slot & ~1) + (y > 0 ? 1 : 0);
        }
        return previous != slot;
    }
    void TickCamera() {
        const View target = Target(slot);
        view.eye.x = Approach(view.eye.x, target.eye.x);
        view.eye.y = Approach(view.eye.y, target.eye.y);
        view.eye.z = Approach(view.eye.z, target.eye.z);
        view.at.x = Approach(view.at.x, target.at.x);
        view.at.y = Approach(view.at.y, target.at.y);
        view.at.z = Approach(view.at.z, target.at.z);
        view.fov = Approach(view.fov, target.fov);
    }
    bool CameraSettled() const {
        const View target = Target(slot);
        return view.eye.x == target.eye.x && view.eye.y == target.eye.y && view.eye.z == target.eye.z &&
               view.at.x == target.at.x && view.at.y == target.at.y && view.at.z == target.at.z &&
               view.fov == target.fov;
    }
};
} // namespace BlackMarketBrowsing
