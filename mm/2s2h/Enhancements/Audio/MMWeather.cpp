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
int sSeason = -1;
bool sNativeStormAmbienceMuted = false;
bool sWritingSeasonAmbience = false;
uint8_t sNativeRainAmbience = 0;
uint8_t sNativeThunderAmbience = 0;
uint32_t sPresentationRandom = 0x4D4D5646;

bool OutdoorEligible(const PlayState* play) {
    if (play == nullptr) {
        return false;
    }
    const Camera* camera = GET_ACTIVE_CAM(play);
    const bool outdoorSky =
        play->sceneId != SCENE_SONCHONOIE && (play->skyboxId == SKYBOX_NORMAL_SKY || play->skyboxId == SKYBOX_3);
    return gSaveContext.gameMode == GAMEMODE_NORMAL && play->gameOverCtx.state == GAMEOVER_INACTIVE && outdoorSky &&
           !play->envCtx.skyboxDisabled && camera != nullptr && !(camera->stateFlags & CAM_STATE_UNDERWATER);
}

int SeasonRainDensity() {
    return sSeason == SEASON_SPRING && (CURRENT_DAY == 1 || CURRENT_DAY == 3) ? 30 : 0;
}

void ApplyStormAmbience(uint8_t rain, uint8_t thunder) {
    sWritingSeasonAmbience = true;
    Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_RAIN, CHANNEL_IO_PORT_1, rain);
    Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_LIGHTNING, CHANNEL_IO_PORT_1, thunder);
    sWritingSeasonAmbience = false;
}

} // namespace

extern "C" void MMWeather_Reset() {
    if (sNativeStormAmbienceMuted) {
        ApplyStormAmbience(sNativeRainAmbience, sNativeThunderAmbience);
    }
    sNativeStormAmbienceMuted = false;
    sState.Reset();
    sPlay = nullptr;
    sScene = sRoom = -1;
    sSeason = -1;
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
    // Outdoor rain is an explicit presentation override. Room storm policy,
    // story lighting and native weather cannot stop it. In particular, both
    // held and quick spins adjust fog throughout the effect and its release.
    // The Mayor's Residence is an interior (scene_table.h), even when its
    // room exposes an outdoor skybox. Keep all of its rooms out of global rain.
    const bool eligible = OutdoorEligible(play);
    // The rod defers to story presentation; the user's explicit weather override
    // continues to follow its existing independent eligibility/settings above.
    sSeason = MMWeather_SeasonForPlay(play);
    if (MMWeather_SeasonClearsRain() || sNativeStormAmbienceMuted) {
        // Native channels stop before precipitation reaches zero. Preserve their
        // actual IO requests, including En_Test4's stop at a rain target of eight.
        sNativeStormAmbienceMuted = MMWeather_SeasonClearsRain();
        ApplyStormAmbience(sNativeStormAmbienceMuted ? 0 : sNativeRainAmbience,
                          sNativeStormAmbienceMuted ? 0 : sNativeThunderAmbience);
    }
    const int ticks = play->pauseCtx.state == PAUSE_STATE_OFF ? std::clamp<int>(R_UPDATE_RATE, 1, 3) : 0;
    const bool strike = sState.Step(sSettings, eligible, ticks);
    if (!eligible) {
        MMWeather_ClearBolts();
        MMWeatherAudio_Reset();
        return;
    }
    const float seasonRain = SeasonRainDensity() > 0 ? 1.0f : 0.0f;
    MMWeatherAudio_SetRain(std::max(sState.Intensity(), seasonRain) *
                           std::clamp(CVarGetInteger(MM_WEATHER_CVAR("RainVolume"), 100), 0, 100) / 100.0f);
    if (strike) {
        MMWeather_StartBolt();
        MMWeatherAudio_Thunder(1.0f);
    }
}

extern "C" int MMWeather_Season() {
    return sSeason;
}

extern "C" int MMWeather_SeasonForPlay(const PlayState* play) {
    if (!OutdoorEligible(play) || play->csCtx.state != CS_STATE_IDLE ||
        play->envCtx.lightSettingOverride != LIGHT_SETTING_OVERRIDE_NONE || play->envCtx.customSkyboxFilter) {
        return -1;
    }
    const auto& nei = gSaveContext.save.shipSaveInfo.nei;
    if (!nei.seasonsOwned || nei.season == SEASON_OFF) {
        return -1;
    }
    if (nei.season < SEASON_COUNT && (nei.seasonsOwned & (1 << nei.season))) {
        return nei.season;
    }
    for (int season = 0; season < SEASON_COUNT; ++season) {
        if (nei.seasonsOwned & (1 << season)) {
            return season;
        }
    }
    return -1;
}

extern "C" int MMWeather_SeasonClearsRain() {
    return sSeason == SEASON_SUMMER && CURRENT_DAY == 2;
}

extern "C" uint32_t MMWeather_ResolveAmbienceSeqCmd(uint32_t cmd) {
    if (((cmd >> 24) & 0xF) != SEQ_PLAYER_AMBIENCE) {
        return cmd;
    }
    const uint32_t op = cmd >> 28;
    if (op == SEQCMD_OP_PLAY_SEQUENCE || op == SEQCMD_OP_STOP_SEQUENCE) {
        sNativeRainAmbience = sNativeThunderAmbience = 0;
        return cmd;
    }
    if (op != SEQCMD_OP_SET_CHANNEL_IO || ((cmd >> 16) & 0xFF) != CHANNEL_IO_PORT_1 || sWritingSeasonAmbience) {
        return cmd;
    }
    const uint32_t channel = (cmd >> 8) & 0xF;
    if (channel == AMBIENCE_CHANNEL_RAIN) {
        sNativeRainAmbience = cmd & 0xFF;
    } else if (channel == AMBIENCE_CHANNEL_LIGHTNING) {
        sNativeThunderAmbience = cmd & 0xFF;
    } else {
        return cmd;
    }
    return sNativeStormAmbienceMuted ? cmd & ~0xFFu : cmd;
}

extern "C" int MMWeather_RainDensity() {
    return std::max(sState.Density(), SeasonRainDensity());
}

extern "C" float MMWeather_Overcast() {
    const float seasonal = SeasonRainDensity() > 0 ? 0.75f : sSeason == SEASON_WINTER ? 0.65f : 0.0f;
    return std::max(sState.Overcast(sSettings), seasonal);
}

extern "C" uint8_t MMWeather_Shade(uint8_t value) {
    return static_cast<uint8_t>(value * (1.0f - 0.35f * MMWeather_Overcast()));
}

extern "C" void MMWeather_ApplySky(uint8_t* first, uint8_t* second, uint8_t* blend) {
    if (MMWeather_SeasonClearsRain()) {
        *first = *second = *blend = 0;
    }
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
