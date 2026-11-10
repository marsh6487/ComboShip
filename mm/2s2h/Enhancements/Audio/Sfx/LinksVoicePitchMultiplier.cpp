#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include <spdlog/spdlog.h>

extern "C" {
#include "variables.h"
#include "mods/forms/custom_forms.h"
}

// For Link's voice pitch SFX modifier
static f32 freqMultiplier = 1;

#define CVAR_NAME "gAudioEditor.LinkVoiceFreqMultiplier.Enable"
#define CVAR CVarGetInteger(CVAR_NAME, 0)

void RegisterLinksVoicePitchMultiplier() {
    // Custom forms still need this hook when the editor is disabled. Human age
    // selection belongs to LinkVoice at the SFX boundary, not a pitch approximation.
    SPDLOG_INFO("[LinkVoice] player voice hook registered; automatic adult pitch reduction disabled");
    COND_VB_SHOULD(VB_LINK_VOICE_PITCH_MULTIPLIER, true, {
        Player* player = GET_PLAYER(gPlayState);
        u16 sfxId = *va_arg(args, u16*);

        u8 isVoice = (sfxId >= NA_SE_VO_LI_SWORD_N && sfxId <= NA_SE_VO_DEMO_394) || sfxId == NA_SE_PL_TRANSFORM_VOICE;
        u8 editor = CVAR != 0;
        u16 formVoice = 0;

        u16 baseVoice = sfxId;
        if (player->transformation == PLAYER_FORM_HUMAN && sfxId >= 0x6820 && sfxId < 0x6840)
            baseVoice -= 0x20;
        if (isVoice && CustomForms_VoiceOverride(baseVoice, &formVoice)) {
            // A custom form speaks with its own bank (Keaton = Deku, Garo = Igos) or stays silent.
            *should = false;
            if (formVoice != 0) {
                AudioSfx_PlaySfx(formVoice, &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
            }
        } else if (isVoice && editor) {
            // Apply only the user's explicit pitch setting. If both adult PCM
            // providers are unavailable, do not turn the native child bank into
            // a misleading "adult" voice by lowering its pitch.
            freqMultiplier = CVarGetFloat("gAudioEditor.LinkVoiceFreqMultiplier.Scale", 1.0f);
            if (freqMultiplier <= 0) {
                freqMultiplier = 1;
            }

            *should = false;
            AudioSfx_PlaySfx(sfxId, &player->actor.projectedPos, 4, &freqMultiplier, &gSfxDefaultFreqAndVolScale,
                             &gSfxDefaultReverb);
        }
    });
}

static RegisterShipInitFunc initFunc(RegisterLinksVoicePitchMultiplier, { CVAR_NAME });
