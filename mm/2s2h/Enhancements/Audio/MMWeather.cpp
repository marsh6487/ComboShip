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

bool NativeWeather(const PlayState* play) {
    const auto& env = play->envCtx;
    for (int i = 0; i < PRECIP_MAX; ++i) {
        if (env.precipitation[i] != 0) {
            return true;
        }
    }
    for (int i = 0; i < 3; ++i) {
        if (env.adjLightSettings.ambientColor[i] != 0 || env.adjLightSettings.light1Color[i] != 0 ||
            env.adjLightSettings.light2Color[i] != 0 || env.adjLightSettings.fogColor[i] != 0) {
            return true;
        }
    }
    return gWeatherMode != WEATHER_MODE_CLEAR || env.lightningState != LIGHTNING_OFF ||
           gLightningStrike.state != LIGHTNING_STRIKE_WAIT || env.stormRequest != STORM_REQUEST_NONE ||
           env.changeSkyboxState != CHANGE_SKYBOX_INACTIVE || env.changeLightEnabled ||
           env.lightSettingOverride != LIGHT_SETTING_OVERRIDE_NONE || env.adjLightSettings.fogNear != 0 ||
           env.adjLightSettings.zFar != 0 || env.sandstormState != SANDSTORM_OFF || env.customSkyboxFilter;
}
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
    const bool eligible = gSaveContext.gameMode == GAMEMODE_NORMAL && play->gameOverCtx.state == GAMEOVER_INACTIVE &&
                          play->csCtx.state == CS_STATE_IDLE && play->skyboxId == SKYBOX_NORMAL_SKY &&
                          !play->envCtx.skyboxDisabled && play->envCtx.lightMode == LIGHT_MODE_TIME &&
                          Environment_GetStormState(play) != STORM_STATE_OFF && camera != nullptr &&
                          !(camera->stateFlags & CAM_STATE_UNDERWATER);
    const bool nativeWeather = NativeWeather(play);
    const int ticks = play->pauseCtx.state == PAUSE_STATE_OFF ? std::clamp<int>(R_UPDATE_RATE, 1, 3) : 0;
    const bool strike = sState.Step(sSettings, eligible, nativeWeather, ticks);
    if (!eligible || nativeWeather) {
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
