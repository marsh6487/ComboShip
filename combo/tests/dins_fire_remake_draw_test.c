/* Boundary fixtures supply the engine arena/resource manager; geometry, materials,
 * actor routing and the player gate are extracted from real production code. */
#include "global.h"
#include "soh/ResourceManagerHelpers.h"
#include "align_asset_macro.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <libultraship/bridge/resourcebridge.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef TEST_HOST_MM
#include "2s2h/BenGui/CosmeticEditor.h"
#include "mods/nei_oot_compat.h"
#endif
#define REQUIRE(c)                                                       \
    do {                                                                 \
        if (!(c)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
            exit(1);                                                     \
        }                                                                \
    } while (0)

static GraphicsContext gfx;
static PlayState play;
static Player player;
static Gfx commands[16384], reference[16384], opa[16384], overlay[16], scroll[1], nativeDL[1];
static Vtx nativeVertices[76];
static Mtx matrix;
static MtxF cpuMatrix, recordedModels[4];
static int modelCount, option, missing, customColors, alt;
static int resourceLoads[144], nativeVertexLoads, childDepth, childOpens;
static int privatePresent;
static unsigned char resources[2][144][16];
static size_t verticesDrawn, quadsDrawn, bytesAllocated;
static size_t peakVertices, peakBytes, peakCommands;
static float minimumRadius, maximumRadius, maximumY;
static unsigned maxAlpha;
static unsigned basePatches, featheredFlames;
static unsigned char savedVertices[262144];
static Gfx* walkedCommands[20000];
static const char* flame = "__OTR__custom/dins_fire_remake/flameTex";
static const char* spark = "__OTR__custom/dins_fire_remake/sparkTex";
static s32 sSw97SpellActive, sOotSw97SpellActive;
static void Player_Action_808507F4(Player* p, PlayState* s) {
}
static void Player_Action_OotMagicSpell(Player* p, PlayState* s) {
}
void* OotAssets_LoadGfx(const char* path);
void* OotAssets_LoadTexOrDList(const char* path);
#define THIS ((MagicFire*)thisx)
#include "dins_draw_production.inc"
#undef THIS
#define TEST_BURN_FRAME (CW_DINS_NATIVE_FADE_START_FRAME + 8)

int32_t CVarGetInteger(const char* key, int32_t fallback) {
    if (!strcmp(key, "gEnhancements.DinsFireRemake")) {
        REQUIRE(fallback == 0);
        return option;
    }
    REQUIRE(strstr(key, "Magic.") && strstr(key, ".Changed"));
    return customColors;
}
Color_RGB8 CVarGetColor24(const char* key, Color_RGB8 fallback) {
    REQUIRE(strstr(key, ".Value"));
    return strstr(key, "Primary") ? (Color_RGB8){ 17, 221, 149 } : (Color_RGB8){ 5, 43, 71 };
}
#ifdef TEST_HOST_MM
Color_RGBA8 CosmeticEditor_GetChangedColor(u8 r, u8 g, u8 b, u8 a, const char* id) {
    if (!customColors)
        return (Color_RGBA8){ r, g, b, a };
    return strstr(id, "Primary") ? (Color_RGBA8){ 17, 221, 149, a } : (Color_RGBA8){ 5, 43, 71, a };
}
#endif
bool ResourceMgr_IsAltAssetsEnabled(void) {
    return alt;
}
uint8_t ResourceMgr_FileAltExists(const char* path) {
    return privatePresent;
}
void* ResourceGetDataByName(const char* path) {
    int index = !strcmp(path, flame) ? 0 : !strcmp(path, spark) ? 1 : -1;
#ifdef CW_DINS_NATIVE_DRAW
    for (int row = 0; row < 3; ++row)
        if (!strcmp(path, sCwDinsRibbonTextures[row]))
            index = 2 + row;
    if (!strcmp(path, sCwDinsHaloTex))
        index = 5;
    if (!strcmp(path, sCwDinsWhiteHaloTex))
        index = 6;
    if (!strcmp(path, sCwDinsTrailTex))
        index = 7;
    for (int band = 0; band < 6; ++band)
        for (int tick = 0; tick < 11; ++tick)
            if (!strcmp(path, sCwDinsRibbonBurnTextures[band][tick]))
                index = 8 + band * 11 + tick;
    for (int row = 0; row < 3; ++row)
        if (!strcmp(path, sCwDinsRibbonPaletteTextures[row]))
            index = 74 + row;
    if (!strcmp(path, sCwDinsHaloPaletteTex))
        index = 77;
    for (int band = 0; band < 6; ++band)
        for (int tick = 0; tick < 11; ++tick)
            if (!strcmp(path, sCwDinsRibbonBurnPaletteTextures[band][tick]))
                index = 78 + band * 11 + tick;
#endif
    REQUIRE(index >= 0);
    ++resourceLoads[index];
    return missing == index ? NULL : resources[alt][index];
}
void* OotAssets_LoadGfx(const char* path) {
    return nativeDL;
}
void* OotAssets_LoadTexOrDList(const char* path) {
    return resources[0][0];
}
Vtx* ResourceMgr_LoadVtxByName(char* path) {
    ++nativeVertexLoads;
    return nativeVertices;
}
void FrameInterpolation_RecordOpenChild(const void* key, int id) {
    ++childDepth;
    ++childOpens;
    REQUIRE(childDepth < 4);
}
void FrameInterpolation_RecordCloseChild(void) {
    REQUIRE(childDepth > 0);
    --childDepth;
}
void Matrix_Get(MtxF* out) {
    *out = cpuMatrix;
}
Mtx* Matrix_MtxFToMtx(MtxF* in, Mtx* out) {
    REQUIRE(modelCount < 4);
    recordedModels[modelCount++] = *in;
    memcpy(out, in, sizeof(*in));
    return out;
}
#include "dins_graph_production.inc"
#include "dins_setup_production.inc"
#ifdef TEST_HOST_MM
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode mode) {
}
Mtx* Matrix_Finalize(GraphicsContext* context) {
    return &matrix;
}
#else
void Matrix_Scale(f32 x, f32 y, f32 z, u8 mode) {
}
void* Graph_Alloc(GraphicsContext* context, size_t size) {
    size = (size + 15) & ~(size_t)15;
    REQUIRE((unsigned char*)context->polyOpa.d - (unsigned char*)context->polyOpa.p >= size);
    context->polyOpa.d = (Gfx*)((unsigned char*)context->polyOpa.d - size);
    return context->polyOpa.d;
}
Mtx* Matrix_NewMtx(GraphicsContext* context, char* file, s32 line) {
    return &matrix;
}
#endif
Gfx* Gfx_TwoTexScrollEx(GraphicsContext* c, s32 a, u32 b, u32 d, s32 e, s32 f, s32 g, u32 h, u32 i, s32 j, s32 k, s32 l,
                        s32 m, s32 n, s32 o) {
    return scroll;
}
void gSPDisplayList(Gfx* p, Gfx* dl) {
    __gSPDisplayList(p, dl);
}
void gSPVertex(Gfx* p, uintptr_t v, int n, int v0) {
    __gSPVertex(p, v, n, v0);
}

static void reset(void) {
    memset(commands, 0, sizeof(commands));
    memset(opa, 0, sizeof(opa));
    gfx.polyXlu.p = commands;
    gfx.polyXlu.d = commands + ARRAY_COUNT(commands);
    gfx.polyOpa.p = opa;
    gfx.polyOpa.d = opa + ARRAY_COUNT(opa);
    gfx.overlay.p = overlay;
    gfx.overlay.d = overlay + ARRAY_COUNT(overlay);
    modelCount = childOpens = 0;
    verticesDrawn = quadsDrawn = 0;
    minimumRadius = 1e9f;
    maximumRadius = maximumY = 0;
    maxAlpha = 0;
    basePatches = featheredFlames = 0;
}
static size_t drawSelected(MagicFire* fire, int legacyHelper) {
    reset();
    MagicFire before = *fire;
    Player playerBefore = player;
    MtxF matrixBefore = cpuMatrix;
    int handled = 0;
    if (legacyHelper) {
        /* Explicit recovery-helper coverage. Actual actor dispatch is tested
         * separately and must not select this retired presentation at runtime. */
        uint32_t primary = customColors ? 0x11DD95 : 0xFFC800;
        uint32_t secondary = customColors ? 0x052B47 : 0xFF0000;
        handled = CwDinsFireRemake_DrawActor(&play, &fire->actor, fire->action, fire->actionTimer,
                                             fire->alphaMultiplier, fire->screenTintIntensity, primary, secondary);
    }
    if (!handled) {
#ifdef TEST_HOST_MM
        MagicFireDins_Draw(&fire->actor, &play);
#else
        MagicFire_Draw(&fire->actor, &play);
#endif
    }
    REQUIRE(!memcmp(&before, fire, sizeof(before)));
    REQUIRE(!memcmp(&playerBefore, &player, sizeof(player)));
    REQUIRE(!memcmp(&matrixBefore, &cpuMatrix, sizeof(cpuMatrix)));
    REQUIRE(childDepth == 0);
    bytesAllocated = (unsigned char*)(opa + ARRAY_COUNT(opa)) - (unsigned char*)gfx.polyOpa.d;
    return gfx.polyXlu.p - commands;
}
static size_t drawLegacyFixture(MagicFire* fire) {
    return drawSelected(fire, 1);
}
static size_t drawActor(MagicFire* fire) {
    return drawSelected(fire, 0);
}

/* Follow only this draw's private child list. Native setup/static companion DLs
 * remain boundary dependencies, while every emitted effect command is decoded. */
static size_t walkSubmitted(void) {
    size_t count = 0;
    for (Gfx* p = commands; p < gfx.polyXlu.p; ++p) {
        REQUIRE(count < ARRAY_COUNT(walkedCommands));
        walkedCommands[count++] = p;
        if ((unsigned)(p->words.w0 >> 24) != G_DL || p->words.w1 < (uintptr_t)gfx.polyOpa.d ||
            p->words.w1 >= (uintptr_t)(opa + ARRAY_COUNT(opa)))
            continue;
        Gfx* child = (Gfx*)p->words.w1;
        for (;;) {
            REQUIRE(child >= (Gfx*)gfx.polyOpa.d && child < opa + ARRAY_COUNT(opa));
            REQUIRE(count < ARRAY_COUNT(walkedCommands));
            walkedCommands[count++] = child;
            if ((unsigned)(child++->words.w0 >> 24) == G_ENDDL)
                break;
        }
    }
    return count;
}
static int countOp(unsigned op) {
    int count = 0;
    size_t total = walkSubmitted();
    for (size_t i = 0; i < total; ++i) {
        Gfx* p = walkedCommands[i];
        if ((unsigned)(p->words.w0 >> 24) == op)
            ++count;
    }
    return count;
}
static int colorSeen(unsigned op, unsigned rgb) {
    size_t total = walkSubmitted();
    for (size_t i = 0; i < total; ++i) {
        Gfx* p = walkedCommands[i];
        if ((unsigned)(p->words.w0 >> 24) == op && (unsigned)(p->words.w1 >> 8) == rgb)
            return 1;
    }
    return 0;
}

static uint32_t little32(const unsigned char* bytes) {
    return bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

/* Read actual encoded RGBA texels selected by the submitted GBI. The renderer's
 * resource boundary remains a stub, but color assertions use production image
 * bytes and the packed native combiner, not a second palette implementation. */
static void nativeTexel(const char* name, float u, float v, float rgba[4]) {
    char path[256];
#ifdef TEST_HOST_MM
    const char* host = "mm";
#else
    const char* host = "soh";
#endif
    REQUIRE(!strncmp(name, "__OTR__", 7));
    REQUIRE(snprintf(path, sizeof(path), "%s/assets/custom/%s", host, name + 7) < sizeof(path));
    FILE* file = fopen(path, "rb");
    REQUIRE(file != NULL);
    unsigned char header[92], pixel[4];
    REQUIRE(fread(header, 1, sizeof(header), file) == sizeof(header));
    REQUIRE(little32(header + 64) == 1 && little32(header + 76) == 3);
    uint32_t width = little32(header + 68), height = little32(header + 72);
    REQUIRE(width > 0 && width <= 768 && height > 0 && height <= 768);
    uint32_t x = (uint32_t)((width - 1) * u), y = (uint32_t)((height - 1) * v);
    REQUIRE(fseek(file, 92 + (y * width + x) * 4, SEEK_SET) == 0);
    REQUIRE(fread(pixel, 1, 4, file) == 4);
    REQUIRE(fclose(file) == 0);
    for (int channel = 0; channel < 4; ++channel)
        rgba[channel] = pixel[channel] / 255.0f;
}

static float colorMux(unsigned selector, unsigned mask, int channel, const float texel[4], uint32_t primitive,
                      uint32_t environment) {
    if (selector == (G_CCMUX_0 & mask))
        return 0;
    if (selector == G_CCMUX_TEXEL0)
        return texel[channel];
    if (selector == G_CCMUX_PRIMITIVE)
        return ((primitive >> (16 - 8 * channel)) & 255) / 255.0f;
    if (selector == G_CCMUX_ENVIRONMENT)
        return ((environment >> (16 - 8 * channel)) & 255) / 255.0f;
    REQUIRE(0);
    return 0;
}

static float alphaMux(unsigned selector, const float texel[4], float shadeAlpha) {
    if (selector == G_ACMUX_0)
        return 0;
    if (selector == G_ACMUX_TEXEL0)
        return texel[3];
    if (selector == G_ACMUX_SHADE)
        return shadeAlpha;
    REQUIRE(0);
    return 0;
}

static void evaluateNativeCombiner(const Gfx* combine, const float texel[4], uint32_t primitive, uint32_t environment,
                                   float shadeAlpha, float result[4]) {
    uint32_t w0 = combine->words.w0, w1 = combine->words.w1;
    REQUIRE((w0 >> 24) == G_SETCOMBINE);
    unsigned selectors[2][4] = { { (w0 >> 20) & 15, (w1 >> 28) & 15, (w0 >> 15) & 31, (w1 >> 15) & 7 },
                                 { (w0 >> 5) & 15, (w1 >> 24) & 15, w0 & 31, (w1 >> 6) & 7 } };
    unsigned alphas[2][4] = { { (w0 >> 12) & 7, (w1 >> 12) & 7, (w0 >> 9) & 7, (w1 >> 9) & 7 },
                              { (w1 >> 21) & 7, (w1 >> 3) & 7, (w1 >> 18) & 7, w1 & 7 } };
    for (int cycle = 0; cycle < 2; ++cycle) {
        for (int channel = 0; channel < 3; ++channel) {
            const unsigned* s = selectors[cycle];
            float value = (colorMux(s[0], 15, channel, texel, primitive, environment) -
                           colorMux(s[1], 15, channel, texel, primitive, environment)) *
                              colorMux(s[2], 31, channel, texel, primitive, environment) +
                          colorMux(s[3], 7, channel, texel, primitive, environment);
            if (cycle)
                REQUIRE(fabsf(result[channel] - value) < .00001f);
            result[channel] = value;
        }
        const unsigned* a = alphas[cycle];
        float alpha = (alphaMux(a[0], texel, shadeAlpha) - alphaMux(a[1], texel, shadeAlpha)) *
                          alphaMux(a[2], texel, shadeAlpha) +
                      alphaMux(a[3], texel, shadeAlpha);
        if (cycle)
            REQUIRE(fabsf(result[3] - alpha) < .00001f);
        result[3] = alpha;
    }
}

static void checkNativePaletteSemantics(void) {
    static const uint32_t palettes[][2] = { { 0xFFC800, 0xFF0000 }, { 0x0066FF, 0x001144 }, { 0x00FF00, 0x0000FF },
                                            { 0xFFC800, 0x0022FF }, { 0x00FFFF, 0xFF0000 }, { 0x34B0D2, 0x34B0D2 } };
    Actor actor = { 0 };
#ifdef TEST_HOST_MM
    actor.id = ACTOR_OOT_DINS_FIRE;
#else
    actor.id = ACTOR_MAGIC_FIRE;
#endif
    option = 1;
    missing = -1;
    const int frames[] = { 20, TEST_BURN_FRAME };
    for (int sample = 0; sample < ARRAY_COUNT(frames); ++sample) {
        int tick = frames[sample];
        size_t originalVertexBytes = 0;
        for (int palette = 0; palette < ARRAY_COUNT(palettes); ++palette) {
            uint32_t primary = palettes[palette][0], secondary = palettes[palette][1];
            reset();
            REQUIRE(CwDinsFireRemake_DrawNativeActor(&play, &actor, tick, primary, secondary));
            size_t vertexBytes = countOp(G_VTX) * 4 * sizeof(Vtx);
            const Vtx* vertices = (const Vtx*)((const Mtx*)gfx.polyOpa.d + 2);
            if (!palette) {
                originalVertexBytes = vertexBytes;
                memcpy(savedVertices, vertices, vertexBytes);
            } else {
                REQUIRE(vertexBytes == originalVertexBytes && !memcmp(savedVertices, vertices, vertexBytes));
            }
            const char* texture = NULL;
            const char* sampled[8] = { 0 };
            int sampleCount = 0;
            uint32_t primitive = 0, environment = 0;
            Gfx combine = { 0 };
            size_t total = walkSubmitted();
            for (size_t i = 0; i < total; ++i) {
                const Gfx* command = walkedCommands[i];
                unsigned op = command->words.w0 >> 24;
                if (op == G_SETCOMBINE)
                    combine = *command;
                if (op == G_SETPRIMCOLOR)
                    primitive = command->words.w1 >> 8;
                if (op == G_SETENVCOLOR)
                    environment = command->words.w1 >> 8;
                if (op == G_SETTIMG)
                    texture = (const char*)command->words.w1;
                if (op != G_VTX || !texture || (!strstr(texture, "/ribbon") && !strstr(texture, "/halo")))
                    continue;
                int seen = 0;
                for (int j = 0; j < sampleCount; ++j)
                    seen |= !strcmp(texture, sampled[j]);
                const Vtx* quad = (const Vtx*)command->words.w1;
                unsigned alpha = 0;
                for (int j = 0; j < 4; ++j)
                    if (quad[j].v.cn[3] > alpha)
                        alpha = quad[j].v.cn[3];
                if (seen || !alpha)
                    continue;
                REQUIRE(sampleCount < ARRAY_COUNT(sampled));
                sampled[sampleCount++] = texture;
                for (int pixel = 0; pixel < 4; ++pixel) {
                    float texel[4], result[4];
                    nativeTexel(texture, .15f + pixel * .20f, .35f + pixel * .10f, texel);
                    evaluateNativeCombiner(&combine, texel, primitive, environment, alpha / 255.0f, result);
                    float brightness =
                        floorf((texel[0] * 299 + texel[1] * 587 + texel[2] * 114) * 255 / 1000 + .5f) / 255;
                    for (int channel = 0; channel < 3; ++channel) {
                        float p = ((primary >> (16 - 8 * channel)) & 255) / 255.0f;
                        float s = ((secondary >> (16 - 8 * channel)) & 255) / 255.0f;
                        float expected = palette ? s + (p - s) * brightness : texel[channel];
                        if (fabsf(result[channel] - expected) >= .00001f)
                            fprintf(
                                stderr,
                                "palette pixel mismatch tick=%d colors=%d image=%s channel=%d actual=%f expected=%f\n",
                                tick, palette, texture, channel, result[channel], expected);
                        REQUIRE(fabsf(result[channel] - expected) < .00001f);
                    }
                    REQUIRE(fabsf(result[3] - texel[3] * alpha / 255.0f) < .00001f);
                    if (palette)
                        REQUIRE(texel[0] == texel[1] && texel[1] == texel[2]);
                }
                REQUIRE(!!strstr(texture, "PaletteTex") == !!palette);
            }
            REQUIRE(sampleCount == (tick < CW_DINS_BURN_FIRST_FRAME ? 4 : 7));
        }
    }
    puts("PASS native palette pixels: actual RGBA samples and packed GBI preserve default RGB, map both colors, retain "
         "alpha and geometry");
}

static void checkNativePaletteResources(void) {
    MagicFire fire = { 0 };
#ifdef TEST_HOST_MM
    fire.actor.id = ACTOR_OOT_DINS_FIRE;
#else
    fire.actor.id = ACTOR_MAGIC_FIRE;
#endif
    fire.actor.world.pos = (Vec3f){ 110, 220, 330 };
    fire.actor.scale.x = fire.actor.scale.y = fire.actor.scale.z = .364362478f;
    fire.action = 3;
    fire.alphaMultiplier = .5f;
    fire.nativeFrame = TEST_BURN_FRAME;
    customColors = 1;
    option = 1;
    missing = -1;
    fire.nativeActive = 0;
    size_t stockCount = drawActor(&fire);
    memcpy(reference, commands, sizeof(commands));
    fire.nativeActive = 1;
    for (missing = 74; missing <= 77; ++missing) {
        REQUIRE(!CwDins_Ready());
        REQUIRE(drawActor(&fire) == stockCount && !memcmp(reference, commands, sizeof(commands)));
        REQUIRE(bytesAllocated == 0 && countOp(G_VTX) == 0);
        reset();
        REQUIRE(CwDinsFireRemake_DrawNativeActor(&play, &fire.actor, TEST_BURN_FRAME, 0xFFC800, 0xFF0000));
    }
    for (missing = 2; missing <= 5; ++missing) {
        REQUIRE(CwDins_Ready()); /* Unselected colored companions are not required. */
        REQUIRE(drawActor(&fire) == 9 && countOp(G_VTX) > 580);
    }
    missing = 78 + TEST_BURN_FRAME - CW_DINS_BURN_FIRST_FRAME;
    reset();
    int optionalBefore = 0;
    for (int i = 78; i < ARRAY_COUNT(resourceLoads); ++i)
        optionalBefore += resourceLoads[i];
    REQUIRE(CwDinsFireRemake_DrawNativeActor(&play, &fire.actor, TEST_BURN_FRAME, 0x11DD95, 0x052B47));
    int optionalAfter = 0;
    for (int i = 78; i < ARRAY_COUNT(resourceLoads); ++i)
        optionalAfter += resourceLoads[i];
    REQUIRE(optionalAfter == optionalBefore + 6);
    const Vtx* vertices = (const Vtx*)((const Mtx*)gfx.polyOpa.d + 2);
    REQUIRE(vertices[4 * 4].v.tc[0] == (int16_t)(-TEST_BURN_FRAME / (float)CW_DINS_NATIVE_RATE * .27f * 64 * .5f * 32));
    int fallback = 0, keptBurn = 0;
    size_t total = walkSubmitted();
    for (size_t i = 0; i < total; ++i) {
        const Gfx* command = walkedCommands[i];
        if ((unsigned)(command->words.w0 >> 24) == G_SETTIMG) {
            const char* name = (const char*)command->words.w1;
            fallback |= !strcmp(name, sCwDinsRibbon0PaletteTex);
            keptBurn |= !strcmp(name, sCwDinsRibbonBurnPaletteTextures[1][TEST_BURN_FRAME - CW_DINS_BURN_FIRST_FRAME]);
            REQUIRE(strcmp(name, sCwDinsRibbonBurnPaletteTextures[0][TEST_BURN_FRAME - CW_DINS_BURN_FIRST_FRAME]));
        }
    }
    REQUIRE(fallback && keptBurn);
    missing = -1;
    customColors = 0;
    reset();
    REQUIRE(CwDinsFireRemake_DrawNativeCharge(&play, &player, .4f));
    size_t greenBytes = (unsigned char*)(opa + ARRAY_COUNT(opa)) - (unsigned char*)gfx.polyOpa.d;
    memcpy(savedVertices, gfx.polyOpa.d, greenBytes);
    customColors = 1;
    reset();
    REQUIRE(CwDinsFireRemake_DrawNativeCharge(&play, &player, .4f));
    REQUIRE(!memcmp(savedVertices, gfx.polyOpa.d, greenBytes));
    customColors = 0;
    puts("PASS native palette resources: selected core readiness, exact stock fallback, bounded neutral burnout "
         "fallback, unchanged green charge");
}

static size_t copySubmittedQuads(Vtx output[CW_DINS_MAX_QUADS][4]) {
    size_t count = 0;
    size_t total = walkSubmitted();
    for (size_t i = 0; i < total; ++i) {
        Gfx* p = walkedCommands[i];
        if ((unsigned)(p->words.w0 >> 24) == G_VTX) {
            REQUIRE(count < CW_DINS_MAX_QUADS && ((p->words.w0 >> 12) & 255) == 4);
            memcpy(output[count++], (Vtx*)p->words.w1, 4 * sizeof(Vtx));
        }
    }
    return count;
}

static int wrappedTexelDelta(int after, int before) {
    int delta = (after - before) % (64 * 32);
    return delta < 0 ? delta + 64 * 32 : delta;
}

static void checkChargeMapping(void) {
    Vtx quads[CW_DINS_MAX_QUADS][4];
    size_t count = copySubmittedQuads(quads);
    REQUIRE(count > 0);
    for (size_t i = 0; i < count; ++i) {
        REQUIRE(quads[i][0].v.tc[0] == 0 && quads[i][3].v.tc[0] == 0);
        REQUIRE(quads[i][1].v.tc[0] == 2016 && quads[i][2].v.tc[0] == 2016);
        REQUIRE(quads[i][0].v.tc[1] - quads[i][3].v.tc[1] == 2016);
        REQUIRE(quads[i][1].v.tc[1] - quads[i][2].v.tc[1] == 2016);
    }
}

static void checkHorizontalShell(MagicFire* fire) {
    Vtx before[CW_DINS_MAX_QUADS][4], after[CW_DINS_MAX_QUADS][4];
    uint32_t savedFrame = play.gameplayFrames;
    uint32_t framePairs[] = { 42, 31, 4095, UINT32_MAX };
    for (size_t pair = 0; pair < ARRAY_COUNT(framePairs); ++pair) {
        play.gameplayFrames = framePairs[pair];
        drawLegacyFixture(fire);
        REQUIRE(copySubmittedQuads(before) == 52);
        ++play.gameplayFrames;
        drawLegacyFixture(fire);
        REQUIRE(copySubmittedQuads(after) == 52);
        for (int band = 0; band < 3; ++band) {
            for (int sector = 0; sector < 12; ++sector) {
                int index = band * 12 + sector, next = band * 12 + (sector + 1) % 12;
                REQUIRE(after[index][1].v.tc[0] - after[index][0].v.tc[0] == 512);
                REQUIRE(after[index][2].v.tc[0] - after[index][3].v.tc[0] == 512);
                REQUIRE(after[index][0].v.tc[1] - after[index][3].v.tc[1] == 1024);
                REQUIRE(after[index][1].v.tc[1] - after[index][2].v.tc[1] == 1024);
                for (int vertex = 0; vertex < 4; ++vertex) {
                    int ds = wrappedTexelDelta(after[index][vertex].v.tc[0], before[index][vertex].v.tc[0]);
                    int dt = wrappedTexelDelta(after[index][vertex].v.tc[1], before[index][vertex].v.tc[1]);
                    REQUIRE(ds == 64 &&
                            (dt == 2 || dt == 3)); // Two texels sideways; only slow secondary vertical drift.
                }
                for (int edge = 0; edge < 2; ++edge) {
                    Vtx* a = &after[index][edge ? 2 : 1];
                    Vtx* b = &after[next][edge ? 3 : 0];
                    REQUIRE(!memcmp(a->v.ob, b->v.ob, sizeof(a->v.ob)));
                    REQUIRE(wrappedTexelDelta(a->v.tc[0], b->v.tc[0]) == 0);
                    REQUIRE(wrappedTexelDelta(a->v.tc[1], b->v.tc[1]) == 0);
                    REQUIRE(a->v.cn[3] == b->v.cn[3]);
                }
                if (band < 2) {
                    for (int edge = 0; edge < 2; ++edge) {
                        Vtx* a = &after[index][edge ? 2 : 3];
                        Vtx* b = &after[index + 12][edge];
                        REQUIRE(!memcmp(a->v.ob, b->v.ob, sizeof(a->v.ob)));
                        REQUIRE(a->v.tc[0] == b->v.tc[0] && a->v.tc[1] == b->v.tc[1]);
                        REQUIRE(a->v.cn[3] == b->v.cn[3]);
                    }
                }
            }
        }
    }
    play.gameplayFrames = savedFrame;
}
static void inspectRemake(int expectFlame, int expectSpark) {
    int textures[2] = { 0 };
    Gfx combine;
    gDPSetCombineLERP(&combine, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, SHADE, 0, PRIMITIVE,
                      ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, SHADE, 0);
    int material = 0;
    int currentTexture = -1;
    size_t total = walkSubmitted();
    for (size_t i = 0; i < total; ++i) {
        Gfx* p = walkedCommands[i];
        unsigned op = p->words.w0 >> 24;
        if (op == G_SETCOMBINE) {
            REQUIRE(p->words.w0 == combine.words.w0 && p->words.w1 == combine.words.w1);
            ++material;
        }
        if (op == G_SETTIMG) {
            const char* path = (const char*)p->words.w1;
            REQUIRE(path &&
                    !strncmp(path, "__OTR__custom/dins_fire_remake/", strlen("__OTR__custom/dins_fire_remake/")));
            REQUIRE(!(p->words.w1 & 1));
            int index = !strcmp(path, flame) ? 0 : !strcmp(path, spark) ? 1 : -1;
            REQUIRE(index >= 0);
            ++textures[index];
            currentTexture = index;
        } else if (op == G_VTX) {
            int count = (p->words.w0 >> 12) & 255;
            Vtx* v = (Vtx*)p->words.w1;
            REQUIRE(count == 4);
            REQUIRE((unsigned char*)v >= (unsigned char*)gfx.polyOpa.d);
            REQUIRE((unsigned char*)(v + count) <= (unsigned char*)(opa + ARRAY_COUNT(opa)));
            verticesDrawn += count;
            ++quadsDrawn;
            float bx = v[0].v.ob[0] / 16.0f, bz = v[0].v.ob[2] / 16.0f;
            float dx = (v[1].v.ob[0] - v[0].v.ob[0]) / 16.0f, dz = (v[1].v.ob[2] - v[0].v.ob[2]) / 16.0f;
            if (currentTexture == 0 && v[0].v.ob[1] == 16 &&
                sqrtf(dx * dx + dz * dz) > sqrtf(bx * bx + bz * bz) * 0.25f)
                ++basePatches;
            int transparent = 0;
            for (int i = 0; i < 4; ++i)
                transparent += v[i].v.cn[3] == 0;
            if (currentTexture == 0 && transparent >= 3)
                ++featheredFlames;
            for (int i = 0; i < count; ++i) {
                float x = v[i].v.ob[0] / 16.0f, y = v[i].v.ob[1] / 16.0f, z = v[i].v.ob[2] / 16.0f;
                REQUIRE(isfinite(x) && isfinite(y) && isfinite(z));
                // Real submitted coordinates stay well inside signed 16-bit
                // range, including all deliberately unwrapped shell seams.
                REQUIRE(v[i].v.tc[0] >= 0 && v[i].v.tc[0] < 8192);
                REQUIRE(v[i].v.tc[1] >= 0 && v[i].v.tc[1] < 5120);
                float radius = sqrtf(x * x + z * z);
                if (radius < minimumRadius)
                    minimumRadius = radius;
                if (radius > maximumRadius)
                    maximumRadius = radius;
                if (y > maximumY)
                    maximumY = y;
                if (v[i].v.cn[3] > maxAlpha)
                    maxAlpha = v[i].v.cn[3];
            }
        }
    }
    REQUIRE(textures[0] == expectFlame && textures[1] == expectSpark);
    REQUIRE(material == 1); // Both material channels must use authored intensity and private vertex alpha.
    REQUIRE(verticesDrawn > 0 && verticesDrawn <= CW_DINS_MAX_QUADS * 4);
    REQUIRE(bytesAllocated > 0 && bytesAllocated <= 16384);
    REQUIRE(gfx.polyXlu.p - commands < 600);
    REQUIRE(modelCount == 2);
    REQUIRE(recordedModels[0].xx == 0.0625f && recordedModels[0].yy == 0.0625f && recordedModels[0].zz == 0.0625f);
    REQUIRE(recordedModels[0].xw == 110 && recordedModels[0].yw == 220 && recordedModels[0].zw == 330);
    REQUIRE(!memcmp(&recordedModels[1], &cpuMatrix, sizeof(cpuMatrix)));
    REQUIRE(childOpens >= 1 && childOpens <= 4);
    if (verticesDrawn > peakVertices)
        peakVertices = verticesDrawn;
    if (bytesAllocated > peakBytes)
        peakBytes = bytesAllocated;
    if ((size_t)(gfx.polyXlu.p - commands) > peakCommands)
        peakCommands = gfx.polyXlu.p - commands;
}

static void checkNestedSetupArenaBoundary(void) {
    CwDinsMesh mesh = { 0 };
    CwDins_Billboard(&mesh, (CwDinsVec){ 0, 40, 0 }, CwDins_Right(&play), CwDins_Up(&play), 4, 4, 1, 0xC8FFE1, 0x29C876,
                     1);
    size_t allocated = (2 * sizeof(Mtx) + 4 * sizeof(Vtx) + 15) & ~(size_t)15;
    for (int active = 0; active < 2; ++active) {
        for (int arena = 0; arena < 2; ++arena) {
            for (int slots = 0; slots <= 3; ++slots) {
                reset();
                if (arena == 0) {
                    // The allocation sits against the real end of the array.
                    // A nested marker beyond these slots would corrupt its first matrix.
                    gfx.polyOpa.p = (Gfx*)((unsigned char*)gfx.polyOpa.d - allocated - slots * sizeof(Gfx));
                } else {
                    // One past the last slot is an ASan red zone, not spare fixture storage.
                    gfx.overlay.p = (Gfx*)gfx.overlay.d - slots;
                }
                Gfx* opaHead = gfx.polyOpa.p;
                Gfx* opaTail = gfx.polyOpa.d;
                Gfx* overlayHead = gfx.overlay.p;
                int handled = CwDins_Submit(&play, &player, (Vec3f){ 110, 220, 330 }, &mesh, active, 0.5f);
                if (slots < 2) {
                    // Outer draw and native setup each open a live marker.
                    REQUIRE(!handled);
                    REQUIRE(gfx.polyOpa.d == opaTail && gfx.polyXlu.p == commands);
                    REQUIRE(modelCount == 0 && childOpens == 0);
                } else {
                    if (!handled || modelCount != 2) {
                        fprintf(stderr, "boundary active=%d arena=%d slots=%d allocated=%zu handled=%d models=%d\n",
                                active, arena, slots, allocated, handled, modelCount);
                    }
                    REQUIRE(handled && modelCount == 2);
                    REQUIRE((unsigned char*)opaTail - (unsigned char*)gfx.polyOpa.d == allocated);
                    REQUIRE(!memcmp(gfx.polyOpa.d, &recordedModels[0], sizeof(Mtx)));
                    REQUIRE(!memcmp((Mtx*)gfx.polyOpa.d + 1, &recordedModels[1], sizeof(Mtx)));
                    REQUIRE(gfx.polyXlu.p > commands && gfx.polyXlu.p <= (Gfx*)gfx.polyXlu.d);
                }
                REQUIRE(gfx.polyOpa.p == opaHead && gfx.overlay.p == overlayHead && childDepth == 0);
            }
        }
    }
}

static void checkNativeDraw(void) {
    Actor actor = { 0 };
#ifdef TEST_HOST_MM
    actor.id = ACTOR_OOT_DINS_FIRE;
#endif
    actor.world.pos = (Vec3f){ 110, 220, 330 };
    actor.scale.x = .364362478f;
    option = 1;
    missing = -1;
    reset();
#ifndef CW_DINS_NATIVE_DRAW
    REQUIRE(CwDinsFireRemake_DrawActor(&play, &actor, 2, 0, 1, 0, 0xFFC800, 0xFF0000));
#else
    REQUIRE(CwDinsFireRemake_DrawNativeActor(&play, &actor, 20, 0xFFC800, 0xFF0000));
#endif
    REQUIRE(countOp(G_VTX) >= 590); /* Submitted accepted mesh, not only builder output. */
#ifdef CW_DINS_NATIVE_DRAW
    /* Inspect every real submitted native vertex/texture/combiner, including
     * camera-relative order across materials and normalized logical-size UVs. */
    static const float azimuths[] = { 0, 18, -55 };
    static const float elevations[] = { 11, 16, 29 };
    size_t nativePeakCommands = 0, nativePeakBytes = 0, nativePeakVertices = 0, nativePeakHead = 0;
    unsigned char resourceBefore[sizeof(resources)];
    memcpy(resourceBefore, resources, sizeof(resources));
    for (int view = 0; view < 3; ++view) {
        float az = azimuths[view] * .017453292519943295f, el = elevations[view] * .017453292519943295f;
        play.billboardMtxF.xx = cosf(az);
        play.billboardMtxF.yx = 0;
        play.billboardMtxF.zx = -sinf(az);
        play.billboardMtxF.xy = -sinf(az) * sinf(el);
        play.billboardMtxF.yy = cosf(el);
        play.billboardMtxF.zy = -cosf(az) * sinf(el);
        CwDinsVec forward = CwDins_Cross(CwDins_Right(&play), CwDins_Up(&play));
        for (int tick = 0; tick < CW_DINS_NATIVE_FIRE_TICKS; ++tick) {
            CwDinsMesh mesh;
            CwDinsFireRemake_BuildNativeFire(&mesh, tick, CwDins_Right(&play), CwDins_Up(&play), 0xFFC800, 0xFF0000);
            reset();
            REQUIRE(CwDinsFireRemake_DrawNativeActor(&play, &actor, tick, 0xFFC800, 0xFF0000));
            REQUIRE(countOp(G_VTX) == mesh.count && countOp(G_FILLRECT) == 0 && modelCount == 2);
            REQUIRE(recordedModels[0].xw == 110 && recordedModels[0].yw == 220 && recordedModels[0].zw == 330);
            REQUIRE(!memcmp(&recordedModels[1], &cpuMatrix, sizeof(cpuMatrix)));
            const Vtx* first = (const Vtx*)((const Mtx*)gfx.polyOpa.d + 2);
            unsigned char seen[CW_DINS_MAX_QUADS] = { 0 };
            float previousDepth = -1e9f;
            int currentMaterial = -1;
            Gfx combineIntensity, combineRgba;
            gDPSetCombineLERP(&combineIntensity, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, SHADE, 0,
                              PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, SHADE, 0);
            gDPSetCombineLERP(&combineRgba, TEXEL0, 0, PRIMITIVE, 0, TEXEL0, 0, SHADE, 0, TEXEL0, 0, PRIMITIVE, 0,
                              TEXEL0, 0, SHADE, 0);
            int combineKind = -1;
            size_t commandCount = walkSubmitted();
            for (size_t commandIndex = 0; commandIndex < commandCount; ++commandIndex) {
                Gfx* command = walkedCommands[commandIndex];
                unsigned op = command->words.w0 >> 24;
                if (op == G_SETCOMBINE) {
                    if (command->words.w0 == combineIntensity.words.w0 &&
                        command->words.w1 == combineIntensity.words.w1)
                        combineKind = 0;
                    else {
                        REQUIRE(command->words.w0 == combineRgba.words.w0 && command->words.w1 == combineRgba.words.w1);
                        combineKind = 1;
                    }
                }
                if (op == G_SETTIMG) {
                    const char* name = (const char*)command->words.w1;
                    REQUIRE(name && !(command->words.w1 & 1));
                    REQUIRE(!strncmp(name, "__OTR__custom/dins_fire_remake/", 31));
                    currentMaterial = strcmp(name, spark) ? 1 : 0;
                    REQUIRE(((command->words.w0 >> 21) & 7) == (currentMaterial ? G_IM_FMT_RGBA : G_IM_FMT_I));
                    /* I8 LOADBLOCK transfers 16-bit units, then sets the render
                     * tile back to I8; RGBA32 keeps its native transfer size. */
                    REQUIRE(((command->words.w0 >> 19) & 3) == (currentMaterial ? G_IM_SIZ_32b : G_IM_SIZ_16b));
                }
                if (op == G_SETTILE && ((command->words.w1 >> 24) & 7) == G_TX_RENDERTILE) {
                    REQUIRE(((command->words.w0 >> 21) & 7) == (currentMaterial ? G_IM_FMT_RGBA : G_IM_FMT_I));
                    REQUIRE(((command->words.w0 >> 19) & 3) == (currentMaterial ? G_IM_SIZ_32b : G_IM_SIZ_8b));
                }
                if (op == G_SETTILESIZE) {
                    REQUIRE(((command->words.w1 >> 12) & 0xFFF) == (currentMaterial ? 31 * 4 : 63 * 4));
                    REQUIRE((command->words.w1 & 0xFFF) == (currentMaterial ? 31 * 4 : 63 * 4));
                }
                if (op != G_VTX)
                    continue;
                const Vtx* vertices = (const Vtx*)command->words.w1;
                ptrdiff_t index = (vertices - first) / 4;
                REQUIRE(index >= 0 && index < (ptrdiff_t)mesh.count && !seen[index]);
                seen[index] = 1;
                CwDinsQuad* q = &mesh.quads[index];
                REQUIRE(currentMaterial == (q->spark >= 2) && combineKind == currentMaterial);
                CwDinsVec center = { 0 };
                float uvScale = q->spark >= 2 ? .5f : 1;
                for (int v = 0; v < 4; ++v) {
                    center = CwDins_Add(center, q->p[v]);
                    REQUIRE(vertices[v].v.ob[0] == (int16_t)(q->p[v].x * 16));
                    REQUIRE(vertices[v].v.ob[1] == (int16_t)(q->p[v].y * 16));
                    REQUIRE(vertices[v].v.ob[2] == (int16_t)(q->p[v].z * 16));
                    REQUIRE(vertices[v].v.cn[3] == q->alpha[v]);
                    REQUIRE(vertices[v].v.tc[0] ==
                            (int16_t)((q->s + ((v == 1 || v == 2) ? q->sSpan : 0)) * uvScale * 32));
                    REQUIRE(vertices[v].v.tc[1] == (int16_t)((q->t + (v < 2 ? q->tSpan : 0)) * uvScale * 32));
                }
                float depth = CwDins_Dot(center, forward) * .25f;
                REQUIRE(depth >= previousDepth - .0001f);
                previousDepth = depth;
            }
            for (size_t i = 0; i < mesh.count; ++i)
                REQUIRE(seen[i]);
            for (int band = 0; band < 6; ++band) {
                for (int k = 0; k < 32; ++k) {
                    for (int cell = 0; cell < 3; ++cell) {
                        const Vtx* q = &first[(4 + band * 96 + k * 3 + cell) * 4];
                        if (k < 31) {
                            const Vtx* next = q + 12;
                            REQUIRE(!memcmp(&q[1], &next[0], sizeof(Vtx)));
                            REQUIRE(!memcmp(&q[2], &next[3], sizeof(Vtx)));
                        }
                        if (cell < 2) {
                            const Vtx* next = q + 4;
                            REQUIRE(!memcmp(&q[3], &next[0], sizeof(Vtx)));
                            REQUIRE(!memcmp(&q[2], &next[1], sizeof(Vtx)));
                        }
                    }
                }
            }
            size_t bytes = (unsigned char*)(opa + ARRAY_COUNT(opa)) - (unsigned char*)gfx.polyOpa.d;
            size_t childCommands = commandCount - (gfx.polyXlu.p - commands);
            REQUIRE(bytes == ((2 * sizeof(Mtx) + mesh.count * 4 * sizeof(Vtx) + childCommands * sizeof(Gfx) + 15) &
                              ~(size_t)15));
            REQUIRE(bytes <= 196608 && gfx.polyXlu.p - commands == 9 && countOp(G_ENDDL) == 1);
            const Gfx* childStart = (const Gfx*)(first + mesh.count * 4);
            REQUIRE(childStart + childCommands == opa + ARRAY_COUNT(opa));
            REQUIRE((unsigned)(childStart[childCommands - 1].words.w0 >> 24) == G_ENDDL);
            nativePeakCommands = fmaxf(nativePeakCommands, commandCount);
            nativePeakHead = fmaxf(nativePeakHead, gfx.polyXlu.p - commands);
            nativePeakBytes = fmaxf(nativePeakBytes, bytes);
            nativePeakVertices = fmaxf(nativePeakVertices, mesh.count * 4);
            REQUIRE(childDepth == 0 && !memcmp(resourceBefore, resources, sizeof(resources)));
        }
    }
    /* Latched native casts continue after a setting change and do not depend on
     * game-frame rollover, player movement, or live resource pointers. */
    reset();
    REQUIRE(CwDinsFireRemake_DrawNativeActor(&play, &actor, 20, 0xFFC800, 0xFF0000));
    size_t unchangedBytes = (unsigned char*)(opa + ARRAY_COUNT(opa)) - (unsigned char*)gfx.polyOpa.d;
    memcpy(savedVertices, gfx.polyOpa.d, unchangedBytes);
    option = 0;
    alt ^= 1;
    play.gameplayFrames = UINT32_MAX;
    player.actor.world.pos = (Vec3f){ -900, 800, 700 };
    reset();
    REQUIRE(CwDinsFireRemake_DrawNativeActor(&play, &actor, 20, 0xFFC800, 0xFF0000));
    REQUIRE(!memcmp(savedVertices, gfx.polyOpa.d, unchangedBytes));
    REQUIRE(recordedModels[0].xw == actor.world.pos.x);
    /* All required resources reject before arena changes. A missing optional
     * burnout frame selects the moving base ribbon while retaining other bands. */
    for (missing = 1; missing <= 7; ++missing) {
        reset();
        REQUIRE(!CwDinsFireRemake_DrawNativeActor(&play, &actor, TEST_BURN_FRAME, 0xFFC800, 0xFF0000));
        REQUIRE(gfx.polyXlu.p == commands && gfx.polyOpa.d == opa + ARRAY_COUNT(opa) && !childOpens);
    }
    missing = 8 + (TEST_BURN_FRAME - CW_DINS_BURN_FIRST_FRAME);
    reset();
    REQUIRE(CwDinsFireRemake_DrawNativeActor(&play, &actor, TEST_BURN_FRAME, 0xFFC800, 0xFF0000));
    const Vtx* vertices = (const Vtx*)((const Mtx*)gfx.polyOpa.d + 2);
    REQUIRE(vertices[4 * 4].v.tc[0] == (int16_t)(-TEST_BURN_FRAME / (float)CW_DINS_NATIVE_RATE * .27f * 64 * .5f * 32));
    missing = -1;
    /* Core readiness is live, while optional fade loads are bounded to exactly
     * the six materials in this frame rather than the complete variant bank. */
    int resourceQueries = 0;
    for (int i = 0; i < ARRAY_COUNT(resourceLoads); ++i)
        resourceQueries += resourceLoads[i];
    REQUIRE(!CwDins_Ready());
    int afterQueries = 0;
    for (int i = 0; i < ARRAY_COUNT(resourceLoads); ++i)
        afterQueries += resourceLoads[i];
    REQUIRE(afterQueries == resourceQueries);
    option = 1;
    REQUIRE(CwDins_Ready());
    int optionalQueries = 0;
    for (int i = 8; i < ARRAY_COUNT(resourceLoads); ++i)
        optionalQueries += resourceLoads[i];
    reset();
    REQUIRE(CwDinsFireRemake_DrawNativeActor(&play, &actor, 20, 0xFFC800, 0xFF0000));
    int afterOptional = 0;
    for (int i = 8; i < ARRAY_COUNT(resourceLoads); ++i)
        afterOptional += resourceLoads[i];
    REQUIRE(afterOptional == optionalQueries);
    reset();
    REQUIRE(CwDinsFireRemake_DrawNativeActor(&play, &actor, TEST_BURN_FRAME, 0xFFC800, 0xFF0000));
    afterOptional = 0;
    for (int i = 8; i < ARRAY_COUNT(resourceLoads); ++i)
        afterOptional += resourceLoads[i];
    REQUIRE(afterOptional == optionalQueries + 6);
    for (int invalid = 0; invalid < 2; ++invalid) {
        reset();
        REQUIRE(CwDinsFireRemake_DrawNativeActor(&play, &actor, invalid ? CW_DINS_NATIVE_FIRE_TICKS : -1, 0xFFC800,
                                                 0xFF0000));
        REQUIRE(gfx.polyXlu.p == commands && gfx.polyOpa.d == opa + ARRAY_COUNT(opa));
    }
    for (int arena = 0; arena < 3; ++arena) {
        reset();
        if (arena == 0)
            gfx.polyOpa.d = gfx.polyOpa.p + 1;
        if (arena == 1)
            gfx.polyXlu.d = gfx.polyXlu.p + 1;
        if (arena == 2)
            gfx.overlay.d = gfx.overlay.p + 1;
        Gfx* tail = gfx.polyOpa.d;
        REQUIRE(!CwDinsFireRemake_DrawNativeActor(&play, &actor, 20, 0xFFC800, 0xFF0000));
        REQUIRE(gfx.polyXlu.p == commands && gfx.polyOpa.d == tail && !childOpens);
    }
    CwDinsMesh boundaryMesh = { 0 };
    CwDins_NativeBillboard(&boundaryMesh, (CwDinsVec){ 0, 40, 0 }, CwDins_Right(&play), CwDins_Up(&play), 4, 4, 1,
                           0xFFFFFF, 0, 1);
    reset();
    REQUIRE(CwDins_SubmitNative(&play, &actor, actor.world.pos, &boundaryMesh));
    size_t boundaryBytes = (unsigned char*)(opa + ARRAY_COUNT(opa)) - (unsigned char*)gfx.polyOpa.d;
    for (int arena = 0; arena < 2; ++arena) {
        for (int slots = 0; slots <= 3; ++slots) {
            reset();
            if (arena == 0)
                gfx.polyOpa.p = (Gfx*)((unsigned char*)gfx.polyOpa.d - boundaryBytes - slots * sizeof(Gfx));
            else
                gfx.overlay.p = (Gfx*)gfx.overlay.d - slots;
            Gfx* beforeOpa = gfx.polyOpa.d;
            int handled = CwDins_SubmitNative(&play, &actor, actor.world.pos, &boundaryMesh);
            REQUIRE(handled == (slots >= 2));
            if (handled) {
                REQUIRE(modelCount == 2 && !memcmp(gfx.polyOpa.d, &recordedModels[0], sizeof(Mtx)));
                REQUIRE(!memcmp((Mtx*)gfx.polyOpa.d + 1, &recordedModels[1], sizeof(Mtx)));
            } else {
                REQUIRE(gfx.polyOpa.d == beforeOpa && gfx.polyXlu.p == commands && !childOpens);
            }
        }
    }
    for (int exact = 0; exact < 2; ++exact) {
        reset();
        gfx.polyXlu.d = commands + CW_DINS_NATIVE_XLU_RESERVE - !exact;
        REQUIRE(CwDins_SubmitNative(&play, &actor, actor.world.pos, &boundaryMesh) == exact);
        REQUIRE(gfx.polyXlu.p <= (Gfx*)gfx.polyXlu.d);
        if (!exact)
            REQUIRE(gfx.polyXlu.p == commands && gfx.polyOpa.d == opa + ARRAY_COUNT(opa));
    }
#ifdef TEST_HOST_MM
    actor.id = ACTOR_SW97_MAGIC_FIRE;
    reset();
    REQUIRE(!CwDinsFireRemake_DrawNativeActor(&play, &actor, 20, 0xFFC800, 0xFF0000));
    REQUIRE(gfx.polyXlu.p == commands);
#endif
    customColors = 0;
    reset();
    REQUIRE(CwDinsFireRemake_DrawNativeCharge(&play, &player, .4f));
    REQUIRE(countOp(G_VTX) == 60);
    REQUIRE(colorSeen(G_SETPRIMCOLOR, 0xE7FFF2));
    reset();
    REQUIRE(CwDinsFireRemake_DrawNativeCharge(&play, &player, .75f));
    REQUIRE(countOp(G_VTX) == 5);
    REQUIRE(colorSeen(G_SETPRIMCOLOR, 0xFFD9D0));
    customColors = 1;
    reset();
    REQUIRE(CwDinsFireRemake_DrawNativeCharge(&play, &player, .75f));
    REQUIRE(!colorSeen(G_SETPRIMCOLOR, 0xFFD9D0));
    customColors = 0;
    printf("PASS native submitted GBI: %d ticks x 3 camera bases, RGBA/I8 combiners, sorted quads, "
           "shared vertices, live resources, held-cast mode, missing-resource fallback\n",
           CW_DINS_NATIVE_FIRE_TICKS);
    printf("Native peak: %zu private vertices, %zu OPA bytes (vertices/matrices/child DL), %zu executed commands (%zu "
           "bytes); two matrices\n",
           nativePeakVertices, nativePeakBytes, nativePeakCommands, nativePeakCommands * sizeof(Gfx));
    printf(
        "Native XLU head: %zu commands (%zu bytes), reserved %zu bytes; mesh scratch %zu bytes; no whole-screen tint\n",
        nativePeakHead, nativePeakHead * sizeof(Gfx), (size_t)CW_DINS_NATIVE_XLU_RESERVE * sizeof(Gfx),
        sizeof(CwDinsMesh));
#endif
}

static void checkNativeActorRouting(void) {
    MagicFire fire = { 0 };
#ifdef TEST_HOST_MM
    fire.actor.id = ACTOR_OOT_DINS_FIRE;
#else
    fire.actor.id = ACTOR_MAGIC_FIRE;
#endif
    fire.actor.world.pos = (Vec3f){ 110, 220, 330 };
    fire.actor.scale.x = fire.actor.scale.y = fire.actor.scale.z = .364362478f;
    fire.action = 2;
    fire.alphaMultiplier = 1;
    fire.nativeActive = 1;
    fire.nativeFrame = -1;
    option = 1;
    customColors = 0;
    missing = -1;
    int beforeNativeLoads = nativeVertexLoads;
    REQUIRE(drawActor(&fire) == 0 && bytesAllocated == 0);
    REQUIRE(nativeVertexLoads == beforeNativeLoads);

    fire.nativeFrame = 20;
    size_t actualCount = drawActor(&fire);
    REQUIRE(countOp(G_VTX) == 590);
    memcpy(reference, commands, sizeof(commands));
    size_t actualBytes = bytesAllocated;
    memcpy(savedVertices, gfx.polyOpa.d, actualBytes);
    reset();
    REQUIRE(CwDinsFireRemake_DrawNativeActor(&play, &fire.actor, 20, 0xFFC800, 0xFF0000));
    REQUIRE((size_t)(gfx.polyXlu.p - commands) == actualCount && !memcmp(reference, commands, sizeof(commands)));
    REQUIRE(!memcmp(savedVertices, gfx.polyOpa.d, actualBytes));

    option = 0;
    ++play.gameplayFrames;
    player.actor.world.pos.x += 500;
    REQUIRE(drawActor(&fire) == actualCount && !memcmp(reference, commands, sizeof(commands)));
    REQUIRE(!memcmp(savedVertices, gfx.polyOpa.d, actualBytes));
    REQUIRE(nativeVertexLoads == beforeNativeLoads);
    customColors = 1;
    REQUIRE(drawActor(&fire) == actualCount);
    REQUIRE(colorSeen(G_SETPRIMCOLOR, 0x11DD95));
    customColors = 0;

    /* Compare the complete submitted fallback commands with the actual native
     * sphere for the same phase, colors and frame. No legacy remake shell may
     * replace a latched native cast when a required new resource disappears. */
    fire.nativeActive = 0;
    size_t nativeCount = drawActor(&fire);
    memcpy(reference, commands, sizeof(commands));
    REQUIRE(bytesAllocated == 0 && nativeVertexLoads > beforeNativeLoads);
    fire.nativeActive = 1;
    option = 1;
    for (missing = 1; missing <= 7; ++missing) {
        REQUIRE(drawActor(&fire) == nativeCount && !memcmp(reference, commands, sizeof(commands)));
        REQUIRE(bytesAllocated == 0 && countOp(G_VTX) == 0);
    }
    missing = -1;
    fire.nativeFrame = CW_DINS_NATIVE_FIRE_TICKS;
    REQUIRE(drawActor(&fire) == 0 && bytesAllocated == 0);
    puts("PASS actual actor routing: pending invisible, released frame exact, latched setting/live cosmetics, "
         "same-phase original sphere fallback, empty expired phase");
}

static void checkInactiveCastStaysStock(void) {
    MagicFire fire = { 0 };
#ifdef TEST_HOST_MM
    fire.actor.id = ACTOR_OOT_DINS_FIRE;
#else
    fire.actor.id = ACTOR_MAGIC_FIRE;
#endif
    fire.actor.world.pos = (Vec3f){ 110, 220, 330 };
    fire.actor.scale.x = fire.actor.scale.y = fire.actor.scale.z = .364362478f;
    fire.action = 2;
    fire.alphaMultiplier = 1;
    fire.nativeActive = 0;
    customColors = 0;
    option = 0;
    missing = -1;
    size_t stockCount = drawActor(&fire);
    memcpy(reference, commands, sizeof(commands));
    REQUIRE(bytesAllocated == 0 && countOp(G_VTX) == 0);
    /* Cast was selected with the checkbox off. Enabling it later cannot change
     * the chosen presentation or resurrect the earlier POC2 shell. */
    option = 1;
    REQUIRE(drawActor(&fire) == stockCount && !memcmp(reference, commands, sizeof(commands)));
    REQUIRE(bytesAllocated == 0 && countOp(G_VTX) == 0);
    /* Setup also selects stock when a new core asset is missing, even if the
     * two protected old effect textures remain available. */
    for (missing = 2; missing <= 7; ++missing) {
        REQUIRE(drawActor(&fire) == stockCount && !memcmp(reference, commands, sizeof(commands)));
        REQUIRE(bytesAllocated == 0 && countOp(G_VTX) == 0);
    }
    missing = -1;
    puts("PASS inactive cast latch: checkbox off-to-on and missing new core assets retain exact stock sphere");
}

static void checkNativeRealCapacity(void) {
    const size_t opaBytes = sizeof(((GfxPool*)0)->polyOpaBuffer);
    const size_t xluBytes = sizeof(((GfxPool*)0)->polyXluBuffer);
    Gfx* realOpa = malloc(opaBytes);
    Gfx* realXlu = malloc(xluBytes);
    REQUIRE(realOpa && realXlu);
    Actor actor = { 0 };
#ifdef TEST_HOST_MM
    actor.id = ACTOR_OOT_DINS_FIRE;
#else
    actor.id = ACTOR_MAGIC_FIRE;
#endif
    option = 1;
    missing = -1;
    size_t palettePeak = 0;
    const float views[][2] = { { 0, 11 }, { 18, 16 }, { -55, 29 } };
    for (int view = 0; view < ARRAY_COUNT(views); ++view) {
        float az = views[view][0] * .017453292519943295f, el = views[view][1] * .017453292519943295f;
        play.billboardMtxF.xx = cosf(az);
        play.billboardMtxF.yx = 0;
        play.billboardMtxF.zx = -sinf(az);
        play.billboardMtxF.xy = -sinf(az) * sinf(el);
        play.billboardMtxF.yy = cosf(el);
        play.billboardMtxF.zy = -cosf(az) * sinf(el);
        for (int palette = 0; palette < 2; ++palette) {
            for (int occupied = 0; occupied < 2; ++occupied) {
                for (int frame = 0; frame < CW_DINS_NATIVE_FIRE_TICKS; ++frame) {
                    reset();
                    gfx.polyOpa.p = realOpa + occupied * opaBytes / (2 * sizeof(Gfx));
                    gfx.polyOpa.d = realOpa + opaBytes / sizeof(Gfx);
                    gfx.polyXlu.p = realXlu + occupied * xluBytes / (2 * sizeof(Gfx));
                    gfx.polyXlu.d = realXlu + xluBytes / sizeof(Gfx);
                    REQUIRE(CwDinsFireRemake_DrawNativeActor(&play, &actor, frame, palette ? 0x0066FF : 0xFFC800,
                                                             palette ? 0x001144 : 0xFF0000));
                    REQUIRE(gfx.polyOpa.p <= (Gfx*)gfx.polyOpa.d && gfx.polyXlu.p <= (Gfx*)gfx.polyXlu.d);
                    size_t bytes = (unsigned char*)(realOpa + opaBytes / sizeof(Gfx)) - (unsigned char*)gfx.polyOpa.d;
                    if (palette && bytes > palettePeak)
                        palettePeak = bytes;
                }
            }
        }
    }
    free(realOpa);
    free(realXlu);
    reset();
    printf("PASS real host arena capacities: OPA=%zu bytes, XLU=%zu bytes; default and custom palettes fit all %d "
           "ticks x 3 bases with half occupied; custom OPA peak=%zu\n",
           opaBytes, xluBytes, CW_DINS_NATIVE_FIRE_TICKS, palettePeak);
}

int main(void) {
    play.state.gfxCtx = &gfx;
#ifdef TEST_HOST_MM
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
#else
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
#endif
    play.gameplayFrames = 42;
    play.billboardMtxF.xx = play.billboardMtxF.yy = play.billboardMtxF.zz = play.billboardMtxF.ww = 1;
    checkNestedSetupArenaBoundary();
    player.actor.world.pos = (Vec3f){ 110, 220, 330 };
    for (int i = 0; i < ARRAY_COUNT(player.bodyPartsPos); ++i)
        player.bodyPartsPos[i] = (Vec3f){ 120, 265, 345 };
    cpuMatrix.xx = cpuMatrix.yy = cpuMatrix.zz = 0;
    cpuMatrix.ww = 1; // The native pre-cast actor has zero scale.
    MagicFire fire = { 0 };
    fire.actor.world.pos = player.actor.world.pos;
#ifdef TEST_HOST_MM
    fire.actor.id = ACTOR_OOT_DINS_FIRE;
#else
    fire.actor.id = ACTOR_MAGIC_FIRE;
#endif
    fire.actionTimer = 20;
    missing = -1;
    option = 1;
    REQUIRE(drawLegacyFixture(&fire) >
            0); // RED: native Din has no lead-in, despite option enabled and resources ready.
    inspectRemake(0, 1);
    checkChargeMapping();
    REQUIRE(maximumRadius > 5 && maximumY > 10);
    REQUIRE(recordedModels[0].xx != 0 || recordedModels[1].xx != 0);
    REQUIRE(colorSeen(G_SETPRIMCOLOR, 0xC8FFE1));
    int firstLoads = resourceLoads[0] + resourceLoads[1];
    fire.actionTimer = 2;
    alt = 1;
    drawLegacyFixture(&fire);
    inspectRemake(1, 1);
    checkChargeMapping();
    REQUIRE(featheredFlames >= 4); // Flame tiles must end in tapered transparent borders, not rectangular sheets.
    REQUIRE(resourceLoads[0] + resourceLoads[1] > firstLoads);
    REQUIRE(colorSeen(G_SETPRIMCOLOR, 0xFFC800));
    fire.action = 2;
    fire.actor.scale = (Vec3f){ 0.4f, 0.4f, 0.4f };
    fire.alphaMultiplier = 1;
    fire.screenTintIntensity = 0.5f;
    drawLegacyFixture(&fire);
    inspectRemake(1, 1);
    REQUIRE(minimumRadius > 30 && maximumRadius > 100 && maximumY > 80); // Rounded, open shell with substantial height.
    REQUIRE(basePatches == 12); // The connected lower band surrounds the caster.
    size_t holdQuads = quadsDrawn;
    memcpy(savedVertices, gfx.polyOpa.d, bytesAllocated);
    ++play.gameplayFrames;
    drawLegacyFixture(&fire);
    inspectRemake(1, 1);
    REQUIRE(
        memcmp(savedVertices, gfx.polyOpa.d, bytesAllocated)); // Hold still has independently moving material/geometry.
    REQUIRE(nativeVertexLoads == 0);                           // No mutation of shared sphere geometry.
    checkHorizontalShell(&fire);
    customColors = 1;
    drawLegacyFixture(&fire);
    inspectRemake(1, 1);
    REQUIRE(colorSeen(G_SETPRIMCOLOR, 0x11DD95) && colorSeen(G_SETENVCOLOR, 0x052B47));
    customColors = 0;
    Vtx fullShell[CW_DINS_MAX_QUADS][4], fadingShell[CW_DINS_MAX_QUADS][4];
    REQUIRE(copySubmittedQuads(fullShell) == 52);
    fire.action = 3;
    fire.alphaMultiplier = 0.15f;
    drawLegacyFixture(&fire);
    inspectRemake(1, 1);
    REQUIRE(quadsDrawn < holdQuads && maxAlpha < 100);
    size_t fadingCount = copySubmittedQuads(fadingShell);
    unsigned survivingBands = 0, partialColumns = 0;
    for (int sector = 0; sector < 12; ++sector) {
        unsigned bands = 0;
        for (int band = 0; band < 3; ++band) {
            for (size_t patch = 0; patch < fadingCount; ++patch) {
                int same = 1;
                for (int vertex = 0; vertex < 4; ++vertex)
                    same &= !memcmp(fullShell[band * 12 + sector][vertex].v.ob, fadingShell[patch][vertex].v.ob,
                                    sizeof(fullShell[0][0].v.ob));
                if (same)
                    bands |= 1u << band;
            }
        }
        survivingBands |= bands;
        partialColumns += bands != 0 && bands != 7;
    }
    REQUIRE(survivingBands == 7 && partialColumns >= 3); // Fade band patches, not complete upright columns.
    for (int iteration = 0; iteration < 256; ++iteration) {
        play.gameplayFrames = UINT32_MAX - 128u + iteration;
        fire.action = (iteration % 3) + 1;
        fire.alphaMultiplier = 0.1f + (iteration % 9) * 0.1f;
        fire.actor.scale.x = 0.1f + (iteration % 11) * 0.05f;
        alt = iteration & 1;
        drawLegacyFixture(&fire);
        inspectRemake(1, 1);
    }
    option = 0;
    size_t nativeCount = drawLegacyFixture(&fire);
    memcpy(reference, commands, sizeof(commands));
    REQUIRE(nativeVertexLoads > 0);
    for (missing = 0; missing < 2; ++missing) {
        option = 1;
        REQUIRE(drawLegacyFixture(&fire) == nativeCount);
        REQUIRE(!memcmp(reference, commands, sizeof(commands)));
        REQUIRE(bytesAllocated == 0);
    }
    missing = -1;
    option = 0;
    fire.action = 0;
    REQUIRE(drawLegacyFixture(&fire) == 0);
    option = 1;
    missing = 0;
    REQUIRE(drawLegacyFixture(&fire) == 0);
    missing = -1;
    for (int arena = 0; arena < 3; ++arena) {
        reset();
        if (arena == 0)
            gfx.polyOpa.d = gfx.polyOpa.p + 1;
        if (arena == 1)
            gfx.polyXlu.d = gfx.polyXlu.p + 1;
        if (arena == 2)
            gfx.overlay.d = gfx.overlay.p;
        Gfx* tail = gfx.polyOpa.d;
        REQUIRE(!CwDinsFireRemake_DrawActor(&play, &fire.actor, 0, 20, 0, 0, 0xFFC800, 0xFF0000));
        REQUIRE(gfx.polyXlu.p == commands && gfx.polyOpa.d == tail && childOpens == 0);
    }
#ifdef TEST_HOST_MM
    option = 1;
    fire.actor.id = ACTOR_SW97_MAGIC_FIRE;
    fire.action = 2;
    int queries = resourceLoads[0] + resourceLoads[1];
    drawLegacyFixture(&fire);
    REQUIRE(resourceLoads[0] + resourceLoads[1] == queries && countOp(G_VTX) == 0);
#endif
    option = 1;
    missing = -1;
    reset();
    REQUIRE(CwDinsFireRemake_DrawCharge(&play, &player, .3f));
    bytesAllocated = (unsigned char*)(opa + ARRAY_COUNT(opa)) - (unsigned char*)gfx.polyOpa.d;
    inspectRemake(0, 1);
    checkChargeMapping();
    option = 0;
    reset();
    REQUIRE(!CwDinsFireRemake_DrawCharge(&play, &player, .3f));
    REQUIRE(gfx.polyXlu.p == commands);
    option = 1;
    missing = 1;
    REQUIRE(!CwDinsFireRemake_DrawCharge(&play, &player, .3f));
    REQUIRE(gfx.polyXlu.p == commands);
    missing = -1;
    checkNativeDraw();
    checkNativeActorRouting();
    checkInactiveCastStaysStock();
    checkNativePaletteSemantics();
    checkNativePaletteResources();
    checkNativeRealCapacity();
    printf("PASS %s legacy fallback renderer: pre-cast/charge, hollow shell/fade, live "
           "cosmetics/resources, exact fallback and bounded immutable draw output\n",
#ifdef TEST_HOST_MM
           "MM"
#else
           "OoT"
#endif
    );
    printf("Peak observed: %zu vertices, %zu arena bytes, %zu XLU commands; one allocation, two matrices per draw\n",
           peakVertices, peakBytes, peakCommands);
    puts("PASS native setup boundaries: zero/one OPA or overlay marker slots reject; exact two slots preserve both "
         "matrices");
    puts("PASS horizontal shell: continuous shared-edge UVs, bounded spans, dominant S scroll through frame rollover, "
         "unchanged charge mapping and band-patch fade");
    return 0;
}
