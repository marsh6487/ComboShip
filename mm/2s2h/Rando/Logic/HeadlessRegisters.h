#pragma once

extern "C" {
#include "regs.h"
}

namespace Rando::Headless {
inline void EnsureRegisterEditor(RegEditor*& editor) {
    // Native Regs_Init zeroes these fields after allocating from the game arena.
    // Headless item grants still use REG macros, without creating that arena.
    static RegEditor registers{};
    if (!editor) {
        editor = &registers;
    }
}
} // namespace Rando::Headless
