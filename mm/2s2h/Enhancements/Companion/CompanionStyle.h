#pragma once

#include <stdbool.h>

#define MM_NAVI_COMPANION_CVAR "gEnhancements.NaviCompanionMM"
#define MM_MIDNA_COMPANION_CVAR "gEnhancements.MidnaCompanionMM"

#ifdef __cplusplus
extern "C" {
#endif

bool MMCompanion_UseNavi(void);
bool MMCompanion_UseMidna(void);

#ifdef __cplusplus
}
#endif
