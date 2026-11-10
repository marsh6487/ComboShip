#pragma once

#include "RitoMaskSettingsKeys.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <cstring>

namespace ComboRando::RitoMaskSettings {
inline bool NativeControl(const char* cvar) {
    return cvar && (!std::strcmp(cvar, OotCvar) || !std::strcmp(cvar, MmCvar));
}

inline void SetShared(int value) {
    CVarSetInteger(SharedCvar, value != 0);
}

inline int Preview() {
    int value = CVarGetInteger(SharedCvar, -1);
    if (value < 0) {
        // One-time migration from the independent controls. An explicit zero
        // never consults either solo preference again.
        value = CVarGetInteger(OotCvar, 0) || CVarGetInteger(MmCvar, 0);
    }
    return value != 0;
}

inline int Shared() {
    const int value = Preview();
    SetShared(value);
    return value;
}

inline void PrepareGeneration() {
    CVarSetInteger(GenerationCvar, Shared());
}

// Native widgets and presets can keep their existing CVar bindings. Present the
// Combo value only while that operation runs, then restore both solo preferences
// (including absent/default CVars). No native preference is saved as an alias.
class NativePreferenceScope {
  public:
    explicit NativePreferenceScope(bool combined) : active(combined) {
        if (!active)
            return;
        oot = CVarGetInteger(OotCvar, -1);
        mm = CVarGetInteger(MmCvar, -1);
        before = Shared();
        CVarSetInteger(OotCvar, before);
        CVarSetInteger(MmCvar, before);
    }
    NativePreferenceScope(const NativePreferenceScope&) = delete;
    NativePreferenceScope& operator=(const NativePreferenceScope&) = delete;
    ~NativePreferenceScope() {
        Finish();
    }

    // Presets pass their authored value explicitly: loading Config can rewrite
    // unrelated native CVars, which must not be mistaken for a Mirror edit.
    bool Finish(int requested = -1) {
        if (!active)
            return false;
        active = false;
        if (requested < 0) {
            const int currentOot = CVarGetInteger(OotCvar, 0) != 0;
            const int currentMm = CVarGetInteger(MmCvar, 0) != 0;
            requested = currentOot != before ? currentOot : currentMm;
        }
        Restore(OotCvar, oot);
        Restore(MmCvar, mm);
        SetShared(requested);
        return (requested != 0) != before;
    }

  private:
    static void Restore(const char* name, int value) {
        if (value < 0)
            CVarClear(name);
        else
            CVarSetInteger(name, value);
    }
    bool active;
    int oot = 0, mm = 0, before = 0;
};
} // namespace ComboRando::RitoMaskSettings
