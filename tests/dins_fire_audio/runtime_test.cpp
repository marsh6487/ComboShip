// Production adapters and extracted native buffer routine; device/synth are boundaries.
#include "z64.h"
#include "functions.h"
#include "variables.h"
#ifdef TEST_HOST_MM
#define CW_DINS_HOST_MM
#endif
#include "combo/audio/DinsFireSfx.h"
#include "combo/audio/DinsFireSfxSamples.h"
#include "combo/audio/DinsFireSfxMixer.h"
#include "archive_fixture.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <thread>
#include <vector>
#ifdef TEST_HOST_MM
#include "z64audio.h"
#else
#include "z64audio.h"
#endif

static void Check(bool pass, const char* what) {
    if (!pass) { std::fprintf(stderr,"FAIL Din audio: %s\n",what); std::exit(1); }
}
static float master = 1, sfx = 1;
static thread_local bool audioThread = false;
// The callback contract excludes both allocating and reclaiming owned banks.
void* operator new(size_t size) {
    Check(!audioThread,"audio callback may not allocate");
    if(void* memory=std::malloc(size?size:1))return memory;
    throw std::bad_alloc();
}
void* operator new[](size_t size) {return ::operator new(size);}
void operator delete(void* memory) noexcept {
    Check(!audioThread,"audio callback may not free");std::free(memory);
}
void operator delete[](void* memory) noexcept {::operator delete(memory);}
void operator delete(void* memory,size_t) noexcept {::operator delete(memory);}
void operator delete[](void* memory,size_t) noexcept {::operator delete(memory);}
struct AudioThreadScope {
    AudioThreadScope() {
        audioThread=true;
#ifndef TEST_BASELINE_SILENT_BRIDGE
        DinsAudioFixture::SetAudioThread(true);
#endif
    }
    ~AudioThreadScope() {
#ifndef TEST_BASELINE_SILENT_BRIDGE
        DinsAudioFixture::SetAudioThread(false);
#endif
        audioThread=false;
    }
};
extern "C" {
SaveContext gSaveContext{};
PlayState* gPlayState = nullptr;
int32_t CVarGetInteger(const char* name, int32_t fallback) {
    Check(!audioThread,"audio callback may not read CVars");
    if (!std::strcmp(name,"gSettings.Volume.Master")) return std::lround(master * 100);
    if (!std::strcmp(name,"gSettings.Volume.SFX")) return std::lround(sfx * 100);
    return fallback;
}
float CVarGetFloat(const char* name,float fallback) {
    Check(!audioThread,"audio callback may not read CVars");
    if (!std::strcmp(name,"gSettings.Audio.MasterVolume")) return master;
    if (!std::strcmp(name,"gSettings.Audio.SoundEffectsVolume")) return sfx;
    return fallback;
}
// Baseline-only weak silent bridge makes RED a behavior assertion, not a link error.
#ifdef TEST_BASELINE_SILENT_BRIDGE
__attribute__((weak)) uint32_t DinsFireSfx_Begin(PlayState*,float,float,float) { return 0; }
__attribute__((weak)) uint32_t DinsFireSfx_Attach(PlayState*,const Actor*) { return 0; }
__attribute__((weak)) void DinsFireSfx_Contact(uint32_t) {}
__attribute__((weak)) void DinsFireSfx_Release(uint32_t) {}
__attribute__((weak)) void DinsFireSfx_UpdateActor(uint32_t,float,float,float,int) {}
__attribute__((weak)) void DinsFireSfx_CancelCast(uint32_t) {}
__attribute__((weak)) void DinsFireSfx_Stop(uint32_t) {}
__attribute__((weak)) void DinsFireSfx_UpdateContext(PlayState*) {}
__attribute__((weak)) void DinsFireSfx_MixInto(int16_t*,uint32_t) {}
#endif
#ifdef TEST_HOST_MM
AudioContext gAudioCtx{};
void AudioThread_ProcessCmds(u32) {}
s32 AudioThread_ScheduleProcessCmds() { return 0; }
#else
AudioContext gAudioContext{};
void Audio_ProcessCmds(u32) {}
s32 Audio_ScheduleProcessCmds() { return 0; }
void MmDirectAudio_MixInto(s16*,u32) {}
void PikaSfx_MixInto(s16*,u32) {}
void VoicePack_MixInto(s16*,u32) {}
void GerudoVoice_MixInto(s16*,u32) {}
#endif
void AudioLoad_DecreaseSampleDmaTtls() {}
void AudioLoad_ProcessLoads(s32) {}
void AudioLoad_ProcessScriptLoads() {}
s32 AudioHeap_ResetStep() { return 1; }
s32 osSendMesg(OSMesgQueue*,OSMesg,s32) { return 0; }
s32 osRecvMesg(OSMesgQueue*,OSMesg*,s32) { return -1; }
Acmd* AudioSynth_Update(Acmd* cmd,s32* written,s16* output,s32 frames) {
    *written=0;std::fill_n(output,frames*2,0);return cmd;
}
void Sm64Audio_MixInto(int16_t*,uint32_t) {}
void StasisSfx_MixInto(s16*,u32) {}
void HourglassSfx_MixInto(s16*,u32) {}
void MM_WolfLinkSfx_MixInto(s16*,u32) {}
void OOT_WolfLinkSfx_MixInto(s16*,u32) {}
void LinkVoice_MixInto(s16*,u32) {}
u32 osGetCount() { return 0; }
#include "native_audio_buffer.inc"
}

static void PlayerUpdate(Actor*,PlayState*) {}
static void Healthy(PlayState& play,Player& player) {
    play = {}; player = {}; play.state.running = true;
    player.actor.update = PlayerUpdate;
#ifdef TEST_HOST_MM
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    gSaveContext.save.saveInfo.playerData.health = 48;
#else
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    gSaveContext.health = 48;
#endif
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    play.view.eye = {0,0,0};
#ifdef TEST_HOST_MM
    play.view.at = {0,0,-100};
#else
    play.view.lookAt = {0,0,-100};
#endif
    gPlayState = &play;
    DinsFireSfx_UpdateContext(&play);
}
static std::vector<int16_t> Mix(uint32_t frames=1600) {
    std::vector<int16_t> out(frames*2);
    {AudioThreadScope audio;AudioMgr_CreateNextAudioBuffer(out.data(),frames);}
    return out;
}
static uint64_t Energy(const std::vector<int16_t>& out,int channel=-1) {
    uint64_t value=0;
    for(size_t i=channel < 0 ? 0 : channel;i<out.size();i+=channel < 0 ? 1 : 2) value += std::abs(int(out[i]));
    return value;
}
static constexpr bool kHostMm =
#ifdef TEST_HOST_MM
    true;
#else
    false;
#endif
static void ResetArchives() {
    DinsAudioFixture::Reset();
#ifdef COMBO_BUILD
    // The active game deliberately disagrees with the bridge being exercised.
    DinsAudioFixture::SetActive(!kHostMm);
#else
    DinsAudioFixture::SetActive(kHostMm);
#endif
}
static std::vector<uint8_t> MonoWav(const std::vector<int16_t>& pcm) {
    std::vector<uint8_t> bytes;
    auto text=[&](const char* value){bytes.insert(bytes.end(),value,value+4);};
    auto u16=[&](uint16_t value){bytes.push_back(value&255);bytes.push_back(value>>8);};
    auto u32=[&](uint32_t value){u16(value&65535);u16(value>>16);};
    text("RIFF");u32(36+pcm.size()*2);text("WAVE");text("fmt ");u32(16);
    u16(1);u16(1);u32(32000);u32(64000);u16(2);u16(16);
    text("data");u32(pcm.size()*2);for(int16_t sample:pcm)u16(uint16_t(sample));
    return bytes;
}
static void MountCue(CwDinsAudio::Cue cue,const std::vector<int16_t>& pcm,bool mm=kHostMm) {
    DinsAudioFixture::Mount(mm,CwDinsAudio::kOverridePaths[static_cast<size_t>(cue)],MonoWav(pcm));
}
static void Drain(PlayState& play,uint32_t frames) {
    while(frames) {
        const auto count=std::min(frames,1600u);
        DinsFireSfx_UpdateContext(&play);Mix(count);frames-=count;
    }
}
using StageSamples=std::array<std::vector<int16_t>,CwDinsAudio::kCueCount>;
static StageSamples CaptureStages(PlayState& play) {
    StageSamples result;
    const auto id=DinsFireSfx_Begin(&play,0,0,-100);
    Check(id!=0,"override capture creates valid generation");
    result[0]=Mix(512);Drain(play,32000);
    DinsFireSfx_Contact(id);result[1]=Mix(512);Drain(play,11200);
    DinsFireSfx_Release(id);result[2]=Mix(512);Drain(play,41600);
    result[3]=Mix(512);
    DinsFireSfx_UpdateActor(id,0,0,-100,126);result[4]=Mix(512);
    DinsFireSfx_Stop(id);
    return result;
}
static void MountSilence() {
    for(size_t i=0;i<CwDinsAudio::kCueCount;++i)MountCue(static_cast<CwDinsAudio::Cue>(i),{0});
}
static void TestOverrides(PlayState& play,Player& player) {
    using CwDinsAudio::Cue;
    ResetArchives();Healthy(play,player);
    MountCue(Cue::Charge,std::vector<int16_t>(1024,1111));
    MountCue(Cue::Charge,std::vector<int16_t>(1024,-2222),!kHostMm);
    auto id=DinsFireSfx_Begin(&play,0,0,-100);
    const auto out=Mix(512);
    Check(out[800]==1111&&out[801]==1111,"own-host raw WAV override reaches native stereo output");
    Check(DinsAudioFixture::ReadCount(!kHostMm)==0,"override never reads the other game's active manager");
    DinsFireSfx_Stop(id);

    ResetArchives();Healthy(play,player);
    const auto fallback=CaptureStages(play);
    ResetArchives();
    MountCue(Cue::Charge,std::vector<int16_t>(1024,1111));
    auto truncated=MonoWav({2000});truncated.pop_back();
    DinsAudioFixture::Mount(kHostMm,CwDinsAudio::kOverridePaths[1],truncated);
    const auto partial=CaptureStages(play);
    Check(partial[0][800]==1111&&partial[0][801]==1111,"partial pack overrides its valid charge cue");
    for(size_t i=1;i<partial.size();++i)
        Check(partial[i]==fallback[i],"missing or invalid cue independently preserves exact compiled fallback");

    ResetArchives();
    auto unsupported=MonoWav({3000});unsupported[20]=3;
    for(const auto* path:CwDinsAudio::kOverridePaths)DinsAudioFixture::Mount(kHostMm,path,unsupported);
    Check(CaptureStages(play)==fallback,"unsupported WAVs preserve exact default output at every cue");
    ResetArchives();
    for(size_t i=0;i<CwDinsAudio::kCueCount;++i)MountCue(static_cast<Cue>(i),{12345},!kHostMm);
    Check(CaptureStages(play)==fallback,"foreign-only pack cannot override missing own-host cues");
    Check(DinsAudioFixture::ReadCount(!kHostMm)==0,"missing own-host cues never search the foreign manager");

    ResetArchives();MountSilence();
    const auto silent=CaptureStages(play);
    for(const auto& samples:silent)Check(Energy(samples)==0,"valid silent WAV remains an intentional silent override");

    ResetArchives();
    MountCue(Cue::Charge,std::vector<int16_t>(1024,1111));
    MountCue(Cue::Contact,std::vector<int16_t>(1024,2222));
    MountCue(Cue::Release,std::vector<int16_t>(1024,-3333));
    const std::array<int16_t,3> loop{800,-400,100};
    MountCue(Cue::Loop,std::vector<int16_t>(loop.begin(),loop.end()));
    MountCue(Cue::Burnout,std::vector<int16_t>(1024,4444));
    id=DinsFireSfx_Begin(&play,0,0,-100);
    const auto reads=DinsAudioFixture::ReadCount(kHostMm);
    Check(reads>0,"valid cast loads raw override files on the game thread");
    const auto charge=Mix(1024);
    for(size_t i=400;i<1024;++i)Check(charge[2*i]==1111&&charge[2*i+1]==1111,"charge keeps exact replacement amplitude");
    DinsFireSfx_Contact(id);const auto contact=Mix(1025);
    for(size_t i=0;i<1025;++i) {
        const int16_t expected=i<1024?2222:0;
        Check(contact[2*i]==expected&&contact[2*i+1]==expected,"contact plays its complete replacement once");
    }
    DinsFireSfx_Release(id);const auto release=Mix(1025);
    for(size_t i=400;i<1025;++i) {
        const int16_t expected=(i<1024?-3333:0)+loop[i%loop.size()];
        Check(release[2*i]==expected&&release[2*i+1]==expected,"release and full replacement loop mix independently");
    }
    DinsFireSfx_UpdateActor(id,0,0,-100,126);const auto burnout=Mix(1025);
    for(size_t i=0;i<1025;++i) {
        const int16_t expected=(i<1024?4444:0)+loop[(1025+i)%loop.size()];
        Check(burnout[2*i]==expected&&burnout[2*i+1]==expected,"burnout uses its replacement and leaves the loop cursor intact");
    }
    const auto bed=Mix(50);
    for(size_t i=0;i<50;++i)
        Check(bed[2*i]==loop[(2050+i)%loop.size()]&&bed[2*i+1]==bed[2*i],"short loop wraps at its own data length");
    Check(DinsAudioFixture::ReadCount(kHostMm)==reads,"cue dispatch and audio callbacks perform no archive reads");
    DinsFireSfx_Stop(id);

    ResetArchives();MountSilence();MountCue(Cue::Charge,{32000});
    id=DinsFireSfx_Begin(&play,0,0,-100);const auto shortCharge=Mix(4);
    Check(shortCharge==std::vector<int16_t>({100,100,0,0,0,0,0,0}),"one-sample cue keeps its amplitude ramp and finishes without out-of-bounds reads");
    DinsFireSfx_Stop(id);
    ResetArchives();MountSilence();
    std::vector<int16_t> longLoop(110003);
    for(size_t i=0;i<longLoop.size();++i)longLoop[i]=(int(i%17)-8)*200;
    MountCue(Cue::Loop,longLoop);
    id=DinsFireSfx_Begin(&play,0,0,-100);DinsFireSfx_Release(id);Drain(play,110001);
    const auto longWrap=Mix(9);
    for(size_t i=0;i<9;++i) {
        const auto expected=longLoop[(110001+i)%longLoop.size()];
        Check(longWrap[2*i]==expected&&longWrap[2*i+1]==expected,"loop longer than the compiled loop reaches its own end before wrapping");
    }
    DinsFireSfx_Stop(id);

    ResetArchives();MountCue(Cue::Charge,std::vector<int16_t>(8192,1700));
    id=DinsFireSfx_Begin(&play,0,0,-100);Mix(512);
    DinsAudioFixture::Remove(kHostMm,CwDinsAudio::kOverridePaths[0]);
    MountCue(Cue::Charge,std::vector<int16_t>(8192,-1900));
    DinsAudioFixture::ReplaceOwner(kHostMm);
    const auto pinned=Mix(512);
    Check(std::all_of(pinned.begin(),pinned.end(),[](int16_t sample){return sample==1700;}),
          "active cast pins decoded samples across archive removal and owner replacement");
    Check(DinsAudioFixture::ReadCount(kHostMm)==0,"owner replacement causes no callback reload");
    id=DinsFireSfx_Begin(&play,0,0,-100);
    const auto replaced=Mix(512);
    Check(replaced[800]==-1900&&replaced[801]==-1900,"next cast loads replacement manager's samples");
    const auto cachedReads=DinsAudioFixture::ReadCount(kHostMm);
    MountCue(Cue::Charge,std::vector<int16_t>(8192,2400));
    id=DinsFireSfx_Begin(&play,0,0,-100);const auto cached=Mix(512);
    Check(cached[800]==-1900&&cached[801]==-1900&&DinsAudioFixture::ReadCount(kHostMm)==cachedReads,
          "same-manager edits remain cached until restart or manager replacement");
    DinsFireSfx_Stop(id);

    ResetArchives();MountCue(Cue::Charge,std::vector<int16_t>(1024,2555));
    DinsAudioFixture::SetAvailable(kHostMm,false);
    id=DinsFireSfx_Begin(&play,0,0,-100);Check(Mix(512)==fallback[0],"temporarily missing manager uses compiled samples");
    DinsAudioFixture::SetAvailable(kHostMm,true);
    id=DinsFireSfx_Begin(&play,0,0,-100);const auto available=Mix(512);
    Check(available[800]==2555&&available[801]==2555,"manager availability is retried on the next valid cast");
    DinsFireSfx_Stop(id);

    ResetArchives();Healthy(play,player);
    Check(DinsFireSfx_Begin(nullptr,0,0,-100)==0,"null override request rejected");
#ifdef TEST_HOST_MM
    play.pauseCtx.state=PAUSE_STATE_MAIN;
#else
    play.pauseCtx.state=6;
#endif
    Check(DinsFireSfx_Begin(&play,0,0,-100)==0,"paused override request rejected");
    play.pauseCtx.state=0;
    Check(DinsFireSfx_Begin(&play,std::numeric_limits<float>::quiet_NaN(),0,-100)==0,"invalid-position override request rejected");
    play.transitionTrigger=1;
    Check(DinsFireSfx_Begin(&play,0,0,-100)==0,"transition override request rejected");
    Check(DinsAudioFixture::ReadCount(kHostMm)==0&&DinsAudioFixture::ReadCount(!kHostMm)==0,
          "invalid or paused casts perform no resource reads");
    ResetArchives();Healthy(play,player);
    std::puts("PASS native WAV overrides: all cues, partial/malformed/silent packs, own host, short/long loops, cache, ownership and invalid casts");
}
#ifndef TEST_BASELINE_SILENT_BRIDGE
static void TestOwnedBanks() {
    CwDinsAudio::Mixer mixer;
    CwDinsAudio::Context context;
    context.play=&mixer;context.live=true;context.gain=1;context.at={0,0,-100};
    const CwDinsAudio::Point position{0,0,-100};
    std::array<int16_t,32002> output{};
    // Stop, cancel, expiry, context loss and both watchdog branches may clear
    // callback state, but only a subsequent valid game-thread Begin may reclaim.
    for(int reason=0;reason<6;++reason) {
        mixer.Publish(context);
        int destroyed=0;
        auto* data=new CwDinsAudio::SampleBank;
        data->clips[0].assign(4096,1200);data->clips[3]={100,200,300};
        auto bank=std::shared_ptr<const CwDinsAudio::SampleBank>(data,[&](const auto* retired){
            Check(!audioThread,"sample bank must be reclaimed on its game-thread caller");
            ++destroyed;
            // Re-entry proves reclamation occurs after the mixer lock is released.
            mixer.Publish(context);
            delete retired;
        });
        const std::weak_ptr<const CwDinsAudio::SampleBank> weak=bank;
        const auto id=mixer.Begin(context.play,position,std::move(bank));
        Check(id!=0&&!weak.expired(),"cast takes ownership of immutable samples");
        {
            AudioThreadScope audio;
            switch(reason) {
                case 0:mixer.Stop(id);break;
                case 1:mixer.Cancel(id);break;
                case 2:mixer.Update(id,position,138);break;
                case 3:mixer.Publish({});break;
                case 4:mixer.Mix(output.data(),16001);break;
                case 5:{auto paused=context;paused.paused=true;mixer.Publish(paused);mixer.Mix(output.data(),16001);break;}
            }
        }
        Check(destroyed==0&&!weak.expired(),"every Clear path retains its owned bank without callback frees");
        mixer.Publish(context);
        Check(mixer.Begin(nullptr,position)==0&&destroyed==0,"rejected Begin leaves the retained bank intact");
        const auto next=mixer.Begin(context.play,position);
        Check(next!=0&&destroyed==1&&weak.expired(),"next valid Begin retires prior bank after unlocking");
        mixer.Stop(next);
    }

    auto first=std::make_shared<CwDinsAudio::SampleBank>();
    first->clips[0].assign(8192,1000);
    mixer.Begin(context.play,position,std::move(first));
    std::atomic<bool> started{false},done{false};
    bool heard=false;
    std::thread consumer([&] {
        std::array<int16_t,256> buffer{};
        AudioThreadScope audio;
        mixer.Mix(buffer.data(),128);
        heard=std::any_of(buffer.begin(),buffer.end(),[](int16_t sample){return sample!=0;});
        started.store(true);
        while(!done.load()) {
            std::fill(buffer.begin(),buffer.end(),0);
            mixer.Mix(buffer.data(),128);
        }
    });
    while(!started.load())std::this_thread::yield();
    for(int i=0;i<128;++i) {
        auto replacement=std::make_shared<CwDinsAudio::SampleBank>();
        replacement->clips[0].assign(1024,int16_t(1000+i));
        replacement->clips[3]={100,200,300};
        mixer.Publish(context);
        const auto id=mixer.Begin(context.play,position,std::move(replacement));
        mixer.Release(id);mixer.Update(id,position,i%126);
        if(i%3==0)mixer.Stop(id);
    }
    done.store(true);consumer.join();
    Check(heard,"concurrent consumer hears owned PCM while game thread replaces banks");
    std::puts("PASS owned bank lifetime: clear/cancel/expiry/watchdog retain, game-thread reclamation outside lock, concurrent replacement");
}
#endif
static void WriteWav(const char* path,const std::vector<int16_t>& pcm) {
    std::ofstream stream(path,std::ios::binary);
    auto u16=[&](uint16_t x){stream.put(x&255);stream.put(x>>8);};
    auto u32=[&](uint32_t x){u16(x&65535);u16(x>>16);};
    stream.write("RIFF",4);u32(36+pcm.size()*2);stream.write("WAVEfmt ",8);u32(16);
    u16(1);u16(2);u32(32000);u32(128000);u16(4);u16(16);
    stream.write("data",4);u32(pcm.size()*2);for(int16_t value:pcm)u16(uint16_t(value));
    Check(bool(stream),"audition WAV saved");
}
int main(int argc,char** argv) {
    PlayState play{};Player player{};Actor fire{},other{};
#ifndef TEST_BASELINE_SILENT_BRIDGE
    TestOverrides(play,player);
    TestOwnedBanks();
#endif
    Healthy(play,player);
    Check(DinsFireSfx_Begin(nullptr,0,0,-100)==0,"null context rejected");
    auto begin=[&](float x=0,float z=-100){DinsFireSfx_UpdateContext(&play);return DinsFireSfx_Begin(&play,x,0,z);};
    auto h=begin();Check(h!=0,"live Begin returns generation");
    Check(DinsFireSfx_Attach(&play,&fire)==h,"charge binds its actor");
    Check(DinsFireSfx_Attach(&play,&other)==0,"charge cannot bind second actor");
    Check(Energy(Mix())>0,"native post-synth seam outputs actual charge PCM");
    DinsFireSfx_CancelCast(h);Check(Energy(Mix())==0,"pre-release cancel silences charge");
    // Duplicate contact/release must not restart cursors or add a second voice.
    auto capture=[&](bool repeat){
        auto id=begin();DinsFireSfx_Attach(&play,&fire);DinsFireSfx_Contact(id);DinsFireSfx_Release(id);
        auto first=Mix();if(repeat){DinsFireSfx_Contact(id);DinsFireSfx_Release(id);}auto second=Mix();
        DinsFireSfx_Stop(id);return std::pair(first,second);
    };
    const auto once=capture(false),twice=capture(true);
    Check(once==twice,"contact/release are idempotent");
    h=begin();DinsFireSfx_Attach(&play,&fire);DinsFireSfx_Release(h);DinsFireSfx_CancelCast(h);
    Check(Energy(Mix())>0,"recovery cancellation preserves released actor");
    auto newHandle=begin();Check(newHandle!=h,"generation changes");
    DinsFireSfx_Stop(h);DinsFireSfx_UpdateActor(h,1000,0,0,138);DinsFireSfx_CancelCast(h);
    Check(Energy(Mix())>0,"stale generation cannot affect new charge");
    DinsFireSfx_Stop(newHandle);Check(Energy(Mix())==0,"explicit stop is immediate");
    auto pausedCapture=[&](bool pause){
        auto id=begin();DinsFireSfx_Release(id);Mix(257);
        if(pause){
#ifdef TEST_HOST_MM
            play.pauseCtx.state = PAUSE_STATE_MAIN;
#else
            play.pauseCtx.state = 6;
#endif
            DinsFireSfx_UpdateContext(&play);
            for(int i=0;i<30;++i){DinsFireSfx_UpdateContext(&play);Check(Energy(Mix())==0,"paused callback silent");}
            play.pauseCtx.state=0;DinsFireSfx_UpdateContext(&play);
        }
        auto result=Mix(977);DinsFireSfx_Stop(id);return result;
    };
    Check(pausedCapture(false)==pausedCapture(true),"pause freezes all sample cursors");
    h=begin();DinsFireSfx_Release(h);
    for(int frame=0;frame<125;++frame){DinsFireSfx_UpdateContext(&play);DinsFireSfx_UpdateActor(h,0,0,-100,frame);Check(Energy(Mix())>0,"fire bed persists after attack");}
    DinsFireSfx_UpdateActor(h,0,0,-100,126);Check(Energy(Mix())>0,"natural burnout emits PCM");
    DinsFireSfx_UpdateActor(h,0,0,-100,138);Check(Energy(Mix())==0,"natural expiration stops all voices");
    auto spatialEnergy=[&](float x,float z){auto id=begin(x,z);DinsFireSfx_Release(id);Mix();auto out=Mix();DinsFireSfx_Stop(id);return out;};
    auto left=spatialEnergy(-200,-200),right=spatialEnergy(200,-200),far=spatialEnergy(0,-20000),near=spatialEnergy(0,-200);
    Check(Energy(left,0)>Energy(left,1)&&Energy(right,1)>Energy(right,0),"anchored source pans both directions");
    Check(Energy(far)<Energy(near)/4,"distant source attenuates");
    const auto full=spatialEnergy(0,-100);master=.5f;sfx=.5f;
    const auto quarter=spatialEnergy(0,-100);master=sfx=1;
    for(size_t i=0;i<full.size();++i)Check(std::abs(int(full[i])-4*int(quarter[i]))<=3,"Master times SFX applied exactly once");
    // Feed the same source samples over near-full-scale existing native output.
    h=begin();DinsFireSfx_Release(h);Mix();const auto clean=Mix();DinsFireSfx_Stop(h);
    h=begin();DinsFireSfx_Release(h);Mix();std::vector<int16_t> saturated(clean.size());
    for(size_t i=0;i<clean.size();++i)saturated[i]=clean[i]>=0?32760:-32760;
    {AudioThreadScope audio;DinsFireSfx_MixInto(saturated.data(),saturated.size()/2);}
    bool clipped=false;
    for(size_t i=0;i<clean.size();++i){
        const auto expected=std::clamp((clean[i]>=0?32760:-32760)+int(clean[i]),-32768,32767);
        Check(saturated[i]==expected,"saturating addition preserves existing native output without wrap");
        clipped|=expected==32767||expected==-32768;
    }
    Check(clipped,"saturation fixture reaches clipping");DinsFireSfx_Stop(h);
    master=0;Check(Energy(spatialEnergy(0,-100))==0,"master mute");master=1;
    sfx=0;Check(Energy(spatialEnergy(0,-100))==0,"SFX mute");sfx=1;
#ifdef TEST_HOST_MM
    master=std::numeric_limits<float>::quiet_NaN();Check(Energy(spatialEnergy(0,-100))==0,"nonfinite master silent");master=1;
#endif
    h=begin();DinsFireSfx_Release(h);play.transitionTrigger=1;DinsFireSfx_UpdateContext(&play);
    Check(Energy(Mix())==0&&begin()==0,"transition stops and rejects Begin");play.transitionTrigger=0;
    h=begin();DinsFireSfx_Release(h);
#ifdef TEST_HOST_MM
    ++play.sceneId;
#else
    ++play.sceneNum;
#endif
    DinsFireSfx_UpdateContext(&play);Check(Energy(Mix())==0,"same PlayState scene replacement stops generation");
    h=begin();DinsFireSfx_Release(h);
#ifdef TEST_HOST_MM
    gSaveContext.save.saveInfo.playerData.health=0;
#else
    gSaveContext.health=0;
#endif
    DinsFireSfx_UpdateContext(&play);Check(Energy(Mix())==0&&begin()==0,"death stops and rejects Begin");
    Healthy(play,player);h=begin();DinsFireSfx_Release(h);DinsFireSfx_UpdateContext(nullptr);
    Check(Energy(Mix())==0,"context loss stops generation");
    Healthy(play,player);h=begin();DinsFireSfx_Release(h);
    for(int i=0;i<12;++i)Mix(1600);Check(Energy(Mix())==0,"watchdog bounds orphan audio without actor/context updates");
    // Both publication threads execute production code; no world reads in Mix.
    Healthy(play,player);h=begin();DinsFireSfx_Release(h);
    std::thread worker([]{std::array<int16_t,256> out{};AudioThreadScope audio;for(int i=0;i<2000;++i)DinsFireSfx_MixInto(out.data(),128);});
    for(int i=0;i<2000;++i){DinsFireSfx_UpdateContext(&play);DinsFireSfx_UpdateActor(h,float(i%100),0,-100,i%126);}
    worker.join();DinsFireSfx_Stop(h);
    if(argc==3&&std::strcmp(argv[1],"--export")==0){
        Healthy(play,player);std::vector<int16_t> audition;h=begin();DinsFireSfx_Attach(&play,&fire);
        for(int tick=0;tick<170;++tick){
            if(tick==20)DinsFireSfx_Contact(h);if(tick==21)DinsFireSfx_Release(h);
            DinsFireSfx_UpdateContext(&play);DinsFireSfx_UpdateActor(h,0,0,-100,tick-21);
            auto out=Mix();audition.insert(audition.end(),out.begin(),out.end());
        }
        WriteWav(argv[2],audition);DinsFireSfx_Stop(h);
    }
    std::puts("PASS Din audio actual host adapter/native buffer: generations, cues, loop, pause, gains, spatial, lifecycle, watchdog, concurrency");
}
