#include "SongEvents.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
#include "functions.h"
#include "variables.h"
}

#ifdef COMBO_BUILD
bool Combo_MmIsForeground(void);
#endif

namespace {
namespace SongMarkerHost {
constexpr char kCvar[] = "gSongEventMarkerMmWindow";
constexpr char kName[] = "MM Song Event Markers";
constexpr int kActorCategories[] = { ACTORCAT_BG, ACTORCAT_NPC, ACTORCAT_PROP, ACTORCAT_ITEMACTION };

bool IsEnabled() {
#ifdef COMBO_BUILD
    if (!Combo_MmIsForeground())
        return false;
#endif
    return IS_RANDO && RANDO_SAVE_OPTIONS[RO_SHUFFLE_SONG_EVENTS];
}

bool IsMirrored() {
    return CVarGetInteger("gModes.MirroredWorld.State", 0) != 0;
}

Actor* FirstActor(PlayState* play, int category) {
    return play->actorCtx.actorLists[category].first;
}

using Rando::SongEvents::MarkerSong;
} // namespace SongMarkerHost
} // namespace

#include "hud/SongEventMarkerWindow.inc"

static RegisterShipInitFunc songEventMarkerInit([]() {
    COND_HOOK(OnInterfaceDrawStart, true, EnsureSongEventMarkerWindow);
});
