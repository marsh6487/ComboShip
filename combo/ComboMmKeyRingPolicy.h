#ifndef COMBO_MM_KEY_RING_POLICY_H
#define COMBO_MM_KEY_RING_POLICY_H

// Native MM totals shared by pool/grant and both receiving-host GI renderers.
#ifdef __cplusplus
static constexpr int ComboMmKeyRing_KeyCount(int dungeon) {
#else
static inline int ComboMmKeyRing_KeyCount(int dungeon) {
#endif
    switch (dungeon) {
        case 0:
            return 1;
        case 1:
            return 3;
        case 2:
            return 1;
        case 3:
            return 4;
        default:
            return 0;
    }
}
#endif
