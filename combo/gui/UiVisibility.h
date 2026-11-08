#pragma once

/* One shared runtime visibility state lives in libultraship. The enhancement
 * enables the mode; L changes its session state without changing minimap or
 * Minimal UI preferences. Native UI draws retain their update side effects. */
#include "libultraship/bridge/uivisibilitybridge.h"
