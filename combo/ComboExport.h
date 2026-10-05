#pragma once

// Export decoration for the combo ABI: the symbols the launcher, comboui, and the
// opposite game resolve at runtime (SOH_*/MM_*/Combo_*/ComboUI_*/OOT_*). On Windows
// the games build as DLLs and need dllexport; on ELF platforms the game libraries
// build with -fvisibility=hidden so their duplicated engine internals cannot
// interpose each other, and the combo ABI opts back into default visibility here.
#ifdef _WIN32
#define COMBO_EXPORT __declspec(dllexport)
#else
#define COMBO_EXPORT __attribute__((visibility("default")))
#endif

// Shared declarations must match the owning DLL's exported definitions without
// asking the opposite game to export symbols it only resolves at runtime.
#if defined(COMBO_BUILD) && defined(SOH_BUILD_DLL)
#define COMBO_OOT_EXPORT COMBO_EXPORT
#else
#define COMBO_OOT_EXPORT
#endif
#if defined(COMBO_BUILD) && defined(MM_BUILD_DLL)
#define COMBO_MM_EXPORT COMBO_EXPORT
#else
#define COMBO_MM_EXPORT
#endif
