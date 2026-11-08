#include "z64.h"
extern "C" {
#include "functions.h"
#include "variables.h"
#include "macros.h"
}
#include "mods/transformation_masks/assets/mm_asset_loader.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Archive loading and resource retention are service boundaries. The production
// binary reader/repatcher below decides which of these donor samples is used.
struct FixtureResource {
    void* pointer;
    void* GetRawPointer() const { return pointer; }
};
struct FixtureFile {
    std::shared_ptr<std::vector<char>> Buffer;
};
struct FixtureArchive {
    std::shared_ptr<FixtureFile> fontFile;
    std::shared_ptr<FixtureFile> LoadFile(const std::string& path) {
        return path == "audio/fonts/Soundfont_0" ? fontFile : nullptr;
    }
};
static std::shared_ptr<FixtureArchive> sMmArchive;
static std::unordered_map<std::string, std::shared_ptr<FixtureResource>> sMmResourceCache;
static std::unordered_map<std::string, SoundFontSample*> donorSamples;
static std::vector<SoundFontSample*> decodedSamples;
static std::vector<s32> fontRequests;
static bool fontAvailable, fontPatched, paused;
static int lockDepth, lockCalls, renderCalls;
static SoundFont testFont;
static Instrument instruments[122];
static Instrument* instrumentPointers[122];
static SoundFontSample mmSamples[122][3], ootSamples[122][3], unrelatedSample;
static AdpcmLoop loops[122];
static u8 sampleBytes[122][3], ootSampleBytes[122][3], unrelatedByte;
static SoundFontSound unrelatedSound;
static GameInfo gameInfo;
GameInfo* gGameInfo = &gameInfo;
PlayState* gPlayState = nullptr;
static struct { s32 currentForm; } gFormState;

#define MMSFX_LOG(...) ((void)0)
#define MMFORM_LOG(...) ((void)0)

extern "C" int32_t CVarGetInteger(const char*, int32_t defaultValue) { return defaultValue; }
static void OTRAudio_LockMutex() { assert(lockDepth == 0); ++lockDepth; ++lockCalls; }
static void OTRAudio_UnlockMutex() { assert(lockDepth == 1); --lockDepth; }
extern "C" int MmSfx_IsGamePaused(void) { return paused; }
extern "C" void MmSfxSynth_RenderInto(short*, unsigned int) { ++renderCalls; }

static void* MmAssets_LoadFromMmArchive(const char* path, size_t*) {
    auto retained = sMmResourceCache.find(path);
    if (retained != sMmResourceCache.end())
        return retained->second->GetRawPointer();
    auto donor = donorSamples.find(path);
    if (donor == donorSamples.end())
        return nullptr;
    sMmResourceCache[path] = std::make_shared<FixtureResource>(FixtureResource{ donor->second });
    return donor->second;
}

// The decoder is covered independently by the MM decoder/PCM suites. Use a
// deterministic synthetic wave here so rate, continuity, lifetime and ownership
// can be observed without relying on unavailable copyrighted sample payloads.
static s16* MmDirectAudio_DecodeADPCM(SoundFontSample* sample, u32* length) {
    assert(lockDepth == 1 && "note allocation must be inside the audio lock");
    decodedSamples.push_back(sample);
    *length = 8192;
    auto pcm = static_cast<s16*>(std::malloc(*length * sizeof(s16)));
    assert(pcm);
    for (u32 i = 0; i < *length; ++i)
        pcm[i] = static_cast<s16>((static_cast<int>(i % 64) - 32) * 64);
    return pcm;
}

static void MmDirectAudio_ComputeSpatial(Vec3f*, f32* volume, f32* pan) {
    *volume = 0.8f;
    *pan = 0.5f;
}

#include "form_audio_production.inc"
#include "form_jingle_production.inc"

extern "C" SoundFont* MmSfx_LoadFont(s32 fontId) {
    assert(lockDepth == 1 && "font selection must stay within the note transaction");
    fontRequests.push_back(fontId);
    if (!fontAvailable || fontId != 0)
        return nullptr;
    if (!fontPatched) {
        MmSfx_PatchFontSamplesFromMmArchive(&testFont, "audio/fonts/Soundfont_0");
        fontPatched = true;
    }
    return &testFont;
}

static std::string SamplePath(u32 index, u32 band) {
    return "audio/samples/fixture_" + std::to_string(index) + "_" + std::to_string(band);
}

static void U8(std::vector<char>& bytes, u8 value) { bytes.push_back(static_cast<char>(value)); }
static void U16(std::vector<char>& bytes, u16 value) {
    U8(bytes, value & 0xff); U8(bytes, value >> 8);
}
static void WriteU32(std::vector<char>& bytes, u32 value) {
    U16(bytes, value & 0xffff); U16(bytes, value >> 16);
}
static void Float(std::vector<char>& bytes, float value) {
    u32 bits;
    std::memcpy(&bits, &value, sizeof(bits));
    WriteU32(bytes, bits);
}
static void String(std::vector<char>& bytes, const std::string& value) {
    WriteU32(bytes, value.size());
    bytes.insert(bytes.end(), value.begin(), value.end());
}

static void Reset() {
    MmDirectAudio_StopAll();
    std::memset(sPlayingSounds, 0, sizeof(sPlayingSounds));
    std::memset(sMmReverbBufL, 0, sizeof(sMmReverbBufL));
    std::memset(sMmReverbBufR, 0, sizeof(sMmReverbBufR));
    std::memset(instruments, 0, sizeof(instruments));
    std::memset(mmSamples, 0, sizeof(mmSamples));
    std::memset(ootSamples, 0, sizeof(ootSamples));
    std::memset(loops, 0, sizeof(loops));
    sMmAudioFrame = sMmReverbBase = 0;
    sFormJingle = {};
    decodedSamples.clear(); fontRequests.clear(); donorSamples.clear(); sMmResourceCache.clear();
    assert(lockDepth == 0);
    lockCalls = renderCalls = 0;
    fontAvailable = true; fontPatched = false; paused = false;
    R_UPDATE_RATE = 3;
    gFormState.currentForm = 2;
    testFont = {};
    testFont.numInstruments = 122;
    testFont.instruments = instrumentPointers;
    unrelatedSample = {};
    unrelatedSample.sampleAddr = &unrelatedByte;
    unrelatedSound = {};
    unrelatedSound.sample = &unrelatedSample;
    unrelatedSound.tuning = 1.0f;
    testFont.numSfx = 1;
    testFont.soundEffects = &unrelatedSound;

    auto bytes = std::make_shared<std::vector<char>>(64, 0);
    WriteU32(*bytes, 0); U8(*bytes, 0); U8(*bytes, 0);
    U16(*bytes, 0); U16(*bytes, 0); U16(*bytes, 0);
    WriteU32(*bytes, 0); WriteU32(*bytes, 122); WriteU32(*bytes, 1);
    for (u32 i = 0; i < 122; ++i) {
        auto& instrument = instruments[i];
        instrumentPointers[i] = &instrument;
        instrument.normalRangeLo = instrument.normalRangeHi = 60;
        instrument.releaseRate = 200;
        loops[i].start = 128;
        loops[i].loopEnd = 4096;
        loops[i].count = (i == 92 || i == 120) ? 0 : 0xffffffff;
        loops[i].sampleEnd = 8192;
        U8(*bytes, 1); U8(*bytes, 0); U8(*bytes, 60); U8(*bytes, 60); U8(*bytes, 200);
        WriteU32(*bytes, 0); // no ADSR points; preserve the existing default envelope
        for (u32 band = 0; band < 3; ++band) {
            mmSamples[i][band].sampleAddr = &sampleBytes[i][band];
            mmSamples[i][band].loop = &loops[i];
            ootSamples[i][band].sampleAddr = &ootSampleBytes[i][band];
            ootSamples[i][band].loop = &loops[i];
            donorSamples[SamplePath(i, band)] = &mmSamples[i][band];
            auto* sound = band == 0 ? &instrument.lowNotesSound :
                          band == 1 ? &instrument.normalNotesSound : &instrument.highNotesSound;
            sound->sample = &ootSamples[i][band];
            sound->tuning = band == 0 ? 0.125f : band == 1 ? 0.5f : 4.0f;
            U8(*bytes, 1); U8(*bytes, 1);
            String(*bytes, SamplePath(i, band)); Float(*bytes, sound->tuning);
        }
    }
    // A separate imported SFX pointer must not be rewritten by a Gakki-only guard.
    U8(*bytes, 1); U8(*bytes, 1);
    String(*bytes, "audio/samples/unrelated_missing"); Float(*bytes, 1.0f);
    sMmArchive = std::make_shared<FixtureArchive>();
    sMmArchive->fontFile = std::make_shared<FixtureFile>(FixtureFile{ bytes });
}

static bool Near(f32 actual, f32 expected) { return std::fabs(actual - expected) < 0.00001f; }
static MmPlayingSound* Active(u16 id) {
    for (auto& sound : sPlayingSounds)
        if (sound.active && sound.mmSfxId == id)
            return &sound;
    return nullptr;
}
static MmPlayingSound* OtherVoice() {
    for (auto& sound : sPlayingSounds)
        if (sound.active && sound.mmSfxId != MM_GAKKI_SFXID)
            return &sound;
    return nullptr;
}
static size_t ActiveCount() {
    size_t count = 0;
    for (const auto& sound : sPlayingSounds)
        count += sound.active != 0;
    return count;
}
static void Mix(u32 count = 128) {
    std::vector<s16> samples(count * 2);
    MmDirectAudio_MixInto(samples.data(), count);
}

static void UpdateHeld(u8 pitch, f32 bend) {
#ifdef FORM_AUDIO_HAS_UPDATE_PITCH
    MmGakki_UpdatePitch(pitch, bend);
#else
    // Reproduce the baseline caller's held-note path through the real function.
    (void)pitch; (void)bend;
    MmGakki_RefreshNote();
#endif
}
static void StopJingle() {
#ifdef FORM_AUDIO_HAS_JINGLE_OWNER
    MmGakki_StopJingleNote();
#else
    MmGakki_StopNote();
#endif
}

static void FormMapping() {
    for (s32 form : { 1, 2, 3 }) {
        for (u8 pitch : { 2, 5, 9, 11, 14, 1, 15 }) {
            MmGakki_PlayPitch(form, pitch, 1.0f, nullptr);
            assert(decodedSamples.back() == &mmSamples[91 + form][1] &&
                   "all ocarina pitches must transpose the selected MM C4 sample");
            assert(fontRequests.back() == 0);
            auto* sound = Active(MM_GAKKI_SFXID);
            assert(sound && Near(sound->advance, 0.5f * std::pow(2.0f, pitch / 12.0f)));
            assert(MmGakki_GetSongInstrument(form) == 91 + form);
        }
        for (u8 button = 0; button < 5; ++button) {
            MmGakki_PlayNote(form, button, nullptr);
            assert(decodedSamples.back() == &mmSamples[91 + form][1]);
            assert(Near(Active(MM_GAKKI_SFXID)->advance,
                        0.5f * std::pow(2.0f, (sOcarinaButtonToMidi[button] - 60) / 12.0f)));
        }
    }
    assert(unrelatedSound.sample == &unrelatedSample);
}

static void NativeCustomControls() {
    for (s32 form : { -1, 0, 4, 5, 7, 8, 9, 10, 99 }) {
        assert(MmGakki_GetVoiceType(form) != GAKKI_VOICE_MM_FONT);
        MmGakki_PlayPitch(form, 9, 1.0f, nullptr);
    }
    assert(decodedSamples.empty() && fontRequests.empty());
    assert(MmGakki_GetNativeInstrument(7) == 2 && MmGakki_GetNativeInstrument(8) == 4 &&
           MmGakki_GetNativeInstrument(9) == 6);
    for (s32 form : { 0, 4 })
        assert(MmGakki_GetSongInstrument(form) == 0);
    MmGakki_PlayPitch(6, 9, 1.0f, nullptr);
    assert(decodedSamples.back() == &mmSamples[120][1] && "preserve Garo's existing voice");
    for (u8 instrument : { 85, 89, 119, 82 }) {
        MmGakki_PlayInstrumentPitch(instrument, 9, 1.0f, nullptr);
        assert(decodedSamples.back() == &mmSamples[instrument][1]);
    }
}

static void HeldBend() {
    MmGakki_PlayPitch(2, 9, 1.0f, nullptr);
    Mix();
    auto* sound = Active(MM_GAKKI_SFXID);
    assert(sound && sound->pcmPosition > 0 && sound->envVolume > 0);
    const auto before = *sound;
    const auto decodes = decodedSamples.size();
    const int locks = lockCalls;
    UpdateHeld(9, 1.122462f);
    assert(Near(sound->advance, 0.5f * std::pow(2.0f, 9.0f / 12) * 1.122462f) &&
           "changing bend on a held pitch must change the active sample rate");
    assert(lockCalls == locks + 1 && lockDepth == 0);
    assert(decodedSamples.size() == decodes && sound->pcmData == before.pcmData);
    assert(sound->pcmPosition == before.pcmPosition && sound->lifeSamples == before.lifeSamples);
    assert(sound->envVolume == before.envVolume && sound->envPhase == before.envPhase &&
           sound->envAttackRate == before.envAttackRate && sound->envReleaseRate == before.envReleaseRate);
    assert(sound->volume == before.volume && sound->pan == before.pan && sound->reverb == before.reverb &&
           sound->loopStart == before.loopStart && sound->loopEnd == before.loopEnd);
    for (int i = 0; i < 100; ++i) {
        UpdateHeld(12, 0.75f);
        assert(Near(sound->advance, 0.75f));
        UpdateHeld(9, 1.0f);
        assert(Near(sound->advance, before.advance) && "updates must use an unbent base, without cumulative drift");
    }
    UpdateHeld(9, 0.0f);
    assert(Near(sound->advance, before.advance));
    Mix();
    assert(sound->pcmPosition > before.pcmPosition && decodedSamples.size() == decodes);
    MmGakki_StopNote();
    UpdateHeld(9, 1.0f);
    assert(!Active(MM_GAKKI_SFXID) && decodedSamples.size() == decodes);
}

static void VoiceOwnership() {
    MmGakki_PlayPitch(2, 9, 1.0f, nullptr);
    Mix();
    auto* live = Active(MM_GAKKI_SFXID);
    auto before = *live;
    MmGakki_PlayInstrumentPitch(94, 5, 1.0f, nullptr);
    assert(ActiveCount() == 2 && "a song jingle must not replace the live/replay instrument");
    auto* jingle = OtherVoice();
    assert(jingle && live->pcmData == before.pcmData && live->pcmPosition == before.pcmPosition);
    const auto jingleBefore = *jingle;
    UpdateHeld(9, 1.1f);
    assert(jingle->advance == jingleBefore.advance && jingle->pcmPosition == jingleBefore.pcmPosition);
    const MmGakkiNoteShape shape = { 0.25f, 0.1f, 12, 1 };
    MmGakki_ShapeActiveNote(&shape);
    assert(live->volume == before.volume && live->pan == before.pan && live->reverb == before.reverb);
    assert(Near(jingle->volume, jingleBefore.volume * 0.25f) && Near(jingle->pan, 12 / 127.0f));
    MmGakki_ReleaseNote();
    assert(jingle->envPhase == ADSR_PHASE_RELEASE && live->envPhase == before.envPhase);
    StopJingle();
    assert(Active(MM_GAKKI_SFXID) == live && !OtherVoice());
    MmGakki_PlayInstrumentPitch(94, 5, 1.0f, nullptr);
    jingle = OtherVoice();
    MmGakki_StopNote();
    assert(!Active(MM_GAKKI_SFXID) && jingle->active && "putaway/replay end cannot stop the jingle");
}

static void JingleLifecycle() {
    assert(FormJingle_Start(8) == 1);
    FormJingle_Update();
    assert(ActiveCount() == 1 && decodedSamples.back() == &mmSamples[93][1]);
    MmGakki_PlayPitch(2, 2, 1.0f, nullptr);
    assert(ActiveCount() == 2 && "the jingle and replay must retain separate notes");
    FormJingle_Stop();
    assert(ActiveCount() == 1 && Active(MM_GAKKI_SFXID) && !sFormJingle.active);
    assert(FormJingle_Start(8) == 1);
    FormJingle_Update();
    MmGakki_StopNote();
    assert(OtherVoice() && sFormJingle.active);
    gFormState.currentForm = 4;
    FormJingle_Update();
    assert(!OtherVoice() && !sFormJingle.active);
    assert(FormJingle_Start(8) == 0 && "human song remains with vanilla");
    gFormState.currentForm = 2;
    assert(FormJingle_Start(0) == 0 && "warp fanfares remain with the original sequence");
}

static void ExplicitSustain() {
    MmGakki_PlayPitch(2, 9, 1.0f, nullptr);
    MmGakki_PlayInstrumentPitch(94, 5, 1.0f, nullptr);
    assert(ActiveCount() == 2);
    {
        MmAudioScopedLock lock;
        assert(MmDirectAudio_PlaySingle(&instruments[93].normalNotesSound, 1.0f, 1.0f,
                                       0x0980, 0.5f, 0.5f, 0, 0));
    }
    for (int frame = 0; frame < 20; ++frame)
        Mix(256);
    assert(Active(MM_GAKKI_SFXID) && OtherVoice());
    assert(!Active(0x0980) && "ordinary continuous SFX still require refresh");
    assert(ActiveCount() == 2 && "both explicitly gated instrument voices must survive without refresh callbacks");
    assert(Active(MM_GAKKI_SFXID)->envPhase == ADSR_PHASE_SUSTAIN);
    assert(OtherVoice()->envPhase == ADSR_PHASE_SUSTAIN);
    auto before = *Active(MM_GAKKI_SFXID);
    paused = true;
    Mix();
    assert(Active(MM_GAKKI_SFXID)->pcmPosition == before.pcmPosition);
    paused = false;
}

static void OneShot() {
    for (s32 form : { 1, 6 }) {
        MmGakki_PlayPitch(form, 9, 1.0f, nullptr);
        auto* sound = Active(MM_GAKKI_SFXID);
        assert(sound && sound->loopEnd == 0);
        for (int frame = 0; frame < 20; ++frame)
            Mix(1024);
        assert(!Active(MM_GAKKI_SFXID) && "Goron/Garo sample exhaustion must not become an invented live sustain loop");
        const auto decodes = decodedSamples.size();
        UpdateHeld(9, 1.0f);
        assert(!Active(MM_GAKKI_SFXID) && decodedSamples.size() == decodes);
    }
}

static void DonorOrigin() {
    for (s32 form : { 1, 2, 3 }) {
        Reset();
        donorSamples.erase(SamplePath(91 + form, 1));
        MmGakki_PlayPitch(form, 9, 1.0f, nullptr);
        assert(decodedSamples.empty() && !Active(MM_GAKKI_SFXID) &&
               "missing required MM sample must not play the importer's retained OoT sample");
        assert(instruments[91 + form].normalNotesSound.sample == &ootSamples[91 + form][1] &&
               "the Gakki guard must not rewrite the shared font");
        MmGakki_PlayNote(form, 2, nullptr);
        MmGakki_PlayInstrumentPitch(91 + form, 9, 1.0f, nullptr);
        assert(decodedSamples.empty() && ActiveCount() == 0);
        assert(unrelatedSound.sample == &unrelatedSample);
    }
    Reset();
    sMmArchive->fontFile = nullptr;
    MmGakki_PlayPitch(2, 9, 1.0f, nullptr);
    assert(decodedSamples.empty() && "an unpatched foreign font sample is not donor proof");
    Reset();
    MmGakki_PlayPitch(2, 9, 1.0f, nullptr);
    donorSamples.clear();
    sMmArchive->fontFile = nullptr;
    MmGakki_PlayPitch(2, 2, 1.0f, nullptr);
    assert(decodedSamples.size() == 2 && decodedSamples.back() == &mmSamples[93][1] &&
           "already retained MM resources must remain usable without reopening the archive");
}

static void MissingResources() {
    MmGakki_PlayInstrumentPitch(94, 5, 1.0f, nullptr);
    fontAvailable = false;
    MmGakki_PlayPitch(2, 9, 1.0f, nullptr);
    assert(OtherVoice() && !Active(MM_GAKKI_SFXID));
    fontAvailable = true;
    testFont.numInstruments = 91;
    MmGakki_PlayPitch(2, 9, 1.0f, nullptr);
    assert(!Active(MM_GAKKI_SFXID));
    testFont.numInstruments = 122;
    instrumentPointers[93] = nullptr;
    MmGakki_PlayPitch(2, 9, 1.0f, nullptr);
    assert(!Active(MM_GAKKI_SFXID));
    instrumentPointers[93] = &instruments[93];
    MmGakki_PlayPitch(2, 9, 1.0f, nullptr);
    MmGakki_PlayInstrumentPitch(255, 9, 1.0f, nullptr);
    assert(Active(MM_GAKKI_SFXID) && !OtherVoice() && "invalid jingle cannot stop the live instrument");
    MmGakki_PlayInstrumentPitch(94, 5, 1.0f, nullptr);
    MmGakki_PlayNote(2, 5, nullptr);
    assert(!Active(MM_GAKKI_SFXID) && OtherVoice());
}

static void UnrelatedAudio() {
    MmGakki_PlayPitch(2, 9, 1.0f, nullptr);
    s32 voiceSlot;
    {
        MmAudioScopedLock lock;
        voiceSlot = MmDirectAudio_PlaySingle(&instruments[93].normalNotesSound, 1.0f, 1.0f,
                                            0x6820, 0.4f, 0.3f, 0, 0);
    }
    assert(voiceSlot > 0);
    auto* voice = &sPlayingSounds[voiceSlot - 1];
    const auto before = *voice;
    UpdateHeld(9, 1.1f);
    MmGakki_PlayInstrumentPitch(94, 5, 1.0f, nullptr);
    const MmGakkiNoteShape shape = { 0.25f, 0.1f, 12, 1 };
    MmGakki_ShapeActiveNote(&shape);
    MmGakki_ReleaseNote();
    MmGakki_StopNote();
    StopJingle();
    assert(voice->active && voice->pcmData == before.pcmData && voice->pcmPosition == before.pcmPosition);
    assert(voice->advance == before.advance && voice->volume == before.volume && voice->pan == before.pan &&
           voice->envPhase == before.envPhase && voice->envVolume == before.envVolume);
    // Preserve existing behavior of custom instruments outside the three required
    // native-form sample checks; no global sample/font fallback policy is changed.
    Reset();
    donorSamples.erase(SamplePath(120, 1));
    MmGakki_PlayPitch(6, 9, 1.0f, nullptr);
    assert(decodedSamples.back() == &ootSamples[120][1]);
}

int main(int argc, char** argv) {
    assert(argc == 2);
    Reset();
    const std::string which = argv[1];
    if (which == "form-mapping") FormMapping();
    else if (which == "native-custom-controls") NativeCustomControls();
    else if (which == "held-bend") HeldBend();
    else if (which == "voice-ownership") VoiceOwnership();
    else if (which == "jingle-lifecycle") JingleLifecycle();
    else if (which == "explicit-sustain") ExplicitSustain();
    else if (which == "one-shot") OneShot();
    else if (which == "donor-origin") DonorOrigin();
    else if (which == "missing-resources") MissingResources();
    else if (which == "unrelated-audio") UnrelatedAudio();
    else assert(false && "unknown scenario");
    MmDirectAudio_StopAll();
    assert(lockDepth == 0);
    std::printf("PASS form instrument audio %s\n", argv[1]);
}
