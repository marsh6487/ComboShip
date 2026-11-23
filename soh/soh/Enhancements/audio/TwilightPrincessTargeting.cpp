#include "audio/TwilightPrincessTargeting.h"

#include "audio/OptionalSfxArchive.h"
#include "audio/OptionalSfxMixer.h"
#include <libultraship/bridge/consolevariablebridge.h>

namespace {
ComboOptionalSfx::Mixer<1, 1> sCameraCue;
}

void SOHTwilightPrincessTargeting_Init() {
    sCameraCue.SetBank({
        ComboOptionalSfx::ReadCameraCue(ComboOptionalSfx::OwnArchives("oot"), "audio/samples/Z Target_META"),
    });
}

extern "C" void SOHTwilightPrincessTargeting_PlayCamera(void) {
    sCameraCue.Play(0, 0);
}

void SOHTwilightPrincessTargeting_Mix(int16_t* output, size_t frames) {
    const float master = std::clamp(CVarGetInteger("gSettings.Volume.Master", 40), 0, 100) / 100.0f;
    const float sfx = std::clamp(CVarGetInteger("gSettings.Volume.SFX", 100), 0, 100) / 100.0f;
    sCameraCue.Mix(output, frames, master * sfx);
}

void SOHTwilightPrincessTargeting_Reset() {
    sCameraCue.Reset();
}
