#include "Rando/MagicHints.h"
#include "Rando/Logic/Logic.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>

#define RI(id, article, name, type, itemId, getItemId, drawId) \
    { id, { id, #id, article, name, type, itemId, getItemId, drawId } }
namespace Rando::StaticData {
#include "items.inc"
std::map<RandoCheckId, RandoStaticCheck> Checks;
}
#undef RI

SaveContext gSaveContext;
static std::map<std::string, int32_t> configuredUpgrades;
extern "C" int32_t CVarGetInteger(const char* name, int32_t fallback) {
    auto found = configuredUpgrades.find(name);
    return found == configuredUpgrades.end() ? fallback : found->second;
}
#ifdef COMBO_BUILD
int gComboGameMode = COMBO_GAME_BOTH;
static ComboRando::ForeignItem foreign;
const ComboRando::ForeignItem* Rando::MiscBehavior::MM_LookupForeign(RandoCheckId) {
    return &foreign;
}
#endif
static RandoItemId tier = RI_SINGLE_MAGIC;
RandoItemId Rando::ConvertItem(RandoItemId item, RandoCheckId) {
    return item == RI_HOOKSHOT ? item : tier;
}
#include "saria.inc"
#include "sensor_menu.inc"
#include "placement.inc"

static bool reachable = true;
namespace Rando::Logic {
std::map<RandoRegionId, RandoRegion> Regions;
std::unordered_map<RandoRegionId, RegionTimeState> InitializeRegionTimeStates(RandoRegionId) { return {}; }
void FindReachableRegions(RandoRegionId, std::set<RandoRegionId>& regions,
                          std::unordered_map<RandoRegionId, RegionTimeState>&) {
    if (reachable)
        regions.insert(static_cast<RandoRegionId>(0));
}
RandoRegionId GetRegionIdFromEntrance(s32) { return static_cast<RandoRegionId>(0); }
}
extern "C" s32 Ship_Random(s32 first, s32) { return first; }
#include "selection.inc"

static RandoItemId Named(const char* name) {
    for (const auto& [id, item] : Rando::StaticData::Items)
        if (std::strcmp(item.name, name) == 0)
            return id;
    return RI_UNKNOWN;
}

int main() {
    using namespace Rando;
#include "../legacy_ids.inc"
    const char* names[] = { "Progressive Magic", "Magic Stat Upgrade", "Defense Upgrade",
                            "Speed Upgrade", "Power Upgrade", "Crawl Speed Upgrade",
                            "Climb Speed Upgrade", "Push Speed Upgrade" };
    const auto saria = GetSariaPriorityItemCandidates();
    const auto sensor = SensorItemChoices();
    std::vector<RandoItemId> wishes;
    for (const char* name : names) {
        auto id = Named(name);
        if (id == RI_UNKNOWN) {
            std::fprintf(stderr, "FAIL missing real MM hint catalogue entry: %s\n", name);
            return 1;
        }
        assert(std::find(saria.begin(), saria.end(), id) != saria.end());
        assert(std::find(sensor.begin(), sensor.end(),
                         std::pair<int32_t, std::string>{id, name}) != sensor.end());
        assert(SensorDesireName(id) == name);
        wishes.push_back(id);
    }

    // A progressive-family wish must remain selectable when the configured pool
    // contains concrete native tiers rather than the progressive parent.
    assert(MagicHintAvailable(RI_PROGRESSIVE_MAGIC, {RI_SINGLE_MAGIC}));
    assert(MagicHintAvailable(RI_PROGRESSIVE_MAGIC, {RI_DOUBLE_MAGIC}));
    assert(!MagicHintAvailable(RI_DOUBLE_MAGIC, {RI_SINGLE_MAGIC}));

    nlohmann::json spoiler;
    SetSariaPriorityItemsInSpoiler(spoiler, wishes);
    assert(GetSariaPriorityItemsFromSpoiler(spoiler) == wishes);
    auto save = std::make_unique<RandoSaveInfo>();
    SetSariaPriorityItemsInSave(*save, wishes);
    assert(GetSariaPriorityItemsFromSave(*save) == wishes);
    wishes.clear();
    SetSariaPriorityItemsInSave(*save, wishes);
    assert(GetSariaPriorityItemsFromSave(*save).empty());

    const auto check = static_cast<RandoCheckId>(1);
    StaticData::Checks[check] = {};
    RANDO_SAVE_CHECKS[check].shuffled = true;
    Logic::Regions[static_cast<RandoRegionId>(0)].checks[check] = { [] { return true; }, "fixture access" };
    std::vector<RandoItemId> magicWish{RI_PROGRESSIVE_MAGIC};
    SetSariaPriorityItemsInSave(gSaveContext.save.shipSaveInfo.rando, magicWish);
    RANDO_SAVE_CHECKS[check].randoItemId = RI_PROGRESSIVE_MAGIC;
    assert(FindMultiItemPlacement(RI_PROGRESSIVE_MAGIC) == std::vector<RandoCheckId>{check});
    assert(FindMultiItemPlacement(RI_SINGLE_MAGIC) == std::vector<RandoCheckId>{check});
    assert(GetProgressiveCheckInLogic() == check);
    assert(SensorFindOutstandingCheck(RI_PROGRESSIVE_MAGIC) == check);
    tier = RI_DOUBLE_MAGIC;
    assert(FindMultiItemPlacement(RI_DOUBLE_MAGIC) == std::vector<RandoCheckId>{check});
    tier = RI_JUNK;
    assert(FindMultiItemPlacement(RI_DOUBLE_MAGIC).empty());
    assert(GetProgressiveCheckInLogic() == RC_UNKNOWN);

#ifdef COMBO_BUILD
    RANDO_SAVE_CHECKS[check].randoItemId = RI_COMBO_FOREIGN;
    foreign.itemGame = ComboRando::GAME_OOT;
    const int statIds[] = { -1, COMBO_RPG_MAGIC, COMBO_RPG_DEFENSE, COMBO_RPG_SPEED,
                            COMBO_RPG_POWER, COMBO_RPG_CRAWL, COMBO_RPG_CLIMB, COMBO_RPG_PUSH };
    const char* settingNames[] = { "", "gRandoSettings.MagicStatUpgrade", "gRandoSettings.DefenseUpgrade",
                                  "gRandoSettings.SpeedUpgrade", "gRandoSettings.PowerUpgrade",
                                  "gRandoSettings.CrawlSpeedUpgrade", "gRandoSettings.ClimbSpeedUpgrade",
                                  "gRandoSettings.PushSpeedUpgrade" };
    auto& rpg = gSaveContext.save.shipSaveInfo.nei.comboRpg;
    for (size_t caseIndex = 1; caseIndex < ARRAY_COUNT(names); ++caseIndex) {
        const char* name = names[caseIndex];
        auto id = Named(name);
        foreign.itemName = name;
        foreign.trap = false;
        assert(MagicHintIdentity(RI_COMBO_FOREIGN, check) == id);
        assert(FindMultiItemPlacement(id) == std::vector<RandoCheckId>{check});
        configuredUpgrades.clear();
        assert(!MagicHintAvailable(id, {}));
        configuredUpgrades[settingNames[caseIndex]] = 1;
        assert(MagicHintAvailable(id, {}));
        std::vector<RandoItemId> rpgWish{id};
        SetSariaPriorityItemsInSave(gSaveContext.save.shipSaveInfo.rando, rpgWish);
        tier = RI_COMBO_FOREIGN;
        assert(GetProgressiveCheckInLogic() == check);
        assert(SensorFindOutstandingCheck(id) == check);
        const int stat = statIds[caseIndex];
        rpg.knownMask = rpg.enabledMask = static_cast<uint8_t>(1u << stat);
        rpg.required[stat] = 5;
        rpg.level[stat] = 4;
        assert(GetProgressiveCheckInLogic() == check);
        rpg.level[stat] = 5;
        assert(GetProgressiveCheckInLogic() == RC_UNKNOWN);
        const auto fallback = static_cast<RandoCheckId>(2);
        StaticData::Checks[fallback] = {};
        RANDO_SAVE_CHECKS[fallback].shuffled = true;
        RANDO_SAVE_CHECKS[fallback].randoItemId = RI_HOOKSHOT;
        Logic::Regions[static_cast<RandoRegionId>(0)].checks[fallback] = { [] { return true; }, "fixture access" };
        assert(GetProgressiveCheckInLogic() == fallback);
        // Plentiful copies beyond a configured cap must also yield to a useful item.
        rpg.level[stat] = 7;
        assert(GetProgressiveCheckInLogic() == fallback);
        rpg.required[stat] = 9;
        assert(GetProgressiveCheckInLogic() == check);
        rpg.required[stat] = 5;
        // Desired Items retains OoT's explicit wish for uncollected raw copies.
        assert(SensorFindOutstandingCheck(id) == check);
        rpg.level[stat] = 0;
        rpg.enabledMask = 0;
        assert(GetProgressiveCheckInLogic() == fallback);
        RANDO_SAVE_CHECKS[fallback].shuffled = false;
        assert(GetProgressiveCheckInLogic() == RC_UNKNOWN);
        // Older paired saves without mirrored RPG rules keep prior guidance.
        rpg.knownMask = 0;
        assert(GetProgressiveCheckInLogic() == check);
        RANDO_SAVE_CHECKS[check].obtained = true;
        assert(GetProgressiveCheckInLogic() == RC_UNKNOWN);
        assert(SensorFindOutstandingCheck(id) == RC_UNKNOWN);
        RANDO_SAVE_CHECKS[check].obtained = false;
        reachable = false;
        assert(GetProgressiveCheckInLogic() == RC_UNKNOWN);
        reachable = true;
        foreign.trap = true;
        assert(FindMultiItemPlacement(id).empty());
        tier = RI_JUNK;
        assert(GetProgressiveCheckInLogic() == RC_UNKNOWN);
        foreign.trap = false;
        foreign.itemGame = ComboRando::GAME_MM;
        assert(FindMultiItemPlacement(id).empty());
        foreign.itemGame = ComboRando::GAME_OOT;
    }
    gComboGameMode = COMBO_GAME_MM;
    for (const char* name : names)
        if (std::strcmp(name, "Progressive Magic") != 0)
            assert(!MagicHintAvailable(Named(name), {}));
#else
    for (const char* name : names)
        if (std::strcmp(name, "Progressive Magic") != 0)
            assert(!MagicHintAvailable(Named(name), {}));
#endif
    std::puts("PASS actual item catalogue, both menu lists, progressive tiers, saved/spoiler priorities and foreign hint identities");
}
