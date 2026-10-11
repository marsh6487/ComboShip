#pragma once

struct PlayState;

// Reversible autumn/winter variants of loaded foliage draws and the exact
// mixed stump/arch grass atlas. The existing update/reset entry points are shared.
void MMAutumnSceneFoliage_Update(const PlayState* play);
void MMAutumnSceneFoliage_Reset();
