#include "global.h"
#include "2s2h/Enhancements/Graphics/MMSummerAtmosphere.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cmath>
#include "summer_native_camera.inc"

SaveContext gSaveContext;
RegEditor editor;
RegEditor* gRegEditor = &editor;
static int season = SEASON_SUMMER, enabled = 1, shafts = 0, rain = 0;
static int matrices = 0, stack = 0, children = 0, scopes = 0, vertexLoads = 0;
static Gfx xlu[1024], opa[512];
static Mtx matrix;
static std::vector<const uint8_t*> textures;
static std::vector<Vec3f> translations;

extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    if (std::strcmp(key, MM_SUMMER_CVAR("Enabled")) == 0) return enabled;
    if (std::strcmp(key, MM_SUMMER_CVAR("Sunbeams")) == 0) return shafts;
    return fallback;
}
extern "C" int MMWeather_SeasonForPlay(const PlayState*) { return season; }
extern "C" int MMWeather_RainDensity() { return rain; }
extern "C" int MMWeather_SeasonClearsRain() { return gSaveContext.save.day == 2 && season == SEASON_SUMMER; }
extern "C" float MMWeather_Overcast() { return 0; }
extern "C" float OTRGetAspectRatio() { return 16.0f / 9.0f; }
extern "C" void FrameInterpolation_RecordOpenChild(const void*, int) { ++children; }
extern "C" void FrameInterpolation_RecordCloseChild() { assert(children > 0); --children; }
extern "C" void Graph_OpenDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) { ++scopes; }
extern "C" void Graph_CloseDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) { assert(scopes > 0); --scopes; }
extern "C" void Gfx_SetupDL25_Xlu(GraphicsContext*) {}
extern "C" void Matrix_Push() { ++stack; }
extern "C" void Matrix_Pop() { assert(stack > 0); --stack; }
extern "C" void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode) { translations.push_back({x,y,z}); }
extern "C" void Matrix_Mult(MtxF*, MatrixMode) {}
extern "C" void Matrix_Scale(f32 x, f32 y, f32, MatrixMode) { assert(x > 0 && y > 0); }
extern "C" Mtx* Matrix_Finalize(GraphicsContext*) { ++matrices; return &matrix; }
extern "C" void gSPDisplayList(Gfx* pkt, Gfx* list) { __gSPDisplayList(pkt, list); }
extern "C" void gSPVertex(Gfx* pkt, uintptr_t address, int count, int start) {
    assert(count == 12 && start == 0);
    ++vertexLoads;
    __gSPVertex(pkt, address, count, start);
}

static size_t Draw(PlayState& play, bool expectTexture = true) {
    auto& gfx = *play.state.gfxCtx;
    gfx.polyXlu.p = xlu;
    gfx.polyOpa.p = opa;
    gfx.polyOpa.d = opa + 512;
    matrices = vertexLoads = 0;
    textures.clear();
    translations.clear();
    MMSummerAtmosphere_Draw(&play);
    const size_t commands = gfx.polyXlu.p - xlu;
    assert(commands < 400); // Includes texture setup, 40 matrices, and optional shafts.
    assert(stack == 0 && children == 0 && scopes == 0);
    bool privateIA = false, depthTest = false;
    for (const Gfx* p = xlu; p < gfx.polyXlu.p; ++p) {
        const unsigned opcode = p->words.w0 >> 24;
        if (opcode == G_SETTIMG) {
            privateIA = ((p->words.w0 >> 21) & 7) == G_IM_FMT_IA;
            textures.push_back(reinterpret_cast<const uint8_t*>(p->words.w1));
        }
        if (opcode == G_SETOTHERMODE_L && (p->words.w1 & Z_CMP)) depthTest = true;
    }
    if (expectTexture && matrices) assert(privateIA && depthTest);
    return commands;
}

int main() {
    static PlayState play{};
    static Camera camera{};
    static GraphicsContext gfx{};
    play.state.gfxCtx = &gfx;
    play.cameraPtrs[0] = &camera;
    play.view.up = { 0, 1, 0 };
    camera.eye = { 0, 100, 0 };
    camera.at = { 0, 100, 1000 };
    camera.fov = 60;
    play.envCtx.sunPos = { 450, 800, -250 };
    gSaveContext.save.time = CLOCK_TIME(12, 0);
    R_UPDATE_RATE = 3;
    for (int i = 0; i < 30; ++i) MMSummerAtmosphere_Update(&play);
    Draw(play);
    assert(matrices == 32 && vertexLoads == 0);
    // A dandelion seed has a tuft and trailing stem, rather than the
    // vertically symmetric glow used by a firefly. Catch a shared-glow fallback.
    assert(textures.size() == 1);
    const auto* fluff = textures[0];
    bool asymmetric = false;
    for (int y = 0; y < 8; ++y) for (int x = 0; x < 16; ++x)
        asymmetric |= fluff[y * 16 + x] != fluff[(15 - y) * 16 + x];
    assert(asymmetric);
    // Actual guLookAtF screen coordinates must agree with the spawn grid:
    // a seed in the leftmost column must appear at screen left, not mirrored.
    float cameraMatrix[4][4];
    guLookAtF(cameraMatrix, 0, 100, 0, 0, 100, 1000, 0, 1, 0);
    auto screenX = [&](Vec3f p) {
        const float x = p.x * cameraMatrix[0][0] + p.y * cameraMatrix[1][0] +
                        p.z * cameraMatrix[2][0] + cameraMatrix[3][0];
        const float z = p.x * cameraMatrix[0][2] + p.y * cameraMatrix[1][2] +
                        p.z * cameraMatrix[2][2] + cameraMatrix[3][2];
        return x / (-z * std::tan(camera.fov * 0.00872664626f) * OTRGetAspectRatio());
    };
    assert(screenX(translations[0]) < -0.60f && screenX(translations[7]) > 0.60f);
    shafts = 1;
    MMSummerAtmosphere_Update(&play);
    Draw(play);
    assert(matrices == 33 && vertexLoads == 3);
    assert(gfx.polyOpa.d == opa + 512 - 3 * sizeof(Vtx) * 12 / sizeof(Gfx));
    gSaveContext.save.time = CLOCK_TIME(23, 0);
    MMSummerAtmosphere_Update(&play);
    Draw(play);
    assert(matrices == 40 && vertexLoads == 0);
    assert(textures.size() == 1 && textures[0] != fluff);
    const auto* glow = textures[0];
    for (int y = 0; y < 8; ++y) for (int x = 0; x < 16; ++x)
        assert(glow[y * 16 + x] == glow[(15 - y) * 16 + x]);
    gSaveContext.save.time = CLOCK_TIME(18, 0);
    MMSummerAtmosphere_Update(&play);
    Draw(play);
    assert(textures.size() == 2 && textures[0] == fluff && textures[1] == glow);
    gSaveContext.save.time = CLOCK_TIME(12, 0);
    rain = 30;
    MMSummerAtmosphere_Update(&play);
    Draw(play);
    assert(matrices == 0 && vertexLoads == 0);
    rain = 0;
    for (int value : { -1, SEASON_SPRING, SEASON_AUTUMN, SEASON_WINTER }) {
        season = value;
        MMSummerAtmosphere_Update(&play);
        assert(Draw(play, false) == 0);
    }
    season = SEASON_SUMMER;
    enabled = 0;
    MMSummerAtmosphere_Update(&play);
    assert(Draw(play, false) == 0);
    enabled = 1;
    shafts = 0;
    MMSummerAtmosphere_Reset();
    assert(Draw(play, false) == 0);
    MMSummerAtmosphere_Update(nullptr);
    MMSummerAtmosphere_Update(&play);
    Draw(play);
    assert(matrices == 0); // Fresh entries fade in rather than reusing stale sprites.
    std::puts("PASS summer renderer: private IA texture, depth testing, command budget, balanced scopes/matrices, "
              "shaft lifetime, night/rain/season gates, disable/reset/re-entry");
}
