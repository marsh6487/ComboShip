#pragma once

// No game structs here: both native adapters supply current, validated state.
namespace PartnerPolicy {
struct ControlConditions {
    bool enabled = false, songOwned = false, activeWorld = false;
    bool partnerLive = false, grounded = false, busy = false, held = false;
    bool damaged = false, transitioning = false, cameraLost = false;
    float distance = 0;
};
inline bool CanBegin(const ControlConditions& c) {
    return c.enabled && c.songOwned && c.activeWorld && c.partnerLive && c.grounded && !c.busy && !c.held &&
           !c.damaged && !c.transitioning && c.distance <= 600;
}
inline bool MustRelease(const ControlConditions& c) {
    return !c.enabled || !c.activeWorld || !c.partnerLive || !c.grounded || c.damaged || c.transitioning ||
           c.cameraLost || c.distance > 650;
}
inline bool CojiroEligible(bool alreadyOwned, bool grogCheckComplete) {
    return alreadyOwned || grogCheckComplete;
}
} // namespace PartnerPolicy
