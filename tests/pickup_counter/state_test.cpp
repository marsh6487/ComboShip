#include "combo/hud/PickupCounter.h"
#include <cassert>
#include <iostream>
#include <limits>

using namespace PickupCounter;

static Row Find(const State& state, Kind kind) {
    for (const Row& row : state.VisibleRows()) {
        if (row.active && row.kind == kind) {
            return row;
        }
    }
    return {};
}
static void RunFor(State& state, float seconds, int fps = 60) {
    for (int i = 0; i < static_cast<int>(seconds * fps); ++i) {
        state.Advance(1.0f / fps);
    }
}

int main() {
    State state;
    Snapshot inventory{};
    inventory[Index(Kind::Rupees)] = { 80, 200, false };
    inventory[Index(Kind::Arrows)] = { 12, 30, false };
    inventory[Index(Kind::Bombs)] = { 4, 20, false };
    state.Sample(inventory);
    assert(state.Empty()); // Opening a file or enabling the option is quiet.

    // A blue rupee, then a rapid chain. One row gains six, with no restart
    // of its entry alpha and no repeated gain on count-up/render-only frames.
    inventory[Index(Kind::Rupees)].current += 5;
    state.Sample(inventory, Mask(Kind::Rupees));
    RunFor(state, 0.25f);
    assert(Find(state, Kind::Rupees).gained == 5);
    const float before = Find(state, Kind::Rupees).Alpha();
    inventory[Index(Kind::Rupees)].current++;
    state.Sample(inventory, Mask(Kind::Rupees));
    assert(Find(state, Kind::Rupees).gained == 6);
    assert(Find(state, Kind::Rupees).Alpha() == before);
    for (int i = 0; i < 1000; ++i) {
        state.Sample(inventory);
    }
    assert(Find(state, Kind::Rupees).gained == 6);

    // Mixed drops stack. Using a bomb or firing an arrow updates the live
    // counter but creates no popup and does not extend its lifetime.
    inventory[Index(Kind::Arrows)].current += 5;
    inventory[Index(Kind::Bombs)].current += 5;
    state.Sample(inventory);
    assert(Find(state, Kind::Rupees).active);
    assert(Find(state, Kind::Arrows).active);
    assert(Find(state, Kind::Bombs).active);
    RunFor(state, 0.2f);
    const float quiet = Find(state, Kind::Bombs).quiet;
    inventory[Index(Kind::Bombs)].current--;
    state.Sample(inventory);
    assert(Find(state, Kind::Bombs).quiet == quiet);
    assert(Find(state, Kind::Bombs).value.current == 8);
    assert(Find(state, Kind::Bombs).gained == 5);
    RunFor(state, 3);
    assert(state.Empty());
    inventory[Index(Kind::Arrows)].current--;
    state.Sample(inventory);
    assert(state.Empty());

    // A native receipt must still appear at capacity, with FULL and zero
    // admitted gain. A wallet with no ownership/capacity stays silent.
    inventory[Index(Kind::Rupees)].current = 200;
    state.Sample(inventory);
    RunFor(state, 3);
    state.Sample(inventory, Mask(Kind::Rupees));
    assert(Find(state, Kind::Rupees).Full());
    assert(Find(state, Kind::Rupees).gained == 0);
    inventory[Index(Kind::Rupees)] = { 999, 0, false };
    state.Sample(inventory, Mask(Kind::Rupees));
    assert(!Find(state, Kind::Rupees).active);

    // Imported independent Bombchu capacity is not the ordinary Bomb Bag.
    inventory[Index(Kind::Bombchus)] = { 9, 20, false };
    inventory[Index(Kind::Bombs)] = {};
    state.Sample(inventory);
    assert(Find(state, Kind::Bombchus).value.capacity == 20);
    assert(!Find(state, Kind::Bombs).active);

    // Only the newest three resource types survive. Repeated receipts remain
    // bounded even if every item type changes on the same tick.
    for (size_t i = 0; i < inventory.size(); ++i) {
        inventory[i] = { 1, 50, false };
    }
    state.Sample(inventory, (1u << kResourceCount) - 1);
    auto rows = state.VisibleRows();
    assert(rows[0].kind == Kind::PowderKeg);
    assert(rows[1].kind == Kind::Beans);
    assert(rows[2].kind == Kind::Nuts);
    for (int tick = 0; tick < 10000; ++tick) {
        state.Sample(inventory, (1u << kResourceCount) - 1);
    }
    assert(state.VisibleRows().size() == 3);

    // Infinite upgrades do not generate a constant stream of refill popups.
    state.Reset();
    inventory[Index(Kind::Arrows)] = { 20, 50, true };
    state.Sample(inventory);
    inventory[Index(Kind::Arrows)].current = 50;
    state.Sample(inventory);
    assert(state.Empty());
    state.Sample(inventory, Mask(Kind::Arrows));
    assert(Find(state, Kind::Arrows).value.infinite);
    assert(!Find(state, Kind::Arrows).Full());

    // Render refresh rates do not change the hold/fade duration.
    for (const int fps : { 20, 30, 60, 144, 240 }) {
        state.Reset();
        state.Sample(inventory);
        state.Sample(inventory, Mask(Kind::Arrows));
        RunFor(state, 2, fps);
        assert(Find(state, Kind::Arrows).Alpha() > 0.99f);
        RunFor(state, 1, fps);
        assert(state.Empty());
    }
    state.Advance(std::numeric_limits<float>::quiet_NaN());
    state.Advance(-1);
    state.Reset();
    state.Sample(inventory);
    assert(state.Empty()); // Reset/host-switch never replays another host's gain.

    // Widescreen, moved windows, mirrored worlds, camera clipping and edge
    // constraints. A behind-camera/NaN anchor must never stick to an edge.
    Point point;
    const Rect view{ 100, 80, 1920, 1080 };
    assert(Project(0, 0, 1, view, false, point));
    assert(point.x == 1060 && point.y == 620);
    assert(Project(0.5f, -0.5f, 1, view, true, point));
    assert(point.x == 580 && point.y == 890);
    assert(!Project(0, 0, -1, view, false, point));
    assert(!Project(2, 0, 1, view, false, point));
    assert(!Project(0, std::numeric_limits<float>::quiet_NaN(), 1, view, false, point));
    for (const Rect frame : { Rect{ 0, 0, 320, 240 }, Rect{ 50, 30, 960, 720 }, view }) {
        for (float x : { 0.0f, frame.width * 0.5f, frame.width }) {
            for (float y : { 0.0f, frame.height * 0.5f, frame.height }) {
                Point placed = Place({ frame.x + x, frame.y + y }, frame, 184, 189, 1);
                assert(placed.x >= frame.x && placed.x + 184 <= frame.x + frame.width);
                assert(placed.y >= frame.y && placed.y + 189 <= frame.y + frame.height);
            }
        }
    }
    std::cout << "PASS pickup lifecycle, receipts, capacities, stack, timing and projection\n";
}
