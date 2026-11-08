#include "din_voice.h"
#include "oot_adult_voice.h"
#include "din_voice_mixer.h"
#include <nlohmann/json.hpp>
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/archive/ArchiveManager.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include <vector>
#include <random>
#include "z64.h"
#include "variables.h"
#include "mods/forms/custom_forms.h"
#include "mods/items/logic/adult_link_render.h"
#include "mods/transformation_masks/wolf_link_form.h"

namespace {
using Clips = std::vector<std::vector<int16_t>>;
std::array<std::array<Clips, 32>, 2> banks;
std::array<std::array<size_t, 32>, 2> variants{}, selectedVariant{};
// Same private PRNG/variant-selection approach as OoT's VoicePack_PlayIfMatch.
std::mt19937 voiceRng{ 0x53767069 };
DinVoiceMixer mixer;
bool attempted = false, available = false, used = false;
const void* playerPosition = nullptr;
std::array<std::array<uint32_t, 32>, 2> lastFrame = [] {
    std::array<std::array<uint32_t, 32>, 2> result;
    for (auto& age : result)
        age.fill(UINT32_MAX);
    return result;
}();

float ReadVolume() {
#ifdef COMBO_BUILD
    const float defaultMaster = std::clamp(CVarGetInteger("gSettings.Volume.Master", 40), 0, 100) / 100.0f;
    const float defaultSfx = std::clamp(CVarGetInteger("gSettings.Volume.SFX", 100), 0, 100) / 100.0f;
#else
    const float defaultMaster = 1.0f, defaultSfx = 1.0f;
#endif
    float master = CVarGetFloat("gSettings.Audio.MasterVolume", defaultMaster);
    float sfx = CVarGetFloat("gSettings.Audio.SoundEffectsVolume", defaultSfx);
    if (!std::isfinite(master))
        master = defaultMaster;
    if (!std::isfinite(sfx))
        sfx = defaultSfx;
    return std::clamp(master, 0.0f, 1.0f) * std::clamp(sfx, 0.0f, 1.0f);
}

void Load() {
    if (attempted)
        return;
    auto* context = Ship::Context::GetRawInstance();
    if (!context)
        return;
#ifdef COMBO_BUILD
    auto manager = Ship::CrossRMRegistry::GetOrActive("mm");
#else
    auto manager = context->GetResourceManager();
#endif
    if (!manager)
        return;
    attempted = true;
    auto file = manager->GetArchiveManager()->LoadFile("custom/din_voice/voices.json");
    if (!file || !file->Buffer)
        return;
    try {
        auto manifest = nlohmann::json::parse(file->Buffer->begin(), file->Buffer->end());
        if (manifest.at("version") != 1 || manifest.at("rate") != 32000)
            return;
        // Publish only a complete validated cache; no partial age bank can become active.
        decltype(banks) loaded;
        for (unsigned age = 0; age < 2; ++age) {
            const auto& bank = manifest.at(age ? "adult" : "child");
            for (unsigned action = 0; action < 32; ++action) {
                for (const auto& path : bank.at(std::to_string(action))) {
                    std::string name = path.get<std::string>();
                    if (name.rfind("custom/din_voice/pcm/", 0) != 0)
                        return;
                    auto pcm = manager->GetArchiveManager()->LoadFile(name);
                    if (!pcm || !pcm->Buffer || pcm->Buffer->empty() || pcm->Buffer->size() % 2)
                        return;
                    std::vector<int16_t> samples;
                    samples.reserve(pcm->Buffer->size() / 2);
                    for (size_t i = 0; i < pcm->Buffer->size(); i += 2)
                        samples.push_back(static_cast<int16_t>(static_cast<uint8_t>((*pcm->Buffer)[i]) |
                                                               (static_cast<uint8_t>((*pcm->Buffer)[i + 1]) << 8)));
                    loaded[age][action].push_back(std::move(samples));
                }
                if (loaded[age][action].empty())
                    return;
            }
        }
        banks = std::move(loaded);
        available = true;
    } catch (const nlohmann::json::exception&) {
        // A missing/bad optional pack leaves native soundfont routing available.
    }
}
} // namespace

extern "C" uint8_t DinVoice_TryPlay(uint16_t sfxId, const void* position) {
    const bool renewable = !(sfxId & 0x0C00);
    if ((sfxId & 0xF000) == 0x6000)
        sfxId = (sfxId & ~0x0C00) | 0x0800;
    // Only Link's own human bank. Other actors and the native transformation banks
    // retain their voices; the custom-form override runs before this SFX boundary.
    if (!gPlayState || sfxId < 0x6820 || sfxId >= 0x6840)
        return 0;
    Player* player = GET_PLAYER(gPlayState);
    if (!player || player->transformation != PLAYER_FORM_HUMAN || position != &player->actor.projectedPos ||
        CustomForms_ActiveForm() != CUSTOM_FORM_NONE || WolfLinkForm_IsSelected())
        return 0;
    Load();
    unsigned age = AdultLink_IsActive() ? 1 : 0;
    if (!available && !age)
        return 0;
    playerPosition = position;
    unsigned action = sfxId - 0x6820;
    if (!renewable && lastFrame[age][action] == gPlayState->gameplayFrames && mixer.IsPlaying(sfxId))
        return 1;
    lastFrame[age][action] = gPlayState->gameplayFrames;
    const int16_t* samples = nullptr;
    uint32_t frames = 0;
    if (available) {
        const auto& choices = banks[age][action];
        ++variants[age][action];
        size_t selected = std::uniform_int_distribution<size_t>(0, choices.size() - 1)(voiceRng);
        selectedVariant[age][action] = selected;
        const auto& pcm = choices[selected];
        samples = pcm.data();
        frames = static_cast<uint32_t>(pcm.size());
    } else {
        ++variants[age][action];
        samples = OotAdultVoice_GetClip(action, voiceRng(), &frames);
        if (!samples || !frames)
            return 0;
    }
    used = true;
    mixer.SetVolume(ReadVolume());
    // Preserve each donor's recorded pitch. Only the explicit audio-editor slider
    // changes it; MM's automatic adult 0.85 multiplier does not apply to these clips.
    float pitch = CVarGetInteger("gAudioEditor.LinkVoiceFreqMultiplier.Enable", 0)
                      ? CVarGetFloat("gAudioEditor.LinkVoiceFreqMultiplier.Scale", 1.0f)
                      : 1.0f;
    if (!std::isfinite(pitch) || pitch <= 0)
        pitch = 1;
    mixer.Play(samples, frames, 1.0f, std::clamp(pitch, 0.4f, 2.5f), sfxId, renewable);
    // Dropping a fifth overlapping voice must not leak a native male fallback.
    return 1;
}

extern "C" void DinVoice_MixInto(int16_t* output, uint32_t frames) {
    mixer.Mix(output, frames);
}

extern "C" void DinVoice_Stop(uint16_t sfxId, const void* position) {
    if ((sfxId & 0xF000) == 0x6000)
        sfxId = (sfxId & ~0x0C00) | 0x0800;
    if (!position || position == playerPosition)
        mixer.Stop(sfxId);
}

extern "C" uint8_t DinVoice_IsPlaying(uint16_t sfxId) {
    if ((sfxId & 0xF000) == 0x6000)
        sfxId = (sfxId & ~0x0C00) | 0x0800;
    return mixer.IsPlaying(sfxId);
}

extern "C" void DinVoice_EndFrame(void) {
    if (used)
        mixer.SetVolume(ReadVolume());
    mixer.EndFrame();
}
