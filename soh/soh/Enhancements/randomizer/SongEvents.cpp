#include "SongEvents.h"
#include "combo/rando/SongEventsBridge.h"
#include "soh/ResourceManagerHelpers.h"
#include <libultraship/bridge/consolevariablebridge.h>
extern "C" {
#include "functions.h"
#include "variables.h"
extern PlayState* gPlayState;
}
using namespace ComboRando::SongEvents;
void Rando::Context::GenerateSongEvents() {
    auto map = Generate(Game::Oot, GetSeed(), GetOption(RSK_SHUFFLE_SONG_EVENTS).Get() != 0);
    if (GetOption(RSK_SHUFFLE_SONG_EVENTS).Get()) {
        for (unsigned event = 0; event < map.size(); ++event) {
            std::string key = std::string("gRandoSettings.SongEventPlando.") + OotNames[event];
            int song = CVarGetInteger(key.c_str(), -1);
            if (song >= 0 && song < 20 && Playable(Game::Oot, song))
                map[event] = song;
        }
    }
    std::copy(map.begin(), map.end(), songEvents.begin());
}
void Rando::Context::ImportSongEvents(const nlohmann::json& source) {
    if (!source.contains("songEvents")) {
        GenerateSongEvents();
        return;
    }
    auto map = ParseMapping(Game::Oot, source.at("songEvents"));
    std::copy(map.begin(), map.end(), songEvents.begin());
}
nlohmann::json Rando::Context::ExportSongEvents() const {
    return songEvents;
}
bool Rando::Logic::CanPlaySongEvent(unsigned event) {
    auto ctx = Context::GetInstance();
    static const RandomizerGet items[] = {
        RG_ZELDAS_LULLABY,    RG_EPONAS_SONG,       RG_SARIAS_SONG,        RG_SONG_OF_STORMS,
        RG_SUNS_SONG,         RG_SONG_OF_TIME,      RG_MINUET_OF_FOREST,   RG_BOLERO_OF_FIRE,
        RG_SERENADE_OF_WATER, RG_REQUIEM_OF_SPIRIT, RG_NOCTURNE_OF_SHADOW, RG_PRELUDE_OF_LIGHT,
    };
    if (event >= 18)
        return false;
    auto song = ctx->GetOption(RSK_SHUFFLE_SONG_EVENTS).Get() ? ctx->songEvents[event] : Defaults(Game::Oot)[event];
    return song < 12 && CanUse(items[song]);
}
static int EventForScene(int scene, int room, int spawn) {
    switch (scene) {
        case SCENE_TEMPLE_OF_TIME:
            return OOT_TEMPLE_OF_TIME;
        case SCENE_WINDMILL_AND_DAMPES_GRAVE:
            return OOT_WINDMILL;
        case SCENE_GRAVEYARD:
            return OOT_GRAVEYARD;
        case SCENE_ZORAS_RIVER:
            return OOT_ZORA_RIVER;
        case SCENE_GORON_CITY:
            return OOT_GORON_CITY;
        case SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS:
            return spawn >= 0 && spawn < 3 ? OOT_GREAT_FAIRY_SPELL_WIND + spawn : -1;
        case SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC:
            return spawn >= 0 && spawn < 3 ? OOT_GREAT_FAIRY_UPGRADE_MAGIC + spawn : -1;
        case SCENE_WATER_TEMPLE:
            return OOT_TEMPLE_WATER;
        case SCENE_SHADOW_TEMPLE:
            return OOT_TEMPLE_SHADOW;
        case SCENE_SPIRIT_TEMPLE:
            if (ResourceMgr_IsSceneMasterQuest(SCENE_SPIRIT_TEMPLE))
                return OOT_TEMPLE_SPIRIT_STATUE;
            return room == 5    ? OOT_TEMPLE_SPIRIT_STATUE
                   : room == 14 ? OOT_TEMPLE_SPIRIT_LOWER
                   : room == 24 ? OOT_TEMPLE_SPIRIT_HIGHER
                                : -1;
        case SCENE_BOTTOM_OF_THE_WELL:
            return OOT_TEMPLE_BOTW;
        case SCENE_INSIDE_GANONS_CASTLE:
            return OOT_TEMPLE_GANON;
        default:
            return -1;
    }
}
static int sArmedOotAction = -1;
static int sArmedOotScene = -1;
extern "C" int SongEvents_OotAction(int scene, int room, int spawn, int vanillaAction) {
    sArmedOotAction = -1;
    sArmedOotScene = -1;
    if (!IS_RANDO || !RAND_GET_OPTION(RSK_SHUFFLE_SONG_EVENTS).Get())
        return vanillaAction;
    int event = EventForScene(scene, room, spawn);
    if (event < 0)
        return vanillaAction;
    static const int actions[] = {
        OCARINA_ACTION_CHECK_LULLABY, OCARINA_ACTION_CHECK_EPONA,    OCARINA_ACTION_CHECK_SARIA,
        OCARINA_ACTION_CHECK_STORMS,  OCARINA_ACTION_CHECK_SUNS,     OCARINA_ACTION_CHECK_TIME,
        OCARINA_ACTION_CHECK_MINUET,  OCARINA_ACTION_CHECK_BOLERO,   OCARINA_ACTION_CHECK_SERENADE,
        OCARINA_ACTION_CHECK_REQUIEM, OCARINA_ACTION_CHECK_NOCTURNE, OCARINA_ACTION_CHECK_PRELUDE,
    };
    // Different ocarina actors/songs in the same scene remain native.
    if (vanillaAction != actions[Defaults(Game::Oot)[event]])
        return vanillaAction;
    auto song = Rando::Context::GetInstance()->songEvents[event];
    if (song >= 12)
        return vanillaAction;
    sArmedOotAction = actions[song];
    sArmedOotScene = scene;
    return actions[song];
}
extern "C" int SongEvents_OotIsEventAction(int action) {
    return IS_RANDO && RAND_GET_OPTION(RSK_SHUFFLE_SONG_EVENTS).Get() && gPlayState &&
           gPlayState->sceneNum == sArmedOotScene && action == sArmedOotAction &&
           action >= OCARINA_ACTION_CHECK_MINUET && action <= OCARINA_ACTION_CHECK_STORMS;
}
extern "C" int SongEvents_OotShouldCapture(int action, int nativeSong) {
    return SongEvents_OotIsEventAction(action) && nativeSong + OCARINA_ACTION_CHECK_MINUET == action;
}
extern "C" int SongEvents_OotClockLocked(void) {
    return IS_RANDO && ClockLocked(RAND_GET_OPTION(RSK_SHUFFLE_CLOCK).Get(), Flags_GetRandomizerInf(RAND_INF_CLOCK));
}
extern "C" void SongEvents_OotApplyClockLock(void) {
    if (!SongEvents_OotClockLocked())
        return;
    auto ctx = Rando::Context::GetInstance();
    gSaveContext.dayTime = ClockStart(ctx->GetOption(RSK_CLOCK_START).Get(), ctx->GetSeed());
    gSaveContext.skyboxTime = gSaveContext.dayTime;
    gSaveContext.nextDayTime = 0xffff;
    gSaveContext.nightFlag = gSaveContext.dayTime >= 0xc000;
}
bool Rando::Logic::ClockAllowsTimePass() {
    return !Context::GetInstance()->GetOption(RSK_SHUFFLE_CLOCK).Get() || HasItem(RG_CLOCK);
}

extern "C" int SongEvents_OotSunSongTimeLocked(void) {
    return SongEvents_OotClockLocked() ||
           (IS_RANDO && RAND_GET_OPTION(RSK_SHUFFLE_CLOCK).Get() && gSaveContext.sunsSongState == SUNSSONG_SPECIAL);
}
