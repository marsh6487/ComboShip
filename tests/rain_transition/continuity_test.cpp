#include "test_require.h"
#include "z64.h"
#include "code/concurrent_weather_audio.h"
#include "mods/extended_inventory.h"
#include "soh/Enhancements/audio/SeasonWeather.h"
#include "soh/Enhancements/audio/GlobalOutdoorRain.h"
#include "soh/Enhancements/audio/GlobalOutdoorRainBridge.h"
#include "soh/Enhancements/audio/WeatherSamplePlayer.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include <array>
#include <map>
#include <string>
#include <vector>
#include <cstdio>

GameInteractor* GameInteractor::Instance = nullptr;
static std::map<std::string, int32_t> settings;
static int nativeRandomCalls;
static ConcurrentWeatherAudioState natureWeather = {};
static int natureRainWrites;
extern "C" {
SaveContext gSaveContext = {};
PlayState* gPlayState = nullptr;
LightningStrike gLightningStrike = {};
static GameInfo gameInfo = {};
GameInfo* gGameInfo = &gameInfo;
static NeiSaveData save = {};
NeiSaveData* Nei_Save() { return &save; }
uint8_t Seasons_SeasonCount() { return 0; }
uint8_t Seasons_GetSeason() { REQUIRE(false); return 0; }
int32_t CVarGetInteger(const char* key, int32_t fallback) {
    auto found = settings.find(key);
    return found == settings.end() ? fallback : found->second;
}
Color_RGB8 CVarGetColor24(const char*, Color_RGB8 fallback) { return fallback; }
s16 Rand_S16Offset(s16 base, s16) { ++nativeRandomCalls; return base; }
u8 Audio_IsNatureRainEnabled() {
    return !ConcurrentWeatherAudio_ShouldPlayRainSfx(&natureWeather, 1, 1);
}
void Audio_SetNatureAmbienceChannelIO(u8 channel, u8 port, u8 value) {
    ++natureRainWrites;
    ConcurrentWeatherAudio_TrackNatureChannel(&natureWeather, channel, port, value, 0x0E, 0x0F, 1);
}
void GlobalOutdoorRain_Log(const char*) {}
SoundFontSample* ResourceMgr_LoadAudioSample(const char*) {
    // Distinct nonzero positions make cursor resets and dropped audio observable.
    static s16 coefficients[16] = {};
    static AdpcmBook book = { 2, 1, coefficients };
    static u8 data[9] = { 0x50, 0x12, 0x34, 0x56, 0x71, 0x23, 0x45, 0x67, 0x12 };
    static AdpcmLoop loop = { 4, 12, UINT32_MAX, 16, {} };
    static SoundFontSample sample = {};
    sample.codec = 0;
    sample.size = sample.fileSize = sizeof(data);
    sample.sampleAddr = data;
    sample.loop = &loop;
    sample.book = &book;
    return &sample;
}
}

struct Fixture {
    PlayState play = {};
    Camera camera = {};
    Fixture(int season, int mode, bool authored = false) {
        SeasonWeather_Reset();
        GlobalOutdoorRain_Reset();
        WeatherSamplePlayer_Init();
        settings.clear();
        settings["gAudioEditor.SeasonWeather.Season"] = season;
        settings["gAudioEditor.SeasonWeather.Spring.RainMode"] = mode;
        settings["gAudioEditor.SeasonWeather.Autumn.RainMode"] = mode;
        settings["gAudioEditor.GlobalOutdoorRain"] = 0;
        settings["gAudioEditor.GlobalOutdoorRainOvercast"] = 0;
        settings["gAudioEditor.ProximityWeatherRainVolume"] = 100;
        gSaveContext = {};
        gSaveContext.gameMode = GAMEMODE_NORMAL;
        play.cameraPtrs[0] = &camera;
        play.skyboxId = SKYBOX_NORMAL_SKY;
        GlobalOutdoorRain_BeginScene(&play, authored ? 30 : 0, 0, 0);
        nativeRandomCalls = 0;
        natureWeather = {};
        natureRainWrites = 0;
    }
    std::array<int, 3> Frame() {
        ++play.state.frames;
        SeasonWeather_Update(&play);
        GlobalOutdoorRain_Update(&play);
        int16_t pcm[2] = {};
        WeatherSamplePlayer_Mix(pcm, 1);
        REQUIRE(pcm[0] == pcm[1]);
        return { play.envCtx.unk_EE[0], pcm[0], GlobalOutdoorRain_HasRainIntent() };
    }
};

static std::vector<std::array<int, 3>> Trace(int season, int mode, bool doors, int owners, bool nativeAmbience = false) {
    Fixture f(season, mode, owners == 2);
    int nativeOwner = 0;
    std::vector<std::array<int, 3>> trace;
    int dryFrames = 0, wetFrames = 0;
    for (int frame = 0; frame < 2400; ++frame) {
        // Rooms change during initial dry, fade-in, sustain, fade-out and pause.
        if (doors && frame % 7 == 0) {
            f.play.roomCtx.curRoom.num = (frame / 7) % 5;
            if (owners == 1) {
                GlobalOutdoorRain_SetNativeRequest(&f.play, &nativeOwner, 0, 0);
                GlobalOutdoorRain_Resolve(&f.play);
            }
        }
        // Scene policy has no room-level retirement API. Rebind only the
        // resolver's policy here to expose the controller's hidden cycle after
        // 157 frames of authored rain and repeated room changes.
        if (owners == 2 && frame == 157)
            GlobalOutdoorRain_BeginScene(&f.play, 0, 0, 0);
        if (nativeAmbience && frame % 17 == 0)
            ConcurrentWeatherAudio_TrackNatureChannel(&natureWeather, 0x0E, 1, 1, 0x0E, 0x0F, 1);
        // Scheduled arrivals/removals are independent of the seasonal phase,
        // including dry/fade intervals. Native density and thunder retain priority.
        int nativeDensity = 0;
        const bool nativeThunder = frame % 3 != 0;
        if (owners == 1) {
            static const int densities[] = { 12, 25, 48 };
            if (frame < 1800 && frame % 7 < 3)
                nativeDensity = densities[(frame / 7) % 3];
            GlobalOutdoorRain_SetNativeRequest(&f.play, &nativeOwner, nativeDensity, nativeThunder);
            GlobalOutdoorRain_Resolve(&f.play);
        }
        f.play.pauseCtx.state = frame >= 250 && frame < 310 ? 1 : 0;
        auto result = f.Frame();
        // Intermittent wet phases must actually mix audio after every dry interval.
        REQUIRE(result[0] == 0 ? result[1] == 0 : result[1] > 0);
        if (nativeDensity > 0) {
            REQUIRE(result[0] == nativeDensity);
            REQUIRE(f.play.envCtx.lightningMode == (nativeThunder ? LIGHTNING_MODE_ON : LIGHTNING_MODE_OFF));
        } else {
            REQUIRE(f.play.envCtx.lightningMode == LIGHTNING_MODE_OFF);
        }
        if (result[0] > 0) REQUIRE(!Audio_IsNatureRainEnabled());
        if (result[0] == 0) ++dryFrames;
        if (result[0] == 30) ++wetFrames;
        trace.push_back(result);
    }
    REQUIRE(nativeRandomCalls == 0);
    if (nativeAmbience) REQUIRE(natureRainWrites > 20);
    if (!doors) REQUIRE(wetFrames > 100);
    if (mode == 2 && owners != 2) REQUIRE(dryFrames > 400);
    return trace;
}

static void RoomDoorsPreserveTheProductionCycleAndPcm() {
    for (int season : { SEASON_SPRING, SEASON_AUTUMN }) {
        for (int mode : { 1, 2 }) {
            const auto unowned = Trace(season, mode, false, 0);
            int dryArrivals = 0, fadeArrivals = 0, dryRemovals = 0, fadeRemovals = 0;
            for (size_t frame = 0; frame < 1800; ++frame) {
                const int density = unowned[frame][0];
                if (frame % 7 == 0) {
                    dryArrivals += density == 0;
                    fadeArrivals += density > 0 && density < 30;
                } else if (frame % 7 == 3) {
                    dryRemovals += density == 0;
                    fadeRemovals += density > 0 && density < 30;
                }
            }
            REQUIRE(dryArrivals > 0 && fadeArrivals > 0 && fadeRemovals > 0);
            if (mode == 2) REQUIRE(dryRemovals > 0);
            for (bool nativeAmbience : { false, true }) {
                for (int owners : { 0, 1, 2 }) {
                    auto control = Trace(season, mode, false, owners, nativeAmbience);
                    auto doors = Trace(season, mode, true, owners, nativeAmbience);
                    for (size_t frame = 0; frame < control.size(); ++frame) {
                        if (control[frame] != doors[frame]) {
                            fprintf(stderr, "room discontinuity season=%d mode=%d owners=%d frame=%zu "
                                    "control density/pcm/intent=%d/%d/%d doors=%d/%d/%d\n", season, mode, owners,
                                    frame, control[frame][0], control[frame][1], control[frame][2],
                                    doors[frame][0], doors[frame][1], doors[frame][2]);
                        }
                        REQUIRE(control[frame] == doors[frame]);
                        // Ownership cannot freeze or restart the concealed season:
                        // once retired its target must match the never-owned cycle.
                        if ((owners == 1 && frame >= 1800) || (owners == 2 && frame >= 157))
                            REQUIRE(doors[frame][0] == unowned[frame][0]);
                    }
                }
            }
        }
    }
}

static void SceneSeasonModeAndPauseRetainIntentionalResets() {
    Fixture f(SEASON_SPRING, 1);
    for (int frame = 0; frame < 30; ++frame) f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 30);
    f.play.pauseCtx.state = 1;
    f.play.roomCtx.curRoom.num = 1;
    REQUIRE(f.Frame()[0] == 30);
    f.play.pauseCtx.state = 0;
    // A new scene intentionally starts the configured fade, even with reused PlayState memory.
    ++f.play.sceneNum;
    REQUIRE(f.Frame()[0] == 0);
    REQUIRE(f.Frame()[0] > 0);
    for (int frame = 0; frame < 30; ++frame) f.Frame();
    settings["gAudioEditor.SeasonWeather.Season"] = SEASON_AUTUMN;
    settings["gAudioEditor.SeasonWeather.Autumn.RainMode"] = 2;
    REQUIRE(f.Frame()[0] == 0);
    for (int frame = 0; frame < 35; ++frame) REQUIRE(f.Frame()[0] == 0);
    settings["gAudioEditor.SeasonWeather.Autumn.RainMode"] = 1;
    REQUIRE(f.Frame()[0] == 0);
    for (int frame = 0; frame < 30; ++frame) f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 30);
    settings["gAudioEditor.SeasonWeather.Autumn.RainMode"] = 0;
    REQUIRE(f.Frame()[0] == 0);
    REQUIRE(f.Frame()[1] == 0);

    settings["gAudioEditor.SeasonWeather.Autumn.RainMode"] = 1;
    for (int frame = 0; frame < 30; ++frame) f.Frame();
    REQUIRE(f.play.envCtx.unk_EE[0] == 30);
    GlobalOutdoorRain_OnPlayDestroy();
    SeasonWeather_Reset();
    int16_t afterExit[2] = {};
    WeatherSamplePlayer_Mix(afterExit, 1);
    REQUIRE(afterExit[0] == 0 && afterExit[1] == 0);
    REQUIRE(!GlobalOutdoorRain_HasRainIntent());
    // Explicit lifetime hooks also reset a reused scene and PlayState address.
    f.play.envCtx = {};
    GlobalOutdoorRain_BeginScene(&f.play, 0, 0, 0);
    REQUIRE(f.Frame()[0] == 0);
    REQUIRE(f.Frame()[0] > 0);
}

int main() {
    RoomDoorsPreserveTheProductionCycleAndPcm();
    SceneSeasonModeAndPauseRetainIntentionalResets();
    puts("PASS seasonal room continuity: Spring/Autumn, persistent/intermittent, no actors/native/authored owners, varied density/thunder, hidden cycle retirement, nature channel handoff, PCM, pause and intentional resets");
}
