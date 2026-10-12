#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#if __has_include("2s2h/Enhancements/Graphics/MMWinterAtmosphereState.h")
#include "2s2h/Enhancements/Graphics/MMWinterAtmosphereState.h"

using namespace MMWinter;

static size_t VisibleMist(const State& state) {
    size_t count = 0;
    for (const auto& wisp : state.Wisps())
        count += wisp.alpha > 0.01f;
    return count;
}
static bool VisibleAurora(const State& state) {
    for (const auto& ribbon : state.Ribbons())
        for (const auto& vertex : ribbon.vertices)
            if (vertex.alpha > 0.01f)
                return true;
    return false;
}
static bool Same(Vec3 a, Vec3 b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

int main() {
    View view{ { 0, 100, 0 }, { 0, 0, 1 }, { -1, 0, 0 }, { 0, 1, 0 }, 0.5773503f, 16.0f / 9.0f };
    Input input;
    input.eligible = input.mist = input.aurora = input.skyVisible = true;
    input.seconds = 0.05f;
    input.hour = 12;
    input.origin = { 0, -50, 0 };
    input.clipFar = 12800;
    State state, control;
    for (int i = 0; i < 40; ++i) {
        state.Step(input, view);
        std::rand(); // Cosmetic seeding must not depend on shared gameplay RNG.
        control.Step(input, view);
    }
    assert(VisibleMist(state) == kMistCount && !VisibleAurora(state));
    for (size_t i = 0; i < kMistCount; ++i) {
        assert(Same(state.Wisps()[i].position, control.Wisps()[i].position));
        assert(state.Wisps()[i].position.y == input.origin.y);
    }
    assert(NightWeight(12) == 0 && NightWeight(18) == 0 && NightWeight(6) == 0);
    assert(NightWeight(20) == 1 && NightWeight(0) == 1 && NightWeight(4) == 1);
    assert(NightWeight(19) == 0.5f && NightWeight(5) == 0.5f);
    assert(NightWeight(-1) == NightWeight(23) && NightWeight(24) == NightWeight(0));
    input.hour = 23;
    state.Step(input, view);
    assert(VisibleMist(state) == kMistCount && VisibleAurora(state));
    const auto wisps = state.Wisps();
    const auto ribbons = state.Ribbons();
    input.paused = true;
    for (int i = 0; i < 100; ++i)
        state.Step(input, view);
    for (size_t i = 0; i < wisps.size(); ++i) {
        assert(Same(wisps[i].position, state.Wisps()[i].position));
        assert(wisps[i].alpha == state.Wisps()[i].alpha && wisps[i].age == state.Wisps()[i].age);
    }
    view.forward = { 1, 0, 0 };
    view.right = { 0, 0, 1 };
    state.Step(input, view);
    for (size_t i = 0; i < ribbons.size(); ++i)
        for (size_t j = 0; j < ribbons[i].vertices.size(); ++j)
            assert(Same(ribbons[i].vertices[j].position, state.Ribbons()[i].vertices[j].position));
    input.mist = false;
    state.Step(input, view);
    assert(VisibleMist(state) == 0 && VisibleAurora(state));
    input.aurora = false;
    input.mist = true;
    state.Step(input, view);
    assert(VisibleMist(state) == kMistCount && !VisibleAurora(state));
    input.aurora = true;
    input.skyVisible = false;
    state.Step(input, view);
    assert(!VisibleAurora(state));
    input.skyVisible = true;
    input.clipFar = 2000;
    state.Step(input, view);
    bool blue = false, green = false;
    for (const auto& ribbon : state.Ribbons())
        for (size_t i = 0; i < ribbon.vertices.size(); ++i) {
            const auto& vertex = ribbon.vertices[i];
            assert(MMSummer::Length(vertex.position - view.eye) < 2000);
            assert(std::isfinite(vertex.alpha) && vertex.alpha >= 0 && vertex.alpha <= 0.44f);
            if (i % 3 != 1 || i < 3 || i >= (kRibbonColumns - 1) * 3)
                assert(vertex.alpha < 0.00001f);
            blue |= vertex.alpha > 0 && vertex.blue > vertex.green;
            green |= vertex.alpha > 0 && vertex.green > vertex.blue;
        }
    assert(blue && green);
    input.clipFar = 200;
    state.Step(input, view);
    assert(!VisibleAurora(state));
    const auto lifetime = state.Generation();
    const auto oldGeneration = state.Wisps()[0].generation;
    input.eligible = false;
    state.Step(input, view);
    assert(VisibleMist(state) == 0 && !VisibleAurora(state));
    assert(state.Generation() != lifetime);
    input.eligible = true;
    input.paused = false;
    input.clipFar = 12800;
    view.eye.x += 100000;
    input.origin.x += 100000;
    state.Step(input, view);
    assert(VisibleMist(state) == 0);
    assert(state.Wisps()[0].generation > oldGeneration);
    for (int frame = 0; frame < 3000; ++frame) {
        const auto before = state.Wisps();
        state.Step(input, view);
        for (size_t i = 0; i < before.size(); ++i) {
            const auto& now = state.Wisps()[i];
            assert(std::isfinite(now.position.x) && std::isfinite(now.position.y) && std::isfinite(now.position.z));
            assert(now.width >= 180 && now.width <= 370 && now.height >= 10 && now.height <= 20);
            assert(now.alpha >= 0 && now.alpha <= 0.24f);
            if (now.generation == before[i].generation)
                assert(MMSummer::Length(now.position - before[i].position) < 2);
            else
                assert(now.alpha == 0);
        }
    }
    view.eye.x += 100000;
    state.Step(input, view);
    assert(VisibleMist(state) == 0); // Camera travel recycles, never interpolates across the map.
    std::puts("PASS MM winter state: independent mist/aurora, private RNG, dusk/dawn, pause, compass orientation, "
              "clipping, continuous motion and generation lifetimes");
}
#else
int main() {
    std::fputs("FAIL: MM snow-independent winter atmosphere state is missing\n", stderr);
    return 1;
}
#endif
