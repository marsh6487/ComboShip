#include "SoulShuffleRuntime.h"
#include "SoulShuffle.h"
#include "SoulShuffleEnemy.h"
#include <initializer_list>
#include "z64.h"
extern "C" {
#include "variables.h"
#include "functions.h"
u8 Randomizer_GetSettingValue(RandomizerSettingKey key);
}

// Scripted rewards have no allocated NPC. Defer before consuming their flags.
extern "C" int Randomizer_SoulCanAccessCheck(int check) {
    if (!IS_RANDO || gSaveContext.gameMode == GAMEMODE_END_CREDITS)
        return true;
    switch (check) {
#define SOUL_CHECK(check, ...)                                                                           \
    case check:                                                                                          \
        for (RandomizerGet item : { __VA_ARGS__ }) {                                                     \
            const auto* soul = Rando::SoulShuffle::Find(item);                                           \
            if (soul && Randomizer_GetSettingValue(soul->option) && !Flags_GetRandomizerInf(soul->flag)) \
                return false;                                                                            \
        }                                                                                                \
        return true;
#include "SoulShuffleChecks.inc"
#undef SOUL_CHECK
        default:
            return true;
    }
}

extern "C" int Randomizer_SoulCanDamageActor(PlayState* play, Actor* actor) {
    if (!play || !actor || !IS_RANDO || gSaveContext.gameMode == GAMEMODE_END_CREDITS) {
        return true;
    }
    const auto item = Rando::SoulShuffle::EnemyForActor(actor->id, static_cast<uint16_t>(actor->params),
                                                        gSaveContext.linkAge == LINK_AGE_ADULT,
                                                        play->sceneNum == SCENE_DEKU_TREE_BOSS);
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

extern "C" int Randomizer_SoulCanInteract(PlayState* play, int actorId, unsigned int params) {
    if (!play || !IS_RANDO || gSaveContext.gameMode == GAMEMODE_END_CREDITS) {
        return true;
    }
    const auto npcAvailable = [](RandomizerGet item) {
        const auto* soul = Rando::SoulShuffle::Find(item);
        return !soul || !Randomizer_GetSettingValue(soul->option) || Flags_GetRandomizerInf(soul->flag);
    };
#define SOUL_NPC(actor, scene, role, soul)                                            \
    if (actorId == actor && play->sceneNum == scene && (role) && !npcAvailable(soul)) \
        return false;
#include "SoulShuffleNpcs.inc"
#undef SOUL_NPC
    RandomizerGet item = RG_NONE;
    switch (actorId) {
        case ACTOR_EN_OSSAN:
#define SOUL_SHOP(scene, role, soul)               \
    if (play->sceneNum == scene && params == role) \
        item = soul;
#include "SoulShuffleShops.inc"
#undef SOUL_SHOP
            break;
        case ACTOR_EN_NIW:
        case ACTOR_EN_NWC:
            item = RG_SOUL_ANIMAL_CUCCO;
            break;
        case ACTOR_EN_COW:
            item = RG_SOUL_ANIMAL_COW;
            break;
        case ACTOR_EN_DOG:
            item = RG_SOUL_ANIMAL_DOG;
            break;
        case ACTOR_EN_BUTTE:
            item = RG_SOUL_ANIMAL_BUTTERFLY;
            break;
        case ACTOR_EN_DNS:
        case ACTOR_EN_SHOPNUTS:
            item = RG_SOUL_MISC_BUSINESS_SCRUB;
            break;
        case ACTOR_EN_SW:
            if (params & 0xE000)
                item = RG_SOUL_MISC_GS;
            break;
        case ACTOR_EN_TA:
            // This native minigame host dereferences all three super-cucco
            // children. Other Talon roles (castle, Kakariko, stable) are independent.
            if (play->sceneNum == SCENE_LON_LON_BUILDINGS && params != 1 && params != 2)
                item = RG_SOUL_ANIMAL_CUCCO;
            break;
        default:
            break; // Generic viewers, shop/cutscene proxies and custom actors are unaffected.
    }
    const auto* soul = Rando::SoulShuffle::Find(item);
    return !soul || !Randomizer_GetSettingValue(soul->option) || Flags_GetRandomizerInf(soul->flag);
}

namespace {
// Transient encounters cover every nonnegative s8 native/extended room.
bool soulPreventedRoomClear[128]{};
} // namespace
extern "C" void Randomizer_SoulResetRoomState(PlayState*, int room) {
    if (room < 0) {
        for (auto& blocked : soulPreventedRoomClear)
            blocked = false;
    } else if (room < 128) {
        soulPreventedRoomClear[room] = false;
    }
}
extern "C" void Randomizer_SoulNoteRemovedActor(PlayState* play, Actor* actor) {
    if (actor && actor->category == ACTORCAT_ENEMY && actor->room >= 0 && !Randomizer_SoulCanDamageActor(play, actor))
        soulPreventedRoomClear[actor->room] = true;
}
extern "C" int Randomizer_SoulCanClearRoom(PlayState* play, int room) {
    return !play || !IS_RANDO || gSaveContext.gameMode == GAMEMODE_END_CREDITS || room < 0 || room >= 128 ||
           !soulPreventedRoomClear[room];
}
extern "C" void Randomizer_SoulUpdateActor(PlayState* play, Actor* actor) {
    if (!play || !actor || !actor->update)
        return;
    if (Randomizer_SoulCanDamageActor(play, actor)) {
        actor->update(actor, play);
        return;
    }
    // Several native enemies subtract hammer-quake HP without AC/ApplyDamage.
    // Isolate only this immune callback, retaining impact for all other actors.
    const u8 impact = play->actorCtx.unk_02;
    play->actorCtx.unk_02 = 0;
    actor->update(actor, play);
    play->actorCtx.unk_02 = impact;
}

extern "C" int Randomizer_SoulCanSpawnNonEnemy(PlayState* play, int actorId, unsigned int params) {
    // Physical blockers remain in place; their native updates gate interaction.
    // Acquiring a soul must never reintroduce a barrier on a later scene load.
    if (play &&
        ((actorId == ACTOR_EN_KZ && play->sceneNum == SCENE_ZORAS_DOMAIN) ||
         (actorId == ACTOR_EN_MD && (play->sceneNum == SCENE_KOKIRI_FOREST || play->sceneNum == SCENE_LOST_WOODS))))
        return true;
    return Randomizer_SoulCanInteract(play, actorId, params);
}
