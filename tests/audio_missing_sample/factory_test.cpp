#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <tinyxml2.h>
#if TEST_MM
#include "2s2h/resource/importer/AudioSampleFactory.h"
#include "2s2h/resource/importer/AudioSoundFontFactory.h"
#else
#include "soh/resource/importer/AudioSampleFactory.h"
#include "soh/resource/importer/AudioSoundFontFactory.h"
#endif
#include "codec.inc"

static std::vector<std::string> errors;
template <typename... Args> static void RecordError(const char*, const std::string& resource, Args&&... details) {
    std::string message = resource;
    auto append = [&](const auto& value) {
        if constexpr (std::is_convertible_v<decltype(value), const char*>) {
            if (value != nullptr) message += std::string(" ") + value;
        }
    };
    (append(details), ...);
    errors.push_back(message);
}
#define SPDLOG_ERROR(...) RecordError(__VA_ARGS__)

// The real ArchiveManager can return nullptr (missing entry), a File with a
// null Buffer, or an empty Buffer. Keep those distinctions at this boundary.
static std::shared_ptr<Ship::File> rawFile;
static std::string loadedPath;
static std::function<std::shared_ptr<Ship::IResource>(const std::string&)> loadResource;
struct ArchiveBoundary {
    std::shared_ptr<Ship::File> LoadFile(const char* path) {
        assert(path != nullptr && path[0] != '\0');
        loadedPath = path;
        return rawFile;
    }
};
struct ResourceBoundary {
    ArchiveBoundary archive;
    ArchiveBoundary* GetArchiveManager() { return &archive; }
    std::shared_ptr<Ship::IResource> LoadResourceProcess(const std::string& path) { return loadResource(path); }
};
static ResourceBoundary ownManager;
namespace Ship {
struct CrossRMRegistry {
    static ResourceBoundary* GetOrActive(const char* game) {
        assert(std::string(game) == TEST_GAME);
        return &ownManager;
    }
};
struct Context {
    static Context* GetRawInstance() { static Context context; return &context; }
    ResourceBoundary* GetResourceManager() { return &ownManager; }
};
}
#ifdef COMBO_BUILD
#define COMBO_OWN_RM() (Ship::CrossRMRegistry::GetOrActive(TEST_GAME))
#else
#define COMBO_OWN_RM() (Ship::Context::GetRawInstance()->GetResourceManager())
#endif

// No codec worker may run for missing input. Successful raw-sample controls
// execute the actual copy, not a replacement decoder.
using drwav_uint64 = uint64_t;
using drwav_bool32 = uint32_t;
struct drwav { uint32_t sampleRate = 32000; uint32_t channels = 1; };
static void UnexpectedDecoder() { throw std::runtime_error("decoder received unavailable audio"); }
static drwav_bool32 drwav_init_memory(drwav*, const void*, size_t, void*) { UnexpectedDecoder(); return 0; }
static void drwav_get_length_in_pcm_frames(drwav*, drwav_uint64*) { UnexpectedDecoder(); }
static void drwav_read_pcm_frames_s16(drwav*, drwav_uint64, int16_t*) { UnexpectedDecoder(); }
static void Mp3DecoderWorker(std::shared_ptr<SOH::AudioSample>, std::shared_ptr<Ship::File>) { UnexpectedDecoder(); }
static void FlacDecoderWorker(std::shared_ptr<SOH::AudioSample>, std::shared_ptr<Ship::File>) { UnexpectedDecoder(); }
static void OggDecoderWorker(std::shared_ptr<SOH::AudioSample>, std::shared_ptr<Ship::File>,
                             std::shared_ptr<Ship::ResourceInitData>) { UnexpectedDecoder(); }
template <typename Worker, typename... Args> static void DecodeSampleAsync(Worker&& worker, Args&&... args) {
    std::invoke(std::forward<Worker>(worker), std::forward<Args>(args)...);
}
template <typename... Args> static void MM_AudioTraceResource(Args&&...) {}
template <typename... Args> static void MM_AudioTraceFormat(Args&&...) {}
#include "factory.inc"

static bool cleared;
#define AudioSynth_ClearBuffer(...) (cleared = true)
static void PlaySilence(SOH::Sample* sample) {
    constexpr int A_CONTINUE = 0;
    constexpr int SAMPLE_SIZE = 2;
    int flags = -1, skipBytes = -1;
    int samplesLenAdjusted = 32, nSamplesProcessed = 0, s5 = 0;
    int numSamplesToLoadAdj = 32, numSamplesProcessed = 0, dmemUncompressedAddrOffset1 = 0;
    cleared = false;
    switch (sample->codec) {
#include "silence.inc"
        default: throw std::runtime_error("missing audio reached a data-reading codec");
    }
skip:
    assert(cleared && flags == A_CONTINUE && skipBytes == 0);
#if TEST_MM
    assert(numSamplesProcessed == 32 && dmemUncompressedAddrOffset1 == 32);
#else
    assert(nSamplesProcessed == 32 && s5 == 32);
#endif
}

static std::shared_ptr<Ship::File> XmlFile(const std::string& xml) {
    auto file = std::make_shared<Ship::File>();
    auto document = std::make_shared<tinyxml2::XMLDocument>();
    assert(document->Parse(xml.c_str()) == tinyxml2::XML_SUCCESS);
    file->Reader = document;
    return file;
}
static const char* rawPath = "custom/samples/Kingdom Hearts - Re Chain of Memories - Graceful AssassinL";
static const char* metadataPath = "custom/samples/Kingdom Hearts - Re Chain of Memories - Graceful AssassinL_META";
static std::shared_ptr<SOH::AudioSample> LoadSample(const std::string& format, const std::string& pathAttribute) {
    const std::string custom = format == "adpcm" ? "" : " CustomFormat=\"" + format + "\"";
    auto file = XmlFile("<Sample Codec=\"ADPCM\" Medium=\"Cart\" bit26=\"1\" Size=\"4\"" + custom + pathAttribute +
        "><ADPCMLoop Start=\"0\" End=\"8000000\" Count=\"4294967295\"/></Sample>");
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = metadataPath;
    init->Format = RESOURCE_FORMAT_XML;
    SOH::ResourceFactoryXMLAudioSampleV0 factory;
    return std::dynamic_pointer_cast<SOH::AudioSample>(factory.ReadResource(file, init));
}
static void CheckSilent(const std::shared_ptr<SOH::AudioSample>& resource) {
    assert(resource != nullptr);
    auto* sample = resource->GetPointer();
    assert(sample != nullptr && sample->sampleAddr == nullptr && sample->size == 0 && sample->fileSize == 0);
    assert(sample->medium == MEDIUM_RAM && sample->unk_bit26 == 0);
    assert(sample->loop != nullptr && sample->loop->start == 0 && sample->loop->end > 0 && sample->loop->count == 0);
    assert(resource->tuning == -1.0f);
    PlaySilence(sample);
}
static void TestMissing(const std::string& state, const std::string& format) {
    errors.clear(); loadedPath.clear(); rawFile.reset();
    if (state == "null-buffer" || state == "empty-buffer") rawFile = std::make_shared<Ship::File>();
    if (state == "empty-buffer") rawFile->Buffer = std::make_shared<std::vector<char>>();
    std::string attribute = " Path=\"" + std::string(rawPath) + "\"";
    if (state == "missing-path") attribute.clear();
    if (state == "empty-path") attribute = " Path=\"\"";
    auto sample = LoadSample(format, attribute);
    CheckSilent(sample);
    assert(errors.size() == 1 && errors[0].find(metadataPath) != std::string::npos);
    if (state != "missing-path" && state != "empty-path") assert(errors[0].find(rawPath) != std::string::npos);
    std::cout << "PASS " << TEST_GAME << " " << state << " " << format << '\n';
}
static void TestValid() {
    errors.clear(); rawFile = std::make_shared<Ship::File>();
    rawFile->Buffer = std::make_shared<std::vector<char>>(std::initializer_list<char>{1, 2, 3, 4});
    auto sample = LoadSample("adpcm", " Path=\"" + std::string(rawPath) + "\"");
    assert(errors.empty() && loadedPath == rawPath && sample != nullptr);
    assert(sample->sample.codec == CODEC_ADPCM && sample->sample.medium == MEDIUM_CART);
    assert(sample->sample.size == 4 && sample->sample.fileSize == 4 && sample->sample.unk_bit26 == 1);
    assert(std::memcmp(sample->sample.sampleAddr, "\x01\x02\x03\x04", 4) == 0);
    assert(sample->loop.end == 8000000 && sample->loop.count == 0xFFFFFFFF);
    std::cout << "PASS " << TEST_GAME << " valid sample bytes and metadata preserved\n";
}
static void TestFont() {
    rawFile.reset(); errors.clear();
    auto sample = LoadSample("flac", " Path=\"" + std::string(rawPath) + "\"");
    loadResource = [sample](const std::string& path) { assert(path == "missing-meta"); return sample; };
    auto xml = XmlFile("<SoundFont Medium=\"Cart\" CachePolicy=\"Temporary\"><Instruments><Instrument>"
        "<LowNotesSound SampleRef=\"missing-meta\" Tuning=\"1.25\"/>"
        "<NormalNotesSound SampleRef=\"missing-meta\" Tuning=\"1.5\"/>"
        "<HighNotesSound SampleRef=\"missing-meta\" Tuning=\"2\"/>"
        "</Instrument></Instruments><SfxTable Count=\"1\">"
        "<Sfx SampleRef=\"missing-meta\" Tuning=\"3\"/></SfxTable></SoundFont>");
    auto init = std::make_shared<Ship::ResourceInitData>(); init->Format = RESOURCE_FORMAT_XML; init->Path = "test-font";
    SOH::ResourceFactoryXMLSoundFontV0 factory;
    auto font = std::dynamic_pointer_cast<SOH::AudioSoundFont>(factory.ReadResource(xml, init));
    assert(font && font->instrumentAddresses.size() == 1 && font->soundEffects.size() == 1);
    auto* instrument = font->instrumentAddresses[0];
    assert(instrument->lowNotesSound.tuning == 1.25f && instrument->normalNotesSound.tuning == 1.5f);
    assert(instrument->highNotesSound.tuning == 2.0f && font->soundEffects[0].tuning == 3.0f);
    for (auto* sound : {&instrument->lowNotesSound, &instrument->normalNotesSound,
                       &instrument->highNotesSound, &font->soundEffects[0]}) {
        assert(sound->sample == sample->GetPointer());
        PlaySilence(sound->sample);
    }
    loadResource = {};
    std::cout << "PASS " << TEST_GAME << " missing sample survives real font parsing and playback\n";
}
int main(int argc, char** argv) {
    const std::string selected = argc > 1 ? argv[1] : "all";
    if (selected == "all" || selected == "valid") TestValid();
    if (selected == "all" || selected == "missing") {
        for (const char* state : {"missing-file", "null-buffer", "empty-buffer", "missing-path", "empty-path"})
            for (const char* format : {"flac", "mp3", "ogg", "wav", "adpcm"}) TestMissing(state, format);
    }
    if (selected == "all" || selected == "font") TestFont();
}
