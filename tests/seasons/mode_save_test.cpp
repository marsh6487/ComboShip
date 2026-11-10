#include "2s2h/BenJsonConversions.hpp"
#include "ComboGameMode.h"
#include "tests/test_require.h"
#include <cstdio>

// Native storage and the combined session identity used by the linked codec.
extern "C" {
SaveContext gSaveContext{};
#ifdef COMBO_BUILD
int gComboGameMode = COMBO_GAME_BOTH;
#endif
}

int main() {
    NeiSaveData original{};
    original.seasonsOwned = 4;
    original.season = SEASON_AUTUMN;
    original.seasonsRodOwned = 1;
    original.seasonsGates = 9;
    original.ownedItems[23] = EXT_ITEM_ROD_OF_SEASONS;
    json encoded = original;
    NeiSaveData restored{};
    encoded.get_to(restored);
    REQUIRE(restored.seasonsRodOwned == 1 && restored.seasonsGates == 9);
    REQUIRE(restored.seasonsOwned == 4 && restored.season == SEASON_AUTUMN);
    REQUIRE(restored.ownedItems[23] == EXT_ITEM_ROD_OF_SEASONS);
    encoded.erase("seasonsRodOwned");
    encoded.erase("seasonsGates");
    encoded.get_to(restored);
    REQUIRE(restored.seasonsRodOwned == 0 && restored.seasonsGates == 0);
    REQUIRE(restored.seasonsOwned == 4 && restored.ownedItems[23] == EXT_ITEM_ROD_OF_SEASONS);
    RandoSaveInfo seed{};
    seed.randoSaveOptions[RO_ROD_OF_SEASONS] = NEI_SEASONS_GATED;
    seed.randoSaveOptions[RO_STARTING_ROD_OF_SEASONS] = 1;
    encoded = seed;
    RandoSaveInfo loaded{};
    encoded.get_to(loaded);
    REQUIRE(loaded.randoSaveOptions[RO_ROD_OF_SEASONS] == NEI_SEASONS_GATED);
    REQUIRE(loaded.randoSaveOptions[RO_STARTING_ROD_OF_SEASONS] == 1);
    encoded["randoSaveOptions"].erase(encoded["randoSaveOptions"].begin() + RO_ROD_OF_SEASONS,
                                    encoded["randoSaveOptions"].end());
    encoded.get_to(loaded);
    REQUIRE(loaded.randoSaveOptions[RO_ROD_OF_SEASONS] == NEI_SEASONS_INDIVIDUAL);
    REQUIRE(loaded.randoSaveOptions[RO_STARTING_ROD_OF_SEASONS] == 0);
    puts("PASS native MM save: Rod/gates/season roundtrip, legacy ownership retained and seed defaults reset");
}
