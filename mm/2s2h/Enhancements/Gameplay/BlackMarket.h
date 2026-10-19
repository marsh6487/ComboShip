#pragma once
#ifdef COMBO_BUILD
#ifdef __cplusplus
#include <nlohmann/json.hpp>
void ComboBlackMarket_SaveState(nlohmann::json& save, int slot);
void ComboBlackMarket_LoadState(const nlohmann::json& save, int slot);
extern "C" {
#endif
int ComboBlackMarket_IsInterior(void);
void ComboBlackMarket_ResetState(void);
void ComboBlackMarket_NormalizeEntry(void);
void ComboBlackMarket_ApplyResume(int resume);
#ifdef __cplusplus
}
#endif
#endif
