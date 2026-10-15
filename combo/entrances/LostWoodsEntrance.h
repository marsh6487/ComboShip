#pragma once

// Local passage coordinates, shared by the trigger, arrival and MM doorway.
// These are scene coordinates, never entrance-shuffle destinations.
enum { COMBO_PORTAL_MARKET = 0, COMBO_PORTAL_LOST_WOODS = 1 };

static inline int ComboLostWoods_IsPlayableSlot(int fileNum) {
    return fileNum >= 0 && fileNum < 3;
}

static inline int ComboLostWoods_InOotTunnel(int scene, int room, float x, float y, float z) {
    return scene == 0x5B && room == 0 && x > -65.0f && x < 65.0f && y > -30.0f && y < 120.0f && z < -360.0f &&
           z > -560.0f;
}

static inline int ComboLostWoods_InMmPassage(int scene, int room, float x, float y, float z) {
    // The north-east wall BETWEEN the maze's north/east corridors, in the entry clearing.
    const float lateral = (x + z) * 0.70710678f;
    const float depth = (x - z - 320.0f) * 0.70710678f;
    return scene == 0x64 && room == 1 && lateral > -42.0f && lateral < 42.0f && depth > -12.0f && depth < 60.0f &&
           y > -20.0f && y < 130.0f;
}
