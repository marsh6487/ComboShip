// Production adapter bodies, actual native SaveContext/flag operations; the
// Context and CanUse services are explicit inventory boundaries.
#include "z64.h"
#include "macros.h"
#include "src/overlays/actors/ovl_En_Rd/z_en_rd.h"
#include "combo/rando/SongEvents.h"
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
#define LUSLOG_INFO(...) ((void)0)
#define GameInteractor_ExecuteOnFlagSet(...) ((void)0)
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
    bool CanUse(RandomizerGet item){return available.count(item);}
    bool HasItem(RandomizerGet item){return item==RG_CLOCK && (gSaveContext.ship.randomizerInf[RAND_INF_CLOCK/16]&(1<<(RAND_INF_CLOCK%16)));}
    bool CanPlaySongEvent(unsigned);bool ClockAllowsTimePass();
};
}
#define RAND_GET_OPTION(key) Rando::Context::GetInstance()->GetOption(key)
using namespace ComboRando::SongEvents;
#include "oot_native_functions.inc"
int main() {
    static PlayState play{};gPlayState=&play;
    auto ctx=Rando::Context::GetInstance();Rando::Logic logic;
    gSaveContext.ship.quest.id=QUEST_RANDOMIZER;ctx->GetOption(RSK_SHUFFLE_SONG_EVENTS).value=1;
    const int scenes[]={SCENE_TEMPLE_OF_TIME,SCENE_WINDMILL_AND_DAMPES_GRAVE,SCENE_GRAVEYARD,SCENE_ZORAS_RIVER,SCENE_GORON_CITY,SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS,SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS,SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS,SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC,SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC,SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC,SCENE_WATER_TEMPLE,SCENE_SHADOW_TEMPLE,SCENE_SPIRIT_TEMPLE,SCENE_SPIRIT_TEMPLE,SCENE_SPIRIT_TEMPLE,SCENE_BOTTOM_OF_THE_WELL,SCENE_INSIDE_GANONS_CASTLE};
    const int rooms[]={0,0,0,0,0,0,0,0,0,0,0,0,0,5,14,24,0,0};
    const int spawns[]={0,0,0,0,0,0,1,2,0,1,2,0,0,0,0,0,0,0};
    const RandomizerGet items[]={RG_ZELDAS_LULLABY,RG_EPONAS_SONG,RG_SARIAS_SONG,RG_SONG_OF_STORMS,RG_SUNS_SONG,RG_SONG_OF_TIME,RG_MINUET_OF_FOREST,RG_BOLERO_OF_FIRE,RG_SERENADE_OF_WATER,RG_REQUIEM_OF_SPIRIT,RG_NOCTURNE_OF_SHADOW,RG_PRELUDE_OF_LIGHT};
    const int actions[]={OCARINA_ACTION_CHECK_LULLABY,OCARINA_ACTION_CHECK_EPONA,OCARINA_ACTION_CHECK_SARIA,OCARINA_ACTION_CHECK_STORMS,OCARINA_ACTION_CHECK_SUNS,OCARINA_ACTION_CHECK_TIME,OCARINA_ACTION_CHECK_MINUET,OCARINA_ACTION_CHECK_BOLERO,OCARINA_ACTION_CHECK_SERENADE,OCARINA_ACTION_CHECK_REQUIEM,OCARINA_ACTION_CHECK_NOCTURNE,OCARINA_ACTION_CHECK_PRELUDE};
    REQUIRE(!SongEvents_OotShouldCapture(OCARINA_ACTION_CHECK_MINUET,OCARINA_SONG_MINUET));
    for(unsigned seed=0;seed<200;++seed) {
        ctx->seed=seed;ctx->GenerateSongEvents();
        for(unsigned event=0;event<18;++event) {
            logic.available.clear();REQUIRE(!logic.CanPlaySongEvent(event));
            logic.available.insert(items[ctx->songEvents[event]]);REQUIRE(logic.CanPlaySongEvent(event));
            play.sceneNum=scenes[event];
            int selected=SongEvents_OotAction(scenes[event],rooms[event],spawns[event],actions[Defaults(Game::Oot)[event]]);
            REQUIRE(selected==actions[ctx->songEvents[event]]);
            REQUIRE(SongEvents_OotShouldCapture(selected,selected-OCARINA_ACTION_CHECK_MINUET));
            REQUIRE(!SongEvents_OotShouldCapture(OCARINA_ACTION_FREE_PLAY_DONE,selected-OCARINA_ACTION_CHECK_MINUET));
            // A different native request in the same scene is not a song event.
            int unrelated=actions[(Defaults(Game::Oot)[event]+1)%12];
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
        NativeTrackerSeedTime(&region,time);
        REQUIRE(unsigned(region.childDay)+region.childNight+region.adultDay+region.adultNight==1);
        Flags_SetRandomizerInf(RAND_INF_CLOCK);region=Region{};NativeTrackerSeedTime(&region,time);
        REQUIRE(unsigned(region.childDay)+region.childNight+region.adultDay+region.adultNight==2);
        memset(gSaveContext.ship.randomizerInf,0,sizeof(gSaveContext.ship.randomizerInf));
        ctx->GetOption(RSK_SHUFFLE_CLOCK).value=0;region=Region{};NativeTrackerSeedTime(&region,time);
        REQUIRE(unsigned(region.childDay)+region.childNight+region.adultDay+region.adultNight==2);
        ctx->GetOption(RSK_SHUFFLE_CLOCK).value=1;
    }
    gSaveContext.ship.quest.id=QUEST_NORMAL;REQUIRE(!SongEvents_OotClockLocked());
}
