#include <nlohmann/json.hpp>
#include "global.h"
#include "mods/voice_pack/link_voice_mixer.h"
#include "mods/forms/custom_forms.h"
#include "mods/items/logic/adult_link_render.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <fstream>
#include <iterator>
#include <memory>
#include <random>
#include <vector>
#include <cassert>
#include <cstring>
#include <string>

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
static PlayState play;
static Player player;
static PlayerAgeProperties ageProps;
static const int16_t nativeAdultClip[]={2345,-5432,0};
static int defaultCalls;
extern "C" const int16_t* OotAdultVoice_GetClip(unsigned action,unsigned variant,uint32_t* frames) {
    ++defaultCalls;*frames=3;return nativeAdultClip;
}
static float masterVolume=1, sfxVolume=1;
static int adult, customForm = CUSTOM_FORM_NONE, editor, nativeId, nativeCalls, giantCalls;
int32_t CVarGetInteger(const char* name, int32_t fallback) {
    return !strcmp(name,"gAudioEditor.LinkVoiceFreqMultiplier.Enable") ? editor : fallback;
}
float CVarGetFloat(const char* name, float fallback) {
    if (!strcmp(name,"gSettings.Audio.MasterVolume")) return masterVolume;
    if (!strcmp(name,"gSettings.Audio.SoundEffectsVolume")) return sfxVolume;
    return 1.5f;
}
extern "C" s32 AdultLink_IsActive() { return adult; }
extern "C" u8 WolfLinkForm_IsSelected() { return 0; }
extern "C" s32 CustomForms_ActiveForm() { return customForm; }
#include "runtime.inc"
void Player_PlaySfx(Player* p, u16 id) { nativeId=id; ++nativeCalls; LinkVoice_TryPlay(id,&p->actor.projectedPos); }
void Audio_PlaySfx_GiantsMask(Vec3f* pos, u16 id) { ++giantCalls; }
bool WolfLinkHost_PlayClimbVoice(Player* p,u16 id) { return false; }
#define this nativePlayer
#include "native_voice.inc"
#undef this
#include "item_voice.inc"
static void Flush() { LinkVoice_Stop(0,nullptr); int16_t out[16]={}; LinkVoice_MixInto(out,8); }
int main(int argc,char** argv) {
    assert(argc==2);resourceRoot=argv[1];
    gPlayState=&play;play.actorCtx.actorLists[ACTORCAT_PLAYER].first=&player.actor;
    player.actor.id=ACTOR_PLAYER;player.transformation=PLAYER_FORM_HUMAN;
    ageProps.voiceSfxIdOffset=0x20;player.ageProperties=&ageProps;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first=nullptr;
    assert(!LinkVoice_TryPlay(0x6825,&player.actor.projectedPos));
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first=&player.actor;
    bool absent=resourceRoot=="missing";
    assert(LinkVoice_TryPlay(0x6825,&player.actor.projectedPos)==!absent);
    if(absent) {
        assert(!LinkVoice_TryPlay(0x6825,&player.actor.projectedPos));
        adult=1;assert(LinkVoice_TryPlay(0x6825,&player.actor.projectedPos));assert(defaultCalls==1);
        int16_t output[6]={};LinkVoice_MixInto(output,3);
        for(int i=0;i<3;++i)assert(output[i*2]==nativeAdultClip[i]);
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
            assert(nativeId==0x6820+action);
            assert(LinkVoice_IsPlaying(nativeId));
            const auto& expected=banks[adult][action].front();
            // Cycle selection varies across earlier probes, so compare to one exact donor variant.
            size_t index=selectedVariant[adult][action];
            const auto& selected=banks[adult][action][index];
            const unsigned frames=std::min<size_t>(selected.size(),4096);
            std::vector<int16_t> output(frames*2);
            LinkVoice_MixInto(output.data(),frames);
            for(unsigned i=0;i<frames;++i)assert(output[i*2]==selected[i] && output[i*2+1]==selected[i]);
        }
    }
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
    player.currentMask=PLAYER_MASK_SCENTS;ItemVoice_PlayId(&player,NA_SE_VO_LI_DAMAGE_S);assert(nativeId==NA_SE_VO_LI_DAMAGE_S+7*0x20);
    ItemVoice_Play(nullptr,0x6805,0x6825);
}
