#include "Partners.h"
extern "C" {
#include "functions.h"
#include "variables.h"
#include "macros.h"
}
#include "soh/ResourceManagerHelpers.h"
#include <libultraship/bridge/consolevariablebridge.h>
#define NEI_MM 0
#include "../../../combo/encounters/NativeCustomEnemies.inc.h"

extern "C" void CustomEnemies_ReportSkipped(int scene, int room, int slot, int kind) {
    osSyncPrintf("[NativeEncounters] skip scene=%d room=%d slot=%d kind=%d: unsuitable collision\n", scene, room, slot,
                 kind);
}
