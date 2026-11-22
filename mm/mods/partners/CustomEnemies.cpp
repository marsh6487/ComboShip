#include "Partners.h"
extern "C" {
#include "functions.h"
#include "variables.h"
#include "macros.h"
}
#include "2s2h/BenPort.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include <libultraship/bridge/consolevariablebridge.h>
#define NEI_MM 1
#include "../../../combo/encounters/NativeCustomEnemies.inc.h"

namespace {
void RegisterEnemyHints() {
    static const bool registered = [] {
        GameInteractor::Instance->RegisterGameHook<GameInteractor::OnActorInit>([](Actor* actor) {
            if (actor->category == ACTORCAT_ENEMY) {
                if (const auto* hint = FindEnemyHint(actor->id)) {
                    actor->hintId = hint->mmId;
                }
            }
        });
        for (const auto& hint : CustomEncounters::EnemyHints) {
            GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(
                CustomEncounters::MmHintTextBase + hint.mmId, [hint](u16*, bool* loadFromMessageTable) {
                    if (*loadFromMessageTable) {
                        CustomMessage::LoadCustomMessageIntoFont(
                            { .textboxType = TEXTBOX_TYPE_8, .msg = std::string(hint.name) + "\n\x05" + hint.advice });
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
