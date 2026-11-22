#include "SongEvents.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
#include "functions.h"
#include "overlays/actors/ovl_En_Okarina_Tag/z_en_okarina_tag.h"
#include "variables.h"
extern PlayState* gPlayState;
u8 Randomizer_GetSettingValue(RandomizerSettingKey key);
}

#ifdef COMBO_BUILD
bool Combo_OotIsForeground(void);
#endif

namespace {
namespace SongMarkerHost {
constexpr char kCvar[] = "gSongEventMarkerOotWindow";
constexpr char kName[] = "OoT Song Event Markers";
constexpr int kActorCategories[] = { ACTORCAT_PROP };

bool IsEnabled() {
#ifdef COMBO_BUILD
    if (!Combo_OotIsForeground())
        return false;
#endif
    return IS_RANDO && Randomizer_GetSettingValue(RSK_SHUFFLE_SONG_EVENTS);
}

bool IsMirrored() {
    return CVarGetInteger("gEnhancements.MirroredWorld", 0) != 0;
}

Actor* FirstActor(PlayState* play, int category) {
    return play->actorCtx.actorLists[category].head;
}

int MarkerSong(Actor* actor, Player*, PlayState* play, Vec3f& position) {
    if (actor->id != ACTOR_EN_OKARINA_TAG)
        return -1;
    auto* tag = reinterpret_cast<EnOkarinaTag*>(actor);
    const int action = EnOkarinaTag_GetPendingSongAction(tag, play);
    const int song = SongEvents_OotAssignedSong(play->sceneNum, actor->room, play->curSpawn, action);
    if (song >= 0) {
        position = actor->world.pos;
        position.y += 42.f;
    }
    return song;
}
} // namespace SongMarkerHost
} // namespace

#include "hud/SongEventMarkerWindow.inc"

static RegisterShipInitFunc songEventMarkerInit([]() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawEnd>(EnsureSongEventMarkerWindow);
});
