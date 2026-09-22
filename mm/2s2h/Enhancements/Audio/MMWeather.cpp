#include "MMWeather.h"
#include "MMWeatherAudio.h"
#include "MMWeatherState.h"

#include <algorithm>
#include "global.h"
#include <libultraship/bridge/consolevariablebridge.h>

namespace {
MMWeather::State sState;
MMWeather::Settings sSettings;
PlayState* sPlay = nullptr;
int sScene = -1;
int sRoom = -1;
uint32_t sPresentationRandom = 0x4D4D5646;

} // namespace

extern "C" void MMWeather_Reset() {
    sState.Reset();
    sPlay = nullptr;
    sScene = sRoom = -1;
    sPresentationRandom = 0x4D4D5646;
    MMWeather_ClearBolts();
    MMWeatherAudio_Reset();
}

extern "C" void MMWeather_Update(PlayState* play) {
    if (play == nullptr) {
        MMWeather_Reset();
        return;
    }
    if (sPlay != play || sScene != play->sceneId || sRoom != play->roomCtx.curRoom.num) {
        MMWeather_Reset();
        sPlay = play;
        sScene = play->sceneId;
        sRoom = play->roomCtx.curRoom.num;
    }
    sSettings.enabled = CVarGetInteger(MM_WEATHER_CVAR("Enabled"), 0) != 0;
    sSettings.intermittent = CVarGetInteger(MM_WEATHER_CVAR("Mode"), 0) == 1;
    sSettings.overcast = CVarGetInteger(MM_WEATHER_CVAR("Overcast"), 1) != 0;
    sSettings.thunder = CVarGetInteger(MM_WEATHER_CVAR("Thunder"), 1) != 0;
    sSettings.thunderFrequency = CVarGetInteger(MM_WEATHER_CVAR("ThunderFrequency"), 100);
    const Camera* camera = GET_ACTIVE_CAM(play);
    // Outdoor rain is an explicit presentation override. Room storm policy,
    // story lighting and native weather cannot stop it. In particular, both
    // held and quick spins adjust fog throughout the effect and its release.
    const bool outdoorSky = play->skyboxId == SKYBOX_NORMAL_SKY || play->skyboxId == SKYBOX_3;
    const bool eligible = gSaveContext.gameMode == GAMEMODE_NORMAL && play->gameOverCtx.state == GAMEOVER_INACTIVE &&
                          outdoorSky && !play->envCtx.skyboxDisabled && camera != nullptr &&
                          !(camera->stateFlags & CAM_STATE_UNDERWATER);
    const int ticks = play->pauseCtx.state == PAUSE_STATE_OFF ? std::clamp<int>(R_UPDATE_RATE, 1, 3) : 0;
    const bool strike = sState.Step(sSettings, eligible, ticks);
    if (!eligible) {
        MMWeather_ClearBolts();
        MMWeatherAudio_Reset();
        return;
    }
    MMWeatherAudio_SetRain(sState.Intensity() * std::clamp(CVarGetInteger(MM_WEATHER_CVAR("RainVolume"), 100), 0, 100) /
                           100.0f);
    if (strike) {
        MMWeather_StartBolt();
        MMWeatherAudio_Thunder(1.0f);
    }
}

extern "C" int MMWeather_RainDensity() {
    return sState.Density();
}

extern "C" float MMWeather_Overcast() {
    return sState.Overcast(sSettings);
}

extern "C" uint8_t MMWeather_Shade(uint8_t value) {
    return static_cast<uint8_t>(value * (1.0f - 0.35f * MMWeather_Overcast()));
}

extern "C" void MMWeather_ApplySky(uint8_t* first, uint8_t* second, uint8_t* blend) {
    const float amount = MMWeather_Overcast();
    if (amount > 0.0f) {
        // MM's texture 1 is its cloudy sky. Only compose the rendered texture blend;
        // retain the native day-specific configuration and color palette.
        if (*first == 0 && *second == 0) {
            *second = 1;
            *blend = static_cast<uint8_t>(255.0f * amount);
        } else if (*first == 0 && *second == 1) {
            *blend = static_cast<uint8_t>(*blend + (255 - *blend) * amount);
        } else if (*first == 1 && *second == 0) {
            *blend = static_cast<uint8_t>(*blend * (1.0f - amount));
        }
    }
}

extern "C" void MMWeather_RainColor(uint8_t* red, uint8_t* green, uint8_t* blue) {
    if (MMWeather_RainDensity() > 0) {
        const Color_RGBA8 fallback = { 150, 255, 255, 255 };
        const auto color = CVarGetColor(MM_WEATHER_CVAR("RainColor"), fallback);
        *red = color.r;
        *green = color.g;
        *blue = color.b;
    }
}

extern "C" void MMWeather_DrawLightning(PlayState* play) {
    if (sState.FlashAlpha() > 0) {
        Environment_DrawLightningFlash(play, 200, 200, 255, sState.FlashAlpha());
    }
}

extern "C" float MMWeather_RandomFloat() {
    sPresentationRandom ^= sPresentationRandom << 13;
    sPresentationRandom ^= sPresentationRandom >> 17;
    sPresentationRandom ^= sPresentationRandom << 5;
    return (sPresentationRandom >> 8) / 16777216.0f;
}
