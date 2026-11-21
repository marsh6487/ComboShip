#ifdef COMBO_BUILD

#include "SoulShuffle.h"
#include "ComboExport.h"
#include "rando/SoulTrackerData.h"
#include "static_data.h"
#include "z64.h"

extern "C" {
#include "functions.h"
#include "variables.h"
u8 Randomizer_GetSettingValue(RandomizerSettingKey key);
}

namespace {
using namespace ComboRando::SoulTracker;

struct Group {
    Family family;
    RandomizerSettingKey option;
};
constexpr Group Groups[] = {
    { Family::Enemy, RSK_SHUFFLE_ENEMY_SOULS },
    { Family::Npc, RSK_SHUFFLE_NPC_SOULS },
    { Family::Animal, RSK_SHUFFLE_ANIMAL_SOULS },
    { Family::Misc, RSK_SHUFFLE_MISC_SOULS },
};

constexpr RandomizerGet BossSouls[] = {
    RG_GOHMA_SOUL,    RG_KING_DODONGO_SOUL, RG_BARINADE_SOUL,    RG_PHANTOM_GANON_SOUL,
    RG_VOLVAGIA_SOUL, RG_MORPHA_SOUL,       RG_BONGO_BONGO_SOUL, RG_TWINROVA_SOUL,
};
} // namespace

extern "C" COMBO_EXPORT int32_t SOH_ReadSoulTrackerSave(int32_t slot, VisitRow visit, void* context) {
    if (slot < 0 || slot > 2 || gSaveContext.fileNum != slot || !IS_RANDO)
        return 0;

    auto visitItem = [&](Family family, RandomizerGet item, bool shuffled) {
        const auto flag = static_cast<RandomizerInf>(Rando::StaticData::RandoGetToRandInf.at(item));
        const bool owned = Flags_GetRandomizerInf(flag) != 0;
        if (shuffled || owned) {
            const auto& name = Rando::StaticData::RetrieveItem(item).GetName().GetEnglish();
            visit(context, family, name.c_str(), owned);
        }
    };

    const bool bosses = Randomizer_GetSettingValue(RSK_SHUFFLE_BOSS_SOULS) != RO_GENERIC_OFF;
    for (const auto item : BossSouls)
        visitItem(Family::Boss, item, bosses);
    visitItem(Family::Boss, RG_GANON_SOUL, Randomizer_GetSettingValue(RSK_GANONS_SOUL) != RO_GANONS_SOUL_STARTWITH);

    for (const Group& group : Groups) {
        const bool shuffled = Randomizer_GetSettingValue(group.option) != RO_GENERIC_OFF;
        for (const auto& soul : Rando::SoulShuffle::Catalog) {
            if (soul.option != group.option)
                continue;
            const bool owned = Flags_GetRandomizerInf(soul.flag) != 0;
            if (shuffled || owned)
                visit(context, group.family, soul.name, owned);
        }
    }

    const bool beans = Randomizer_GetSettingValue(RSK_SHUFFLE_BEAN_SOULS) != RO_GENERIC_OFF;
    for (const auto item : Rando::StaticData::beanSouls)
        visitItem(Family::Bean, item, beans);
    return 1;
}

#endif
