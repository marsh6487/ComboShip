// Native solo Fill and independent native replay; no combined algorithm.
#ifdef COMBO_BUILD
namespace Rando::RustyDoorKeys {
void PrepareNewGeneration();
}
extern "C" COMBO_EXPORT const char* SOH_FillSoloRandoSeed(uint32_t seed) {
    static std::string cached;
    nlohmann::json result = { { "success", false }, { "seed", seed }, { "mode", "solo-oot" } };
    try {
        if (!gComboHeadlessRando || gComboGameMode != COMBO_GAME_OOT) {
            throw std::runtime_error("native solo fill requires headless OoT mode");
        }
        sComboRandoSeed = seed;
        sComboRandoSeedSet = true;
        auto ctx = OTRGlobals::Instance->gRandoContext;
        ctx->SetSeed(seed);
        ctx->SetSeedString(std::to_string(seed));
        ctx->SetSpoilerLoaded(false);
        ctx->ClearItemLocations();
        ctx->HintReset();
        ClearProgress();
        Rando::RustyDoorKeys::PrepareNewGeneration();
        SOH_PrepRandoContext();
        if (!sComboContextPrepared)
            throw std::runtime_error("native solo context preparation failed");
        const int filled = Fill();
        result["fillResult"] = filled;
        if (filled < 0)
            throw std::runtime_error("native solo Fill exhausted its attempts");
        ctx->GetLogic()->Reset();
        result["nativeReplayBeatable"] = CheckBeatable();
        result["placements"] = nlohmann::json::object();
        result["checks"] = nlohmann::json::object();
        for (RandomizerCheck rc : ctx->allLocations) {
            auto* location = Rando::StaticData::GetLocation(rc);
            const auto item = ctx->GetItemLocation(rc)->GetPlacedRandomizerGet();
            if (!location || location->GetName().empty() || item == RG_NONE)
                continue;
            result["placements"][location->GetName()] = Rando::StaticData::RetrieveItem(item).GetName().GetEnglish();
            result["checks"][location->GetName()] = { { "scene", location->GetScene() },
                                                      { "excluded", ctx->GetItemLocation(rc)->IsExcluded() } };
        }
        result["settings"] = nlohmann::json::parse(SOH_DumpRandoSettings());
        result["layouts"] = nlohmann::json::parse(SOH_DumpDungeonLayouts());
        result["success"] = result["nativeReplayBeatable"];
    } catch (const std::exception& e) { result["error"] = e.what(); } catch (...) {
        result["error"] = "unknown native solo fill failure";
    }
    cached = result.dump();
    return cached.c_str();
}
#endif
