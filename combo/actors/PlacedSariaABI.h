#ifndef COMBO_PLACED_SARIA_ABI_H
#define COMBO_PLACED_SARIA_ABI_H

/* Only opaque owner handles and the games' matching normal-animation/flex
 * headers cross this boundary. No Actor, PlayState, C++ object or save data. */
typedef struct {
    void* owner;
    void* skeleton;
    void* animation;
    const char* eyes[3];
    const char* mouth;
    const char* heads[3];
    const char* poseLimbs[17];
    int altEnabled;
    int custom;
} ComboPlacedSariaResources;

typedef int (*ComboPlacedSariaLoadFunc)(unsigned pose, ComboPlacedSariaResources* output);
typedef void (*ComboPlacedSariaReleaseFunc)(void* owner);
typedef int (*ComboPlacedSariaAltFunc)(void);

#endif
