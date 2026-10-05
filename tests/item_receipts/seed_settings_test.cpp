// Native settings/storage probe. run_seed_settings_tests.py supplies verbatim
// production boundaries. This does not run the fill worker or either game loop.
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include "z64.h"
#include "soh/SohGui/UIWidgets.hpp"
#include "soh/cvar_prefixes.h"
#include "soh/Enhancements/randomizer/entrance.h"
#include "soh/Enhancements/randomizer/logic.h"
#include "soh/Enhancements/randomizer/dungeon.h"
#include "soh/Enhancements/randomizer/trial.h"
#include "soh/Enhancements/randomizer/fishsanity.h"
#include "soh/Enhancements/randomizer/rng.h"
#include "rando/SharedItems.h"
#include "ComboExport.h"
// Expose only native registration/section state to this synchronous fixture.
#define private public
#include "soh/Enhancements/randomizer/settings.h"
#include "soh/Enhancements/randomizer/static_data.h"
#include "soh/SaveManager.h"
#undef private

using json = nlohmann::json;
extern "C" {
SaveContext gSaveContext{};
int gComboGoalHunt = 0, gComboGoalRequired = 0, gComboGoalPieces = 0;
int gComboStartingGameMM = 1, gComboSharedMask = 0;
}

// CVar storage stands in for the shared console service; native Option reads
// and writes it through its real configured name/default.
static std::map<std::string, int> integerCVars;
static std::map<std::string, std::string> stringCVars;
extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    const auto found = integerCVars.find(key);
    return found == integerCVars.end() ? fallback : found->second;
}
extern "C" void CVarSetInteger(const char* key, int32_t value) { integerCVars[key] = value; }
extern "C" const char* CVarGetString(const char* key, const char* fallback) {
    const auto found = stringCVars.find(key);
    return found == stringCVars.end() ? fallback : found->second.c_str();
}
extern "C" void CVarSetString(const char* key, const char* value) { stringCVars[key] = value; }

// Pack availability is a fixture input to the complete native finalizer.
struct OTRGlobals {
    static OTRGlobals* Instance;
    bool HasOriginal() const { return true; }
    bool HasMasterQuest() const { return true; }
};
static OTRGlobals globals;
OTRGlobals* OTRGlobals::Instance = &globals;

namespace Rando {
std::weak_ptr<Context> Context::mContext;
std::shared_ptr<Settings> Settings::mInstance;
std::array<Location, RC_MAX> StaticData::locationTable;
Location* StaticData::GetLocation(RandomizerCheck rc) { return &locationTable[rc]; }
std::unordered_map<uint32_t, RandomizerHintTextKey> StaticData::trialData;
std::array<std::pair<RandomizerCheck, RandomizerCheck>, 17> StaticData::randomizerFishingPondFish{};
}

SaveManager* SaveManager::Instance = nullptr;
SaveManager::SaveManager() {}
#include "seed_settings_production.inc"

static void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

static void ResetContext(std::shared_ptr<Rando::Context>& context) {
    context->GetLogic()->SetContext(nullptr);
    Rando::Settings::GetInstance()->ClearContext();
    context.reset();
    context = Rando::Context::CreateInstance();
    Rando::Settings::GetInstance()->AssignContext(context);
}

int main() {
    SaveManager storage;
    SaveManager::Instance = &storage;
    auto settings = Rando::Settings::GetInstance();
    RegisterInformationOption(*settings);
    auto context = Rando::Context::CreateInstance();
    settings->AssignContext(context);
    auto& option = settings->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION);
    Check(option.GetCVarName() == "gRandoSettings.MapsCompassesGiveInformation", "production CVar prefix/name");
    Check(option.GetMenuOptionDefault() == RO_GENERIC_OFF && option.GetOptionIndex() == RO_GENERIC_OFF,
          "native checkbox default is Off");
    Check(settings->PopulateOptionNameToEnum().at("Maps and Compasses Give Information") ==
              RSK_MAPS_COMPASSES_GIVE_INFORMATION, "native option-name translation includes appended option");
    static_assert(RSK_MAPS_COMPASSES_GIVE_INFORMATION + 1 == RSK_MAX);

    // Fresh generation follows the actual critical order: native options copy,
    // native finalizer (including the MM-start forces), then settings snapshot
    // and native save array. Exercise multiple seeds in one process.
    for (int enabled : {0, 1, 0, 1}) {
        CVarSetInteger(option.GetCVarName().c_str(), enabled);
        settings->SetAllToContext();
        context->FinalizeSettings({}, {});
        context->SetSeedGenerated(true);
        Check(context->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Get() == enabled,
              "generation CVar survived real SetAllToContext and MM-start FinalizeSettings");
        Check(context->GetOption(RSK_SELECTED_STARTING_AGE).Is(RO_AGE_CHILD), "MM start selected child age");
        const auto baked = json::parse(SOH_DumpRandoSettings());
        Check(baked.at(option.GetCVarName()).get<int>() == enabled, "consolidated snapshot contains seed choice");
        json section = json::object();
        storage.currentJsonContext = &section;
        SaveNativeSettings();
        Check(section.at("randoSettings").size() == RSK_MAX &&
                  section.at("randoSettings").at(RSK_MAPS_COMPASSES_GIVE_INFORMATION) == enabled,
              "native save array includes authoritative appended value");
        // Opening a file recreates Context and loads the saved section. The
        // current menu can disagree; receipt queries must see the saved value.
        CVarSetInteger(option.GetCVarName().c_str(), !enabled);
        ResetContext(context);
        section = json::parse(section.dump());
        storage.currentJsonContext = &section;
        LoadNativeSettings();
        Check(context->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Get() == enabled,
              "save reload restores seed value independently of live menu");
    }

    // Reload/drop restore feeds the same native generation prep. An old blob
    // without this newly appended key must not inherit a local checked box.
    for (const auto& snapshot : {json{{option.GetCVarName(), 1}}, json::object(),
                                 json{{option.GetCVarName(), 0}}}) {
        CVarSetInteger(option.GetCVarName().c_str(), 1);
        SOH_RestoreRandoSettings(snapshot.dump().c_str());
        settings->SetAllToContext();
        context->FinalizeSettings({}, {});
        Check(context->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Get() ==
                  snapshot.value(option.GetCVarName(), 0), "restored/old seed choice remains authoritative");
    }
    json oldSection{{"randoSettings", json::array()}};
    oldSection["randoSettings"] = std::vector<int>(RSK_MAPS_COMPASSES_GIVE_INFORMATION, 0);
    context->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Set(1);
    storage.currentJsonContext = &oldSection;
    LoadNativeSettings();
    Check(context->GetOption(RSK_MAPS_COMPASSES_GIVE_INFORMATION).Is(0), "old save array defaults missing option Off");
    context->GetLogic()->SetContext(nullptr);
    std::puts("PASS native CVar registration, MM-start generation prep, seed snapshot, save/reload, seed changes and old defaults");
}
