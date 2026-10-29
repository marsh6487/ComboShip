// Exercises real WWSkyEnv functions against controlled native environment state.
// Regressions caught: clock/AM-PM palette swaps, loss of final-bolt overcast,
// sampling weather twice per frame, narrowing the horizon's world coordinates.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include "soh/soh/Enhancements/Graphics/WWSkyEnv.h"
using s16 = int16_t;
struct Vec3s { s16 x, y, z; };
struct Vec3f { float x, y, z; };
struct Gfx { int right; };
struct GraphicsContext { Gfx* p; };
struct EnvironmentContext {
    Vec3s windDirection{};
    float windSpeed = 0;
    uint8_t unk_17 = 0, unk_18 = 0, unk_19 = 0, skyboxBlend = 0;
    uint8_t unk_EE[4]{};
    int lightningMode = 0;
    struct { uint8_t fogColor[3]{}; } lightSettings;
};
struct PlayState {
    struct { GraphicsContext* gfxCtx; uint32_t frames; } state{};
    EnvironmentContext envCtx;
    struct { Vec3f eye{}; } view;
    int16_t sceneNum = 0;
};
struct { uint16_t dayTime; } gSaveContext;
constexpr int LIGHTNING_MODE_ON = 1, LIGHTNING_MODE_LAST = 2;
constexpr int G_SC_NON_INTERLACE = 0;
int gScreenWidth = 640, gScreenHeight = 480;
std::map<std::string, double> cvars;
int CVarGetInteger(const char* key, int fallback) { return cvars.contains(key) ? cvars[key] : fallback; }
float CVarGetFloat(const char* key, float fallback) { return cvars.contains(key) ? cvars[key] : fallback; }
float Math_CosS(s16 angle) { return std::cos(float(angle) * (6.2831853071795864769f / 65536.0f)); }
#define CVAR_ENHANCEMENT(s) "gEnhancements." s
#define OPEN_DISPS(ctx) auto* testGfx = (ctx)
#define CLOSE_DISPS(ctx) ((void)(ctx))
#define POLY_OPA_DISP testGfx->p
#define gDPSetScissor(pkt, mode, x, y, xRight, bottom) ((pkt)->right = (xRight))
/* PRODUCTION_SKY_ENV */
static void Require(bool value, const char* reason) { if (!value) { std::fprintf(stderr, "FAIL: %s\n", reason); std::exit(1); } }
static void Near(float got, float want, const char* why) { Require(std::fabs(got - want) < 0.0001f, why); }
int main() {
    PlayState play;
    play.envCtx.windDirection = {3, 0, 4}; play.envCtx.windSpeed = 255;
    float x, z, power; WWSkyEnv_Wind(&play, &x, &z, &power);
    Near(x, .6f, "wind X normalization"); Near(z, .8f, "wind Z normalization"); Near(power, .9f, "wind power maximum");
    play.envCtx.windDirection = {}; play.envCtx.windSpeed = 0;
    WWSkyEnv_Wind(&play, &x, &z, &power);
    Near(x*x + z*z, 1, "calm wind must retain a unit breeze"); Near(power, .3f, "calm breeze minimum");
    Near(WWSkyEnv_HorizonYForEye(100000), 24592, "wide-world horizon must not narrow to s16");
    cvars[CVAR_ENHANCEMENT("Graphics.WWClouds.HorizonBandParallax")] = 1;
    Near(WWSkyEnv_HorizonYForEye(100000), -408, "pinned horizon");
    cvars.clear();
    WWSkyWeather clear{}; WWSkyColors colors{};
    gSaveContext.dayTime = 0x8000; WWSkyEnv_SampleColors(&play, &clear, &colors);
    Require(colors.sky[0] == 80 && colors.sky[1] == 120 && colors.sky[2] == 255, "noon palette follows sun");
    gSaveContext.dayTime = 0; WWSkyEnv_SampleColors(&play, &clear, &colors);
    Require(colors.sky[0] == 10 && colors.sky[1] == 50 && colors.sky[2] == 85, "midnight palette");
    WWSkyColors fileSelect{}; WWSkyEnv_NightColors(&fileSelect);
    Require(fileSelect.sky[0] == 10 && fileSelect.sky[2] == 85, "file-select uses fixed night");
    gSaveContext.dayTime = 0x8000; clear.cloudiness = 1; WWSkyEnv_SampleColors(&play, &clear, &colors);
    Require(colors.sky[0] == 105 && colors.sky[1] == 130 && colors.sky[2] == 119, "rain palette");
    play.sceneNum = 5; play.state.frames = 1; play.envCtx.unk_EE[1] = 30;
    play.envCtx.lightSettings.fogColor[0] = 73;
    auto wet = WWSkyEnv_Sample(&play); Near(wet.cloudiness, 1, "rain forces full clouds");
    Require(wet.fogColor[0] == 73, "native fog color forwarded");
    play.envCtx.unk_EE[1] = 0; play.state.frames++;
    auto clearing = WWSkyEnv_Sample(&play); Near(clearing.cloudiness, .988f, "slow storm clear-up");
    Near(WWSkyEnv_Sample(&play).cloudiness, .988f, "multiple sky layers advance weather only once");
    play.sceneNum = 6; play.state.frames++; play.envCtx.lightningMode = LIGHTNING_MODE_LAST;
    auto finalBolt = WWSkyEnv_Sample(&play); Near(finalBolt.cloudiness, 1, "pending final bolt stays overcast"); Near(finalBolt.storm, 1, "pending final bolt stays stormy");
    Gfx commands[2]{}; GraphicsContext gfx{commands}; play.state.gfxCtx = &gfx;
    cvars[CVAR_ENHANCEMENT("Graphics.WWSky.SplitDebug")] = 1;
    WWSkyEnv_SplitDebugBegin(&play); WWSkyEnv_SplitDebugEnd(&play);
    Require(gfx.p == commands + 2 && commands[0].right == 320 && commands[1].right == 640, "debug split restores full-world scissor");
    std::puts("cel sky environment: PASS");
}
