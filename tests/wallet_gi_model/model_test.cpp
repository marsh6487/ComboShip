#include "combo/render/ZfgWalletRupees.h"

#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    using namespace ComboZfgWallet;
    assert(Fade(0.f) == 0.f && Fade(.25f) == 1.f && Fade(1.f) == 0.f);
    assert(Fade(.10f) > Fade(.05f) && Fade(.80f) > Fade(.90f));

    for (unsigned frame = 0; frame < 72; ++frame) {
        const auto slots = SampleRupees(frame, kClassicMouth);
        const auto next = SampleRupees(frame + 72, kClassicMouth);
        const auto twilight = SampleRupees(frame, kTwilightMouth);
        for (size_t i = 0; i < slots.size(); ++i) {
            const auto& r = slots[i];
            assert(r.color == i && r.alpha <= 240);
            assert(r.center.y >= kClassicMouth.y && r.center.y < 60.f);
            assert(r.scale > .14f && r.scale < .19f);
            assert(r.alpha == next[i].alpha && r.yaw == next[i].yaw && r.roll == next[i].roll);
            assert(std::abs(twilight[i].center.x - r.center.x - 4.f) < .0001f);
            assert(std::abs(twilight[i].center.y - r.center.y - 1.f) < .0001f);
            assert(std::abs(twilight[i].center.z - r.center.z + 1.f) < .0001f);
        }
    }
    const auto at = [](uint32_t frame) { return SampleRupees(frame, kClassicMouth)[0]; };
    assert(at(0).alpha == 0 && at(4).alpha > 0 && at(8).alpha > at(4).alpha);
    assert(at(20).alpha == 240 && at(36).alpha + 30 < at(20).alpha);
    assert(at(36).alpha > at(48).alpha && at(48).alpha > at(60).alpha && at(68).alpha < at(60).alpha);
    assert(at(71).alpha == 0 && at(72).alpha == 0);
    int intermediateTicks = 0;
    for (uint32_t frame = 22; frame < 72; ++frame) {
        assert(at(frame).alpha <= at(frame - 1).alpha);
        assert(at(frame - 1).alpha - at(frame).alpha <= 8);
        intermediateTicks += at(frame).alpha > 20 && at(frame).alpha < 220;
    }
    assert(intermediateTicks >= 30);
    assert(at(4).center.y < 30.f && at(24).center.y > 34.f && at(68).center.y > at(55).center.y);
    std::cout << "PASS stock and TP emitters rise from their own pouch openings and fade through a seamless loop\n";
}
