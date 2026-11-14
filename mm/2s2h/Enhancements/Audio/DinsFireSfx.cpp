#include "z64.h"
#include "functions.h"
#include "variables.h"
// Standalone MM also owns MM bridge symbols; it need not define MM_BUILD_DLL.
#define CW_DINS_HOST_MM
#define CW_DINS_AUDIO_MM
#include "../../../../combo/audio/DinsFireSfxAdapter.inc.cpp"
