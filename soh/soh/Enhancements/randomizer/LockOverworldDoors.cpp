#include "RustyDoorKeys.h"
#include <libultraship/bridge/consolevariablebridge.h>
namespace Rando::RustyDoorKeys {
void PrepareNewGeneration() {
    CVarSetInteger(CatalogVersionCVar, GenerationVersion(CVarGetInteger("gRandoSettings.LockOverworldDoors", 0) != 0));
}
} // namespace Rando::RustyDoorKeys
extern "C" {
#include "variables.h"
#include "functions.h"
#include "overlays/actors/ovl_En_Door/z_en_door.h"
u8 Randomizer_GetSettingValue(RandomizerSettingKey key);
// Source scene ownership is independent of entrance shuffle targets.
int Randomizer_RustyCastleGateEnabled(PlayState* play) {
    return play && play->sceneNum == SCENE_HYRULE_CASTLE && IS_RANDO &&
           Rando::RustyDoorKeys::CastleGateUsesKey(Randomizer_GetSettingValue(RSK_LOCK_OVERWORLD_DOORS),
                                                   Randomizer_GetSettingValue(RSK_RUSTY_DOOR_KEY_VERSION));
}
int Randomizer_RustyCastleGateLocked(PlayState* play) {
    return Randomizer_RustyCastleGateEnabled(play) && !Flags_GetRandomizerInf(RAND_INF_RUSTY_KEY_HYRULE_CASTLE) &&
           !Flags_GetRandomizerInf(RAND_INF_HAS_SKELETON_KEY);
}
// The current physical scene/index is preserved independently of any shuffled exit target.
int Randomizer_RustyDoorLocked(PlayState* play, Actor* actor) {
    if (!IS_RANDO || !Randomizer_GetSettingValue(RSK_LOCK_OVERWORLD_DOORS))
        return false;
    const auto version = Randomizer_GetSettingValue(RSK_RUSTY_DOOR_KEY_VERSION);
    // Treasure-game room doors keep their key; its native scene-exit door always permits escape.
    if (Rando::RustyDoorKeys::IsExtended(version) && play->sceneNum == SCENE_TREASURE_BOX_SHOP &&
        (((u16)actor->params >> 7) & 7) == DOOR_SCENEEXIT)
        return false;
    const int item =
        Rando::RustyDoorKeys::IsExtended(version)
            ? Rando::RustyDoorKeys::SourceItem(play->sceneNum, (u16)actor->params >> 10, gSaveContext.entranceIndex)
            : Rando::RustyDoorKeys::LegacySourceItem(play->sceneNum, (u16)actor->params >> 10);
    if (item < 0)
        return false;
    const auto* key = Rando::RustyDoorKeys::Find(static_cast<RandomizerGet>(item));
    if (!key || Flags_GetRandomizerInf(key->flag) || Flags_GetRandomizerInf(RAND_INF_HAS_SKELETON_KEY))
        return false;
    // Preserve paired unlocked flags from older saved seeds.
    if (key->flag >= RAND_INF_GUARD_HOUSE_KEY_OBTAINED && key->flag <= RAND_INF_FISHING_HOLE_KEY_OBTAINED &&
        Flags_GetRandomizerInf(static_cast<RandomizerInf>(key->flag - 1)))
        return false;
    return true;
}
}
