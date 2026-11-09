#include "SoulShuffleRuntime.h"
#include "SoulShuffle.h"
#include "SoulShuffleEnemy.h"
#include "z64.h"
extern "C" {
#include "variables.h"
#include "functions.h"
u8 Randomizer_GetSettingValue(RandomizerSettingKey key);
}

extern "C" int Randomizer_SoulCanDamageActor(PlayState* play, Actor* actor) {
    if (!play || !actor || !IS_RANDO || gSaveContext.gameMode == GAMEMODE_END_CREDITS) {
        return true;
    }
    const auto item = Rando::SoulShuffle::EnemyForActor(actor->id, static_cast<uint16_t>(actor->params),
                                                      gSaveContext.linkAge == LINK_AGE_ADULT, play->sceneNum == SCENE_DEKU_TREE_BOSS);
    const auto* soul = Rando::SoulShuffle::Find(item);
    return !soul || !Randomizer_GetSettingValue(soul->option) || Flags_GetRandomizerInf(soul->flag);
}

extern "C" int Randomizer_SoulShouldUpdateActor(PlayState* play, Actor* actor) {
    // These actors die from wall/AT or flame-circle interactions outside the AC
    // damage path. Keep their initialized actor/pointers/room membership intact.
    // Ordinary enemies retain their native update and use collision immunity.
    if (!actor || (actor->id != ACTOR_EN_TUBO_TRAP && actor->id != ACTOR_EN_ANUBICE)) {
        return true;
    }
    return Randomizer_SoulCanDamageActor(play, actor);
}

extern "C" int Randomizer_SoulCanSpawnNonEnemy(PlayState* play, int actorId, unsigned int params) {
    if (!play || !IS_RANDO || gSaveContext.gameMode == GAMEMODE_END_CREDITS) {
        return true;
    }
    RandomizerGet item = RG_NONE;
    switch (actorId) {
        case ACTOR_EN_OSSAN:
#define SOUL_SHOP(scene, role, soul) if (play->sceneNum == scene && params == role) item = soul;
#include "SoulShuffleShops.inc"
#undef SOUL_SHOP
            break;
        case ACTOR_EN_NIW:
        case ACTOR_EN_NWC: item = RG_SOUL_ANIMAL_CUCCO; break;
        case ACTOR_EN_COW: item = RG_SOUL_ANIMAL_COW; break;
        case ACTOR_EN_DOG: item = RG_SOUL_ANIMAL_DOG; break;
        case ACTOR_EN_BUTTE: item = RG_SOUL_ANIMAL_BUTTERFLY; break;
        case ACTOR_EN_DNS:
        case ACTOR_EN_SHOPNUTS: item = RG_SOUL_MISC_BUSINESS_SCRUB; break;
        case ACTOR_EN_SW:
            if (params & 0xE000) item = RG_SOUL_MISC_GS;
            break;
        case ACTOR_EN_TA:
            // This native minigame host dereferences all three super-cucco
            // children. Other Talon roles (castle, Kakariko, stable) are independent.
            if (play->sceneNum == SCENE_LON_LON_BUILDINGS && params != 1 && params != 2)
                item = RG_SOUL_ANIMAL_CUCCO;
            break;
        default: break; // Generic viewers, shop/cutscene proxies and custom actors are unaffected.
    }
    const auto* soul = Rando::SoulShuffle::Find(item);
    return !soul || !Randomizer_GetSettingValue(soul->option) || Flags_GetRandomizerInf(soul->flag);
}
