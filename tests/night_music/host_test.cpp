// Native state/queues are boundary fixtures; the complete host updater is production.
#include "audio/NightMusic.h"
#include <cassert>
#include <cstring>
#include <map>
#include <vector>
#include <iostream>
using u8=uint8_t;
constexpr uint16_t NA_BGM_DISABLED=0xffff,NA_BGM_NO_MUSIC=0x7f,NA_BGM_GENERAL_SFX=0;
constexpr uint16_t NA_BGM_NATURE_AMBIENCE=1,NA_BGM_FIELD_LOGIC=2;
constexpr int NATURE_ID_NONE=19,AMBIENCE_ID_13=19,SCENE_HYRULE_FIELD=81,SCENE_LON_LON_RANCH=99;
constexpr int GAMEMODE_NORMAL=0,CS_STATE_IDLE=0,CS_ID_NONE=-1;
constexpr int SEQ_PLAYER_BGM_MAIN=0,SEQ_PLAYER_FANFARE=1,SEQ_PLAYER_BGM_SUB=3;
struct PlayState {
    int sceneNum=82,sceneId=82;
    struct { uint16_t seqId=0x22; uint8_t natureAmbienceId=2; } sequenceCtx;
    struct { uint16_t seqId=0x22; uint8_t ambienceId=2; } sceneSequences;
    struct { int state=0; } csCtx;
    int transitionTrigger=0;
};
static struct {
    int gameMode=0,fileNum=0,forcedSeqId=0,magicState=0;
    uint16_t dayTime=0xd000;
    struct { int day=2; uint16_t time=0xd000; } save;
} gSaveContext;
#define CURRENT_TIME gSaveContext.save.time
#define CLOCK_TIME(hour, minute) ((uint16_t)(((hour)*60+(minute))*65536/1440))
static PlayState play;
static PlayState* gPlayState=&play;
static uint16_t active[4]{1,0xffff,0xffff,0xffff},resolvedSub=0xffff,registered=0xffff;
static bool pending=false,combat=false,finalHours=false,cutscene=false;
static std::map<std::string,int> cvars;
static int lookups=0;
static uint16_t lastLookup=0,sceneRow=0;
struct Command { int player; uint16_t id; bool stop; };
static std::vector<Command> queue,history;
static void Queue(int player,uint16_t id,bool stop=false) {
    queue.push_back({player,id,stop}); history.push_back({player,id,stop}); pending=true;
}
static void Flush() {
    for (auto cmd:queue) {
        active[cmd.player]=cmd.stop?0xffff:cmd.id;
        if (cmd.player==3) { resolvedSub=cmd.stop?0xffff:cmd.id; active[3]=cmd.stop?0xffff:cmd.id&0xff; }
    }
    queue.clear();pending=false;
}
static int CVarGetInteger(const char* name,int fallback) {
    const auto it=cvars.find(name);return it==cvars.end()?fallback:it->second;
}
static uint16_t func_800FA0B4(int p) { return active[p]; }
static uint16_t AudioSeq_GetActiveSeqId(int p) { return active[p]; }
#if TEST_MM
static bool Player_InCsMode(PlayState*) { return cutscene; }
#else
using s32=int;
constexpr int PLAYER_STATE1_LOADING=1,PLAYER_STATE1_TALKING=1<<6,PLAYER_STATE1_DEAD=1<<7;
constexpr int PLAYER_STATE1_IN_CUTSCENE=1<<29,PLAYER_STATE3_FLYING_WITH_HOOKSHOT=1<<7;
constexpr int TRANS_TRIGGER_START=1,MAGIC_STATE_IDLE=0;
struct Actor { int textId=0; };
struct Player {
    uint32_t stateFlags1=0,stateFlags3=0;
    int csAction=0,unk_6AD=0,itemAction=0;
    Actor actor;
    Actor *talkActor=nullptr,*focusActor=nullptr;
};
static Player playerState;
#define GET_PLAYER(play) (&playerState)
static int Player_ActionToMagicSpell(Player*,int) { return -1; }
static void Player_Action_Talk() {}
static void Player_SetupActionPreserveAnimMovement(PlayState*,Player*,void(*)(),int) {}
static void Message_StartTextbox(PlayState*,int,Actor*) {}
/* NATIVE_PLAYER */
#endif
static int CutsceneManager_GetCurrentCsId() { return cutscene?1:-1; }
static bool Environment_IsFinalHours(PlayState*) { return finalHours; }
static bool AudioSeq_IsSeqCmdNotQueued(uint32_t,uint32_t) { return !pending; }
static bool Audio_HasPendingSubBgmCommand() { return pending; }
static bool Audio_IsSceneNightMusicInCombat() { return combat; }
static bool Audio_IsSceneNightBgmActive() { return registered!=0xffff&&active[3]!=0xffff&&registered==resolvedSub; }
static void Audio_RegisterSceneNightBgm(uint16_t id) { registered=id; }
static void Audio_SetVolScale(int p,int,int,int) { assert(p==3); }
static void Audio_QueueResolvedSeqCmd(int p,uint16_t id,int) { Queue(p,id); }
static void Audio_QueueSeqCmd(uint32_t cmd) { assert(cmd>>28==1);Queue((cmd>>24)&15,0xffff,true); }
static void Audio_StopSceneNightBgm() {
    if(Audio_IsSceneNightBgmActive()&&!pending) Queue(3,0xffff,true);
    registered=0xffff;
}
static void Audio_InvalidateSceneSequence() {}
static uint16_t AudioEditor_GetSceneMusicRow(uint16_t) { return sceneRow; }
struct AudioCollection {
    static AudioCollection* Instance;
    bool HasSequenceNum(uint16_t id) { return id!=0xffff&&id!=0x7f; }
    uint16_t GetReplacementSequence(uint16_t id) { ++lookups;lastLookup=id;return id==0xff00?0x1c6:0x15d; }
};
static AudioCollection collection;
AudioCollection* AudioCollection::Instance=&collection;
static void Audio_PlaySceneSequence(uint16_t id,u8 day) {
    assert(id==0x22&&day==1); // Native identity/day IO retained; one downstream resolution.
    collection.GetReplacementSequence(sceneRow?sceneRow:id);
    Queue(0,id);
}
#define SEQCMD_STOP_SEQUENCE(player,fade) Queue(player,0xffff,true)
/* PRODUCTION */

int main() {
    constexpr int player=TEST_MM?0:3;
    const char* hostLabel=TEST_MM?"MM":"OoT";
    uint16_t expectedEntry=0x22;
#if !TEST_MM && TEST_DEBUG_WARP
    // Booting directly into Debug Warp uses a temporary save in normal gameplay.
    gSaveContext.gameMode=GAMEMODE_NORMAL;
    gSaveContext.fileNum=0xff;
    gSaveContext.dayTime=0x0000;
    play.sceneNum=0x55; // SCENE_KOKIRI_FOREST
    play.sequenceCtx.seqId=0x3c; // NA_BGM_KOKIRI
    play.sequenceCtx.natureAmbienceId=4; // NATURE_ID_KOKIRI_REGION
    expectedEntry=0x3c;
    hostLabel="OoT debug warp";
#endif
    auto frame=[] { UpdateSceneNightMusic(); };
    frame();assert(history.empty());
    cvars["gAudioEditor.EnableNightMusic"]=1;
#if !TEST_MM
    // Allowing debug gameplay must not enable music in title/demo or invalid slots.
    for(int mode:{1,2,3}) { // Title/demo, file select, end credits.
        gSaveContext.gameMode=mode;frame();
        assert(history.empty()&&!sSceneNight.Owns());
    }
    gSaveContext.gameMode=GAMEMODE_NORMAL;
    const auto initialFileNum=gSaveContext.fileNum;
    for(int file:{3,0xfe,0x100}) {
        gSaveContext.fileNum=file;frame();
        assert(history.empty()&&!sSceneNight.Owns());
    }
    gSaveContext.fileNum=initialFileNum;
    play.sequenceCtx.seqId=NA_BGM_NO_MUSIC;frame();
    assert(history.empty()&&!sSceneNight.Owns());
    play.sequenceCtx.seqId=expectedEntry;
    const auto initialAmbience=play.sequenceCtx.natureAmbienceId;
    play.sequenceCtx.natureAmbienceId=NATURE_ID_NONE;frame();
    assert(history.empty()&&!sSceneNight.Owns());
    play.sequenceCtx.natureAmbienceId=initialAmbience;
    gSaveContext.forcedSeqId=0x3c;frame();
    assert(history.empty()&&!sSceneNight.Owns());
    gSaveContext.forcedSeqId=NA_BGM_GENERAL_SFX;
#endif
    if(TEST_MM) active[0]=0xffff;
    frame();assert(history.size()==1&&history.back().player==player&&lookups==1&&
                   "normal gameplay must start night music for a saved game or debug warp");
    if(!TEST_MM) assert(history.back().id==0x15d&&active[0]==1);
    frame();assert(history.size()==1); // Pending command must not be repeated/stopped.
    Flush();frame();assert(history.size()==1&&SceneNightMusic_CurrentEntry()==expectedEntry);
    SceneNightMusic_RequestRefresh();frame();Flush();assert(lookups==2);
#if !TEST_MM
    // Real talk setup sets IN_CUTSCENE even for ordinary gossip-stone dialogue.
    Player_SetupTalk(&play,&playerState);
    assert(Player_InCsMode(&play));
    const auto dialogue=history.size();
    const auto dialogueLookups=lookups;
    for(int page=0;page<3;++page) for(int tick=0;tick<60;++tick) { frame();Flush(); }
    assert(history.size()==dialogue && "ordinary dialogue must not stop/restart night music");
    assert(lookups==dialogueLookups&&resolvedSub==0x15d&&sSceneNight.Owns());
    playerState.stateFlags1=0;
    frame();Flush();assert(history.size()==dialogue); // Closing must retain playback position.
    Player_SetupTalk(&play,&playerState);
    SceneNightMusic_RequestRefresh();frame();Flush();
    assert(history.size()==dialogue); // Defer requested edits while player is busy.
    playerState.stateFlags1=0;frame();Flush();
    assert(history.size()==dialogue+1&&!history.back().stop&&lookups==dialogueLookups+1);
    // A scripted cutscene remains an audio override, including dialogue inside it.
    Player_SetupTalk(&play,&playerState);
    play.csCtx.state=1;frame();Flush();
    assert(history.back().stop&&history.back().player==3&&!sSceneNight.Owns());
    play.csCtx.state=0;playerState.stateFlags1=0;frame();Flush();
    assert(!history.back().stop&&sSceneNight.Owns());
#endif
    const auto before=history.size();
#if !TEST_MM
    Player_SetupTalk(&play,&playerState);
#endif
    active[1]=0x20;frame();
    if(!TEST_MM) { assert(history.size()==before+1&&history.back().stop&&history.back().player==3);Flush(); }
    else assert(history.size()==before);
    active[1]=0xffff;
#if !TEST_MM
    playerState.stateFlags1=0;
#endif
    frame();Flush();
    if(!TEST_MM) assert(active[0]==1&&resolvedSub==0x15d);
    cvars["gAudioEditor.Playing"]=0x50;active[0]=0x50;
    const auto preview=history.size();frame();Flush();
    assert(TEST_MM?history.size()==preview:history.back().stop&&history.back().player==3);
    cvars["gAudioEditor.Playing"]=0;active[0]=TEST_MM?uint16_t(0xffff):uint16_t(1);
    frame();Flush();assert(!history.back().stop&&history.back().player==player);
    if(!TEST_MM) {
        combat=true;Queue(3,0x081a);const auto n=history.size();frame();assert(history.size()==n);
        Flush();frame();assert(history.size()==n); // Never overwrite the queued battle command.
        combat=false;active[3]=0xffff;frame();Flush();assert(resolvedSub==0x15d);
        resolvedSub=0x25d;active[3]=0x5d;const auto n2=history.size();frame();
        assert(history.size()==n2&&!Audio_IsSceneNightBgmActive()); // Same low byte is a different owner.
        active[3]=0xffff;frame();Flush();
    }
#if !TEST_MM
    Player_SetupTalk(&play,&playerState);
#endif
    cvars["gAudioEditor.EnableNightMusic"]=0;frame();Flush();
    assert(history.back().stop&&history.back().player==player);
    const auto off=history.size();frame();assert(history.size()==off);
#if !TEST_MM
    playerState.stateFlags1=0;frame();assert(history.size()==off);
#endif
    cvars["gAudioEditor.EnableNightMusic"]=1;frame();Flush();
    gSaveContext.dayTime=gSaveContext.save.time=0x7000;
#if !TEST_MM
    Player_SetupTalk(&play,&playerState);
#endif
    const auto dawn=history.size();frame();Flush();
    assert(TEST_MM?history.size()==dawn:history.size()==dawn+1&&history.back().stop);
#if !TEST_MM
    playerState.stateFlags1=0;
#endif
    // Re-entry to a new scene cannot stop its native music or inherit ownership.
    play.sceneNum=play.sceneId=90;active[0]=0x70;
    const auto transition=history.size();frame();assert(history.size()==transition&&!sSceneNight.Owns());
    gSaveContext.dayTime=gSaveContext.save.time=0xd000;
    active[0]=TEST_MM?uint16_t(0xffff):uint16_t(1);active[3]=0xffff;
    sceneRow=0xff00;frame();Flush();assert(lastLookup==0xff00);
    assert(history.back().id!=0xff00); // Virtual row metadata never enters a player.
    if(!TEST_MM) {
        const auto cleanup=history.size();
        ResetSceneNightMusic();Flush();
        assert(history.size()==cleanup+1&&history.back().stop&&history.back().player==3);
        assert(!Audio_IsSceneNightBgmActive());
        play.sceneNum=81;play.sequenceCtx.seqId=2;cvars["gAudioEditor.HyruleFieldNightMusic"]=1;
        frame();Flush();const auto priority=history.size();frame();assert(history.size()==priority&&!sSceneNight.Owns());
    } else {
        finalHours=true;active[0]=0x60;const auto n=history.size();frame();assert(history.size()==n);
    }
    std::cout<<"PASS "<<hostLabel<<" production night adapter: custom resolution, ambience player preservation, preview/combat/fanfare, dawn, toggle, scene boundary\n";
}
