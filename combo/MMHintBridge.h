#pragma once
#include "ComboExport.h"
#ifdef COMBO_BUILD
// Read-only native OoT placement query. reachableOnly uses current owned inventory.
// Result is a transient JSON array of itemGame/itemName/checkName identities.
extern "C" COMBO_EXPORT const char* OOT_GetMMHintCandidates(int fileNum, int reachableOnly);
#endif
