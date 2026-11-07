#pragma once

// Magic Stat Upgrades retain their guidance even in older seeds whose classic
// magic-meter hint was automatically disabled. This does not rewrite settings.
static inline int SariaHint_IsEnabled(int classicHint, int magicStatUpgrade) {
    return classicHint || magicStatUpgrade;
}
