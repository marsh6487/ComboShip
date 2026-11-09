#include <nlohmann/json.hpp>
#include "global.h"
#include "mods/voice_pack/link_voice_mixer.h"
#include "mods/forms/custom_forms.h"
#include "mods/items/logic/adult_link_render.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <fstream>
#include <iterator>
#include <memory>
#include <random>
#include <vector>
#include <cassert>
#include <cstring>
#include <string>
#include <sstream>
#include <cstdio>

// Capture the diagnostic boundary, keeping production routing and callback
// execution real without compiling a logging backend into this fixture.
struct LogRecord { std::string level, message; std::vector<std::string> arguments; };
static std::vector<LogRecord> logs;
template <typename T> static std::string LogArgument(const T& value) {
    std::ostringstream output;output<<value;return output.str();
}
template <typename... Args> static void RecordLog(const char* level,const char* message,const Args&... args) {
    logs.push_back({level,message,{LogArgument(args)...}});
}
#define SPDLOG_INFO(...) RecordLog("info",__VA_ARGS__)
#define SPDLOG_WARN(...) RecordLog("warning",__VA_ARGS__)

static std::string resourceRoot;
namespace Ship {
struct File { std::shared_ptr<std::vector<char>> Buffer; };
struct ArchiveManager {
    std::shared_ptr<File> LoadFile(const std::string& path) {
        std::ifstream file(resourceRoot + "/" + path, std::ios::binary);
        if (!file) return nullptr;
        auto result = std::make_shared<File>();
        result->Buffer = std::make_shared<std::vector<char>>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        return result;
    }
};
struct ResourceManager {
    std::shared_ptr<ArchiveManager> GetArchiveManager() { static auto manager = std::make_shared<ArchiveManager>(); return manager; }
};
struct Context {
    static Context* GetRawInstance() { static Context context; return &context; }
    std::shared_ptr<ResourceManager> GetResourceManager() { static auto manager = std::make_shared<ResourceManager>(); return manager; }
};
struct CrossRMRegistry { static auto GetOrActive(const char*) { return Context::GetRawInstance()->GetResourceManager(); } };
}
PlayState* gPlayState;
SaveContext gSaveContext;
static PlayState play;
static Player player;
static PlayerAgeProperties ageProps;
PlayerAgeProperties sPlayerAgeProperties[PLAYER_FORM_MAX];
static const int16_t nativeAdultClip[]={2345,-5432,0};
static int defaultCalls;
static bool nativeAdultAvailable=true;
extern "C" const int16_t* OotAdultVoice_GetClip(unsigned action,unsigned variant,uint32_t* frames) {
    ++defaultCalls;*frames=nativeAdultAvailable ? 3 : 0;return nativeAdultAvailable ? nativeAdultClip : nullptr;
}
static float masterVolume=1, sfxVolume=1, editorPitch=1.5f;
static auto& adult=gSaveContext.save.shipSaveInfo.nei.timeGateAdultMode;
static int forceAdult, customForm = CUSTOM_FORM_NONE, editor, giantCalls;
static u8 wolfSelected;
int32_t CVarGetInteger(const char* name, int32_t fallback) {
    if (!strcmp(name,"gAdultLink.ForceOn")) return forceAdult;
    return !strcmp(name,"gAudioEditor.LinkVoiceFreqMultiplier.Enable") ? editor : fallback;
}
float CVarGetFloat(const char* name, float fallback) {
    if (!strcmp(name,"gSettings.Audio.MasterVolume")) return masterVolume;
    if (!strcmp(name,"gSettings.Audio.SoundEffectsVolume")) return sfxVolume;
    if (!strcmp(name,"gAudioEditor.LinkVoiceFreqMultiplier.Scale")) return editorPitch;
    return fallback;
}
#include "age_state.inc"
extern "C" u8 WolfLinkForm_IsSelected() { return wolfSelected; }
extern "C" s32 CustomForms_ActiveForm() { return customForm; }
u16 GaroForm_VoiceFor(u16 id) { return 0x68F0; }
#include "form_voice.inc"
#include "runtime.inc"
static GameInteractor interactor;
GameInteractor* GameInteractor::Instance=&interactor;
#include "interactor.inc"
#include "sfx_queue.inc"
#include "pitch_hook.inc"
#include "player_sfx.inc"
void Audio_PlaySfx_GiantsMask(Vec3f* pos, u16 id) { ++giantCalls; }
void Audio_PlaySfx_AtPosWithPresetLowFreqAndHighReverb(Vec3f* pos,u16 id) { ++giantCalls; }
bool WolfLinkHost_PlayClimbVoice(Player* p,u16 id) { return false; }
#define this nativePlayer
#include "native_voice.inc"
#undef this
#include "item_voice.inc"
// Headless native synth/scheduler boundaries. The production buffer routine must
// call LinkVoice_MixInto itself; removing that seam makes the PCM comparison fail.
AudioContext gAudioCtx;
void AudioLoad_DecreaseSampleDmaTtls() {}
void AudioLoad_ProcessLoads(s32 resetStatus) {}
void AudioLoad_ProcessScriptLoads() {}
s32 AudioHeap_ResetStep() { return 1; }
s32 osSendMesg(OSMesgQueue* queue,OSMesg message,s32 flags) { return 0; }
s32 osRecvMesg(OSMesgQueue* queue,OSMesg* message,s32 flags) { return -1; }
void AudioThread_ProcessCmds(u32 message) {}
s32 AudioThread_ScheduleProcessCmds() { return 0; }
Acmd* AudioSynth_Update(Acmd* commands,s32* written,s16* output,s32 frames) {
    *written=0;std::fill_n(output,frames*2,0);return commands;
}
void Sm64Audio_MixInto(int16_t* output,uint32_t frames) {}
void StasisSfx_MixInto(s16* output,u32 frames) {}
void HourglassSfx_MixInto(s16* output,u32 frames) {}
void MM_WolfLinkSfx_MixInto(s16* output,u32 frames) {}
u32 osGetCount() { return 0; }
#include "audio_mix.inc"
static void Flush() {
    LinkVoice_Stop(0,nullptr); int16_t out[16]={}; LinkVoice_MixInto(out,8);
    sSfxRequestWriteIndex=sSfxRequestReadIndex=0;
}
static void CheckQueued(u16 id,float pitch) {
    assert(sSfxRequestWriteIndex==1 && sSfxRequestReadIndex==0);
    const auto& request=sSfxRequests[0];
    assert(request.sfxId==id && request.pos==&player.actor.projectedPos && request.token==4);
    if(*request.freqScale!=pitch) {
        std::fprintf(stderr,"native voice 0x%04X: expected pitch %.2f, got %.2f\n",id,pitch,*request.freqScale);
        std::abort();
    }
    assert(*request.volume==1.0f && *request.reverbAdd==0);
}
static size_t LogCount(const char* message,const char* level="info") {
    return std::count_if(logs.begin(),logs.end(),[&](const auto& record) {
        return record.level==level && record.message.find(message)!=std::string::npos;
    });
}
static void CheckRoute(const char* age,const char* source,float pitch,bool editorEnabled) {
    bool found=false;
    for(const auto& record:logs) {
        if(record.message.find("age={} source={}")==std::string::npos || record.arguments.size()!=7)
            continue;
        const auto& args=record.arguments;
        if(args[0]!=age || args[1]!=source || std::stof(args[5])!=pitch || args[6]!=LogArgument(editorEnabled))
            continue;
        assert(!args[3].empty() && std::stoul(args[4])>0);
        found=true;
    }
    assert(found);
}
static void CheckPitchedOptional(unsigned age) {
    const auto& pcm=banks[age][5][selectedVariant[age][5]];
    assert(pcm.size()>=6);
    // Four output frames at 1.5x visit donor positions 0, 1.5, 3, 4.5.
    const int16_t expected[]={pcm[0],static_cast<int16_t>((int(pcm[1])+pcm[2])/2),
                              pcm[3],static_cast<int16_t>((int(pcm[4])+pcm[5])/2)};
    int16_t output[8]={};LinkVoice_MixInto(output,4);
    for(unsigned i=0;i<4;++i)assert(output[i*2]==expected[i] && output[i*2+1]==expected[i]);
}
int main(int argc,char** argv) {
    assert(argc==3);resourceRoot=argv[1];nativeAdultAvailable=std::string(argv[2])=="available";
    RegisterLinksVoicePitchMultiplier();
    assert(LogCount("player voice hook registered")==1);
    assert(!AdultLink_IsActive());
    Nei_Save()->timeGateOwned=1;assert(!AdultLink_IsActive());
    Nei_Save()->timeGateOwned=0;adult=1;assert(AdultLink_IsActive());
    adult=0;forceAdult=1;assert(AdultLink_IsActive());
    forceAdult=0;assert(!AdultLink_IsActive());
    gPlayState=&play;play.actorCtx.actorLists[ACTORCAT_PLAYER].first=&player.actor;
    player.actor.id=ACTOR_PLAYER;player.transformation=PLAYER_FORM_HUMAN;
    ageProps.voiceSfxIdOffset=0x20;player.ageProperties=&ageProps;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first=nullptr;
    assert(!LinkVoice_TryPlay(0x6825,&player.actor.projectedPos));
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first=&player.actor;
    bool malformed=resourceRoot=="malformed";
    bool absent=resourceRoot=="missing" || malformed;
    const char* loadEvent=malformed ? "optional PCM pack rejected" : "optional PCM manifest unavailable";
    const char* loadLevel=malformed ? "warning" : "info";
    assert(LinkVoice_TryPlay(0x6825,&player.actor.projectedPos)==!absent);
    if(absent) {
        assert(!LinkVoice_TryPlay(0x6825,&player.actor.projectedPos));
        Player_AnimSfx_PlayVoice(&player,0x6805);CheckQueued(0x6825,1.0f);Flush();
        adult=1;Player_AnimSfx_PlayVoice(&player,0x6805);assert(defaultCalls==1);
        if(!nativeAdultAvailable) {
            // Both recording providers declined. The native request must never
            // disguise a child recording as an adult through automatic pitching.
            CheckQueued(0x6825,1.0f);
            for(u16 id:{0x6825,0x6025,0x6425}) {
                Flush();Player_PlaySfx(&player,id);Player_PlaySfx(&player,id);
                CheckQueued(id,1.0f); // Preserve the caller's native SFX flags.
            }
            Flush();editor=1;Player_AnimSfx_PlayVoice(&player,0x6805);CheckQueued(0x6825,1.5f);
            assert(LogCount("adult native provider unavailable","warning")==1);
            assert(LogCount(loadEvent,loadLevel)==1);
            return 0;
        }
        assert(sSfxRequestWriteIndex==0 && LinkVoice_IsPlaying(0x6825));
        int16_t output[6]={};LinkVoice_MixInto(output,3);
        for(int i=0;i<3;++i)assert(output[i*2]==nativeAdultClip[i]);
        CheckRoute("adult","oot-native",1.0f,false);
        Flush();editor=1;Player_AnimSfx_PlayVoice(&player,0x6805);
        assert(sSfxRequestWriteIndex==0);
        int16_t pitched[4]={};LinkVoice_MixInto(pitched,2);
        assert(pitched[0]==2345 && pitched[1]==2345 && pitched[2]==-2716 && pitched[3]==-2716);
        CheckRoute("adult","oot-native",1.5f,true);
        Flush();editor=0;adult=0;forceAdult=1;
        Player_AnimSfx_PlayVoice(&player,0x6805);assert(sSfxRequestWriteIndex==0 && LinkVoice_IsPlaying(0x6825));
        Flush();forceAdult=0;Player_AnimSfx_PlayVoice(&player,0x6805);CheckQueued(0x6825,1.0f);
        assert(LogCount("adult native provider unavailable","warning")==0);
        assert(LogCount(loadEvent,loadLevel)==1);
        assert(LogCount("optional PCM pack ready")==0);
        return 0;
    }
    Flush();
    assert(LinkVoice_TryPlay(0x6805,&player.actor.projectedPos)==0);
    Vec3f npc={};assert(LinkVoice_TryPlay(0x6825,&npc)==0);
    for(int form=0;form<PLAYER_FORM_HUMAN;++form) {
        player.transformation=form;assert(LinkVoice_TryPlay(0x6825,&player.actor.projectedPos)==0);
    }
    player.transformation=PLAYER_FORM_HUMAN;
    customForm=CUSTOM_FORM_GARO;assert(!LinkVoice_TryPlay(0x6825,&player.actor.projectedPos));customForm=CUSTOM_FORM_NONE;
    for(adult=0;adult<2;++adult) {
        for(unsigned action=0;action<32;++action) {
            Flush();
            ItemVoice_PlayId(&player,0x6800+action);
            assert(sSfxRequestWriteIndex==0);
            assert(LinkVoice_IsPlaying(0x6820+action));
            // Cycle selection varies across earlier probes, so compare to one exact donor variant.
            size_t index=selectedVariant[adult][action];
            const auto& selected=banks[adult][action][index];
            const unsigned frames=std::min<size_t>(selected.size(),4096);
            std::vector<int16_t> output(frames*2);
            AudioMgr_CreateNextAudioBuffer(output.data(),frames);
            for(unsigned i=0;i<frames;++i)assert(output[i*2]==selected[i] && output[i*2+1]==selected[i]);
        }
    }
    assert(defaultCalls==0);
    assert(LogCount("optional PCM pack ready")==1);
    assert(LogCount("age={} source={}")==2); // Steady playback does not flood the log.
    CheckRoute("child","optional-pcm",1.0f,false);
    CheckRoute("adult","optional-pcm",1.0f,false);
    for(adult=0;adult<2;++adult) {
        editor=1;
        for(u16 id:{0x6825,0x6025,0x6425}) {
            Flush();Player_PlaySfx(&player,id);
            assert(sSfxRequestWriteIndex==0 && LinkVoice_IsPlaying(0x6825));
            CheckPitchedOptional(adult);
        }
        CheckRoute(adult ? "adult" : "child","optional-pcm",1.5f,true);
    }
    editor=0;adult=0;forceAdult=1;Flush();Player_AnimSfx_PlayVoice(&player,0x6805);
    const auto& forced=banks[1][5][selectedVariant[1][5]];
    int16_t forcedOutput[8]={};LinkVoice_MixInto(forcedOutput,4);
    for(unsigned i=0;i<4;++i)assert(forcedOutput[i*2]==forced[i]);
    forceAdult=0;adult=1;
    Flush();
    for (float volume : {0.0f,0.5f}) {
        masterVolume=sfxVolume=volume;
        assert(LinkVoice_TryPlay(0x6825,&player.actor.projectedPos));
        size_t index=selectedVariant[1][5];
        const auto& pcm=banks[1][5][index];
        std::vector<int16_t> output(2048);
        LinkVoice_MixInto(output.data(),1024);
        for(unsigned i=0;i<std::min<size_t>(1024,pcm.size());++i)
            assert(output[i*2]==static_cast<int16_t>(pcm[i]*volume*volume));
        Flush();
    }
    masterVolume=sfxVolume=1;
    Flush();
    assert(LinkVoice_TryPlay(0x6025,&player.actor.projectedPos));
    LinkVoice_EndFrame();assert(LinkVoice_IsPlaying(0x6825));
    assert(LinkVoice_TryPlay(0x6025,&player.actor.projectedPos));
    LinkVoice_EndFrame();assert(LinkVoice_IsPlaying(0x6825));
    LinkVoice_EndFrame();assert(!LinkVoice_IsPlaying(0x6825));Flush();
    play.gameplayFrames++;
    assert(LinkVoice_TryPlay(0x6825,&player.actor.projectedPos));
    auto before=variants[1][5];
    assert(LinkVoice_TryPlay(0x6825,&player.actor.projectedPos));assert(variants[1][5]==before);
    play.gameplayFrames++;
    assert(LinkVoice_TryPlay(0x6825,&player.actor.projectedPos));assert(variants[1][5]==before+1);
    int16_t sample[2]={};LinkVoice_MixInto(sample,1);
    const auto& restarted=banks[1][5][selectedVariant[1][5]];assert(sample[0]==restarted[0]);Flush();
    Flush();play.gameplayFrames++;
    assert(LinkVoice_TryPlay(0x6425,&player.actor.projectedPos));
    LinkVoice_EndFrame();LinkVoice_EndFrame();assert(LinkVoice_IsPlaying(0x6825));Flush();
    editor=1;assert(LinkVoice_TryPlay(0x6025,&player.actor.projectedPos));assert(LinkVoice_IsPlaying(0x6825));
    LinkVoice_Stop(0x6025,nullptr);assert(!LinkVoice_IsPlaying(0x6825));Flush();editor=0;
    for(unsigned action=0;action<5;++action)assert(LinkVoice_TryPlay(0x6820+action,&player.actor.projectedPos));
    assert(!LinkVoice_IsPlaying(0x6824));Flush();
    player.currentMask=PLAYER_MASK_GIANT;ItemVoice_PlayId(&player,NA_SE_VO_LI_AUTO_JUMP);assert(giantCalls==1);
    player.currentMask=PLAYER_MASK_NONE;
    player.currentMask=PLAYER_MASK_SCENTS;ItemVoice_PlayId(&player,NA_SE_VO_LI_DAMAGE_S);CheckQueued(NA_SE_VO_LI_DAMAGE_S+7*0x20,1.0f);
    player.currentMask=PLAYER_MASK_NONE;Flush();
    const u16 formOffsets[]={0,0xC0,0xA0,0x80};
    for(unsigned form=0;form<PLAYER_FORM_HUMAN;++form) {
        player.transformation=form;ageProps.voiceSfxIdOffset=formOffsets[form];
        Player_AnimSfx_PlayVoice(&player,0x6805);CheckQueued(0x6805+formOffsets[form],1.0f);Flush();
    }
    player.transformation=PLAYER_FORM_HUMAN;ageProps.voiceSfxIdOffset=0x20;
    sPlayerAgeProperties[PLAYER_FORM_DEKU].voiceSfxIdOffset=0x80;
    editor=1;customForm=CUSTOM_FORM_GARO;
    Player_AnimSfx_PlayVoice(&player,0x6805);CheckQueued(0x68F0,1.0f);Flush();
    customForm=CUSTOM_FORM_KEATON;
    Player_AnimSfx_PlayVoice(&player,0x6805);CheckQueued(0x6885,1.0f);Flush();
    customForm=CUSTOM_FORM_GERUDO;
    Player_AnimSfx_PlayVoice(&player,0x6805);
    assert(sSfxRequestWriteIndex==0 && !LinkVoice_IsPlaying(0x6825));
    customForm=CUSTOM_FORM_NONE;editor=0;wolfSelected=1;
    Player_AnimSfx_PlayVoice(&player,0x6805);CheckQueued(0x6825,1.0f);Flush();wolfSelected=0;
    AudioSfx_PlaySfx(0x6825,&npc,4,&gSfxDefaultFreqAndVolScale,&gSfxDefaultFreqAndVolScale,&gSfxDefaultReverb);
    assert(sSfxRequestWriteIndex==1 && sSfxRequests[0].pos==&npc && !LinkVoice_IsPlaying(0x6825));Flush();
    editor=1;Player_PlaySfx(&player,NA_SE_PL_WALK_GROUND);CheckQueued(NA_SE_PL_WALK_GROUND,1.0f);Flush();editor=0;
    gSfxBankMuted[6]=1;Player_AnimSfx_PlayVoice(&player,0x6805);
    assert(sSfxRequestWriteIndex==0 && !LinkVoice_IsPlaying(0x6825));gSfxBankMuted[6]=0;
    ItemVoice_Play(nullptr,0x6805,0x6825);
}
