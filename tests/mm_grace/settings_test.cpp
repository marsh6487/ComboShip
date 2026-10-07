#include "2s2h/Rando/Rando.h"
#include "NeiGracePolicy.h"
#include "tests/test_require.h"
#include <cstdio>
#include <cstring>

#define COMBO_EXPORT
#define CVAR_RANDOMIZER_SETTING(s) "gRandoSettings." s
static nlohmann::json config = nlohmann::json::object();
static int appliedMode, appliedRewards, creationResult, g_MmSaveInMemorySlot = -1;

extern "C" {
int32_t CVarGetInteger(const char* key, int32_t fallback) { return config.value(key, fallback); }
void CVarSetInteger(const char* key, int32_t value) { config[key] = value; }
void CVarSetString(const char* key, const char* value) { config[key] = value; }
}
namespace Rando {
void SetExcludedChecksInConfig(std::vector<RandoCheckId>&) {}
void SetStartingItemsInConfig(std::vector<RandoItemId>&) {}
namespace StaticData {
RandoCheckId GetCheckIdFromName(const char*) { return RC_UNKNOWN; }
RandoItemId GetItemIdFromName(const char*) { return RI_UNKNOWN; }
}
}

namespace ComboRando {
enum { GAME_MM = 1 };
// Placement translation is unrelated to settings; the engine boundary receives
// this seed's own MM placement object, unchanged by the fixture.
nlohmann::json ApplyPayloadFromConsolidated(const nlohmann::json& seed, int) { return seed["mm"]["placements"]; }
}

static const char* MM_DumpRandoSettings() {
    static std::string snapshot; snapshot = config.dump(); return snapshot.c_str();
}
static void MM_SetCheckPrices(const char*) {}
static void SOH_GetCurrentPlayerName(unsigned char* name) { std::memset(name, 0x3E, 8); }
static int MM_InitRandoSaveFile(int, const char*, const unsigned char*) {
    appliedMode = CVarGetInteger("gRando.Options.RO_HYLIAS_GRACE", NEI_GRACE_OFF);
    appliedRewards = CVarGetInteger("gRando.Options.RO_HYLIAS_GRACE_REWARDS", 4);
    return creationResult;
}

#include "settings_production.inc"

static void SeedApplication() {
    config["gRando.Options.RO_HYLIAS_GRACE"] = NEI_GRACE_ON;
    config["gRando.Options.RO_HYLIAS_GRACE_REWARDS"] = 1;
    nlohmann::json seed = {{"mm", {{"placements", nlohmann::json::object()},
        {"settings", {{"gRando.Options.RO_HYLIAS_GRACE", NEI_GRACE_GATED},
                      {"gRando.Options.RO_HYLIAS_GRACE_REWARDS", 7}}}}}};
    REQUIRE(Combo_WriteMMSaveForSlot(2, seed));
    REQUIRE(appliedMode == NEI_GRACE_GATED && appliedRewards == 7 && g_MmSaveInMemorySlot == 2);
    REQUIRE(CVarGetInteger("gRando.Options.RO_HYLIAS_GRACE", -1) == NEI_GRACE_ON);
    REQUIRE(CVarGetInteger("gRando.Options.RO_HYLIAS_GRACE_REWARDS", -1) == 1);
    creationResult = -1;
    REQUIRE(!Combo_WriteMMSaveForSlot(1, seed));
    REQUIRE(appliedMode == NEI_GRACE_GATED && appliedRewards == 7);
    REQUIRE(CVarGetInteger("gRando.Options.RO_HYLIAS_GRACE", -1) == NEI_GRACE_ON);
    creationResult = 0;
    seed["mm"]["settings"] = nlohmann::json::object(); // Legacy consolidated seed.
    config["gRando.Options.RO_HYLIAS_GRACE"] = NEI_GRACE_OFF;
    REQUIRE(Combo_WriteMMSaveForSlot(1, seed));
    REQUIRE(appliedMode == NEI_GRACE_ON && appliedRewards == 4);
    REQUIRE(CVarGetInteger("gRando.Options.RO_HYLIAS_GRACE", -1) == NEI_GRACE_OFF);
    puts("PASS fresh/repaired MM save uses seed policy after UI edits; UI restores on success/failure, legacy seeds remain On");
}

int main(int argc, char** argv) {
    if (argc > 1 && !std::strcmp(argv[1], "--apply-later")) { SeedApplication(); return 0; }
    for (int mode = NEI_GRACE_ON; mode <= NEI_GRACE_GATED; ++mode) {
        config["gCombo.Sync.Enabled"] = 0;
        config["gRando.Options.RO_HYLIAS_GRACE"] = mode;
        config["gRando.Options.RO_HYLIAS_GRACE_REWARDS"] = 8;
        config["gRandoSettings.HyliasGrace"] = (mode + 1) % 3;
        config["gRandoSettings.HyliasGraceRewards"] = 1;
        SOH_NormalizeComboGraceFromMM();
        REQUIRE(CVarGetInteger("gRandoSettings.HyliasGrace", -1) == mode);
        REQUIRE(CVarGetInteger("gRandoSettings.HyliasGraceRewards", -1) == 8);
        REQUIRE(CVarGetInteger("gCombo.Sync.Enabled", -1) == 0);
    }
    config["gRando.Options.RO_HYLIAS_GRACE"] = 99;
    config["gRando.Options.RO_HYLIAS_GRACE_REWARDS"] = 100;
    SOH_NormalizeComboGraceFromMM();
    REQUIRE(CVarGetInteger("gRandoSettings.HyliasGrace", -1) == NEI_GRACE_OFF);
    REQUIRE(CVarGetInteger("gRandoSettings.HyliasGraceRewards", -1) == 13);
    MM_RestoreRandoSettings("{}"); SOH_RestoreRandoSettings("{}");
    REQUIRE(CVarGetInteger("gRando.Options.RO_HYLIAS_GRACE", -1) == NEI_GRACE_ON);
    REQUIRE(CVarGetInteger("gRandoSettings.HyliasGrace", -1) == NEI_GRACE_ON);
    puts("PASS native combined generation normalization with sync Off, validation and legacy replay policy");
    SeedApplication();
}
