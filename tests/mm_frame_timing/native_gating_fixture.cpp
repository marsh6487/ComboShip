// The runner inserts MMFrameTiming_Eligible's exact native body at the marker.
#include "2s2h/Enhancements/Debug/FrameTiming.h"
#include <cassert>
#include <cstring>

struct GameState {
    bool running = true;
};
struct PlayState {
    GameState state;
};
constexpr int GAMEMODE_NORMAL = 0;
struct SaveContext {
    int gameMode = GAMEMODE_NORMAL;
} gSaveContext;
PlayState play;
GameState otherState;
PlayState* gPlayState = &play;
GameState* gGameState = &play.state;
int requested = 1;
bool foreground = true, interior = true;
int CVarGetInteger(const char* name, int fallback) {
    assert(std::strcmp(name, "gDeveloperTools.FrameTimingProbe") == 0);
    return requested < 0 ? fallback : requested;
}
bool FleetShipCombo_IsThisGameActive() {
    return foreground;
}
bool ComboBlackMarket_IsInterior() {
    return interior;
}
void Reset() {
    gPlayState = &play;
    gGameState = &play.state;
    play.state.running = true;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    requested = 1;
    foreground = interior = true;
}

/* PRODUCTION_ELIGIBLE */

int main() {
    Reset();
#ifdef COMBO_BUILD
    assert(MMFrameTiming_Eligible());
    requested = -1; // An absent diagnostic setting uses the real function's default off.
    assert(!MMFrameTiming_Eligible());
    Reset();
    requested = 0;
    assert(!MMFrameTiming_Eligible());
    Reset();
    gPlayState = nullptr;
    assert(!MMFrameTiming_Eligible());
    Reset();
    gGameState = nullptr;
    assert(!MMFrameTiming_Eligible());
    Reset();
    gGameState = &otherState;
    assert(!MMFrameTiming_Eligible());
    Reset();
    play.state.running = false;
    assert(!MMFrameTiming_Eligible());
    Reset();
    gSaveContext.gameMode = GAMEMODE_NORMAL + 1;
    assert(!MMFrameTiming_Eligible());
    Reset();
    foreground = false;
    assert(!MMFrameTiming_Eligible());
    Reset();
    interior = false;
    assert(!MMFrameTiming_Eligible());
    Reset();
    assert(MMFrameTiming_Eligible());
#else
    assert(!MMFrameTiming_Eligible()); // The shop diagnostic belongs to the combined build.
    requested = 0;
    assert(!MMFrameTiming_Eligible());
#endif
}
