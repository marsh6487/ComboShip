// TU glue: include after native engine/GBI declarations.
#ifndef COMBO_SPIN_ATTACK_ACCENTS_DRAW_H
#define COMBO_SPIN_ATTACK_ACCENTS_DRAW_H
#include "SpinAttackAccents.h"

static inline void* CwSpinDraw_Alloc(GraphicsContext* gfx, size_t bytes) {
#ifdef CW_SPIN_HOST_MM
    return GRAPH_ALLOC(gfx, bytes);
#else
    return Graph_Alloc(gfx, bytes);
#endif
}

static inline void CwSpinAccents_DrawPass(PlayState* play, uint8_t element, int great, float phase, float alpha,
                                          size_t limit, uint32_t tint, int charge) {
    CwSpinAccentParticle particles[24];
    if (limit > 24)
        limit = 24;
    size_t count = charge ? CwSpinAccents_BuildCharge(element, great, phase, alpha, particles, limit)
                          : CwSpinAccents_Build(element, great, phase, alpha, particles, limit);
    if (count == 0)
        return;
    unsigned spokes = element == 3 ? 8 : 4;
    unsigned stride = spokes + 1;
    Vtx* vertices = (Vtx*)CwSpinDraw_Alloc(play->state.gfxCtx, count * stride * sizeof(Vtx));
    if (vertices == NULL)
        return;
    memset(vertices, 0, count * stride * sizeof(Vtx));
    for (size_t i = 0; i < count; ++i) {
        CwSpinAccentParticle* p = &particles[i];
        if (tint != 0) {
            p->r = (uint8_t)(tint >> 16);
            p->g = (uint8_t)(tint >> 8);
            p->b = (uint8_t)tint;
        }
        float heading = atan2f(p->z, p->x);
        for (unsigned v = 0; v < stride; ++v) {
            float theta = v == 0 ? 0 : (float)(v - 1) * 6.28318531f / spokes;
            float radius = v == 0 ? 0 : p->size * ((element == 3 && (v & 1)) ? 0.3f : 1.0f);
            float dx = cosf(theta) * radius;
            Vtx* vertex = &vertices[i * stride + v];
            vertex->v.ob[0] = (int16_t)(p->x + cosf(heading) * dx);
            vertex->v.ob[1] = (int16_t)(p->y + sinf(theta) * radius * (element == 1 ? 1.5f : 1));
            vertex->v.ob[2] = (int16_t)(p->z + sinf(heading) * dx);
            vertex->v.cn[0] = p->r;
            vertex->v.cn[1] = p->g;
            vertex->v.cn[2] = p->b;
            vertex->v.cn[3] = v == 0 ? p->a : (element == 2 ? p->a / 3 : 0);
        }
    }
    OPEN_DISPS(play->state.gfxCtx);
    gDPPipeSync(POLY_XLU_DISP++);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING | G_CULL_BOTH);
    gDPSetCombineMode(POLY_XLU_DISP++, G_CC_SHADE, G_CC_SHADE);
    for (size_t i = 0; i < count; ++i) {
        gSPVertex(POLY_XLU_DISP++, (uintptr_t)(vertices + i * stride), stride, 0);
        for (unsigned v = 1; v <= spokes; ++v)
            gSP1Triangle(POLY_XLU_DISP++, 0, v, v == spokes ? 1 : v + 1, 0);
    }
    // Subsequent charge glows/actors start from the same native XLU setup.
#ifdef CW_SPIN_HOST_MM
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
#else
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
#endif
    CLOSE_DISPS(play->state.gfxCtx);
}

static inline void CwSpinAccents_DrawLimited(PlayState* play, uint8_t element, int great, float phase, float alpha,
                                             size_t limit, uint32_t tint) {
    CwSpinAccents_DrawPass(play, element, great, phase, alpha, limit, tint, 0);
}

static inline void CwSpinAccents_DrawCharge(PlayState* play, uint8_t element, int great, float phase, float alpha) {
    CwSpinAccents_DrawPass(play, element, great, phase, alpha, great ? 8 : 4, 0, 1);
}

static inline void CwSpinAccents_Draw(PlayState* play, uint8_t element, int great, float phase, float alpha) {
    CwSpinAccents_DrawLimited(play, element, great, phase, alpha, great ? 24 : 12, 0);
}
#endif
