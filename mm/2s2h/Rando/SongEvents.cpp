#include "SongEvents.h"
#include "SongEventsNativeActors.h"
#include <cmath>
#include "combo/rando/SongEventsBridge.h"
#include <libultraship/bridge/consolevariablebridge.h>
namespace SE = ComboRando::SongEvents;
namespace Rando::SongEvents {
static uint32_t importedSeed;
static std::vector<uint8_t> importedMap;
static uint32_t preparedSeed;
static std::vector<uint8_t> preparedMap;
void ClearImportedMapping() {
    importedMap.clear();
    preparedMap.clear();
}
void RememberSavedMapping(const RandoSaveInfo& info) {
    if (!info.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS] || (preparedSeed == info.finalSeed && !preparedMap.empty()))
        return;
    std::vector<uint8_t> map(info.songEvents, info.songEvents + 13);
    SE::Validate(SE::Game::Mm, map);
    importedSeed = info.finalSeed;
    importedMap = std::move(map);
}
void Generate(RandoSaveInfo& info, bool prepareForOracle) {
    auto map = SE::Generate(SE::Game::Mm, info.finalSeed, info.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS]);
    if (info.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS]) {
        for (unsigned event = 0; event < map.size(); ++event) {
            std::string key = std::string("gRando.Options.SongEventPlando.") + SE::MmNames[event];
            int song = CVarGetInteger(key.c_str(), -1);
            if (song >= 0 && song < 20 && SE::Playable(SE::Game::Mm, song))
                map[event] = song;
        }
    }
    if (prepareForOracle) {
        preparedSeed = info.finalSeed;
        preparedMap = map;
    }
    std::copy(map.begin(), map.end(), info.songEvents);
}
void RestoreForOracle(RandoSaveInfo& info) {
    Generate(info);
    if (info.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS] && info.finalSeed == importedSeed && !importedMap.empty())
        std::copy(importedMap.begin(), importedMap.end(), info.songEvents);
}
void Import(RandoSaveInfo& info, const nlohmann::json& spoiler) {
    if (!spoiler.contains("songEvents")) {
        Generate(info);
        return;
    }
    auto map = SE::ParseMapping(SE::Game::Mm, spoiler.at("songEvents"));
    importedSeed = info.finalSeed;
    importedMap = map;
    std::copy(map.begin(), map.end(), info.songEvents);
}
static int NativeSong(unsigned song) {
    if (song == 16 && CHECK_QUEST_ITEM(QUEST_SONG_LULLABY))
        return OCARINA_SONG_GORON_LULLABY;
    static const int songs[] = {
        -1,
        OCARINA_SONG_EPONAS,
        -1,
        OCARINA_SONG_STORMS,
        -1,
        OCARINA_SONG_TIME,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        OCARINA_SONG_HEALING,
        OCARINA_SONG_SOARING,
        OCARINA_SONG_SONATA,
        OCARINA_SONG_GORON_LULLABY,
        OCARINA_SONG_GORON_LULLABY_INTRO,
        OCARINA_SONG_NEW_WAVE,
        -1,
        OCARINA_SONG_OATH,
    };
    return song < 20 ? songs[song] : -1;
}
bool CanPlay(unsigned event) {
    if (event >= 13 || !HAS_ITEM(ITEM_OCARINA_OF_TIME))
        return false;
    // These native listeners accept only their required playable form.
    if (event >= 4 && event <= 7) {
        if (!Rando::Logic::CanReturnToHuman())
            return false;
    }
    if (event == 10 && !CAN_BE_GORON)
        return false;
    auto song = RANDO_SAVE_OPTIONS[RO_SHUFFLE_SONG_EVENTS] ? gSaveContext.save.shipSaveInfo.rando.songEvents[event]
                                                           : SE::Defaults(SE::Game::Mm)[event];
    int native = NativeSong(song);
    if (native < 0)
        return false;
    int quest = native == OCARINA_SONG_GORON_LULLABY_INTRO ? QUEST_SONG_LULLABY_INTRO
                                                           : QUEST_SONG_SONATA + native - OCARINA_SONG_SONATA;
    return CHECK_QUEST_ITEM(quest) && Rando::Logic::canPlaySong(native);
}
} // namespace Rando::SongEvents
extern "C" int SongEvents_MmSong(int event, int vanillaSong) {
    if (!IS_RANDO || !RANDO_SAVE_OPTIONS[RO_SHUFFLE_SONG_EVENTS] || event < 0 || event >= 13)
        return vanillaSong;
    return Rando::SongEvents::NativeSong(gSaveContext.save.shipSaveInfo.rando.songEvents[event]);
}
// Use the actual accepting callbacks, not just actor IDs. A healed/completed
// actor can remain loaded; its former assignment must never steal normal travel.
static int AcceptingEvent(Actor* actor, Player* player, PlayState* play) {
    const float x = player->actor.world.pos.x, z = player->actor.world.pos.z;
    const float height = std::fabs(actor->playerHeightRel);
    switch (actor->id) {
        case ACTOR_DM_CHAR01:
            return play->sceneId == SCENE_21MITURINMAE &&
                           reinterpret_cast<DmChar01*>(actor)->actionFunc == func_80AA8698 &&
                           !CHECK_WEEKEVENTREG(WEEKEVENTREG_20_01) && x > -40.f && x < 40.f && z > 1000.f && z < 1078.f
                       ? 0
                       : -1;
        case ACTOR_EN_DAI:
            return play->sceneId == SCENE_12HAKUGINMAE &&
                           reinterpret_cast<EnDai*>(actor)->actionFunc == func_80B3EEDC &&
                           !CHECK_WEEKEVENTREG(WEEKEVENTREG_30_01) && actor->xzDistToPlayer < 2500.f
                       ? 1
                       : -1;
        case ACTOR_DM_CHAR08:
            return play->sceneId == SCENE_31MISAKI &&
                           reinterpret_cast<DmChar08*>(actor)->actionFunc == DmChar08_WaitForSong &&
                           !CHECK_WEEKEVENTREG(WEEKEVENTREG_53_20) && x > -5780.f && x < -5385.f && z > 1120.f &&
                           z < 2100.f
                       ? 2
                       : -1;
        case ACTOR_EN_GB2:
            return play->sceneId == SCENE_TOUGITES && reinterpret_cast<EnGb2*>(actor)->actionFunc == func_80B0FEBC &&
                           actor->xzDistToPlayer < 300.f
                       ? 3
                       : -1;
        case ACTOR_EN_GG: {
            auto* ghost = reinterpret_cast<EnGg*>(actor);
            return play->sceneId == SCENE_GORON_HAKA && ghost->actionFunc == func_80B35450 &&
                           CHECK_WEEKEVENTREG(WEEKEVENTREG_91_10) && play->csCtx.state == CS_STATE_IDLE &&
                           !ghost->unk_307 && actor->xzDistToPlayer < 200.f
                       ? 4
                       : -1;
        }
        case ACTOR_EN_HG: {
            auto* father = reinterpret_cast<EnHg*>(actor);
            return play->sceneId == SCENE_MUSICHOUSE && actor->colChkInfo.health == 1 && height < 80.f &&
                           father->actionFunc != EnHg_PlayCutscene && father->actionFunc != EnHg_HandleCutscene &&
                           actor->xzDistToPlayer < 300.f
                       ? 5
                       : -1;
        }
        case ACTOR_EN_YB:
            return play->sceneId == SCENE_00KEIKOKU && reinterpret_cast<EnYb*>(actor)->actionFunc == EnYb_Idle &&
                           actor->xzDistToPlayer < 180.f && height < 50.f
                       ? 6
                       : -1;
        case ACTOR_EN_ZOG: {
            auto callback = reinterpret_cast<EnZog*>(actor)->actionFunc;
            return play->sceneId == SCENE_30GYOSON && (callback == func_80B9451C || callback == func_80B948A8) &&
                           actor->xzDistToPlayer < 120.f
                       ? 7
                       : -1;
        }
        case ACTOR_EN_BSB:
            return play->sceneId == SCENE_BOTI && reinterpret_cast<EnBsb*>(actor)->actionFunc == func_80C0BFE8 &&
                           actor->xzDistToPlayer <= 300.f &&
                           std::abs(int(int16_t(actor->yawTowardsPlayer - actor->shape.rot.y))) < 0x4300
                       ? 8
                       : -1;
        case ACTOR_EN_HIDDEN_NUTS:
            return play->sceneId == SCENE_KINSTA1 &&
                           reinterpret_cast<EnHiddenNuts*>(actor)->actionFunc == func_80BDB2B8 &&
                           actor->xzDistToPlayer <= 120.f
                       ? 9
                       : -1;
        case ACTOR_EN_GK:
            return play->sceneId == SCENE_16GORON_HOUSE &&
                           reinterpret_cast<EnGk*>(actor)->actionFunc == func_80B5202C && actor->xzDistToPlayer < 100.f
                       ? 10
                       : -1;
        case ACTOR_EN_PO_COMPOSER:
            return play->sceneId == SCENE_IKANA && !(actor->params & 0x8000) &&
                           reinterpret_cast<EnPoComposer*>(actor)->actionFunc == EnPoComposer_PlayCurse &&
                           actor->xzDistToPlayer < 300.f
                       ? 11
                       : -1;
        case ACTOR_EN_TIME_TAG:
            return play->sceneId == SCENE_OKUJOU && ((actor->params & 0xe000) >> 13) == 2 &&
                           reinterpret_cast<EnTimeTag*>(actor)->actionFunc == EnTimeTag_RooftopOath_Wait &&
                           height < 80.f && actor->xzDistToPlayer < 300.f
                       ? 12
                       : -1;
        default:
            return -1;
    }
}
extern "C" int SongEvents_MmInterceptSong(int song) {
    if (!IS_RANDO || !RANDO_SAVE_OPTIONS[RO_SHUFFLE_SONG_EVENTS] || !gPlayState)
        return false;
    Player* player = GET_PLAYER(gPlayState);
    if (!player)
        return false;
    for (int category = 0; category < ACTORCAT_MAX; ++category) {
        for (Actor* actor = gPlayState->actorCtx.actorLists[category].first; actor; actor = actor->next) {
            int event = AcceptingEvent(actor, player, gPlayState);
            if (event < 0)
                continue;
            if (event < 3) {
                unsigned mode = RANDO_SAVE_OPTIONS[RO_ACCESS_DUNGEONS];
                bool correctForm = player->transformation == (event == 0   ? PLAYER_FORM_DEKU
                                                              : event == 1 ? PLAYER_FORM_GORON
                                                                           : PLAYER_FORM_ZORA);
                if (mode == RO_ACCESS_DUNGEONS_OPEN ||
                    ((mode == RO_ACCESS_DUNGEONS_FORM_AND_SONG || mode == RO_ACCESS_DUNGEONS_FORM_ONLY) &&
                     !correctForm))
                    continue;
            }
            if (event >= 4 && event <= 7 && player->transformation != PLAYER_FORM_HUMAN)
                continue;
            if (event == 10 && player->transformation != PLAYER_FORM_GORON)
                continue;
            if (SongEvents_MmSong(event, -1) == song)
                return true;
        }
    }
    return false;
}
