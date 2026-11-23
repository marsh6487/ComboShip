#include "global.h"
#include "BenPort.h"
#include "2s2h/Enhancements/Audio/MMWeather.h"
#include <assert.h>
#include <stdio.h>

typedef struct {
    u32 x, y;
    s32 width, height, xStep, yStep;
} ScrollLayer;

static ScrollLayer layers[2];
static int layerCount;
static int winter;
static int colorStep;
s32 sMatAnimStep;
u32 sMatAnimFlags;
f32 sMatAnimAlphaRatio;

int MMWeather_IsWinterWater(const PlayState* play) {
    return winter;
}

int ResourceMgr_OTRSigCheck(char* data) {
    return 0;
}

AnimationHeaderCommon* ResourceMgr_LoadAnimByName(const char* path) {
    return NULL;
}

void* Lib_SegmentedToVirtual(void* ptr) {
    return ptr;
}

Gfx* Gfx_TexScrollEx(GraphicsContext* gfxCtx, u32 x, u32 y, s32 width, s32 height, s32 xStep, s32 yStep) {
    layers[0] = (ScrollLayer){ x, y, width, height, xStep, yStep };
    layerCount = 1;
    return NULL;
}

Gfx* Gfx_TwoTexScrollEx(GraphicsContext* gfxCtx, s32 tile1, u32 x1, u32 y1, s32 width1, s32 height1, s32 tile2, u32 x2,
                        u32 y2, s32 width2, s32 height2, s32 xStep1, s32 yStep1, s32 xStep2, s32 yStep2) {
    layers[0] = (ScrollLayer){ x1, y1, width1, height1, xStep1, yStep1 };
    layers[1] = (ScrollLayer){ x2, y2, width2, height2, xStep2, yStep2 };
    layerCount = 2;
    return NULL;
}

void AnimatedMat_DrawTexScroll(PlayState* play, s32 segment, void* params) {
    AnimatedMat_TexScroll(play, params);
}

void AnimatedMat_DrawTwoTexScroll(PlayState* play, s32 segment, void* params) {
    AnimatedMat_TwoLayerTexScroll(play, params);
}

void AnimatedMat_DrawColor(PlayState* play, s32 segment, void* params) {
    colorStep = sMatAnimStep;
}

void AnimatedMat_DrawColorLerp(PlayState* play, s32 segment, void* params) {
}
void AnimatedMat_DrawColorNonLinearInterp(PlayState* play, s32 segment, void* params) {
}
void AnimatedMat_DrawTexCycle(PlayState* play, s32 segment, void* params) {
}
void CosmeticEditor_ClearMatAnimCosmetics(void) {
}

#include "winter_water_scene.inc"

static void TestScroll(PlayState* play, int segment, int type, int water, int sceneOwned) {
    static AnimatedMatTexScrollParams params[] = { { 1, -2, 32, 64 }, { 3, -4, 16, 32 } };
    AnimatedMaterial mat = { -segment, type, params };
    play->sceneMaterialAnims = sceneOwned ? &mat : NULL;
    for (u32 frame = 37; frame < 40; frame++) {
        AnimatedMat_DrawMain(play, &mat, 1.0f, frame, 3);
        assert(layerCount == type + 1);
        for (int i = 0; i < layerCount; i++) {
            assert(layers[i].width == params[i].width && layers[i].height == params[i].height);
            assert(layers[i].xStep == (water ? 0 : params[i].xStep) &&
                   layers[i].yStep == (water ? 0 : -params[i].yStep));
            assert(layers[i].x == (u32)(frame * layers[i].xStep) && layers[i].y == (u32)(frame * layers[i].yStep));
        }
    }
}

int main(void) {
    static PlayState play;
    play.sceneId = SCENE_20SICHITAI;
    TestScroll(&play, 3, 1, 0, 1);
    winter = 1;
    TestScroll(&play, 3, 1, 1, 1); // Southern Swamp surface.
    TestScroll(&play, 3, 0, 1, 1); // Single-layer replacement on the same segment.
    TestScroll(&play, 3, 1, 0, 0); // Actor or item material uses the same segment number.
    TestScroll(&play, 2, 1, 0, 1); // Nearby animated scenery.
    play.sceneId = SCENE_SEA;
    TestScroll(&play, 3, 1, 1, 1); // Great Bay Temple pools.
    TestScroll(&play, 2, 1, 0, 1); // Great Bay Temple vertical flow.
    AnimatedMaterial color = { -3, 2, NULL };
    play.sceneMaterialAnims = &color;
    AnimatedMat_DrawMain(&play, &color, 1.0f, 41, 3);
    assert(colorStep == 41);
    play.sceneId = SCENE_MAX;
    TestScroll(&play, 3, 1, 0, 1);
    puts(
        "PASS MM Winter water offsets and interpolation, scene ownership, season switching, and independent materials");
}
