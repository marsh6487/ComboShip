#pragma once

#include <stdint.h>

#define MM_WEATHER_CVAR(name) "gAudioEditor.MMWeather." name

#ifdef __cplusplus
extern "C" {
#endif
struct PlayState;
void MMWeather_Update(struct PlayState* play);
void MMWeather_Reset(void);
// Eligible owned season, or -1 for Off/unowned/interior. Does not change native weather flags.
int MMWeather_Season(void);
// Resolve current eligibility before the environment update (actors and HUD confirmations).
int MMWeather_SeasonForPlay(const struct PlayState* play);
// Summer clears the native second-day rain presentation while its source stays live.
int MMWeather_SeasonClearsRain(void);
// Observe native storm channel requests before composing Summer's ambience mute.
uint32_t MMWeather_ResolveAmbienceSeqCmd(uint32_t cmd);
int MMWeather_RainDensity(void);
float MMWeather_Overcast(void);
uint8_t MMWeather_Shade(uint8_t value);
void MMWeather_ApplySky(uint8_t* first, uint8_t* second, uint8_t* blend);
void MMWeather_RainColor(uint8_t* red, uint8_t* green, uint8_t* blue);
void MMWeather_DrawLightning(struct PlayState* play);
float MMWeather_RandomFloat(void);
// Environment-owned geometry for the enhancement, separate from native bolt slots.
void MMWeather_StartBolt(void);
void MMWeather_ClearBolts(void);
#ifdef __cplusplus
}
#endif
