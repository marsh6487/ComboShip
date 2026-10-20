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
    // Leave the native bottom textbox (Y142 at 4:3) below every selected
    // item/seal, while retaining both rows and the keeper's counter pose.
    return { { 0, 80, 300 },
             { slot < 0   ? 0.0f
               : slot < 4 ? 52.5f
                          : -52.5f,
               slot < 0 ? 66.0f : 52.0f, 110 },
             slot < 0 ? 55.0f : 64.0f };
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
        view.at.x = Approach(view.at.x, target.at.x);
        view.at.y = Approach(view.at.y, target.at.y);
        view.fov = Approach(view.fov, target.fov);
    }
};
} // namespace BlackMarketBrowsing
