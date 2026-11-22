// Production adapter bodies, actual native SaveContext/flag operations; the
// Context and CanUse services are explicit inventory boundaries.
#include "z64.h"
#include "macros.h"
#include "src/overlays/actors/ovl_Bg_Haka_Water/z_bg_haka_water.h"
#include "src/overlays/actors/ovl_Bg_Mizu_Water/z_bg_mizu_water.h"
#include "src/overlays/actors/ovl_En_Okarina_Tag/z_en_okarina_tag.h"
#include "src/overlays/actors/ovl_En_Rd/z_en_rd.h"
#include "soh/Enhancements/game-interactor/vanilla-behavior/GIVanillaBehavior.h"
#include "combo/rando/SongEvents.h"
#include "combo/rando/SongEventsBridge.h"
#include "tests/test_require.h"
#include <nlohmann/json.hpp>
#include <array>
#include <set>
#include <memory>
#include <algorithm>
#include <string>
#include <cstring>
SaveContext gSaveContext{};
PlayState* gPlayState;
void Audio_PlayActorSound2(Actor*, u16) {}
void Actor_SetColorFilter(Actor*, s16, s16, s16, s16) {}
void EnRd_Stunned(EnRd*, PlayState*) {}
static bool masterQuest=false;
uint32_t ResourceMgr_IsSceneMasterQuest(s16){return masterQuest;}
int32_t CVarGetInteger(const char*,int32_t fallback){return fallback;}
void func_80ABEF2C(EnOkarinaTag*, PlayState*);
void func_80ABF0CC(EnOkarinaTag*, PlayState*);
void func_80ABF28C(EnOkarinaTag*, PlayState*);
void func_80ABF4C8(EnOkarinaTag*, PlayState*);
void func_80ABF708(EnOkarinaTag*, PlayState*) {}
void Actor_Kill(Actor* actor) { actor->update = nullptr; }
void Sfx_PlaySfxCentered(u16) {}
void Audio_SetMainBgmTempoFreqAfterFanfare(f32, u8) {}
s16 OnePointCutscene_Init(PlayState*, s16, s16, Actor*, s16) { return 0; }
void BgMizuWater_ChangeWaterLevel(BgMizuWater*, PlayState*) {}
void BgHakaWater_ChangeWaterLevel(BgHakaWater*, PlayState*);
void BgHakaWater_Draw(Actor*, PlayState*) {}
void Actor_PlaySfx_FlaggedCentered2(Actor*, u16) {}
s32 Math_StepToF(f32* value, f32 target, f32 step) {
    if (*value < target)
        *value = std::min(*value + step, target);
    else
        *value = std::max(*value - step, target);
    return *value == target;
}
static int requestedAction = -1;
void func_8010BD58(PlayState*, u16 action) { requestedAction = action; }
static CutsceneData D_80ABF9D0[1], D_80ABFB40[1];
static char spot02_scene_Cs_003C80, spot02_scene_Cs_005020;
#undef bREG
static int waterDebugRegisters[2]{};
#define bREG(index) waterDebugRegisters[index]
static bool allowWellToggle;
static bool TestVanillaBehavior(int behavior, bool state) {
    // Boundary from the Vanilla-layout Bottom of the Well randomizer hook.
    return behavior == VB_OKARINA_TAG_COMPLETED && allowWellToggle ? false : state;
}
#define GameInteractor_Should(behavior, state, ...) TestVanillaBehavior(behavior, state)
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)
#undef LUSLOG_INFO
#define LUSLOG_INFO(...) ((void)0)
#define GameInteractor_ExecuteOnFlagSet(...) ((void)0)
#define GameInteractor_ExecuteOnSceneFlagSet(...) ((void)0)
#define GameInteractor_ExecuteOnSceneFlagUnset(...) ((void)0)
namespace Rando {
struct OptionValue {uint8_t value=0;uint8_t Get(){return value;}};
class Context {
public:
    std::array<uint8_t,18> songEvents{};std::array<OptionValue,RSK_MAX> options{};uint32_t seed=0;
    static std::shared_ptr<Context> GetInstance(){static auto ctx=std::make_shared<Context>();return ctx;}
    OptionValue& GetOption(RandomizerSettingKey key){return options[key];}
    uint32_t GetSeed(){return seed;}
    void GenerateSongEvents();void ImportSongEvents(const nlohmann::json&);nlohmann::json ExportSongEvents() const;
};
class Logic {
public:
    std::set<RandomizerGet> available;
    bool IsAdult = false, AtDay = false;
    bool CanUse(RandomizerGet item){return available.count(item);}
    bool HasItem(RandomizerGet item){return item==RG_CLOCK && (gSaveContext.ship.randomizerInf[RAND_INF_CLOCK/16]&(1<<(RAND_INF_CLOCK%16)));}
    bool CanPlaySongEvent(unsigned);bool ClockAllowsTimePass();
};
}
#define RAND_GET_OPTION(key) Rando::Context::GetInstance()->GetOption(key)
using namespace ComboRando::SongEvents;
#include "oot_native_functions.inc"

static EnOkarinaTag SpawnSongTag(PlayState& play, int type, int action, int flag) {
    EnOkarinaTag tag{};
    tag.actor.id = ACTOR_EN_OKARINA_TAG;
    tag.actor.update = +[](Actor*, PlayState*) {};
    tag.actor.params = (type << 10) | ((action - OCARINA_ACTION_CHECK_SARIA) << 6) | flag;
    EnOkarinaTag_Init(&tag.actor, &play);
    return tag;
}

static void TestSongMarkerCompletion() {
    static PlayState play{};
    static Player player{};
    auto* previousPlay = gPlayState;
    gPlayState = &play;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    gSaveContext.linkAge = LINK_AGE_CHILD;
    const auto ctx = Rando::Context::GetInstance();
    Vec3f anchor{};
    const auto markerFor = [&](EnOkarinaTag& tag) { return MarkerSong(&tag.actor, &player, &play, anchor); };
    const auto beginSong = [&](EnOkarinaTag& tag, u32 stateFlags, int expected) {
        requestedAction = -1;
        player.stateFlags2 = stateFlags;
        tag.actionFunc(&tag, &play);
        REQUIRE(requestedAction == expected);
    };
    const auto finishSong = [&](EnOkarinaTag& tag, OcarinaMode mode) {
        play.msgCtx.ocarinaMode = mode;
        tag.actionFunc(&tag, &play);
    };

    play.sceneNum = SCENE_GRAVEYARD;
    ctx->songEvents[OOT_GRAVEYARD] = 5; // Song of Time, instead of Lullaby.
    auto tomb = SpawnSongTag(play, 6, OCARINA_ACTION_CHECK_SARIA, 0x3F);
    REQUIRE(markerFor(tomb) == 5 && anchor.y == 42.f);
    beginSong(tomb, PLAYER_STATE2_ATTEMPT_PLAY_FOR_ACTOR, OCARINA_ACTION_CHECK_TIME);
    REQUIRE(markerFor(tomb) == -1); // The common marker path has no glyph or text during song input.
    finishSong(tomb, OCARINA_MODE_04);
    REQUIRE(markerFor(tomb) == 5); // Canceled without finishing.
    beginSong(tomb, PLAYER_STATE2_ATTEMPT_PLAY_FOR_ACTOR, OCARINA_ACTION_CHECK_TIME);
    finishSong(tomb, OCARINA_MODE_03);
    REQUIRE(Flags_GetEventChkInf(EVENTCHKINF_DESTROYED_ROYAL_FAMILY_TOMB));
    REQUIRE(markerFor(tomb) == -1);
    auto reloadedTomb = SpawnSongTag(play, 6, OCARINA_ACTION_CHECK_SARIA, 0x3F);
    REQUIRE(markerFor(reloadedTomb) == -1);

    play.sceneNum = SCENE_WATER_TEMPLE;
    ctx->songEvents[OOT_TEMPLE_WATER] = 2; // Saria's Song for all three crests.
    Actor timeBlock{};
    timeBlock.id = ACTOR_OBJ_TIMEBLOCK;
    timeBlock.update = +[](Actor*, PlayState*) {};
    REQUIRE(MarkerSong(&timeBlock, &player, &play, anchor) == -1);
    REQUIRE(SongEvents_OotAssignedSong(play.sceneNum, 0, 0, OCARINA_ACTION_CHECK_TIME) == -1);
    Flags_SetSwitch(&play, WATER_TEMPLE_WATER_F3_FLAG);
    auto lower = SpawnSongTag(play, 7, OCARINA_ACTION_CHECK_LULLABY, WATER_TEMPLE_WATER_F1_FLAG);
    auto upper = SpawnSongTag(play, 7, OCARINA_ACTION_CHECK_LULLABY, WATER_TEMPLE_WATER_F3_FLAG);
    REQUIRE(markerFor(lower) == 2 && markerFor(upper) == 2);
    beginSong(upper, PLAYER_STATE2_PLAY_FOR_ACTOR, -1); // The already-active level cannot trigger twice.
    beginSong(lower, PLAYER_STATE2_PLAY_FOR_ACTOR, OCARINA_ACTION_CHECK_SARIA);
    REQUIRE(markerFor(lower) == -1);
    finishSong(lower, OCARINA_MODE_03);
    BgMizuWater surface{};
    surface.actor.params = WATER_TEMPLE_WATER_F3_FLAG;
    surface.baseY = surface.targetY = surface.actor.world.pos.y = WATER_TEMPLE_WATER_F3_Y;
    BgMizuWater_WaitForAction(&surface, &play);
    REQUIRE(surface.actor.params == WATER_TEMPLE_WATER_F1_FLAG);
    REQUIRE(Flags_GetSwitch(&play, WATER_TEMPLE_WATER_F1_FLAG));
    REQUIRE(!Flags_GetSwitch(&play, WATER_TEMPLE_WATER_F3_FLAG));
    REQUIRE(markerFor(lower) == 2 && markerFor(upper) == 2);
    beginSong(lower, PLAYER_STATE2_PLAY_FOR_ACTOR, -1);
    auto reloadedLower = SpawnSongTag(play, 7, OCARINA_ACTION_CHECK_LULLABY, WATER_TEMPLE_WATER_F1_FLAG);
    REQUIRE(markerFor(reloadedLower) == 2);
    beginSong(upper, PLAYER_STATE2_PLAY_FOR_ACTOR, OCARINA_ACTION_CHECK_SARIA);
    finishSong(upper, OCARINA_MODE_03);
    BgMizuWater_WaitForAction(&surface, &play);
    REQUIRE(surface.actor.params == WATER_TEMPLE_WATER_F3_FLAG);
    REQUIRE(!Flags_GetSwitch(&play, WATER_TEMPLE_WATER_F1_FLAG));
    beginSong(reloadedLower, PLAYER_STATE2_PLAY_FOR_ACTOR, OCARINA_ACTION_CHECK_SARIA);
    ctx->GetOption(RSK_SHUFFLE_SONG_EVENTS).value = 0;
    auto vanillaUpper = SpawnSongTag(play, 7, OCARINA_ACTION_CHECK_LULLABY, WATER_TEMPLE_WATER_F3_FLAG);
    REQUIRE(!vanillaUpper.actor.update && markerFor(vanillaUpper) == -1);
    ctx->GetOption(RSK_SHUFFLE_SONG_EVENTS).value = 1;

    play.sceneNum = SCENE_BOTTOM_OF_THE_WELL;
    allowWellToggle = true;
    constexpr int wellDrainSwitch = 11;
    ctx->songEvents[OOT_TEMPLE_BOTW] = 3;
    auto well = SpawnSongTag(play, 7, OCARINA_ACTION_CHECK_LULLABY, wellDrainSwitch);
    REQUIRE(markerFor(well) == 3);
    beginSong(well, PLAYER_STATE2_PLAY_FOR_ACTOR, OCARINA_ACTION_CHECK_STORMS);
    REQUIRE(markerFor(well) == -1);
    finishSong(well, OCARINA_MODE_03);
    REQUIRE(Flags_GetSwitch(&play, wellDrainSwitch) && markerFor(well) == -1);
    BgHakaWater wellWater{};
    CollisionHeader collision{};
    WaterBox waterBoxes[9]{};
    collision.waterBoxes = waterBoxes;
    play.colCtx.colHeader = &collision;
    wellWater.actor.params = wellDrainSwitch;
    wellWater.actionFunc = BgHakaWater_Wait;
    for (int i = 0; i <= 400; ++i) {
        wellWater.actionFunc(&wellWater, &play);
        REQUIRE(markerFor(well) == -1);
    }
    REQUIRE(wellWater.isLowered && wellWater.actionFunc == BgHakaWater_Wait && waterBoxes[0].ySurface == -208);
    auto reloadedWell = SpawnSongTag(play, 7, OCARINA_ACTION_CHECK_LULLABY, wellDrainSwitch);
    REQUIRE(reloadedWell.actor.update && markerFor(reloadedWell) == -1);
    allowWellToggle = false;

    play.sceneNum = SCENE_WINDMILL_AND_DAMPES_GRAVE;
    ctx->songEvents[OOT_WINDMILL] = 4;
    auto windmill = SpawnSongTag(play, 2, OCARINA_ACTION_CHECK_SARIA, 0x3F);
    REQUIRE(markerFor(windmill) == 4);
    Flags_SetEventChkInf(EVENTCHKINF_DRAINED_WELL_IN_KAKARIKO);
    REQUIRE(markerFor(windmill) == -1);

    gPlayState = previousPlay;
    std::memset(gSaveContext.eventChkInf, 0, sizeof(gSaveContext.eventChkInf));
    gSaveContext.cutsceneTrigger = 0;
}

int main() {
    static PlayState play{};gPlayState=&play;
    auto ctx=Rando::Context::GetInstance();Rando::Logic logic;
    gSaveContext.ship.quest.id=QUEST_RANDOMIZER;ctx->GetOption(RSK_SHUFFLE_SONG_EVENTS).value=1;
    const int scenes[]={SCENE_TEMPLE_OF_TIME,SCENE_WINDMILL_AND_DAMPES_GRAVE,SCENE_GRAVEYARD,SCENE_ZORAS_RIVER,SCENE_GORON_CITY,SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS,SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS,SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS,SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC,SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC,SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC,SCENE_WATER_TEMPLE,SCENE_SHADOW_TEMPLE,SCENE_SPIRIT_TEMPLE,SCENE_SPIRIT_TEMPLE,SCENE_SPIRIT_TEMPLE,SCENE_BOTTOM_OF_THE_WELL,SCENE_INSIDE_GANONS_CASTLE};
    const int rooms[]={0,0,0,0,0,0,0,0,0,0,0,0,0,5,14,24,0,0};
    const int spawns[]={0,0,0,0,0,0,1,2,0,1,2,0,0,0,0,0,0,0};
    const RandomizerGet items[]={RG_ZELDAS_LULLABY,RG_EPONAS_SONG,RG_SARIAS_SONG,RG_SONG_OF_STORMS,RG_SUNS_SONG,RG_SONG_OF_TIME,RG_MINUET_OF_FOREST,RG_BOLERO_OF_FIRE,RG_SERENADE_OF_WATER,RG_REQUIEM_OF_SPIRIT,RG_NOCTURNE_OF_SHADOW,RG_PRELUDE_OF_LIGHT};
    const int actions[]={OCARINA_ACTION_CHECK_LULLABY,OCARINA_ACTION_CHECK_EPONA,OCARINA_ACTION_CHECK_SARIA,OCARINA_ACTION_CHECK_STORMS,OCARINA_ACTION_CHECK_SUNS,OCARINA_ACTION_CHECK_TIME,OCARINA_ACTION_CHECK_MINUET,OCARINA_ACTION_CHECK_BOLERO,OCARINA_ACTION_CHECK_SERENADE,OCARINA_ACTION_CHECK_REQUIEM,OCARINA_ACTION_CHECK_NOCTURNE,OCARINA_ACTION_CHECK_PRELUDE};
    EnOkarinaTag tag{};
    REQUIRE(EnOkarinaTag_GetPendingSongAction(&tag, &play) == -1);
    tag.actor.update = +[](Actor*, PlayState*) {};
    tag.actionFunc = func_80ABF28C;
    tag.type = 2;
    REQUIRE(EnOkarinaTag_GetPendingSongAction(&tag, &play) == OCARINA_ACTION_CHECK_STORMS);
    Flags_SetSwitch(&play, tag.switchFlag);
    REQUIRE(EnOkarinaTag_GetPendingSongAction(&tag, &play) == -1);
    Flags_UnsetSwitch(&play, tag.switchFlag);
    tag.type = 6;
    REQUIRE(EnOkarinaTag_GetPendingSongAction(&tag, &play) == OCARINA_ACTION_CHECK_LULLABY);
    Flags_SetEventChkInf(EVENTCHKINF_DESTROYED_ROYAL_FAMILY_TOMB);
    REQUIRE(EnOkarinaTag_GetPendingSongAction(&tag, &play) == -1);
    tag.type = 4;
    Flags_SetEventChkInf(EVENTCHKINF_OPENED_THE_DOOR_OF_TIME);
    REQUIRE(EnOkarinaTag_GetPendingSongAction(&tag, &play) == -1);
    std::memset(gSaveContext.eventChkInf, 0, sizeof(gSaveContext.eventChkInf));
    REQUIRE(EnOkarinaTag_GetPendingSongAction(&tag, &play) == OCARINA_ACTION_CHECK_TIME);
    tag.actionFunc = func_80ABEF2C;
    tag.ocarinaSong = OCARINA_ACTION_CHECK_LULLABY - OCARINA_ACTION_CHECK_SARIA;
    REQUIRE(EnOkarinaTag_GetPendingSongAction(&tag, &play) == OCARINA_ACTION_CHECK_LULLABY);
    TestSongMarkerCompletion();
    REQUIRE(!SongEvents_OotShouldCapture(OCARINA_ACTION_CHECK_MINUET,OCARINA_SONG_MINUET));
    for(unsigned seed=0;seed<200;++seed) {
        ctx->seed=seed;ctx->GenerateSongEvents();
        for(unsigned event=0;event<18;++event) {
            logic.available.clear();REQUIRE(!logic.CanPlaySongEvent(event));
            logic.available.insert(items[ctx->songEvents[event]]);REQUIRE(logic.CanPlaySongEvent(event));
            play.sceneNum=scenes[event];
            REQUIRE(SongEvents_OotAssignedSong(scenes[event], rooms[event], spawns[event],
                                              actions[Defaults(Game::Oot)[event]]) == ctx->songEvents[event]);
            REQUIRE(!SongEvents_OotIsEventAction(actions[ctx->songEvents[event]]));
            int selected=SongEvents_OotAction(scenes[event],rooms[event],spawns[event],actions[Defaults(Game::Oot)[event]]);
            REQUIRE(selected==actions[ctx->songEvents[event]]);
            REQUIRE(SongEvents_OotShouldCapture(selected,selected-OCARINA_ACTION_CHECK_MINUET));
            REQUIRE(!SongEvents_OotShouldCapture(OCARINA_ACTION_FREE_PLAY_DONE,selected-OCARINA_ACTION_CHECK_MINUET));
            // A different native request in the same scene is not a song event.
            int unrelated=actions[(Defaults(Game::Oot)[event]+1)%12];
            REQUIRE(SongEvents_OotAssignedSong(scenes[event], rooms[event], spawns[event], unrelated) == -1);
            REQUIRE(SongEvents_OotAction(scenes[event],rooms[event],spawns[event],unrelated)==unrelated);
            REQUIRE(!SongEvents_OotShouldCapture(selected,selected-OCARINA_ACTION_CHECK_MINUET));
        }
        auto saved=ctx->ExportSongEvents();ctx->songEvents.fill(255);ctx->ImportSongEvents({{"songEvents",saved}});
        REQUIRE(saved==ctx->ExportSongEvents());
    }
    // Execute the real message-mode and recognition-mask branches for all six
    // warp assignments. Owned inventory, not a forced single-song mask, drives audio.
    for(unsigned i=0;i<32;++i)gBitFlags[i]=1u<<i;
    for(unsigned warp=0;warp<6;++warp){
        ctx->songEvents[0]=warp+6;play.sceneNum=SCENE_TEMPLE_OF_TIME;
        int action=SongEvents_OotAction(play.sceneNum,0,0,OCARINA_ACTION_CHECK_TIME);
        REQUIRE(action==OCARINA_ACTION_CHECK_MINUET+warp&&SongEvents_OotIsEventAction(action));
        play.msgCtx.ocarinaAction=action;gSaveContext.inventory.questItems=0;
        DispatchNativeEventCheck(&play);
        REQUIRE(play.msgCtx.msgMode==MSGMODE_OCARINA_PLAYING&&observedRecognitionFlags==0);
        gSaveContext.inventory.questItems=1u<<(QUEST_SONG_MINUET+warp);
        DispatchNativeEventCheck(&play);
        REQUIRE(play.msgCtx.msgMode==MSGMODE_OCARINA_PLAYING&&observedRecognitionFlags==(1u<<warp));
        REQUIRE(SongEvents_OotShouldCapture(action,warp));
        gSaveContext.inventory.questItems=1u<<(QUEST_SONG_MINUET+(warp+1)%6);
        DispatchNativeEventCheck(&play);
        REQUIRE(observedRecognitionFlags==(1u<<((warp+1)%6))&&!(observedRecognitionFlags&(1u<<warp)));
        gSaveContext.inventory.questItems=1u<<(QUEST_SONG_MINUET+warp);
        SongEvents_OotAction(SCENE_HYRULE_FIELD,0,0,OCARINA_ACTION_FREE_PLAY);
        REQUIRE(!SongEvents_OotIsEventAction(action));
        play.msgCtx.ocarinaAction=OCARINA_ACTION_FREE_PLAY;DispatchNativeEventCheck(&play);
        REQUIRE(observedRecognitionFlags==((1u<<warp)+0xc000)); // normal travel/free play is unchanged
        play.msgCtx.ocarinaAction=OCARINA_ACTION_PLAYBACK_MINUET;DispatchNativeEventCheck(&play);
        REQUIRE(play.msgCtx.msgMode==MSGMODE_SONG_PLAYBACK&&observedRecognitionFlags==0x8001); // native teacher playback
        ctx->GetOption(RSK_SHUFFLE_SONG_EVENTS).value=0;play.msgCtx.ocarinaAction=action;DispatchNativeEventCheck(&play);
        REQUIRE(play.msgCtx.msgMode==MSGMODE_SONG_PLAYBACK);ctx->GetOption(RSK_SHUFFLE_SONG_EVENTS).value=1;
    }
    auto overflow=ctx->ExportSongEvents();overflow[0]=256;bool rejected=false;
    try{ctx->ImportSongEvents({{"songEvents",overflow}});}catch(const std::invalid_argument&){rejected=true;}REQUIRE(rejected);
    REQUIRE(SongEvents_OotAction(SCENE_SPIRIT_TEMPLE,99,0,123)==123);
    masterQuest=true;play.sceneNum=SCENE_SPIRIT_TEMPLE;
    REQUIRE(SongEvents_OotAction(SCENE_SPIRIT_TEMPLE,99,0,OCARINA_ACTION_CHECK_LULLABY)==actions[ctx->songEvents[13]]);
    REQUIRE(SongEvents_OotAction(SCENE_SPIRIT_TEMPLE,99,0,OCARINA_ACTION_CHECK_EPONA)==OCARINA_ACTION_CHECK_EPONA);
    masterQuest=false;
    ctx->GetOption(RSK_SHUFFLE_SONG_EVENTS).value=0;REQUIRE(SongEvents_OotAction(SCENE_TEMPLE_OF_TIME,0,0,123)==123);
    REQUIRE(SongEvents_OotAssignedSong(SCENE_TEMPLE_OF_TIME, 0, 0, OCARINA_ACTION_CHECK_TIME) == -1);
    ctx->GetOption(RSK_SHUFFLE_CLOCK).value=1;
    for(unsigned mode=0;mode<3;++mode) {
        ctx->GetOption(RSK_CLOCK_START).value=mode;memset(gSaveContext.ship.randomizerInf,0,sizeof(gSaveContext.ship.randomizerInf));
        gSaveContext.dayTime=0x1234;gSaveContext.sunsSongState=SUNSSONG_START;
        REQUIRE(SongEvents_OotClockLocked() && !logic.ClockAllowsTimePass());SongEvents_OotApplyClockLock();
        auto locked=ClockStart(mode,ctx->seed);
        REQUIRE(gSaveContext.dayTime==locked && gSaveContext.skyboxTime==locked && gSaveContext.nextDayTime==0xffff && gSaveContext.sunsSongState==SUNSSONG_START);
        REQUIRE(SongEvents_OotSunSongTimeLocked());
        gSaveContext.sunsSongState=SUNSSONG_SPECIAL;
        EnRd undead{};EnRd_SetupStunned(&undead);
        REQUIRE(undead.stunnedBySunsSong && undead.sunsSongStunTimer==0x258);
        Flags_SetRandomizerInf(RAND_INF_CLOCK);Flags_SetRandomizerInf(RAND_INF_CLOCK);
        REQUIRE(!SongEvents_OotClockLocked() && logic.ClockAllowsTimePass());
        REQUIRE(SongEvents_OotSunSongTimeLocked()); // A freeze-only request cannot start delayed fast-forward after the grant.
        gSaveContext.sunsSongState=SUNSSONG_START;REQUIRE(!SongEvents_OotSunSongTimeLocked());
        gSaveContext.dayTime=0x1234;SongEvents_OotApplyClockLock();REQUIRE(gSaveContext.dayTime==0x1234);
        static SaveContext saved;saved=gSaveContext;gSaveContext={};gSaveContext=saved;REQUIRE(!SongEvents_OotClockLocked());
    }
    // Actual non-root ReachabilitySearch initialization must honor the same
    // Clock gate as Region::ApplyTimePass for each native age/time identity.
    for(auto time:{RAT_CHILD_DAY,RAT_CHILD_NIGHT,RAT_ADULT_DAY,RAT_ADULT_NIGHT}){
        Region region;memset(gSaveContext.ship.randomizerInf,0,sizeof(gSaveContext.ship.randomizerInf));
        SeedRegionAgeTime(&region,time);
        REQUIRE(unsigned(region.childDay)+region.childNight+region.adultDay+region.adultNight==1);
        Flags_SetRandomizerInf(RAND_INF_CLOCK);region=Region{};SeedRegionAgeTime(&region,time);
        REQUIRE(unsigned(region.childDay)+region.childNight+region.adultDay+region.adultNight==2);
        memset(gSaveContext.ship.randomizerInf,0,sizeof(gSaveContext.ship.randomizerInf));
        ctx->GetOption(RSK_SHUFFLE_CLOCK).value=0;region=Region{};SeedRegionAgeTime(&region,time);
        REQUIRE(unsigned(region.childDay)+region.childNight+region.adultDay+region.adultNight==2);
        ctx->GetOption(RSK_SHUFFLE_CLOCK).value=1;
    }
    // A frozen adult-night start can use the pedestal inside Castle Town.
    // Child savewarp is a separate option and loses that inside-the-gate position.
    auto& root = *RegionTable(RR_ROOT);
    auto& pedestal = *RegionTable(RR_TOT_BEYOND_DOOR_OF_TIME);
    root.timePass = pedestal.timePass = false;
    root.adultNight = pedestal.adultNight = true;
    GetAccessibleLocationsStruct fromStart;
    PropagateTimeTravel(fromStart);
    REQUIRE(root.childNight && pedestal.childNight && fromStart.logicUpdated);
    REQUIRE(!root.childDay && !pedestal.childDay);
    fromStart.logicUpdated = false;
    PropagateTimeTravel(fromStart);
    REQUIRE(!fromStart.logicUpdated);

    NativeTrackerSeedRoot(RAT_CHILD_NIGHT);
    pedestal.ResetVariables();
    Region field;
    SeedRegionAgeTime(&field, RAT_CHILD_NIGHT);
    field.ApplyTimePass();
    GetAccessibleLocationsStruct fromSavewarp;
    PropagateTimeTravel(fromSavewarp);
    REQUIRE(root.childNight && !root.Adult() && !pedestal.Child() && !field.childDay);
    REQUIRE(!FieldCanReachMarket() && !MarketCanReachField() && !CanWarpWithPrelude());

    trackerLogic.available.insert(RG_PRELUDE_OF_LIGHT);
    REQUIRE(CanWarpWithPrelude());
    REQUIRE(!FieldCanReachMarket());
    pedestal.childNight = true; // Vanilla Prelude destination.
    PropagateTimeTravel(fromSavewarp);
    REQUIRE(pedestal.adultNight && root.adultNight && !root.adultDay);

    // An entrance layout can lead to a time-capable area before the Clock is collected.
    Region entranceProbe;
    entranceProbe.childDay = entranceProbe.childNight = entranceProbe.adultDay = entranceProbe.adultNight = true;
    GetAccessibleLocationsStruct preflight;
    preflight.haveTimeAccess = false;
    NativeEntranceTimePreflight(&entranceProbe, preflight);
    REQUIRE(preflight.haveTimeAccess && !entranceProbe.TimePass());

    Flags_SetRandomizerInf(RAND_INF_CLOCK);
    field.ApplyTimePass();
    REQUIRE(field.childDay && root.childDay);
    trackerLogic.AtDay = true;
    REQUIRE(FieldCanReachMarket() && MarketCanReachField());
    trackerLogic.AtDay = false;
    trackerLogic.IsAdult = true;
    REQUIRE(FieldCanReachMarket() && MarketCanReachField());
    gSaveContext.ship.quest.id=QUEST_NORMAL;REQUIRE(!SongEvents_OotClockLocked());
}
