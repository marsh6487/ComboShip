#pragma once

#define ZELDA_RUN_CVAR "gEnhancements.ZeldaRun"
#define ZELDA_RUN_AGES_CVAR "gEnhancements.ZeldaRun.Ages"
enum ZeldaRunAgeFilter { ZELDA_RUN_CHILD, ZELDA_RUN_ADULT, ZELDA_RUN_BOTH };

#ifdef __cplusplus
extern "C" {
#endif
struct Player;
// Select the header before native helpers calculate the animation's last frame.
void* ZeldaRun_Select(struct Player* player, void* original);
#ifdef __cplusplus
}
#endif
