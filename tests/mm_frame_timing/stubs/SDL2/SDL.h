#pragma once
// The ROM-free logger fixture never initializes audio. Context.h includes the
// concrete SDL audio class, whose only SDL declaration is this device ID type.
// This is not used for native game compilation or a substitute for SDL2 SDKs.
#include <stdint.h>
typedef uint32_t SDL_AudioDeviceID;
