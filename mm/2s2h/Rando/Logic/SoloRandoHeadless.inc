// Actual shared native generation, without the file-create UI/audio/write tail.
#include "Rando/SoloEntrances.h"
extern "C" int gFcCombo_SuppressRecord;
extern "C" COMBO_EXPORT const char* MM_FillSoloRandoSeed(uint32_t seed) {
    static std::string cached;
    nlohmann::json result = { { "success", false }, { "seed", seed }, { "mode", "solo-mm" } };
    try {
        if (gComboGameMode != COMBO_GAME_MM || OTRGlobals::Instance || sMM_OracleActive)
            throw std::runtime_error("native solo fill requires an idle headless MM session");
        if (CVarGetInteger("gRando.Options.RO_LOGIC", RO_LOGIC_GLITCHLESS) != RO_LOGIC_GLITCHLESS)
            throw std::runtime_error("native solo headless fill requires Glitchless logic");
        const auto previousSeed = sMMComboRandoSeed;
        const auto previousSet = sMMComboRandoSeedSet;
        struct RestoreNativeState {
            PlayState* play;
            uint64_t seed;
            bool seedSet;
            bool dormantGive = Rando::gComboDormantGive;
            int suppressRecord = gFcCombo_SuppressRecord;
            decltype(sMMComboCheckPrices) checkPrices = sMMComboCheckPrices;
            ~RestoreNativeState() {
                gPlayState = play;
                Rando::gComboDormantGive = dormantGive;
                gFcCombo_SuppressRecord = suppressRecord;
                sMMComboCheckPrices = std::move(checkPrices);
                Combo_MM_Rando_Restore();
                sMMComboRandoSeed = seed;
                sMMComboRandoSeedSet = seedSet;
            }
        } restore{ gPlayState, previousSeed, previousSet };
        // Reset parses native settings/starting items and can throw after its
        // live-save snapshot; cover setup with the same transaction as filling.
        MM_SetComboRandoSeed(seed);
        Combo_MM_Rando_Reset();
        gPlayState = nullptr;
        // Existing offscreen presentation/trap policy; retain real ownership accounting.
        Rando::gComboDormantGive = true;
        Sram_InitNewSave();
        Rando::PrepareNewRandomizedSave();
        result["inputSeed"] = Rando::GenerateSeedInSaveContext(std::to_string(seed));
        result["finalSeed"] = gSaveContext.save.shipSaveInfo.rando.finalSeed;
        result["soloEntrances"] = Rando::SoloEntrances::Seed(gSaveContext.save.shipSaveInfo.rando);
        result["placements"] = nlohmann::json::object();
        result["prices"] = nlohmann::json::object();
        result["checks"] = nlohmann::json::object();
        for (const auto& [id, check] : Rando::StaticData::Checks) {
            if (id == RC_UNKNOWN)
                continue;
            const auto& name = Rando::StaticData::GetCheckDisplayName(id);
            const auto& saved = RANDO_SAVE_CHECKS[id];
            const auto item = saved.shuffled ? saved.randoItemId : check.randoItemId;
            if (name.empty())
                continue;
            // Native generation owns these rolls. Include zero and every check
            // type so replay never reuses stale probe prices or rerolls them.
            result["prices"][name] = saved.price;
            if (item == RI_UNKNOWN)
                continue;
            result["placements"][name] = Rando::StaticData::GetItemDisplayName(item);
            result["checks"][name] = { { "scene", check.sceneId },
                                       { "sourceName", check.name },
                                       { "shuffled", saved.shuffled } };
        }
        result["settings"] = nlohmann::json::parse(MM_DumpRandoSettings());
        result["success"] = true;
    } catch (const std::exception& e) { result["error"] = e.what(); } catch (...) {
        result["error"] = "unknown native MM solo fill failure";
    }
    cached = result.dump();
    return cached.c_str();
}
