#include "SoulShuffleRuntime.h"
#include "SoulShuffle.h"
#include <nlohmann/json.hpp>
#include "Rando.h"
extern "C" {
#include "functions.h"
#include "variables.h"
}

extern "C" int MM_SoulCanSpawnNonEnemy(PlayState* play, int actorId, unsigned int params) {
    if (!play || !IS_RANDO || gSaveContext.gameMode == GAMEMODE_END_CREDITS) {
        return true;
    }
    RandoItemId item = RI_UNKNOWN;
    switch (actorId) {
        case ACTOR_EN_NIW:
        case ACTOR_EN_NWC: item = RI_SOUL_ANIMAL_CUCCO; break;
        case ACTOR_EN_COW: item = RI_SOUL_ANIMAL_COW; break;
        case ACTOR_EN_DG:
        case ACTOR_EN_RACEDOG: item = RI_SOUL_ANIMAL_DOG; break;
        case ACTOR_EN_BUTTE: item = RI_SOUL_ANIMAL_BUTTERFLY; break;
        case ACTOR_EN_SELLNUTS:
        case ACTOR_EN_SCOPENUTS:
        case ACTOR_EN_AKINDONUTS: item = RI_SOUL_MISC_BUSINESS_SCRUB; break;
        case ACTOR_EN_SW:
            // Gold Skulltulas retain the existing enemy-soul immunity as well.
            // This additional category controls only their optional existence.
            if (params & 3) item = RI_SOUL_MISC_GS;
            break;
        case ACTOR_EN_HS:
            if (play->sceneId == SCENE_F01C) item = RI_SOUL_ANIMAL_CUCCO;
            break;
        case ACTOR_EN_AOB_01:
            if (play->sceneId == SCENE_F01_B) item = RI_SOUL_ANIMAL_DOG;
            break;
        default: break; // Native shop, generic cutscene and custom viewer roles are unaffected.
    }
    const auto* soul = Rando::SoulShuffle::Find(item);
    return !soul || !RANDO_SAVE_OPTIONS[soul->option] || Flags_GetRandoInf(soul->flag);
}
