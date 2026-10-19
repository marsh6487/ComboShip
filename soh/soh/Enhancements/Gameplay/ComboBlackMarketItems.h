#pragma once
#ifdef COMBO_BUILD
#include "rando/BlackMarket.h"
#include "soh/Enhancements/randomizer/randomizerEnums.h"
namespace ComboBlackMarketOotItems {
RandomizerGet Id(const ComboRando::BlackMarket::Item& item);
bool Owned(const ComboRando::BlackMarket::Item& item);
bool Available(const ComboRando::BlackMarket::Item& item);
bool Grant(const ComboRando::BlackMarket::Item& item);
} // namespace ComboBlackMarketOotItems
#endif
