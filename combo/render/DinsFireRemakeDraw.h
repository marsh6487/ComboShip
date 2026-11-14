/* Include after the native host declarations; CW_DINS_HOST_MM selects MM names. */
#ifndef COMBO_DINS_FIRE_REMAKE_DRAW_H
#define COMBO_DINS_FIRE_REMAKE_DRAW_H
#include "DinsFireRemake.h"
#include "DinsFireColors.h"
#include "align_asset_macro.h"
#include "soh/frame_interpolation.h"
#include "../../tools/dins_fire_remake/native_resources.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <libultraship/bridge/resourcebridge.h>
#include <string.h>
#define CW_DINS_NATIVE_DRAW 1

static const ALIGN_ASSET(2) char sCwDinsFlameTex[] = "__OTR__custom/dins_fire_remake/flameTex";
static const ALIGN_ASSET(2) char sCwDinsSparkTex[] = "__OTR__custom/dins_fire_remake/sparkTex";

static inline uint32_t CwDins_RGB(uint8_t r, uint8_t g, uint8_t b) {
    return (uint32_t)r << 16 | (uint32_t)g << 8 | b;
}
static inline int CwDins_LegacyReady(void) {
    if (!CVarGetInteger("gEnhancements.DinsFireRemake", 0))
        return 0;
    /* Resolve every draw. Submit the names below, never these transient pointers:
     * the resource's IMG flags and physical resolution belong to the renderer. */
    void* flame = ResourceGetDataByName(sCwDinsFlameTex);
    void* spark = ResourceGetDataByName(sCwDinsSparkTex);
    return flame != NULL && spark != NULL;
}
/* Cast latching requires the small core set. Optional burnout frames are
 * resolved only for the current actor age, never all 66 on every draw. */
static inline int CwDinsFireRemake_ColorResourcesReady(uint32_t primary, uint32_t secondary) {
    int palette = CwDinsFire_CustomPalette(primary, secondary);
    const char* core[] = { sCwDinsSparkTex,
                           palette ? sCwDinsRibbon0PaletteTex : sCwDinsRibbon0Tex,
                           palette ? sCwDinsRibbon1PaletteTex : sCwDinsRibbon1Tex,
                           palette ? sCwDinsRibbon2PaletteTex : sCwDinsRibbon2Tex,
                           palette ? sCwDinsHaloPaletteTex : sCwDinsHaloTex,
                           sCwDinsWhiteHaloTex,
                           sCwDinsTrailTex };
    for (size_t i = 0; i < sizeof(core) / sizeof(core[0]); ++i)
        if (!ResourceGetDataByName(core[i]))
            return 0;
    return 1;
}
static inline int CwDinsFireRemake_ResourcesReady(void) {
    uint32_t primary, secondary;
    CwDinsFire_GetColors(&primary, &secondary);
    return CwDinsFireRemake_ColorResourcesReady(primary, secondary);
}
static inline int CwDins_Ready(void) {
    return CVarGetInteger("gEnhancements.DinsFireRemake", 0) && CwDinsFireRemake_ResourcesReady();
}
static inline CwDinsVec CwDins_Right(PlayState* play) {
    return (CwDinsVec){ play->billboardMtxF.xx, play->billboardMtxF.yx, play->billboardMtxF.zx };
}
static inline CwDinsVec CwDins_Up(PlayState* play) {
    return (CwDinsVec){ play->billboardMtxF.xy, play->billboardMtxF.yy, play->billboardMtxF.zy };
}
static inline CwDinsVec CwDins_Hand(Player* player) {
#ifdef CW_DINS_HOST_MM
    Vec3f hand = player->bodyPartsPos[PLAYER_BODYPART_RIGHT_HAND];
#else
    Vec3f hand = player->bodyPartsPos[PLAYER_BODYPART_R_HAND];
#endif
    CwDinsVec result = { hand.x - player->actor.world.pos.x, hand.y - player->actor.world.pos.y,
                         hand.z - player->actor.world.pos.z };
    /* A replacement player model may not have published a hand yet. Keep an
     * ordinary body-relative origin instead of drawing at the world origin. */
    if (!isfinite(result.x) || !isfinite(result.y) || !isfinite(result.z) || fabsf(result.x) > 150 ||
        fabsf(result.y) > 180 || fabsf(result.z) > 150)
        result = (CwDinsVec){ 0, 40, 0 };
    return result;
}
static inline void CwDins_Setup(GraphicsContext* gfx) {
#ifdef CW_DINS_HOST_MM
    Gfx_SetupDL25_Xlu(gfx);
#else
    Gfx_SetupDL_25Xlu(gfx);
#endif
}

/* One arena allocation, two matrices, at most 256 private vertices. No shared
 * vertex/resource writes and no CPU matrix-stack changes. Reserve the native
 * OpenDisps markers as well as all commands before touching any arena. The
 * native XLU setup opens a nested display scope, so both OPA and overlay need
 * two simultaneous marker slots even though neither submits visible geometry. */
static inline int CwDins_Submit(PlayState* play, const void* identity, Vec3f origin, CwDinsMesh* mesh, int active,
                                float screenTint) {
    if (mesh->count == 0)
        return 1;
    GraphicsContext* gfx = play->state.gfxCtx;
    size_t bytes = 2 * sizeof(Mtx) + mesh->count * 4 * sizeof(Vtx);
    size_t commandBytes = (64 + mesh->count * 5) * sizeof(Gfx);
    size_t markerBytes = 2 * sizeof(Gfx);
    uintptr_t opaHead = (uintptr_t)gfx->polyOpa.p, opaTail = (uintptr_t)gfx->polyOpa.d;
    uintptr_t xluHead = (uintptr_t)gfx->polyXlu.p, xluTail = (uintptr_t)gfx->polyXlu.d;
    uintptr_t overlayHead = (uintptr_t)gfx->overlay.p, overlayTail = (uintptr_t)gfx->overlay.d;
    if (opaTail < opaHead || opaTail - opaHead < ((bytes + 15) & ~(size_t)15) + markerBytes || xluTail < xluHead ||
        xluTail - xluHead < commandBytes || overlayTail < overlayHead || overlayTail - overlayHead < markerBytes)
        return 0;
#ifdef CW_DINS_HOST_MM
    Mtx* matrices = (Mtx*)GRAPH_ALLOC(gfx, bytes);
#else
    Mtx* matrices = (Mtx*)Graph_Alloc(gfx, bytes);
#endif
    if (!matrices)
        return 0;
    Vtx* vertices = (Vtx*)(matrices + 2);
    for (size_t i = 0; i < mesh->count; ++i) {
        CwDinsQuad* q = &mesh->quads[i];
        for (int j = 0; j < 4; ++j) {
            Vtx* v = &vertices[i * 4 + j];
            memset(v, 0, sizeof(*v));
            v->v.ob[0] = (int16_t)CwDins_Clamp(q->p[j].x * CW_DINS_VERTEX_SCALE, -32760, 32760);
            v->v.ob[1] = (int16_t)CwDins_Clamp(q->p[j].y * CW_DINS_VERTEX_SCALE, -32760, 32760);
            v->v.ob[2] = (int16_t)CwDins_Clamp(q->p[j].z * CW_DINS_VERTEX_SCALE, -32760, 32760);
            v->v.tc[0] = (int16_t)((q->s + ((j == 1 || j == 2) ? q->sSpan : 0.0f)) * 32.0f);
            v->v.tc[1] = (int16_t)((q->t + ((j < 2) ? q->tSpan : 0.0f)) * 32.0f);
            v->v.cn[0] = v->v.cn[1] = v->v.cn[2] = 255;
            v->v.cn[3] = q->alpha[j];
        }
    }
    MtxF model = { 0 }, restore;
    Matrix_Get(&restore);
    model.xx = model.yy = model.zz = 1.0f / CW_DINS_VERTEX_SCALE;
    model.ww = 1;
    model.xw = origin.x;
    model.yw = origin.y;
    model.zw = origin.z;
    FrameInterpolation_RecordOpenChild(identity, active ? 0xD1F1 : 0xD1F0);
    Matrix_MtxFToMtx(&model, matrices);
    Matrix_MtxFToMtx(&restore, matrices + 1);
    OPEN_DISPS(gfx);
    if (active) {
#ifdef CW_DINS_HOST_MM
        POLY_XLU_DISP = Gfx_SetupDL57(POLY_XLU_DISP);
#else
        POLY_XLU_DISP = Gfx_SetupDL_57(POLY_XLU_DISP);
#endif
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, (uint8_t)(60 * screenTint), (uint8_t)(20 * screenTint), 0,
                        (uint8_t)(120 * screenTint));
        gDPSetAlphaDither(POLY_XLU_DISP++, G_AD_DISABLE);
        gDPSetColorDither(POLY_XLU_DISP++, G_CD_DISABLE);
#ifdef CW_DINS_HOST_MM
        gSPDisplayList(POLY_XLU_DISP++, D_0E000000_TO_SEGMENTED(fillRect));
#else
        gDPFillRectangle(POLY_XLU_DISP++, 0, 0, 319, 239);
#endif
    }
    CwDins_Setup(gfx);
    gDPPipeSync(POLY_XLU_DISP++);
    gDPSetCycleType(POLY_XLU_DISP++, G_CYC_1CYCLE);
    gDPSetRenderMode(POLY_XLU_DISP++, G_RM_ZB_XLU_SURF, G_RM_ZB_XLU_SURF2);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING | G_FOG | G_CULL_BOTH | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gSPSetGeometryMode(POLY_XLU_DISP++, G_SHADE | G_SHADING_SMOOTH | G_ZBUFFER);
    gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
    gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetCombineLERP(POLY_XLU_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, SHADE, 0, PRIMITIVE,
                      ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, SHADE, 0);
    gSPMatrix(POLY_XLU_DISP++, matrices, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    for (int material = 0; material < 2; ++material) {
        int loaded = 0;
        for (size_t i = 0; i < mesh->count; ++i) {
            CwDinsQuad* q = &mesh->quads[i];
            if (q->spark != material)
                continue;
            if (!loaded) {
                gDPLoadTextureBlock(POLY_XLU_DISP++, material ? sCwDinsSparkTex : sCwDinsFlameTex, G_IM_FMT_I,
                                    G_IM_SIZ_8b, 64, 64, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 6, 6,
                                    G_TX_NOLOD, G_TX_NOLOD);
                loaded = 1;
            }
            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, (q->primary >> 16) & 255, (q->primary >> 8) & 255,
                            q->primary & 255, 255);
            gDPSetEnvColor(POLY_XLU_DISP++, (q->secondary >> 16) & 255, (q->secondary >> 8) & 255, q->secondary & 255,
                           255);
            gSPVertex(POLY_XLU_DISP++, (uintptr_t)&vertices[i * 4], 4, 0);
            gSP2Triangles(POLY_XLU_DISP++, 0, 1, 2, 0, 0, 2, 3, 0);
        }
    }
    gSPMatrix(POLY_XLU_DISP++, matrices + 1, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    CwDins_Setup(gfx);
    CLOSE_DISPS(gfx);
    FrameInterpolation_RecordCloseChild();
    return 1;
}

static inline int CwDinsFireRemake_DrawActor(PlayState* play, Actor* actor, int action, int timer, float alpha,
                                             float screenTint, uint32_t primary, uint32_t secondary) {
#ifdef CW_DINS_HOST_MM
    if (actor->id != ACTOR_OOT_DINS_FIRE)
        return 0;
#endif
    if (!CwDins_LegacyReady())
        return 0;
    CwDinsMesh mesh;
    if (action == 0) {
        Player* player = GET_PLAYER(play);
        CwDinsFireRemake_BuildCharge(&mesh, 0.65f + 0.35f * (1.0f - CwDins_Clamp(timer / 20.0f, 0, 1)),
                                     play->gameplayFrames, CwDins_Hand(player), CwDins_Right(play), CwDins_Up(play),
                                     primary, secondary);
    } else {
        CwDinsFireRemake_BuildShell(&mesh, play->gameplayFrames, actor->scale.x, alpha, action == 3, CwDins_Right(play),
                                    CwDins_Up(play), primary, secondary);
    }
    return CwDins_Submit(play, actor, actor->world.pos, &mesh, action > 0, screenTint);
}

static inline int CwDinsFireRemake_DrawCharge(PlayState* play, Player* player, float progress) {
    if (!CwDins_LegacyReady())
        return 0;
    uint32_t primary = 0xFFC800, secondary = 0xFF0000;
#ifdef CW_DINS_HOST_MM
    Color_RGBA8 p = CosmeticEditor_GetChangedColor(255, 200, 0, 255, COSMETIC_ID("Magic.DinsPrimary"));
    Color_RGBA8 e = CosmeticEditor_GetChangedColor(255, 0, 0, 255, COSMETIC_ID("Magic.DinsSecondary"));
    primary = CwDins_RGB(p.r, p.g, p.b);
    secondary = CwDins_RGB(e.r, e.g, e.b);
#else
    if (CVarGetInteger(CVAR_COSMETIC("Magic.DinsPrimary.Changed"), 0)) {
        Color_RGB8 p = CVarGetColor24(CVAR_COSMETIC("Magic.DinsPrimary.Value"), (Color_RGB8){ 255, 200, 0 });
        primary = CwDins_RGB(p.r, p.g, p.b);
    }
    if (CVarGetInteger(CVAR_COSMETIC("Magic.DinsSecondary.Changed"), 0)) {
        Color_RGB8 e = CVarGetColor24(CVAR_COSMETIC("Magic.DinsSecondary.Value"), (Color_RGB8){ 255, 0, 0 });
        secondary = CwDins_RGB(e.r, e.g, e.b);
    }
#endif
    CwDinsMesh mesh;
    CwDinsFireRemake_BuildCharge(&mesh, CwDins_Clamp(progress, 0, 1) * 0.65f, play->gameplayFrames, CwDins_Hand(player),
                                 CwDins_Right(play), CwDins_Up(play), primary, secondary);
    return CwDins_Submit(play, player, player->actor.world.pos, &mesh, 0, 0);
}

/* Whole-quad centroid sorting approximates back-to-front alpha-over on the
 * native pipeline. Intersecting transparent sheets cannot reproduce the offline
 * compositor's per-pixel fragment ordering; configured runtime review is needed.
 * Sorting includes all materials, so ribbons are not grouped in front of halos. */
static inline void CwDins_NativeOrder(const CwDinsMesh* mesh, CwDinsVec forward, uint16_t order[CW_DINS_MAX_QUADS]) {
    float depths[CW_DINS_MAX_QUADS];
    for (size_t i = 0; i < mesh->count; ++i) {
        CwDinsVec center = { 0 };
        for (int v = 0; v < 4; ++v)
            center = CwDins_Add(center, mesh->quads[i].p[v]);
        depths[i] = CwDins_Dot(center, forward) * .25f;
        size_t at = i;
        while (at > 0 && depths[order[at - 1]] > depths[i]) {
            order[at] = order[at - 1];
            --at;
        }
        order[at] = i;
    }
}

#define CW_DINS_NATIVE_XLU_RESERVE 32
#define CW_DINS_STATE_TEXTURE 1
#define CW_DINS_STATE_COMBINE 2
#define CW_DINS_STATE_PRIMARY 4
#define CW_DINS_STATE_SECONDARY 8
typedef struct {
    const char* texture;
    uint32_t primary, secondary;
    int modulate;
} CwDinsMaterialState;

static inline const char* CwDins_NativeTexture(const CwDinsQuad* q, const char* const ribbons[6]) {
    return q->spark == 2   ? ribbons[q->band]
           : q->spark == 3 ? (q->palette ? sCwDinsHaloPaletteTex : sCwDinsHaloTex)
           : q->spark == 4 ? sCwDinsWhiteHaloTex
           : q->spark == 5 ? sCwDinsTrailTex
           : q->spark == 1 ? sCwDinsSparkTex
                           : sCwDinsFlameTex;
}
/* The same state transition describes the exact command reservation and the
 * emitted stream. Consecutive identical materials do not repeat color/sync work. */
static inline unsigned CwDins_NativeMaterialChanges(CwDinsMaterialState* state, const CwDinsQuad* q,
                                                    const char* texture) {
    int modulate = q->spark >= 2 && !q->palette;
    int first = state->modulate < 0;
    unsigned changes = (state->texture != texture ? CW_DINS_STATE_TEXTURE : 0) |
                       (state->modulate != modulate ? CW_DINS_STATE_COMBINE : 0) |
                       (first || state->primary != q->primary ? CW_DINS_STATE_PRIMARY : 0) |
                       (first || state->secondary != q->secondary ? CW_DINS_STATE_SECONDARY : 0);
    *state = (CwDinsMaterialState){ texture, q->primary, q->secondary, modulate };
    return changes;
}
static inline size_t CwDins_NativeChildCount(const CwDinsMesh* mesh, const uint16_t* order,
                                             const char* const ribbons[6]) {
    CwDinsMaterialState state = { NULL, 0, 0, -1 };
    size_t count = 12; /* Ten setup commands, restore matrix, terminating EndDL. */
    for (size_t i = 0; i < mesh->count; ++i) {
        const CwDinsQuad* q = &mesh->quads[order[i]];
        unsigned changes = CwDins_NativeMaterialChanges(&state, q, CwDins_NativeTexture(q, ribbons));
        count += 2 + (changes != 0) + ((changes & CW_DINS_STATE_TEXTURE) ? 7 : 0) +
                 ((changes & CW_DINS_STATE_COMBINE) != 0) + ((changes & CW_DINS_STATE_PRIMARY) != 0) +
                 ((changes & CW_DINS_STATE_SECONDARY) != 0);
    }
    return count;
}

/* Each emitted RGBA texture describes a 32x32 native tile (4 KiB); IMG metadata
 * makes the host upload the complete physical image. Normalized UVs are preserved
 * by halving the recipe's 64-unit coordinates. Names, rather than transient
 * resource pointers, preserve IMG flags, Alt Assets and reload semantics.
 * MM's actual XLU head has only 4096 commands. Store this effect's exact-sized
 * child list with the private OPA tail data, and submit nine XLU head commands.
 * The 32-command head reservation includes spare room beyond our nested markers. */
static inline int CwDins_SubmitNative(PlayState* play, const void* identity, Vec3f origin, CwDinsMesh* mesh) {
    if (mesh->count == 0)
        return 1;
    if (mesh->count > CW_DINS_MAX_QUADS)
        return 0;
    const char* ribbonTextures[6];
    for (int band = 0; band < 6; ++band)
        ribbonTextures[band] = sCwDinsRibbonTextures[band % 3];
    int resolvedBurn[6] = { 0 };
    for (size_t i = 0; i < mesh->count; ++i) {
        CwDinsQuad* q = &mesh->quads[i];
        if (q->spark == 2 && q->band >= 6)
            return 0;
        if (q->spark != 2)
            continue;
        int band = q->band, frame = q->burnFrame;
        if (!resolvedBurn[band])
            ribbonTextures[band] =
                q->palette ? sCwDinsRibbonPaletteTextures[band % 3] : sCwDinsRibbonTextures[band % 3];
        if (!frame)
            continue;
        if (band >= 6 || frame < CW_DINS_BURN_FIRST_FRAME ||
            frame >= CW_DINS_BURN_FIRST_FRAME + CW_DINS_BURN_FRAME_COUNT)
            return 0;
        if (!resolvedBurn[band]) {
            const char* candidate = q->palette
                                        ? sCwDinsRibbonBurnPaletteTextures[band][frame - CW_DINS_BURN_FIRST_FRAME]
                                        : sCwDinsRibbonBurnTextures[band][frame - CW_DINS_BURN_FIRST_FRAME];
            if (ResourceGetDataByName(candidate)) {
                ribbonTextures[band] = candidate;
                resolvedBurn[band] = 1;
            } else {
                resolvedBurn[band] = -1;
            }
        }
        if (resolvedBurn[band] < 0) {
            /* A missing optional frame retains the current base motion, not an
             * unscrolled sheet or a blank band. The actor's lifetime is intact. */
            q->s -= frame / (float)CW_DINS_NATIVE_RATE * (.27f + band * .025f) * 64;
            q->burnFrame = 0;
        }
    }
    GraphicsContext* gfx = play->state.gfxCtx;
    uint16_t order[CW_DINS_MAX_QUADS];
    CwDins_NativeOrder(mesh, CwDins_Normalize(CwDins_Cross(CwDins_Right(play), CwDins_Up(play))), order);
    size_t childCount = CwDins_NativeChildCount(mesh, order, ribbonTextures);
    size_t dataBytes = 2 * sizeof(Mtx) + mesh->count * 4 * sizeof(Vtx);
    size_t bytes = dataBytes + childCount * sizeof(Gfx);
    size_t alignedBytes = (bytes + 15) & ~(size_t)15;
    size_t commandBytes = CW_DINS_NATIVE_XLU_RESERVE * sizeof(Gfx);
    size_t markerBytes = 2 * sizeof(Gfx);
    uintptr_t opaHead = (uintptr_t)gfx->polyOpa.p, opaTail = (uintptr_t)gfx->polyOpa.d & ~(uintptr_t)15;
    uintptr_t xluHead = (uintptr_t)gfx->polyXlu.p, xluTail = (uintptr_t)gfx->polyXlu.d;
    uintptr_t overlayHead = (uintptr_t)gfx->overlay.p, overlayTail = (uintptr_t)gfx->overlay.d;
    if (opaTail < opaHead || opaTail - opaHead < alignedBytes + markerBytes || xluTail < xluHead ||
        xluTail - xluHead < commandBytes || overlayTail < overlayHead || overlayTail - overlayHead < markerBytes)
        return 0;
#ifdef CW_DINS_HOST_MM
    Mtx* matrices = (Mtx*)GRAPH_ALLOC(gfx, bytes);
#else
    Mtx* matrices = (Mtx*)Graph_Alloc(gfx, bytes);
#endif
    if (!matrices)
        return 0;
    Vtx* vertices = (Vtx*)(matrices + 2);
    for (size_t i = 0; i < mesh->count; ++i) {
        CwDinsQuad* q = &mesh->quads[i];
        float uvScale = q->spark >= 2 ? CW_DINS_RGBA_LOGICAL_SIZE / 64.0f : 1;
        for (int j = 0; j < 4; ++j) {
            Vtx* v = &vertices[i * 4 + j];
            memset(v, 0, sizeof(*v));
            v->v.ob[0] = (int16_t)CwDins_Clamp(q->p[j].x * CW_DINS_VERTEX_SCALE, -32760, 32760);
            v->v.ob[1] = (int16_t)CwDins_Clamp(q->p[j].y * CW_DINS_VERTEX_SCALE, -32760, 32760);
            v->v.ob[2] = (int16_t)CwDins_Clamp(q->p[j].z * CW_DINS_VERTEX_SCALE, -32760, 32760);
            v->v.tc[0] = (int16_t)((q->s + ((j == 1 || j == 2) ? q->sSpan : 0)) * uvScale * 32);
            v->v.tc[1] = (int16_t)((q->t + (j < 2 ? q->tSpan : 0)) * uvScale * 32);
            v->v.cn[0] = v->v.cn[1] = v->v.cn[2] = 255;
            v->v.cn[3] = q->alpha[j];
        }
    }
    Gfx* childStart = (Gfx*)((unsigned char*)matrices + dataBytes);
    Gfx* child = childStart;
    MtxF model = { 0 }, restore;
    Matrix_Get(&restore);
    model.xx = model.yy = model.zz = 1 / CW_DINS_VERTEX_SCALE;
    model.ww = 1;
    model.xw = origin.x;
    model.yw = origin.y;
    model.zw = origin.z;
    FrameInterpolation_RecordOpenChild(identity, 0xD1F2);
    Matrix_MtxFToMtx(&model, matrices);
    Matrix_MtxFToMtx(&restore, matrices + 1);
    gDPPipeSync(child++);
    gDPSetCycleType(child++, G_CYC_1CYCLE);
    gDPSetRenderMode(child++, G_RM_ZB_XLU_SURF, G_RM_ZB_XLU_SURF2);
    gSPClearGeometryMode(child++, G_LIGHTING | G_FOG | G_CULL_BOTH | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gSPSetGeometryMode(child++, G_SHADE | G_SHADING_SMOOTH | G_ZBUFFER);
    gDPSetTextureLUT(child++, G_TT_NONE);
    gDPSetTextureFilter(child++, G_TF_BILERP);
    gDPSetAlphaCompare(child++, G_AC_NONE);
    gSPTexture(child++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gSPMatrix(child++, matrices, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    CwDinsMaterialState materialState = { NULL, 0, 0, -1 };
    for (size_t position = 0; position < mesh->count; ++position) {
        size_t i = order[position];
        CwDinsQuad* q = &mesh->quads[i];
        int rgba = q->spark >= 2;
        const char* texture = CwDins_NativeTexture(q, ribbonTextures);
        unsigned changes = CwDins_NativeMaterialChanges(&materialState, q, texture);
        if (changes)
            gDPPipeSync(child++);
        if (changes & CW_DINS_STATE_COMBINE) {
            /* Full-color artwork keeps its authored RGB. Neutral palette
             * companions interpolate exact editor endpoints, with independent
             * TEXEL0 alpha times the same private vertex alpha in both modes. */
            if (materialState.modulate) {
                gDPSetCombineLERP(child++, TEXEL0, 0, PRIMITIVE, 0, TEXEL0, 0, SHADE, 0, TEXEL0, 0, PRIMITIVE, 0,
                                  TEXEL0, 0, SHADE, 0);
            } else {
                gDPSetCombineLERP(child++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, SHADE, 0, PRIMITIVE,
                                  ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, SHADE, 0);
            }
        }
        if (changes & CW_DINS_STATE_TEXTURE) {
            if (rgba) {
                gDPLoadTextureBlock(child++, texture, G_IM_FMT_RGBA, G_IM_SIZ_32b, CW_DINS_RGBA_LOGICAL_SIZE,
                                    CW_DINS_RGBA_LOGICAL_SIZE, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_CLAMP,
                                    5, 5, G_TX_NOLOD, G_TX_NOLOD);
            } else {
                gDPLoadTextureBlock(child++, texture, G_IM_FMT_I, G_IM_SIZ_8b, 64, 64, 0, G_TX_NOMIRROR | G_TX_WRAP,
                                    G_TX_NOMIRROR | G_TX_WRAP, 6, 6, G_TX_NOLOD, G_TX_NOLOD);
            }
        }
        if (changes & CW_DINS_STATE_PRIMARY)
            gDPSetPrimColor(child++, 0, 0x80, (q->primary >> 16) & 255, (q->primary >> 8) & 255, q->primary & 255, 255);
        if (changes & CW_DINS_STATE_SECONDARY)
            gDPSetEnvColor(child++, (q->secondary >> 16) & 255, (q->secondary >> 8) & 255, q->secondary & 255, 255);
        gSPVertex(child++, (uintptr_t)&vertices[i * 4], 4, 0);
        gSP2Triangles(child++, 0, 1, 2, 0, 0, 2, 3, 0);
    }
    gSPMatrix(child++, matrices + 1, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPEndDisplayList(child++);
    OPEN_DISPS(gfx);
    CwDins_Setup(gfx);
    gSPDisplayList(POLY_XLU_DISP++, childStart);
    CwDins_Setup(gfx);
    CLOSE_DISPS(gfx);
    FrameInterpolation_RecordCloseChild();
    return 1;
}

static inline int CwDinsFireRemake_DrawNativeActor(PlayState* play, Actor* actor, int fireFrame, uint32_t primary,
                                                   uint32_t secondary) {
#ifdef CW_DINS_HOST_MM
    if (actor->id != ACTOR_OOT_DINS_FIRE)
        return 0;
#endif
    if (fireFrame < 0 || fireFrame >= CW_DINS_NATIVE_FIRE_TICKS)
        return 1;
    if (!CwDinsFireRemake_ColorResourcesReady(primary, secondary))
        return 0;
    CwDinsMesh mesh;
    CwDinsFireRemake_BuildNativeFire(&mesh, fireFrame, CwDins_Right(play), CwDins_Up(play), primary, secondary);
    return CwDins_SubmitNative(play, actor, actor->world.pos, &mesh);
}

static inline int CwDinsFireRemake_DrawNativeCharge(PlayState* play, Player* player, float phaseSeconds) {
    uint32_t primary, secondary;
    CwDinsFire_GetColors(&primary, &secondary);
    if (!CwDinsFireRemake_ColorResourcesReady(primary, secondary))
        return 0;
    CwDinsMesh mesh;
    CwDinsFireRemake_BuildNativeCharge(&mesh, phaseSeconds, CwDins_Hand(player), CwDins_Right(play), CwDins_Up(play),
                                       primary, secondary);
    return CwDins_SubmitNative(play, player, player->actor.world.pos, &mesh);
}
#endif
