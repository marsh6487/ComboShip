#include "mm/mods/voice_pack/link_voice_mixer.h"
#include <cassert>
#include <thread>
#include <atomic>
int main() {
    LinkVoiceMixer mix;
    const int16_t pcm[]={1000,2000,-3000,32767,-32768};
    int16_t out[16]={};
    assert(mix.Play(pcm,5,1,1,0x6825,true));
    assert(mix.Play(pcm,5,1,1,0x6825,true)); // duplicate refresh cannot overlap itself
    mix.Mix(out,5);
    for(int i=0;i<5;++i)assert(out[i*2]==pcm[i]&&out[i*2+1]==pcm[i]);
    mix.Mix(out,1);assert(!mix.IsPlaying(0x6825));
    assert(mix.Play(pcm,5,1,1,1));mix.Stop(1);assert(!mix.IsPlaying(1));
    int16_t zero[16]={};mix.Mix(zero,8);for(auto x:zero)assert(x==0);
    assert(!mix.Play(nullptr,1,1,1));assert(!mix.Play(pcm,5,1,0));
    for(int i=1;i<=4;++i)assert(mix.Play(pcm,5,1,1,i));
    assert(!mix.Play(pcm,5,1,1,5));mix.Mix(zero,5);assert(zero[0]==4000);
    mix.Stop();mix.Mix(zero,1);
    // Immutable PCM + acquire/release slot publication during real concurrent mixing.
    std::atomic<bool> done{false};
    std::thread consumer([&]{while(!done.load()){int16_t b[32]={};mix.Mix(b,16);}});
    for(int i=0;i<20000;++i){mix.Play(pcm,5,1,1,1);if(i%3==0)mix.Stop(1);}
    done.store(true);consumer.join();
}
