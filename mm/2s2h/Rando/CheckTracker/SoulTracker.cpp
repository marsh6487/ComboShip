#include "SoulTrackerModel.h"
#include <nlohmann/json.hpp>
#include "Rando/EnemyDropProgress.h"
#include "Rando/Rando.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include "combo/ComboGameMode.h"
#include "combo/gui/SoulTrackerView.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/Context.h>
#include <ship/window/Window.h>
#include <ship/window/gui/Gui.h>
#include <ship/window/gui/GuiWindow.h>
#include <imgui.h>
#include <algorithm>
#include <memory>
#include <string>
#include <string_view>

#ifdef COMBO_BUILD
#include "ComboExport.h"
#include "ComboMenuSharedContext.h"
#include "rando/SoulTrackerData.h"
bool Combo_MmIsForeground(void);
#endif

extern "C" {
#include "functions.h"
#include "variables.h"
}

namespace {
using namespace Rando::SoulTracker;
using namespace ComboRando::SoulTracker;

bool sOpen = false;
std::size_t sSection = static_cast<std::size_t>(Family::Npc);
std::size_t sPage = 0;
constexpr const char* Sections[] = {
    "Bosses", "Enemies", "NPCs / shopkeepers", "Animals", "Miscellaneous", FirstSlainHeading,
};
constexpr std::size_t FirstSlainSection = std::size(Sections) - 1;

bool FirstSlainEnabled() {
    return RANDO_SAVE_OPTIONS[RO_SHUFFLE_ENEMY_DROPS] != 0;
}

int32_t ReadMmFirstSlain(int32_t slot, VisitFirstSlain visit, void* context) {
    if (slot < 0 || slot > 2 || gSaveContext.fileNum != slot || !IS_RANDO || !FirstSlainEnabled())
        return 0;

    constexpr std::string_view prefix = "Enemy Drop ";
    const auto& rando = gSaveContext.save.shipSaveInfo.rando;
    for (int id = RC_ENEMY_DROP_ALIEN; id <= RC_ENEMY_DROP_WOLFOS; ++id) {
        const auto& check = rando.randoSaveChecks[id];
        if (!check.shuffled || check.skipped)
            continue;
        const auto& name = Rando::StaticData::CheckNames[id];
        const bool defeated =
            rando.firstSlainEnemies & Rando::EnemyDropProgress::DefeatedBit(static_cast<RandoCheckId>(id));
        visit(context, name.c_str() + (name.starts_with(prefix) ? prefix.size() : 0),
              FirstSlainProgress(defeated || check.eligible, check.obtained));
    }
    return 1;
}

bool IsMapReady() {
    if (!Combo_IsCombinedGame() && !CVarGetInteger(SoloPauseMapCvar, 1))
        return false;
#ifdef COMBO_BUILD
    if (!Combo_MmIsForeground())
        return false;
#endif
    if (!gPlayState || gGameState != &gPlayState->state || gGameState->main != Play_Main || !gGameState->running ||
        !IS_RANDO)
        return false;
    const auto& pause = gPlayState->pauseCtx;
    return pause.state == PAUSE_STATE_MAIN && pause.pageIndex == PAUSE_MAP &&
           pause.mainState == PAUSE_MAIN_STATE_IDLE && pause.mapPageRoll == 0 &&
           pause.debugEditor == DEBUG_EDITOR_NONE && !pause.itemDescriptionOn;
}

bool HaveSoulOptions() {
    bool enabled = false;
    ForEach([&](const Entry& entry) { enabled = enabled || RANDO_SAVE_OPTIONS[entry.option] != 0; });
    return enabled;
}

const char* SoulName(const Entry& entry) {
    return entry.name ? entry.name : Rando::StaticData::Items.at(entry.item).name;
}

struct PauseRow {
    const char* name;
    const char* label;
    bool recorded;
};

std::vector<PauseRow> CurrentRows() {
    std::vector<PauseRow> rows;
    if (sSection == FirstSlainSection) {
        const auto append = [](void* context, const char* name, FirstSlainStatus status) {
            static_cast<std::vector<PauseRow>*>(context)->push_back(
                { name, FirstSlainLabels[static_cast<std::size_t>(status)], status != FirstSlainStatus::Unrecorded });
        };
        ReadMmFirstSlain(gSaveContext.fileNum, append, &rows);
    } else {
        for (const auto& entry : Rows(static_cast<Family>(sSection),
                                      [](RandoOptionId option) { return RANDO_SAVE_OPTIONS[option] != 0; })) {
            const bool owned = Flags_GetRandoInf(entry.flag) != 0;
            rows.push_back({ SoulName(entry), owned ? "Owned" : "Missing", owned });
        }
    }
    return rows;
}

void Consume(Input& input, u16 buttons) {
    input.cur.button &= ~buttons;
    input.press.button &= ~buttons;
    input.rel.button &= ~buttons;
}

// This hook runs before native Kaleido and the debug-editor L shortcut. No
// in-game map/cursor/Soaring action receives the overlay's navigation buttons.
void HandleInput() {
    if (!IsMapReady()) {
        sOpen = false;
        return;
    }
    const bool souls = HaveSoulOptions();
    const bool firstSlain = FirstSlainEnabled();
    if (!souls && !firstSlain) {
        sOpen = false;
        return;
    }
    const std::size_t sections = std::size(Sections) - !firstSlain;
    sSection = souls ? std::min(sSection, sections - 1) : FirstSlainSection;
    auto gui = Ship::Context::GetRawInstance()->GetWindow()->GetGui();
    if (!gui || gui->GetMenuOrMenubarVisible()) {
        sOpen = false;
        return;
    }
    Input& input = gPlayState->state.input[0];
    if (CHECK_BTN_ALL(input.press.button, BTN_L)) {
        sOpen = !sOpen;
        sPage = 0;
        Consume(input, BTN_L);
        Audio_PlaySfx(NA_SE_SY_DECIDE);
    }
    if (!sOpen)
        return;
    if (CHECK_BTN_ANY(input.press.button, BTN_B | BTN_START)) {
        sOpen = false;
        Consume(input, BTN_B | BTN_START);
        return;
    }
    if (souls && CHECK_BTN_ALL(input.press.button, BTN_DLEFT)) {
        sSection = (sSection + sections - 1) % sections;
        sPage = 0;
    } else if (souls && CHECK_BTN_ALL(input.press.button, BTN_DRIGHT)) {
        sSection = (sSection + 1) % sections;
        sPage = 0;
    }
    const auto pages = PageCount(CurrentRows().size());
    if (CHECK_BTN_ALL(input.press.button, BTN_DUP))
        sPage = (sPage + pages - 1) % pages;
    else if (CHECK_BTN_ALL(input.press.button, BTN_DDOWN))
        sPage = (sPage + 1) % pages;
    Consume(input, BTN_A | BTN_B | BTN_START | BTN_R | BTN_Z | BTN_CUP | BTN_CDOWN | BTN_CLEFT | BTN_CRIGHT | BTN_DUP |
                       BTN_DDOWN | BTN_DLEFT | BTN_DRIGHT);
    input.cur.stick_x = input.cur.stick_y = input.rel.stick_x = input.rel.stick_y = 0;
    gPlayState->pauseCtx.stickAdjX = gPlayState->pauseCtx.stickAdjY = 0;
}

class SoulsWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    void InitElement() override {
    }
    void DrawElement() override {
    }
    void UpdateElement() override {
    }
    void Draw() override {
#ifdef COMBO_BUILD
        ComboMenuContext::UseSharedImGuiContext();
#endif
        if (!IsMapReady() || (!HaveSoulOptions() && !FirstSlainEnabled()) || !ImGui::GetCurrentContext())
            return;
        auto gui = Ship::Context::GetRawInstance()->GetWindow()->GetGui();
        if (!gui || gui->GetMenuOrMenubarVisible())
            return;
        auto* viewport = ImGui::GetMainViewport();
        if (!viewport)
            return;
        auto* draw = ImGui::GetForegroundDrawList(viewport);
        const float scale = std::max(0.6f, std::min(viewport->Size.x / 960.0f, viewport->Size.y / 720.0f));
        const float font = ImGui::GetFontSize() * scale;
        const ImVec2 origin(viewport->Pos.x + viewport->Size.x * 0.5f - 250.0f * scale,
                            viewport->Pos.y + viewport->Size.y * 0.5f - 195.0f * scale);
        auto text = [&](float x, float y, ImU32 color, const std::string& value) {
            draw->AddText(ImGui::GetFont(), font, ImVec2(origin.x + x * scale, origin.y + y * scale), color,
                          value.c_str());
        };
        if (!sOpen) {
            text(0, 385, IM_COL32(255, 255, 220, 255), "L: Souls Tracker");
            return;
        }
        draw->AddRectFilled(ImVec2(origin.x - 12 * scale, origin.y - 12 * scale),
                            ImVec2(origin.x + 512 * scale, origin.y + 405 * scale), IM_COL32(12, 18, 29, 245),
                            8.0f * scale);
        auto rows = CurrentRows();
        sPage = std::min(sPage, PageCount(rows.size()) - 1);
        const bool firstSlain = sSection == FirstSlainSection;
        const auto recorded = std::count_if(rows.begin(), rows.end(), [](const PauseRow& row) { return row.recorded; });
        text(0, 0, IM_COL32(225, 235, 255, 255), "Souls Tracker - MM");
        text(0, 28, IM_COL32(240, 215, 155, 255),
             std::string(Sections[sSection]) + "  " + std::to_string(recorded) + "/" + std::to_string(rows.size()));
        if (rows.empty())
            text(0, 65, IM_COL32(190, 195, 205, 255),
                 firstSlain ? "No first-slay checks in this seed." : "This soul family is not shuffled in this save.");
        const ImU32 accent = firstSlain ? FirstSlainAccent : IM_COL32(140, 235, 165, 255);
        for (std::size_t row = 0; row < RowsPerPage && sPage * RowsPerPage + row < rows.size(); ++row) {
            const auto& entry = rows[sPage * RowsPerPage + row];
            text(0, 62 + row * 23, entry.recorded ? accent : IM_COL32(185, 190, 200, 255),
                 std::string("[") + entry.label + "]  " + entry.name);
        }
        text(0, 348, IM_COL32(210, 220, 235, 255),
             "Page " + std::to_string(sPage + 1) + "/" + std::to_string(PageCount(rows.size())));
        if (firstSlain)
            text(130, 348, FirstSlainAccent, FirstSlainLegend);
        text(0, 373, IM_COL32(210, 220, 235, 255), "D-pad Left/Right: family   Up/Down: page   L/B: back");
    }
};

std::shared_ptr<SoulsWindow> sWindow;
void EnsureWindow() {
    if (sWindow)
        return;
    auto* ctx = Ship::Context::GetRawInstance();
    if (!ctx || !ctx->GetWindow() || !ctx->GetWindow()->GetGui())
        return;
    sWindow = std::make_shared<SoulsWindow>("gSoulsMapOverlayMM", "Collected Souls##MM");
    ctx->GetWindow()->GetGui()->AddGuiWindow(sWindow);
}

RegisterShipInitFunc init([] {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnInterfaceDrawStart>(EnsureWindow);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateMainStart>(HandleInput);
});
} // namespace

#ifdef COMBO_BUILD
extern "C" COMBO_EXPORT int32_t MM_ReadSoulTrackerSave(int32_t slot, ComboRando::SoulTracker::VisitRow visit,
                                                       void* context) {
    if (slot < 0 || slot > 2 || gSaveContext.fileNum != slot || !IS_RANDO)
        return 0;

    Rando::SoulTracker::ForEach([&](const Rando::SoulTracker::Entry& entry) {
        const bool owned = Flags_GetRandoInf(entry.flag) != 0;
        if (RANDO_SAVE_OPTIONS[entry.option] || owned)
            visit(context, entry.family, SoulName(entry), owned);
    });
    return 1;
}

extern "C" COMBO_EXPORT int32_t MM_ReadFirstSlainSave(int32_t slot, VisitFirstSlain visit, void* context) {
    return ReadMmFirstSlain(slot, visit, context);
}
#endif
