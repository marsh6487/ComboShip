#pragma once

#include "ship/Api.h"

/** True Hidden UI is armed by its saved checkbox; the L-toggle state is transient. */
API_EXPORT int UiVisibility_IsEnabled(void);
/** Returns nonzero only while armed and currently hiding passive UI. */
API_EXPORT int UiVisibility_IsHidden(void);
/** Flips the shared transient state when armed; does nothing while disabled. */
API_EXPORT void UiVisibility_Toggle(void);
