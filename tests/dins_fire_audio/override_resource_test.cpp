#include "combo/audio/DinsFireSfxSamples.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>
#ifdef TEST_BASELINE_WAV
namespace CwDinsAudio { static std::vector<int16_t> DecodeOverrideWav(const std::vector<uint8_t>&) { return {}; } }
#else
#include "combo/audio/DinsFireSfxWav.h"
#endif
#ifndef TEST_PARSER_ONLY
#include "archive_fixture.h"
#ifdef TEST_BASELINE_PROVIDER
static std::shared_ptr<const CwDinsAudio::SampleBank> DinsLoadOverrideBank() { return {}; }
#else
#include "combo/audio/DinsFireSfxResources.inc.cpp"
#endif
#endif

static void Check(bool value,const char* message) {
    if(!value){std::fprintf(stderr,"FAIL Din override: %s\n",message);std::exit(1);}
}
static void U16(std::vector<uint8_t>& out,uint16_t value) {out.push_back(value&255);out.push_back(value>>8);}
static void U32(std::vector<uint8_t>& out,uint32_t value) {U16(out,value&65535);U16(out,value>>16);}
static void Set32(std::vector<uint8_t>& out,size_t offset,uint32_t value) {for(int i=0;i<4;++i)out[offset+i]=(value>>(i*8))&255;}
static void Chunk(std::vector<uint8_t>& out,const char* tag,const std::vector<uint8_t>& bytes) {
    out.insert(out.end(),tag,tag+4);U32(out,bytes.size());out.insert(out.end(),bytes.begin(),bytes.end());if(bytes.size()%2)out.push_back(0xA5);
}
static std::vector<uint8_t> Wav(const std::vector<int16_t>& samples,bool unknown=false,bool dataFirst=false,unsigned fmtExtension=0) {
    std::vector<uint8_t> out={'R','I','F','F',0,0,0,0,'W','A','V','E'},fmt,pcm;
    U16(fmt,1);U16(fmt,1);U32(fmt,32000);U32(fmt,64000);U16(fmt,2);U16(fmt,16);
    if(fmtExtension){U16(fmt,0);fmt.resize(16+fmtExtension);}
    for(int16_t value:samples)U16(pcm,uint16_t(value));
    if(unknown)Chunk(out,"JUNK",{1,2,3});
    if(dataFirst){Chunk(out,"data",pcm);Chunk(out,"fmt ",fmt);}else{Chunk(out,"fmt ",fmt);Chunk(out,"data",pcm);}
    Set32(out,4,out.size()-8);return out;
}
static void Reject(const std::vector<uint8_t>& data,const char* what) {Check(CwDinsAudio::DecodeOverrideWav(data).empty(),what);}
static void ParserChecks() {
    const std::vector<int16_t> samples={-32768,-123,0,123,32767};
    auto good=Wav(samples);
    Check(CwDinsAudio::DecodeOverrideWav(good)==samples,"valid PCM preserves exact signed samples/amplitudes");
    Check(CwDinsAudio::DecodeOverrideWav(Wav({0,0,0}))==std::vector<int16_t>({0,0,0}),"valid silence is an override");
    Check(CwDinsAudio::DecodeOverrideWav(Wav({7}))==std::vector<int16_t>({7}),"single-sample clips are valid");
    Check(CwDinsAudio::DecodeOverrideWav(Wav(samples,true,true))==samples,"bounded odd unknown chunks and data-before-fmt accepted");
    Check(CwDinsAudio::DecodeOverrideWav(Wav(samples,false,false,2))==samples,"PCM fmt18 with zero extension supported");
    Check(CwDinsAudio::DecodeOverrideWav(Wav(samples,false,false,24))==samples,"bounded extended PCM fmt with zero cbSize supported");
    Check(CwDinsAudio::DecodeOverrideWav(Wav(std::vector<int16_t>(CwDinsAudio::kMaxOverrideSamples,12))).size()==CwDinsAudio::kMaxOverrideSamples,"exact ten-second limit accepted");
    Reject(Wav(std::vector<int16_t>(CwDinsAudio::kMaxOverrideSamples+1,12)),"over ten seconds rejected");
    Reject(Wav({}),"empty data rejected");
    for(size_t size=0;size<good.size();++size)Reject(std::vector<uint8_t>(good.begin(),good.begin()+size),"all truncated extents rejected");
    for(auto field:std::vector<std::pair<size_t,uint32_t>>{{4,0},{20,3},{22,2},{24,44100},{28,128000},{32,4},{34,8}}){
        auto bad=good;if(field.first==4||field.first==24||field.first==28)Set32(bad,field.first,field.second);else{bad[field.first]=field.second;bad[field.first+1]=0;}
        Reject(bad,"unsupported RIFF/fmt declaration rejected");
    }
    auto bad=good;bad[0]='R';bad[1]='F';Reject(bad,"RF64 rejected");
    bad=good;bad[8]='X';Reject(bad,"non-WAVE rejected");
    bad=good;bad.push_back(0);Reject(bad,"trailing bytes outside RIFF rejected");
    bad=good;Set32(bad,40,0xFFFFFFFF);Reject(bad,"oversized chunk declaration rejected without overflow");
    bad=Wav(samples,true);bad.erase(bad.begin()+23);Set32(bad,4,bad.size()-8);Reject(bad,"missing odd chunk padding rejected");
    bad=good;Chunk(bad,"fmt ",std::vector<uint8_t>(good.begin()+20,good.begin()+36));Set32(bad,4,bad.size()-8);Reject(bad,"duplicate fmt rejected");
    bad=good;Chunk(bad,"data",{0,0});Set32(bad,4,bad.size()-8);Reject(bad,"duplicate data rejected");
    bad=Wav(samples,false,false,2);bad[36]=1;Reject(bad,"nonzero PCM extension rejected");
    bad=good;Set32(bad,16,15);Reject(bad,"short fmt rejected");
    bad=good;Set32(bad,40,9);Reject(bad,"odd data byte count rejected");
    bad=good;bad.resize(CwDinsAudio::kMaxOverrideFileBytes+1);Set32(bad,4,bad.size()-8);Reject(bad,"file byte bound enforced");
    // Structured mutations and arbitrary short streams exercise bounded reads under sanitizers.
    for(size_t byte=0;byte<good.size();++byte)for(uint8_t value:{uint8_t(0),uint8_t(0xFF)}){
        bad=good;bad[byte]=value;(void)CwDinsAudio::DecodeOverrideWav(bad);
    }
    for(size_t size=0;size<256;++size){std::vector<uint8_t> arbitrary(size,0xFF);(void)CwDinsAudio::DecodeOverrideWav(arbitrary);}
    std::puts("PASS Din strict WAV parser: exact PCM, silence, chunk bounds/padding, format, duration and malformed streams");
}
#ifndef TEST_PARSER_ONLY
static void ResourcesChecks() {
    using namespace DinsAudioFixture;
#ifdef CW_DINS_AUDIO_MM
    constexpr bool own=true;
#else
    constexpr bool own=false;
#endif
    auto select=[](bool own){
#ifdef COMBO_BUILD
        SetActive(!own);
#else
        SetActive(own);
#endif
    };
    Reset();select(own);
    SetAvailable(own,false);
    auto missing=DinsLoadOverrideBank();
    Check(!missing,"missing manager returns no bank and may retry");
    SetAvailable(own,true);
    const auto& paths=CwDinsAudio::kOverridePaths;
    Mount(!own,paths[0],Wav({999}));
    Mount(own,paths[0],Wav({-1234,0,4321}));
    Mount(own,paths[1],{1,2,3}); // malformed per-cue fallback
    Mount(own,paths[3],Wav({0,0,0})); // valid silent loop
    auto first=DinsLoadOverrideBank();
    Check(first&&first->clips[0]==std::vector<int16_t>({-1234,0,4321}),"own raw archive WAV overrides exact cue");
    Check(first->clips[1].empty()&&first->clips[2].empty()&&first->clips[4].empty(),"invalid and missing cues fall back independently");
    Check(first->clips[3]==std::vector<int16_t>({0,0,0}),"provider preserves valid silence");
    Check(ReadCount(own)==3&&ReadCount(!own)==0,"provider reads only own existing raw .wav entries");
    const auto before=ReadCount(own);Mount(own,paths[0],Wav({77}));
    Check(DinsLoadOverrideBank()==first&&ReadCount(own)==before,"same-manager cache does no repeat I/O; edits require restart");
    ReplaceOwner(own);select(own);
    auto next=DinsLoadOverrideBank();
    Check(next&&next!=first&&next->clips[0]==std::vector<int16_t>({77}),"fresh manager identity reloads overrides");
    Check(first->clips[0]==std::vector<int16_t>({-1234,0,4321}),"previous immutable bank survives owner replacement");
    Remove(own,paths[0]);ReplaceOwner(own);select(own);
    auto removed=DinsLoadOverrideBank();Check(removed&&removed->clips[0].empty(),"removed cue falls back after restart");
    for(size_t i=0;i<paths.size();++i)Mount(own,paths[i],Wav({int16_t(10+i)}));
    ReplaceOwner(own);select(own);auto all=DinsLoadOverrideBank();
    for(size_t i=0;i<paths.size();++i)Check(all->clips[i]==std::vector<int16_t>({int16_t(10+i)}),"all five cue paths override independently");
    Check(ReadCount(own)==5,"all-five bank read once each");
    // Unregistration may precede DLL unload; provider must discard weak foreign control blocks.
    SetAvailable(own,false);Check(!DinsLoadOverrideBank(),"own manager teardown cannot use active-other fallback");
    SetAvailable(own,true);select(own);Check(DinsLoadOverrideBank()!=all,"teardown drops cache before a same-pointer registration");
    std::puts("PASS Din native resource provider: own manager, partial/all overrides, invalid fallback, silence, cache/retry/restart/lifetime");
}
#endif
int main(){ParserChecks();
#ifndef TEST_PARSER_ONLY
    ResourcesChecks();
#endif
}
