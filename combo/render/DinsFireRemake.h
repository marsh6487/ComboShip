#ifndef COMBO_DINS_FIRE_REMAKE_H
#define COMBO_DINS_FIRE_REMAKE_H

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include "DinsFireTiming.h"

/* Stateless, world-unit geometry. These are visual samples of the existing cast,
 * never a clock, particle simulation, collider, or gameplay-RNG consumer. */
#define CW_DINS_MAX_QUADS 600
#define CW_DINS_VERTEX_SCALE 16.0f
#define CW_DINS_NATIVE_RENDERER 1
#define CW_DINS_NATIVE_RIBBONS 6
#define CW_DINS_NATIVE_SEGMENTS 32
#define CW_DINS_NATIVE_VERTICAL_CELLS 3
typedef struct {
    float x, y, z;
} CwDinsVec;
typedef struct {
    CwDinsVec p[4];
    float s, t, sSpan, tSpan;
    uint32_t primary, secondary;
    uint8_t alpha[4], spark;
    uint8_t band, burnFrame, palette;
    float burn;
} CwDinsQuad;
typedef struct {
    CwDinsQuad quads[CW_DINS_MAX_QUADS];
    size_t count;
} CwDinsMesh;

static inline float CwDins_Clamp(float x, float lo, float hi) {
    return x < lo ? lo : x > hi ? hi : x;
}
static inline CwDinsVec CwDins_Add(CwDinsVec a, CwDinsVec b) {
    return (CwDinsVec){ a.x + b.x, a.y + b.y, a.z + b.z };
}
static inline CwDinsVec CwDins_Scale(CwDinsVec a, float s) {
    return (CwDinsVec){ a.x * s, a.y * s, a.z * s };
}
static inline uint32_t CwDins_Mix(uint32_t a, uint32_t b, float f) {
    uint32_t color = 0;
    for (int shift = 0; shift <= 16; shift += 8) {
        float channel = (float)((a >> shift) & 255) * (1.0f - f) + (float)((b >> shift) & 255) * f;
        color |= (uint32_t)(channel + 0.5f) << shift;
    }
    return color;
}
static inline CwDinsQuad* CwDins_Quad(CwDinsMesh* mesh, uint8_t spark, uint32_t primary, uint32_t secondary,
                                      float alpha) {
    if (mesh->count >= CW_DINS_MAX_QUADS)
        return NULL;
    CwDinsQuad* q = &mesh->quads[mesh->count++];
    q->spark = spark;
    q->band = q->burnFrame = q->palette = 0;
    q->burn = 0;
    q->primary = primary;
    q->secondary = secondary;
    q->s = q->t = 0;
    q->sSpan = q->tSpan = 63;
    for (int i = 0; i < 4; ++i)
        q->alpha[i] = (uint8_t)CwDins_Clamp(alpha * 255, 0, 255);
    return q;
}
static inline void CwDins_Billboard(CwDinsMesh* mesh, CwDinsVec at, CwDinsVec right, CwDinsVec up, float width,
                                    float height, uint8_t spark, uint32_t primary, uint32_t secondary, float alpha) {
    CwDinsQuad* q = CwDins_Quad(mesh, spark, primary, secondary, alpha);
    if (!q)
        return;
    right = CwDins_Scale(right, width);
    up = CwDins_Scale(up, height);
    q->p[0] = CwDins_Add(CwDins_Add(at, CwDins_Scale(right, -1)), CwDins_Scale(up, -1));
    q->p[1] = CwDins_Add(CwDins_Add(at, right), CwDins_Scale(up, -1));
    q->p[2] = CwDins_Add(CwDins_Add(at, right), up);
    q->p[3] = CwDins_Add(CwDins_Add(at, CwDins_Scale(right, -1)), up);
}
static inline void CwDins_FeatheredFlame(CwDinsMesh* mesh, CwDinsVec at, CwDinsVec right, CwDinsVec up, float width,
                                         float height, uint32_t primary, uint32_t secondary, float alpha, float phase) {
    CwDinsVec grid[9];
    for (int row = 0; row < 3; ++row) {
        float taper = row == 0 ? 0.35f : row == 1 ? 1.0f : 0.05f;
        float sway = row == 2 ? sinf(phase * 0.23f) * width * 0.55f : 0;
        for (int col = 0; col < 3; ++col) {
            grid[row * 3 + col] = CwDins_Add(at, CwDins_Add(CwDins_Scale(right, (col - 1) * width * taper + sway),
                                                            CwDins_Scale(up, (row - 1) * height)));
        }
    }
    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 2; ++col) {
            int indices[4] = { row * 3 + col, row * 3 + col + 1, (row + 1) * 3 + col + 1, (row + 1) * 3 + col };
            CwDinsQuad* q = CwDins_Quad(mesh, 0, primary, secondary, alpha);
            if (!q)
                return;
            for (int j = 0; j < 4; ++j) {
                q->p[j] = grid[indices[j]];
                // Only the interior point is opaque: every outer edge fades to
                // zero, including the tapered foot and independently swaying tip.
                if (indices[j] != 4)
                    q->alpha[j] = 0;
            }
            q->t = fmodf(phase, 64.0f);
        }
    }
}
static inline CwDinsVec CwDins_TrailPoint(CwDinsVec hand, float phase, float f, float progress, int trail) {
    float radius = (1.0f - f) * (29.0f - 12.0f * progress);
    float angle = phase * 0.09f + f * 8.5f + trail * 3.14159265f;
    return (CwDinsVec){ hand.x * f + cosf(angle) * radius, hand.y * f + (1.0f - f) * (18.0f + f * 30.0f),
                        hand.z * f + sinf(angle) * radius };
}

/* Initial player animation covers 0..0.65; the unchanged 20-update actor
 * lead-in covers 0.65..1.0. Both meet at exactly the same green hand star. */
static inline void CwDinsFireRemake_BuildCharge(CwDinsMesh* mesh, float progress, uint32_t frame, CwDinsVec hand,
                                                CwDinsVec right, CwDinsVec up, uint32_t primary, uint32_t secondary) {
    mesh->count = 0;
    if (!isfinite(progress))
        return;
    progress = CwDins_Clamp(progress, 0, 1);
    float phase = (float)(frame & 4095u);
    float warm = CwDins_Clamp((progress - 0.72f) / 0.18f, 0, 1);
    uint32_t core = CwDins_Mix(0xC8FFE1, primary, warm);
    uint32_t edge = CwDins_Mix(0x29C876, secondary, warm);
    float opacity = (0.55f + 0.45f * progress) * (1.0f - 0.75f * warm);
    for (int trail = 0; trail < 2; ++trail) {
        for (int i = 0; i < 16; ++i) {
            float f0 = i / 16.0f, f1 = (i + 1) / 16.0f;
            CwDinsVec a = CwDins_TrailPoint(hand, phase, f0, progress, trail);
            CwDinsVec b = CwDins_TrailPoint(hand, phase, f1, progress, trail);
            CwDinsVec width = CwDins_Scale(right, 0.48f + 0.7f * f1);
            CwDinsQuad* q = CwDins_Quad(mesh, 1, core, edge, opacity * (0.25f + 0.75f * f1));
            q->p[0] = CwDins_Add(a, CwDins_Scale(width, -1));
            q->p[1] = CwDins_Add(a, width);
            q->p[2] = CwDins_Add(b, width);
            q->p[3] = CwDins_Add(b, CwDins_Scale(width, -1));
        }
        for (int i = 0; i < 4; ++i) {
            float f = fmodf(phase * 0.019f + i * 0.24f + trail * 0.12f, 1.0f);
            CwDinsVec p = CwDins_TrailPoint(hand, phase, f, progress, trail);
            CwDins_Billboard(mesh, p, right, up, 0.8f + f * 1.3f, 0.8f + f * 1.3f, 1, core, edge, opacity);
        }
    }
    float pulse = 1.0f + 0.14f * sinf(phase * 0.5f);
    float starSize = (3.0f + progress * 5.0f) * pulse * (1.0f - 0.45f * warm);
    CwDins_Billboard(mesh, hand, right, up, starSize, starSize, 1, core, edge, 0.8f);
    CwDins_Billboard(mesh, hand, right, up, starSize * 0.35f, starSize * 1.5f, 1, 0xFFFFFF, core, 0.9f);
    if (warm > 0.55f) {
        for (int i = 0; i < 3; ++i) {
            CwDinsVec p = hand;
            p.x += sinf(phase * 0.23f + i * 2.1f) * 1.2f;
            p.y += 3.0f + i * 1.2f;
            CwDins_FeatheredFlame(mesh, p, right, up, 3.0f + i * 0.7f, 5.5f + i * 1.1f,
                                  i == 0 ? CwDins_Mix(primary, 0xFFFFFF, 0.55f) : primary, secondary, warm,
                                  phase * (1.0f + i * 0.4f));
        }
    }
}

/* Share all four latitude rings, including the closing sector, so the flame
 * flows around a rounded, hollow volume. Three texture repeats span the entire
 * circumference; sideways material travel dominates a small vertical drift.
 * Unsigned 1/64-texel phases stay periodic through gameplay-frame rollover. */
static inline void CwDinsFireRemake_BuildShell(CwDinsMesh* mesh, uint32_t frame, float scale, float alpha, int fading,
                                               CwDinsVec right, CwDinsVec up, uint32_t primary, uint32_t secondary) {
    mesh->count = 0;
    if (!isfinite(scale) || !isfinite(alpha) || scale <= 0 || alpha <= 0)
        return;
    const float tau = 6.2831853f;
    const float ringRadius[4] = { 1.0f, 0.97f, 0.76f, 0.32f };
    const float ringHeight[4] = { 0.0f, 0.28f, 0.64f, 0.90f };
    const float ringOpacity[4] = { 1.0f, 0.92f, 0.70f, 0.08f };
    float phase = (float)(frame & 511u) * (tau / 512.0f);
    float sScroll = (float)((frame * 128u) & 4095u) / 64.0f;
    float tScroll = (float)((frame * 5u) & 4095u) / 64.0f;
    float radius = CwDins_Clamp(scale * 325.0f, 1.0f, 1200.0f);
    alpha = CwDins_Clamp(alpha, 0, 1);
    float thickness = fading ? 0.2f + 0.8f * alpha : 1.0f;
    CwDinsVec ring[4][12];
    for (int row = 0; row < 4; ++row) {
        for (int sector = 0; sector < 12; ++sector) {
            float angle = sector * (tau / 12.0f);
            float wave = sinf(angle * 2.0f + phase * 4.0f + row * 0.7f);
            float curl = row ? 0.06f * row + 0.035f * wave : 0;
            float height = ringHeight[row] + (row ? 0.018f * wave : 0);
            ring[row][sector] = (CwDinsVec){ cosf(angle + curl) * radius * ringRadius[row], 1.0f + radius * height,
                                             sinf(angle + curl) * radius * ringRadius[row] };
        }
    }
    for (int band = 0; band < 3; ++band) {
        for (int sector = 0; sector < 12; ++sector) {
            // A permutation over individual band patches breaks up the shell
            // without selecting whole upright columns. Every patch starts full.
            float patch = fading ? CwDins_Clamp((alpha - (((band * 12 + sector) * 7) % 36) / 48.0f) * 4.0f, 0, 1) : 1;
            if (patch <= 0)
                continue;
            CwDinsQuad* q = CwDins_Quad(mesh, 0,
                                        band == 0   ? CwDins_Mix(primary, 0xFFFFFF, 0.55f)
                                        : band == 1 ? CwDins_Mix(primary, 0xFFFFFF, 0.22f)
                                                    : primary,
                                        band == 0 ? CwDins_Mix(secondary, primary, 0.25f) : secondary, alpha * patch);
            int next = (sector + 1) % 12;
            q->p[0] = ring[band][sector];
            q->p[1] = ring[band][next];
            q->p[2] = ring[band + 1][next];
            q->p[3] = ring[band + 1][sector];
            q->s = sScroll + sector * 16.0f;
            q->t = tScroll + (2 - band) * 32.0f;
            q->sSpan = 16;
            q->tSpan = 32;
            q->alpha[0] = q->alpha[1] = (uint8_t)(alpha * patch * ringOpacity[band] * 255);
            q->alpha[2] = q->alpha[3] = (uint8_t)(alpha * patch * ringOpacity[band + 1] * 255);
        }
    }
    for (int i = 0; i < 8; ++i) {
        if (fading && i / 8.0f > alpha)
            continue;
        float angle = i * (tau / 8.0f) - phase;
        float height = 0.15f + ((i * 3) % 8) / 7.0f * 0.50f + 0.018f * sinf(phase * 4.0f + i);
        float r = radius * sqrtf(1.0f - height * height) * 1.012f;
        float width = 0.16f * thickness, halfHeight = radius * 0.025f * thickness;
        CwDinsQuad* q = CwDins_Quad(mesh, 0, primary, secondary, alpha * 0.32f);
        q->p[0] = (CwDinsVec){ cosf(angle - width) * r, 1 + radius * height - halfHeight, sinf(angle - width) * r };
        q->p[1] = (CwDinsVec){ cosf(angle + width) * r, 1 + radius * height - halfHeight, sinf(angle + width) * r };
        q->p[2] = (CwDinsVec){ cosf(angle + width + 0.04f) * r, 1 + radius * height + halfHeight,
                               sinf(angle + width + 0.04f) * r };
        q->p[3] = (CwDinsVec){ cosf(angle - width + 0.04f) * r, 1 + radius * height + halfHeight,
                               sinf(angle - width + 0.04f) * r };
        q->s = sScroll + i * 24.0f;
        q->t = tScroll + 32;
        q->sSpan = 16;
        q->tSpan = 16;
        q->alpha[2] = q->alpha[3] = (uint8_t)(alpha * 24);
        CwDinsVec p = { cosf(angle) * r, 1 + radius * (height + 0.05f), sinf(angle) * r };
        CwDins_Billboard(mesh, p, right, up, 0.7f + radius * 0.007f, 0.7f + radius * 0.007f, 1,
                         CwDins_Mix(primary, 0xFFFFFF, 0.45f), secondary, alpha * 0.65f);
    }
}

/* Native accepted recipe: one sample per actor update, never gameplayFrames.
 * Geometry and UV coefficients follow the preserved longer-hold/burnout study.
 * The caller supplies the actual billboard basis; no camera is requested here. */
static inline CwDinsVec CwDins_Sub(CwDinsVec a, CwDinsVec b) {
    return (CwDinsVec){ a.x - b.x, a.y - b.y, a.z - b.z };
}
static inline float CwDins_Dot(CwDinsVec a, CwDinsVec b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
static inline CwDinsVec CwDins_Cross(CwDinsVec a, CwDinsVec b) {
    return (CwDinsVec){ a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
static inline CwDinsVec CwDins_Normalize(CwDinsVec value) {
    return CwDins_Scale(value, 1.0f / fmaxf(.0001f, sqrtf(CwDins_Dot(value, value))));
}
static inline float CwDins_Smooth(float value) {
    float x = CwDins_Clamp(value, 0, 1);
    return x * x * (3 - 2 * x);
}
static inline uint32_t CwDins_NativeColor(uint32_t authored, uint32_t primary, uint32_t secondary) {
    if (primary == 0xFFC800 && secondary == 0xFF0000)
        return authored;
    float brightness =
        (((authored >> 16) & 255) * .299f + ((authored >> 8) & 255) * .587f + (authored & 255) * .114f) / 255.0f;
    return CwDins_Mix(secondary, primary, brightness);
}
static inline CwDinsQuad* CwDins_NativeQuad(CwDinsMesh* mesh, uint8_t material, uint32_t primary, uint32_t secondary,
                                            float opacity) {
    CwDinsQuad* q = CwDins_Quad(mesh, material, primary, secondary, opacity);
    if (q)
        q->sSpan = q->tSpan = 64;
    return q;
}
static inline void CwDins_NativeBillboard(CwDinsMesh* mesh, CwDinsVec at, CwDinsVec right, CwDinsVec up, float width,
                                          float height, uint8_t material, uint32_t primary, uint32_t secondary,
                                          float opacity) {
    size_t previous = mesh->count;
    CwDins_Billboard(mesh, at, right, up, width, height, material, primary, secondary, opacity);
    if (mesh->count > previous)
        mesh->quads[previous].sSpan = mesh->quads[previous].tSpan = 64;
}
static inline void CwDins_NativeGround(CwDinsMesh* mesh, float radius, float height, uint8_t material, uint32_t primary,
                                       uint32_t secondary, float opacity) {
    CwDinsQuad* q = CwDins_NativeQuad(mesh, material, primary, secondary, opacity);
    if (!q)
        return;
    q->p[0] = (CwDinsVec){ -radius, height, -radius };
    q->p[1] = (CwDinsVec){ radius, height, -radius };
    q->p[2] = (CwDinsVec){ radius, height, radius };
    q->p[3] = (CwDinsVec){ -radius, height, radius };
}

static inline void CwDinsFireRemake_BuildNativeCharge(CwDinsMesh* mesh, float time, CwDinsVec hand, CwDinsVec right,
                                                      CwDinsVec up, uint32_t primary, uint32_t secondary) {
    mesh->count = 0;
    if (!isfinite(time) || time < 0 || time >= 1.10f)
        return;
    right = CwDins_Normalize(right);
    up = CwDins_Normalize(up);
    CwDinsVec forward = CwDins_Normalize(CwDins_Cross(right, up));
    if (time < .75f) {
        float collapse = CwDins_Clamp((time - .43f) / .30f, 0, 1);
        for (int trail = 0; trail < 2; ++trail) {
            CwDinsVec path[25], low[25], high[25];
            for (int i = 0; i <= 24; ++i) {
                float u = i / 24.0f;
                float angle = time * 12 + u * 6.283185307179586f + trail * 3.141592653589793f;
                float radius = (1 - u) * 34 * (1 - .88f * collapse);
                path[i] = (CwDinsVec){ hand.x * u + cosf(angle) * radius, hand.y * u + (1 - u) * (19 + u * 47),
                                       hand.z * u + sinf(angle) * radius };
            }
            for (int i = 0; i <= 24; ++i) {
                CwDinsVec tangent = CwDins_Sub(path[i < 24 ? i + 1 : 24], path[i > 0 ? i - 1 : 0]);
                float width = (1.6f + 1.4f * i / 24.0f) * fminf(1, fminf((i + 1) / 4.0f, (25 - i) / 4.0f));
                CwDinsVec side = CwDins_Scale(CwDins_Normalize(CwDins_Cross(forward, tangent)), width);
                low[i] = CwDins_Sub(path[i], side);
                high[i] = CwDins_Add(path[i], side);
            }
            for (int i = 0; i < 24; ++i) {
                CwDinsQuad* q = CwDins_NativeQuad(mesh, 5, 0xFFFFFF, 0, .75f + collapse * .20f);
                if (!q)
                    return;
                q->p[0] = low[i];
                q->p[1] = low[i + 1];
                q->p[2] = high[i + 1];
                q->p[3] = high[i];
            }
            for (int i = 0; i < 5; ++i) {
                CwDinsVec point = path[(i * 5 + (int)(time * 25)) % 25];
                CwDins_NativeBillboard(mesh, point, right, up, 1.1f, 1.1f, 1, 0xDDFFF0, 0x29DA85, 1);
            }
        }
        float size = 3 + collapse * 5;
        CwDins_NativeBillboard(mesh, hand, right, up, size, size, 1, 0xE7FFF2, 0x34FF9F, 1);
        CwDins_NativeBillboard(mesh, hand, right, up, size * 2, size * 2, 4, 0x5DFFB4, 0xFF2408, .4f);
    }
    if (time >= .70f && time < .85f) {
        float relative = (time - .76f) / .055f;
        float flare = expf(-relative * relative);
        CwDins_NativeBillboard(mesh, hand, right, up, 75, .85f, 4, CwDins_NativeColor(0xFFD9D0, primary, secondary),
                               0xFF2408, flare);
        CwDins_NativeBillboard(mesh, hand, right, up, 44, 2.4f, 4, CwDins_NativeColor(0xFF6F58, primary, secondary),
                               0xFF2408, flare * .85f);
        CwDins_NativeBillboard(mesh, hand, right, up, 24, 6, 1, CwDins_NativeColor(0xFFFFFF, primary, secondary),
                               CwDins_NativeColor(0xFF2210, primary, secondary), flare);
        CwDins_NativeBillboard(mesh, hand, right, up, 5, 14, 1, CwDins_NativeColor(0xFFF7EA, primary, secondary),
                               CwDins_NativeColor(0xFF1207, primary, secondary), flare);
        CwDins_NativeBillboard(mesh, hand, right, up, 15, 15, 4, CwDins_NativeColor(0xFF2813, primary, secondary),
                               0xFF2408, flare * .7f);
    }
    if (time >= .82f && time < 1.0f) {
        CwDins_NativeBillboard(mesh, hand, right, up, 5, 5, 4, CwDins_NativeColor(0xFF1909, primary, secondary),
                               0xFF2408, .85f);
        CwDins_NativeBillboard(mesh, hand, right, up, 2, 3, 1, CwDins_NativeColor(0xFFE8CC, primary, secondary),
                               CwDins_NativeColor(0xFF1205, primary, secondary), .95f);
    }
    if (time >= 1.0f) {
        CwDins_NativeGround(mesh, 64, .4f, 4, CwDins_NativeColor(0xFF3812, primary, secondary), 0, .9f);
        CwDins_NativeBillboard(mesh, (CwDinsVec){ 0, 16, 0 }, right, up, 38, 28, 4,
                               CwDins_NativeColor(0xFF6524, primary, secondary), 0xFF2408, .8f);
    }
}

/* Grid and alpha are evaluated once per shared vertex, then copied into private
 * native Vtx storage. This keeps the curved cells' grazing/floor fades continuous. */
static inline void CwDins_NativeRibbonGrid(CwDinsVec grid[33][4], int band, float age, float radius, float peel) {
    static const float bands[6][6] = {
        { 1.02f, .17f, .28f, 5.65f, .2f, .03f }, { .99f, .39f, .31f, 5.25f, 2.2f, .12f },
        { .78f, .52f, .29f, 4.90f, 4.7f, .30f }, { .69f, .73f, .27f, 4.70f, 1.4f, .45f },
        { .38f, .87f, .23f, 4.25f, 4.0f, .65f }, { .52f, .42f, .28f, 4.10f, 3.0f, -.15f }
    };
    const float* b = bands[band];
    float rotation = age * (3.30f + band * .21f) + b[4];
    float length = (b[3] + .45f * sinf(age * 2.1f + band * 1.7f)) * (1 - .75f * peel);
    for (int k = 0; k <= 32; ++k) {
        float u = k / 32.0f;
        float angle = rotation + u * length;
        float radial = radius * (b[0] + .045f * sinf(angle * 2.1f - age * 1.5f + band) + peel * (.14f + .28f * u));
        float height = radius * (b[1] + .10f * sinf(angle * (1.1f + band * .17f) - age * 1.1f + band * 1.7f) +
                                 .13f * (u - .5f) + peel * (.12f + .22f * u));
        float taper = .48f + .52f * powf(fmaxf(0, sinf(3.141592653589793f * u)), .35f);
        float width =
            radius * b[2] * (.88f + .15f * sinf(angle * 3.1f - age * 2 + band)) * powf(1 - peel, 1.35f) * taper;
        float bend = b[5] + .20f * sinf(angle + age * 1.2f + band);
        for (int v = 0; v <= 3; ++v) {
            float across = 2 * v / 3.0f - 1;
            float roll = .20f * sinf(angle * 2 - age * 1.8f + band);
            float offsetR = -sinf(bend) * width * across - width * .32f * across * across;
            float offsetY = cosf(bend) * width * across + roll * width * (1 - across * across);
            offsetY += powf(fabsf(across), 3) * width * .10f * sinf(angle * 5.3f + age * 5 + band);
            grid[k][v] =
                (CwDinsVec){ cosf(angle) * (radial + offsetR), height + offsetY, sinf(angle) * (radial + offsetR) };
        }
    }
}
static inline float CwDins_RecipeBurn(float time, int band, float fadeDuration) {
    static const float starts[6] = { 3.13f, 3.11f, 3.09f, 3.07f, 3.05f, 3.10f };
    return powf(CwDins_Smooth((time - starts[band]) / (3.05f + fadeDuration - starts[band])), 1.35f);
}
static inline float CwDins_RecipeRadius(float age, float fadeAge, float fadeDuration) {
    float peel = CwDins_Clamp((age - fadeAge) / fadeDuration, 0, 1);
    float growth = .16f + .84f * CwDins_Smooth(age / (CW_DINS_NATIVE_GROW_TICKS / (float)CW_DINS_NATIVE_RATE));
    return .364362478f * (age >= fadeAge ? 1 + peel * .25f : growth) * 325 * 1.12f;
}
static inline float CwDins_NativeRadius(float age) {
    return CwDins_RecipeRadius(age, CW_DINS_NATIVE_FADE_AGE, CW_DINS_NATIVE_FADE_TICKS / (float)CW_DINS_NATIVE_RATE);
}
static inline void CwDins_NativeBurnEmbers(CwDinsMesh* mesh, float time, CwDinsVec right, CwDinsVec up,
                                           uint32_t primary, uint32_t secondary, float fadeAge, float fadeDuration) {
    static const int bands[6] = { 3, 4, 2, 3, 4, 1 };
    static const int columns[6] = { 5, 19, 11, 26, 7, 23 };
    float shift = fadeAge - 2.0f;
    for (int i = 0; i < 6; ++i) {
        float birth = 3.11f + i * .034f + shift, death = 3.55f + i * .016f + shift;
        if (time <= birth || time >= death)
            continue;
        /* Deterministic anchors interpolate the two preserved 20 Hz samples at
         * birth; there is no particle state and no gameplay RNG consumption. */
        float lower = floorf(birth * CW_DINS_NATIVE_RATE) / CW_DINS_NATIVE_RATE;
        float fraction = (birth - lower) * CW_DINS_NATIVE_RATE;
        CwDinsVec grids[2][33][4];
        for (int sample = 0; sample < 2; ++sample) {
            float age = lower + sample / (float)CW_DINS_NATIVE_RATE - CW_DINS_NATIVE_FIRE_START_TIME;
            CwDins_NativeRibbonGrid(grids[sample], bands[i], age, CwDins_RecipeRadius(age, fadeAge, fadeDuration),
                                    CwDins_Clamp((age - fadeAge) / fadeDuration, 0, 1));
        }
        CwDinsVec anchor = CwDins_Add(CwDins_Scale(grids[0][columns[i]][2], 1 - fraction),
                                      CwDins_Scale(grids[1][columns[i]][2], fraction));
        float age = time - birth, span = death - birth, u = age / span;
        float opacity = (.36f + .025f * (i % 3)) * CwDins_Smooth(age / .09f) * CwDins_Smooth((death - time) / .15f);
        float drift = atan2f(anchor.z, anchor.x) + .30f * sinf(i * 2.4f);
        CwDinsVec point = { anchor.x + cosf(drift) * age * 4, anchor.y + age * (22 + 1.5f * i),
                            anchor.z + sinf(drift) * age * 4 };
        float width = (.48f + .055f * (i % 3)) * (.9f + .1f * u);
        CwDins_NativeBillboard(mesh, point, right, up, width, width * 1.35f, 1,
                               CwDins_NativeColor(0xFFE9A3, primary, secondary),
                               CwDins_NativeColor(0xFF7410, primary, secondary), opacity);
    }
}
/* A parameterized recipe preserves the original recorded 40/12 appearance
 * proof. Native gameplay selects its longer hold below. Only the envelopes move:
 * every ribbon oscillator, texture scroll and ember anchor uses real fire age. */
static inline void CwDinsFireRemake_BuildFireRecipe(CwDinsMesh* mesh, int frame, int fadeStartFrame, int fadeFrames,
                                                    CwDinsVec right, CwDinsVec up, uint32_t primary,
                                                    uint32_t secondary) {
    mesh->count = 0;
    if (frame < 0 || fadeStartFrame < CW_DINS_NATIVE_GROW_TICKS || fadeFrames <= 0 ||
        frame >= fadeStartFrame + fadeFrames)
        return;
    right = CwDins_Normalize(right);
    up = CwDins_Normalize(up);
    CwDinsVec forward = CwDins_Normalize(CwDins_Cross(right, up));
    float age = frame / (float)CW_DINS_NATIVE_RATE, time = CW_DINS_NATIVE_FIRE_START_TIME + age;
    float fadeAge = fadeStartFrame / (float)CW_DINS_NATIVE_RATE;
    float fadeDuration = fadeFrames / (float)CW_DINS_NATIVE_RATE;
    float peel = CwDins_Clamp((age - fadeAge) / fadeDuration, 0, 1);
    float radius = CwDins_RecipeRadius(age, fadeAge, fadeDuration);
    float opacity = frame >= fadeStartFrame ? powf(1 - peel, .65f) : 1;
    uint32_t white = CwDins_NativeColor(0xFFFFFF, primary, secondary);
    float glowRadius = fmaxf(frame < 6 ? 125 : 38, radius * 1.35f);
    CwDins_NativeGround(mesh, glowRadius, .3f, 3, white, 0, opacity * .58f);
    CwDins_NativeBillboard(mesh, (CwDinsVec){ 0, radius * .48f, -radius * .15f }, right, up, fmaxf(50, radius * 1.04f),
                           fmaxf(45, radius * .85f), 3, white, 0xFF2408, opacity * .85f);
    CwDins_NativeBillboard(mesh, (CwDinsVec){ 0, radius * .16f, radius * .82f }, right, up, fmaxf(70, radius * 1.28f),
                           fmaxf(38, radius * .44f), 3, white, 0xFF2408, opacity * .95f);
    CwDins_NativeBillboard(mesh, (CwDinsVec){ radius * .10f, radius * .38f, radius * .28f }, right, up,
                           fmaxf(48, radius * .79f), fmaxf(38, radius * .55f), 3, white, 0xFF2408, opacity * .55f);
    for (int band = 0; band < 6; ++band) {
        CwDinsVec grid[33][4];
        uint8_t alpha[33][4];
        CwDins_NativeRibbonGrid(grid, band, age, radius, peel);
        for (int k = 0; k <= 32; ++k) {
            float wave = .56f + .12f * sinf(k / 32.0f * 6.283185307179586f * 2.1f + band * 1.8f + age);
            float tip = fminf(1, fminf(k / (32 * .13f), (32 - k) / (32 * .13f)));
            for (int v = 0; v <= 3; ++v) {
                CwDinsVec a = CwDins_Sub(grid[k < 32 ? k + 1 : 32][v], grid[k ? k - 1 : 0][v]);
                CwDinsVec b = CwDins_Sub(grid[k][v < 3 ? v + 1 : 3], grid[k][v ? v - 1 : 0]);
                CwDinsVec normal = CwDins_Normalize(CwDins_Cross(a, b));
                float grazing = CwDins_Smooth((fabsf(CwDins_Dot(normal, forward)) - .035f) / .30f);
                float edge = v == 0 || v == 3 ? 0 : powf(sinf(3.141592653589793f * (v / 3.0f)), .23f);
                alpha[k][v] = (uint8_t)CwDins_Clamp(
                    255 * opacity * wave * edge * tip * grazing * CwDins_Clamp(grid[k][v].y / 12, 0, 1), 0, 255);
            }
        }
        float burn =
            CwDins_RecipeBurn(3.05f + (frame - fadeStartFrame) / (float)CW_DINS_NATIVE_RATE, band, fadeDuration);
        for (int k = 0; k < 32; ++k) {
            for (int v = 0; v < 3; ++v) {
                CwDinsQuad* q = CwDins_NativeQuad(mesh, 2, white, 0, opacity);
                if (!q)
                    return;
                q->band = band;
                q->burn = burn;
                q->burnFrame = frame > fadeStartFrame ? frame : 0;
                q->p[0] = grid[k][v];
                q->p[1] = grid[k + 1][v];
                q->p[2] = grid[k + 1][v + 1];
                q->p[3] = grid[k][v + 1];
                q->s = 64 * (k / 32.0f - (q->burnFrame ? 0 : age * (.27f + band * .025f)));
                q->sSpan = 2;
                q->t = 64 * (1 - (v + 1) / 3.0f);
                q->tSpan = 64 / 3.0f;
                q->alpha[0] = alpha[k][v];
                q->alpha[1] = alpha[k + 1][v];
                q->alpha[2] = alpha[k + 1][v + 1];
                q->alpha[3] = alpha[k][v + 1];
            }
        }
    }
    if (frame < 6) {
        float flash = expf(-fmaxf(0, age - .10f) * 12);
        CwDins_NativeBillboard(mesh, (CwDinsVec){ 0, 40, 40 }, right, up, 170 + age * 200, 145 + age * 100, 3, white, 0,
                               flash);
        CwDins_NativeBillboard(mesh, (CwDinsVec){ 0, 42, 42 }, right, up, 100 + age * 100, 82 + age * 70, 4,
                               CwDins_NativeColor(0xFFFBD5, primary, secondary), 0xFF2408, flash);
    }
    for (int i = 0; i < 10; ++i) {
        float angle = i * 2.399f + age * .60f;
        float r = radius * (.70f + .25f * sinf(i * 4.2f + age));
        float height = radius * (.15f + fmodf(i * .217f + age * .19f, 1) * 1.15f);
        float width = .65f + ((i * 3) % 5) * .12f;
        CwDins_NativeBillboard(mesh, (CwDinsVec){ cosf(angle) * r, height, sinf(angle) * r }, right, up, width, width,
                               1, CwDins_NativeColor(0xFFF7C4, primary, secondary),
                               CwDins_NativeColor(0xFFA512, primary, secondary), opacity * .85f);
    }
    CwDins_NativeBurnEmbers(mesh, time, right, up, primary, secondary, fadeAge, fadeDuration);
    if (primary != 0xFFC800 || secondary != 0xFF0000) {
        /* Colored atlas materials use neutral companions for editor palettes.
         * Keep both endpoints exact; multiplying the orange artwork by the
         * primary color would discard secondary and contaminate blue/green. */
        for (size_t i = 0; i < mesh->count; ++i) {
            CwDinsQuad* q = &mesh->quads[i];
            if (q->spark == 2 || q->spark == 3) {
                q->palette = 1;
                q->primary = primary;
                q->secondary = secondary;
            }
        }
    }
}
static inline void CwDinsFireRemake_BuildNativeFire(CwDinsMesh* mesh, int frame, CwDinsVec right, CwDinsVec up,
                                                    uint32_t primary, uint32_t secondary) {
    CwDinsFireRemake_BuildFireRecipe(mesh, frame, CW_DINS_NATIVE_FADE_START_FRAME, CW_DINS_NATIVE_FADE_TICKS, right, up,
                                     primary, secondary);
}
#endif
