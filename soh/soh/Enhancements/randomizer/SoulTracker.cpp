#include "SoulTracker.h"
#include "EnemyFirstDrops.h"
#include "SoulShuffle.h"
#include "combo/gui/SoulTrackerView.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/cvar_prefixes.h"
#include <ship/window/gui/Gui.h>
#include <ship/window/gui/GuiWindow.h>
#include <memory>
#ifdef COMBO_BUILD
#include "ComboExport.h"
#endif
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
int32_t ReadOotSave(int32_t slot, VisitRow visit, void* context, bool includeBeanPatches) {
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

    if (includeBeanPatches) {
        const bool beans = Randomizer_GetSettingValue(RSK_SHUFFLE_BEAN_SOULS) != RO_GENERIC_OFF;
        for (const auto item : Rando::StaticData::beanSouls)
            visitItem(Family::Bean, item, beans);
    }
    return 1;
}

int32_t ReadOotFirstSlain(int32_t slot, VisitFirstSlain visit, void* context) {
    if (slot < 0 || slot > 2 || gSaveContext.fileNum != slot || !IS_RANDO ||
        !Randomizer_GetSettingValue(RSK_SHUFFLE_ENEMY_DROPS))
        return 0;

    namespace Drops = Rando::EnemyFirstDrops;
    for (const auto& entry : Drops::Catalog) {
        if (Drops::HasSeededCheck(entry.check) && Drops::IsAvailable(entry.check, Drops::SeedUsesMQ)) {
            visit(context, entry.name,
                  FirstSlainProgress(Flags_GetRandomizerInf(entry.defeatedFlag), Flags_GetRandomizerInf(entry.flag)));
        }
    }
    return 1;
}

class SoulWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

  protected:
    void InitElement() override {
    }
    void UpdateElement() override {
    }
    void DrawElement() override {
        if (!GameInteractor::IsSaveLoaded() || !IS_RANDO) {
            ImGui::TextDisabled("Load a randomizer save to view your souls.");
            return;
        }

        ImGui::Checkbox("Show missing souls", &mShowMissing);
        const auto read = [](int32_t slot, VisitRow visit, void* context) {
            return ReadOotSave(slot, visit, context, true);
        };
        DrawFirstSlain(gSaveContext.fileNum, ReadOotFirstSlain);
        DrawGame(gSaveContext.fileNum, read, mShowMissing);
    }

  private:
    bool mShowMissing = false;
};
} // namespace

void Rando::SoulTracker::RegisterWindow(Ship::Gui& gui) {
    gui.AddGuiWindow(std::make_shared<SoulWindow>(CVAR_WINDOW("SoulTracker"), WindowName, ImVec2(440.0f, 450.0f),
                                                  ImGuiWindowFlags_NoFocusOnAppearing));
}

#ifdef COMBO_BUILD
extern "C" COMBO_EXPORT int32_t SOH_ReadSoulTrackerSave(int32_t slot, VisitRow visit, void* context) {
    return ReadOotSave(slot, visit, context, false);
}

extern "C" COMBO_EXPORT int32_t SOH_ReadFirstSlainSave(int32_t slot, VisitFirstSlain visit, void* context) {
    return ReadOotFirstSlain(slot, visit, context);
}
#endif
