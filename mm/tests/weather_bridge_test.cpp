#include "2s2h/Enhancements/Audio/MMWeather.h"
#include "2s2h/Enhancements/Audio/MMWeatherAudio.h"
#include "global.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>

SaveContext gSaveContext;
u8 gWeatherMode;
LightningStrike gLightningStrike;
RegEditor editor;
RegEditor* gRegEditor = &editor;
static std::unordered_map<std::string, int> settings;
static float rainGain;
static int resets;
static int rainDraws;

extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    auto found = settings.find(key);
    return found == settings.end() ? fallback : found->second;
}
extern "C" Color_RGBA8 CVarGetColor(const char*, Color_RGBA8 fallback) {
    return fallback;
}
extern "C" u32 Environment_GetStormState(PlayState* play) {
    return play->envCtx.stormState;
}
extern "C" void Environment_DrawRain(PlayState*, View*, GraphicsContext*) {
    ++rainDraws;
}
#include "weather_draw_gate.inc"
#include "weather_bolts.inc"
extern "C" void Environment_DrawLightningFlash(PlayState*, u8, u8, u8, u8) {
}
extern "C" void MMWeatherAudio_Reset() {
    rainGain = 0;
    ++resets;
}
extern "C" void MMWeatherAudio_SetRain(float gain) {
    rainGain = gain;
}
extern "C" void MMWeatherAudio_Thunder(float) {
}

int main() {
    static PlayState play{};
    Camera camera{};
    play.cameraPtrs[0] = &camera;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.envCtx.stormState = STORM_STATE_ON;
    play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
    R_UPDATE_RATE = 3;
    for (auto& bolt : sLightningBolts) {
        bolt.state = LIGHTNING_BOLT_INACTIVE;
    }
    auto native = play.envCtx;
    settings["gAudioEditor.GlobalOutdoorRain"] = 1;
    settings["gAudioEditor.GlobalOutdoorRainMode"] = 1;
    for (int i = 0; i < 25; ++i) {
        MMWeather_Update(&play);
    }
    assert(MMWeather_RainDensity() == 0 && rainGain == 0);
    settings[MM_WEATHER_CVAR("Enabled")] = 1;
    for (int i = 0; i < 25; ++i) {
        MMWeather_Update(&play);
    }
    assert(MMWeather_RainDensity() == 25 && rainGain > 0);
    DrawWeatherFromPlay(&play);
    assert(rainDraws == 1); // Actual Play draw caller must admit enhanced-only rain.
    settings[MM_WEATHER_CVAR("RainVolume")] = 0;
    MMWeather_Update(&play);
    assert(rainGain == 0 && MMWeather_RainDensity() == 25);
    settings[MM_WEATHER_CVAR("RainVolume")] = 100;
    settings[MM_WEATHER_CVAR("Overcast")] = 0;
    MMWeather_Update(&play);
    assert(MMWeather_Overcast() == 0 && MMWeather_Shade(255) == 255);
    settings[MM_WEATHER_CVAR("Overcast")] = 1;
    MMWeather_Update(&play);
    assert(std::memcmp(&native, &play.envCtx, sizeof(native)) == 0);
    assert(gWeatherMode == WEATHER_MODE_CLEAR && gLightningStrike.state == LIGHTNING_STRIKE_WAIT);
    uint8_t first = 0, second = 0, blend = 0;
    MMWeather_ApplySky(&first, &second, &blend);
    assert(first == 0 && second == 1 && blend == 255);

    for (int condition = 0; condition < 10; ++condition) {
        play.envCtx = native;
        camera.stateFlags = 0;
        play.csCtx.state = CS_STATE_IDLE;
        play.skyboxId = SKYBOX_NORMAL_SKY;
        switch (condition) {
            case 0:
                play.envCtx.precipitation[PRECIP_RAIN_MAX] = 60;
                break;
            case 1:
                play.envCtx.precipitation[PRECIP_SNOW_MAX] = 128;
                break;
            case 2:
                play.envCtx.precipitation[PRECIP_SOS_MAX] = 32;
                break;
            case 3:
                play.envCtx.lightningState = LIGHTNING_ON;
                break;
            case 4:
                play.csCtx.state = 1;
                break;
            case 5:
                camera.stateFlags = CAM_STATE_UNDERWATER;
                break;
            case 6:
                play.skyboxId = SKYBOX_NONE;
                break;
            case 7:
                play.envCtx.changeSkyboxState = CHANGE_SKYBOX_ACTIVE;
                break;
            case 8:
                play.envCtx.lightSettingOverride = 1;
                break;
            case 9:
                play.envCtx.adjLightSettings.fogNear = -50;
                break;
        }
        auto before = play.envCtx;
        MMWeather_Update(&play);
        assert(MMWeather_RainDensity() == 0 && rainGain == 0 && MMWeather_Overcast() == 0);
        assert(std::memcmp(&before, &play.envCtx, sizeof(before)) == 0);
    }
    play.envCtx = native;
    for (int i = 0; i < 10; ++i) {
        MMWeather_Update(&play);
    }
    int density = MMWeather_RainDensity();
    play.pauseCtx.state = PAUSE_STATE_MAIN;
    for (int i = 0; i < 1000; ++i) {
        MMWeather_Update(&play);
    }
    assert(MMWeather_RainDensity() == density);
    MMWeather_Reset();
    assert(MMWeather_RainDensity() == 0 && rainGain == 0 && resets > 0);
    // Same PlayState address and same scene after reset must start a new fade.
    play.pauseCtx.state = PAUSE_STATE_OFF;
    MMWeather_Update(&play);
    assert(MMWeather_RainDensity() < 5);
    play.roomCtx.curRoom.num++;
    MMWeather_Update(&play);
    assert(MMWeather_RainDensity() < 5);
    for (int i = 0; i < 600; ++i) {
        MMWeather_Update(&play);
    }
    for (const auto& bolt : sLightningBolts) {
        assert(bolt.state == LIGHTNING_BOLT_INACTIVE); // Added strikes cannot occupy native slots.
    }
    assert(sMMWeatherLightningBolt.state != LIGHTNING_BOLT_INACTIVE);
    play.envCtx.precipitation[PRECIP_SNOW_MAX] = 128;
    MMWeather_Update(&play);
    assert(sMMWeatherLightningBolt.state == LIGHTNING_BOLT_INACTIVE);
    Environment_AddLightningBolts(&play, 1);
    MMWeather_Reset();
    assert(sLightningBolts[0].state == LIGHTNING_BOLT_START); // Native slots survive enhancement cleanup.
    play.envCtx = native;
    for (int i = 0; i < 600; ++i) {
        MMWeather_Update(&play);
    }
    assert(sMMWeatherLightningBolt.state != LIGHTNING_BOLT_INACTIVE);
    MMWeather_Reset();
    assert(sMMWeatherLightningBolt.state == LIGHTNING_BOLT_INACTIVE);
    std::puts("PASS MM weather bridge: native state preservation, preemption, sky, pause, scene/room reset");
}
