#include "2s2h/BenJsonConversions.hpp"
#include "tests/test_require.h"
#include <cstdio>
// Native storage for the linked SoloEntrances implementation; JSON remains production code.
extern "C" {
SaveContext gSaveContext{};
#ifdef COMBO_BUILD
int gComboGameMode = COMBO_GAME_BOTH;
#endif
}
int main() {
    RandoSaveInfo source{};
    for (size_t i = 0; i < ARRAY_COUNT(source.randoInf); ++i) source.randoInf[i] = i + 1;
    json current = source;
    RandoSaveInfo loaded{};
    current.get_to(loaded);
    REQUIRE(memcmp(source.randoInf, loaded.randoInf, sizeof(source.randoInf)) == 0);
    json old = current;
    old["randoInf"] = std::vector<u16>{1, 2};
    try { old.get_to(loaded); }
    catch (const json::exception&) { puts("FAIL: old shorter native randoInf array cannot load"); return 1; }
    REQUIRE(loaded.randoInf[0] == 1 && loaded.randoInf[1] == 2);
    for (size_t i = 2; i < ARRAY_COUNT(loaded.randoInf); ++i) REQUIRE(loaded.randoInf[i] == 0);
    current["randoInf"].push_back(65535);
    current.get_to(loaded);
    REQUIRE(memcmp(source.randoInf, loaded.randoInf, sizeof(source.randoInf)) == 0);
    old["randoInf"] = json::array();
    old.get_to(loaded);
    for (auto flag : loaded.randoInf) REQUIRE(flag == 0);
    puts("PASS native MM soul flags: roundtrip, shorter legacy/reused/empty tails, longer future tail");
}
