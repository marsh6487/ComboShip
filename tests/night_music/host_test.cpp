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
};
static struct {
    int gameMode=0,fileNum=0,forcedSeqId=0;
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
static bool Player_InCsMode(PlayState*) { return cutscene; }
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
    auto frame=[] { UpdateSceneNightMusic(); };
    frame();assert(history.empty());
    cvars["gAudioEditor.EnableNightMusic"]=1;
    if(TEST_MM) active[0]=0xffff;
    frame();assert(history.size()==1&&history.back().player==player&&lookups==1);
    if(!TEST_MM) assert(history.back().id==0x15d&&active[0]==1);
    frame();assert(history.size()==1); // Pending command must not be repeated/stopped.
    Flush();frame();assert(history.size()==1&&SceneNightMusic_CurrentEntry()==0x22);
    SceneNightMusic_RequestRefresh();frame();Flush();assert(lookups==2);
    const auto before=history.size();
    active[1]=0x20;frame();
    if(!TEST_MM) { assert(history.size()==before+1&&history.back().stop&&history.back().player==3);Flush(); }
    else assert(history.size()==before);
    active[1]=0xffff;frame();Flush();
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
    cvars["gAudioEditor.EnableNightMusic"]=0;frame();Flush();
    assert(history.back().stop&&history.back().player==player);
    const auto off=history.size();frame();assert(history.size()==off);
    cvars["gAudioEditor.EnableNightMusic"]=1;frame();Flush();
    gSaveContext.dayTime=gSaveContext.save.time=0x7000;
    const auto dawn=history.size();frame();Flush();
    assert(TEST_MM?history.size()==dawn:history.size()==dawn+1&&history.back().stop);
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
    std::cout<<"PASS "<<(TEST_MM?"MM":"OoT")<<" production night adapter: custom resolution, ambience player preservation, preview/combat/fanfare, dawn, toggle, scene boundary\n";
}
