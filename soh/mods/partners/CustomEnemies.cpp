#include "Partners.h"
extern "C" {
#include "functions.h"
#include "variables.h"
#include "macros.h"
}
#include "soh/ResourceManagerHelpers.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include <algorithm>
#include <libultraship/bridge/consolevariablebridge.h>
#define NEI_MM 0
#include "../../../combo/encounters/NativeCustomEnemies.inc.h"

namespace {
void RegisterEnemyHints() {
    static const bool registered = [] {
        GameInteractor::Instance->RegisterGameHook<GameInteractor::OnActorInit>([](void* actorPtr) {
            auto* actor = static_cast<Actor*>(actorPtr);
            if (actor->category == ACTORCAT_ENEMY) {
                if (const auto* hint = FindEnemyHint(actor->id)) {
                    actor->naviEnemyId = hint->ootId;
                }
            }
        });
        for (const auto& hint : CustomEncounters::EnemyHints) {
            GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(
                CustomEncounters::OotHintTextBase + hint.ootId, [hint](u16*, bool* loadFromMessageTable) {
                    if (*loadFromMessageTable) {
                        std::string text = std::string(hint.name) + "&%c" + hint.advice + "%w";
                        std::replace(text.begin(), text.end(), '\n', '&');
                        CustomMessage message(text, text, text, TEXTBOX_TYPE_BLUE, TEXTBOX_POS_VARIABLE);
                        message.AutoFormat();
                        message.LoadIntoFont();
                        *loadFromMessageTable = false;
                    }
                });
        }
        return true;
    }();
}

RegisterShipInitFunc sRegisterEnemyHints(RegisterEnemyHints);
} // namespace

extern "C" void CustomEnemies_ReportSkipped(int scene, int room, int slot, int kind) {
    osSyncPrintf("[NativeEncounters] skip scene=%d room=%d slot=%d kind=%d: unsuitable collision\n", scene, room, slot,
                 kind);
}
