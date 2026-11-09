#include "Rando/RustyDoorKeys.h"
#include "2s2h/GameInteractor/GameInteractor.h"
extern "C" {
#include "global.h"
// Native player interaction only; schedule actors retain their own opening behavior.
int Rando_RustyDoorLocked(PlayState* play, Actor* actor) {
    if (!IS_RANDO || !RANDO_SAVE_OPTIONS[RO_SHUFFLE_RUSTY_KEYS])
        return false;
    const int item = Rando::RustyDoorKeys::SourceItem(play->sceneId, (u16)actor->params >> 10);
    return item >= 0 && !Rando::RustyDoorKeys::CanUse(static_cast<RandoItemId>(item));
}
}
