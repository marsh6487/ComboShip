#include "NaviAudio.h"

#include "CompanionStyle.h"
#include "MidnaAudioResources.h"
#include "audio/OptionalSfxArchive.h"
#include "audio/OptionalSfxMixer.h"
#include "audio/TwilightPrincessTargeting.h"

namespace {
enum Clip {
    CameraCue,
    Hello,
    WatchOut,
    Look,
    Hey,
    Listen,
    ClipCount,
};
enum Voice { Camera, Speech, VoiceCount };
ComboOptionalSfx::Mixer<ClipCount, VoiceCount> sMixer;

std::vector<int16_t> ReadNaviCue(const std::shared_ptr<Ship::ArchiveManager>& mmArchives,
                                 const std::shared_ptr<Ship::ArchiveManager>& ootArchives, const char* path) {
    // 2Ship already mounts a supplied OoT extraction below its MM archives for NEI.
    // Combo can also use the separately registered OoT manager (including MQ).
    for (const auto& archives : { mmArchives, ootArchives }) {
        if (archives && archives->HasFile(path)) {
            auto clip = ComboOptionalSfx::ReadClip(archives->LoadFile(path));
            if (!clip.empty()) {
                return clip;
            }
        }
    }
    return {};
}
} // namespace

void MMOptionalSfx_Init() {
    const auto mmArchives = ComboOptionalSfx::OwnArchives("mm");
#ifdef COMBO_BUILD
    const auto ootArchives = ComboOptionalSfx::OwnArchives("oot");
#else
    const std::shared_ptr<Ship::ArchiveManager> ootArchives;
#endif
    sMixer.SetBank({
        ComboOptionalSfx::ReadCameraCue(mmArchives, "audio/samples/ZTarget_META"),
        ReadNaviCue(mmArchives, ootArchives, "audio/samples/Navi - Hello!_META"),
        ReadNaviCue(mmArchives, ootArchives, "audio/samples/Navi - Watch Out!_META"),
        ReadNaviCue(mmArchives, ootArchives, "audio/samples/Navi - Look!_META"),
        ReadNaviCue(mmArchives, ootArchives, "audio/samples/Navi - Hey!_META"),
        ReadNaviCue(mmArchives, ootArchives, "audio/samples/Navi - Listen!_META"),
    });
}

extern "C" void MMTwilightPrincessTargeting_PlayCamera(void) {
    sMixer.Play(CameraCue, Camera);
}

extern "C" bool MMCompanionAudio_TryPlay(MMMidnaAudioEvent event) {
    if (!MMCompanion_UseNavi()) {
        return MMMidnaAudio_TryPlay(event);
    }
    switch (event) {
        case MM_MIDNA_AUDIO_TARGET_NPC:
            return sMixer.Play(Hello, Speech);
        case MM_MIDNA_AUDIO_TARGET_ENEMY:
            return sMixer.Play(WatchOut, Speech);
        case MM_MIDNA_AUDIO_TARGET_OTHER:
            return sMixer.Play(Look, Speech);
        case MM_MIDNA_AUDIO_CALL:
        case MM_MIDNA_AUDIO_HINT:
            return sMixer.Play(Hey, Speech);
        case MM_MIDNA_AUDIO_TALK:
            return sMixer.Play(Listen, Speech);
        default:
            return false;
    }
}

void MMOptionalSfx_Mix(int16_t* output, size_t frames) {
    if (!MMCompanion_UseNavi()) {
        sMixer.Stop(Speech);
    }
    sMixer.Mix(output, frames, MMMidnaAudioResources::Gain());
}

extern "C" void MMCompanionAudio_Reset(void) {
    sMixer.Stop(Speech);
    MMMidnaAudio_Reset();
}

void MMOptionalSfx_Reset() {
    sMixer.Reset();
}
