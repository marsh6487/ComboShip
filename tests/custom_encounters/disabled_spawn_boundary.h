#pragma once
#include "../../combo/encounters/NativeCustomEnemies.h"
#include <cassert>

// Scene/camera fixtures leave optional encounters disabled. Keep the native
// hook ABI and validate inputs without replacing their authored actor entries.
namespace DisabledEncounterFixture {
inline PlayState* recordedPlay;
inline int16_t recordedId, recordedParams;
inline float recordedX, recordedY, recordedZ;
inline int recordedEntries, replacementEntries;
}

extern "C" inline void CustomEnemies_RecordEntry(PlayState* play, int16_t id,
                                                  float x, float y, float z, int16_t params) {
    using namespace DisabledEncounterFixture;
    recordedPlay = play;
    recordedId = id;
    recordedParams = params;
    recordedX = x;
    recordedY = y;
    recordedZ = z;
    ++recordedEntries;
}

extern "C" inline void CustomEnemies_ReplaceEntry(PlayState* play, int16_t* id,
                                                   int16_t* params, float x, float y, float z) {
    using namespace DisabledEncounterFixture;
    assert(recordedEntries > replacementEntries);
    assert(play == recordedPlay && *id == recordedId && *params == recordedParams);
    assert(x == recordedX && y == recordedY && z == recordedZ);
    ++replacementEntries;
}
