#ifndef OBJECT_SPRING_PETALS_H
#define OBJECT_SPRING_PETALS_H
#include "global.h"

// A reusable curved cherry-blossom petal, with two rounded lobes, a shallow
// cleft and tapered base. No native texture, autumn artwork or asset pack is used.
static Vtx sSpringPetalVertices[] = {
    VTX(0, 0, 12, 0, 0, 255, 255, 255, 255),     VTX(0, -160, 0, 0, 0, 225, 210, 215, 255),
    VTX(72, -112, 2, 0, 0, 242, 233, 236, 255),  VTX(124, -24, 5, 0, 0, 255, 246, 249, 255),
    VTX(130, 74, 1, 0, 0, 255, 252, 253, 255),   VTX(86, 145, -4, 0, 0, 255, 255, 255, 255),
    VTX(34, 151, -7, 0, 0, 255, 255, 255, 255),  VTX(0, 116, -5, 0, 0, 250, 241, 245, 255),
    VTX(-34, 151, -7, 0, 0, 255, 255, 255, 255), VTX(-86, 145, -4, 0, 0, 255, 255, 255, 255),
    VTX(-130, 74, 1, 0, 0, 255, 252, 253, 255),  VTX(-124, -24, 5, 0, 0, 255, 246, 249, 255),
    VTX(-72, -112, 2, 0, 0, 242, 233, 236, 255),
};
static Gfx sSpringPetalGeometry[] = {
    gsSPVertex(sSpringPetalVertices, 13, 0),   gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(0, 3, 4, 0, 0, 4, 5, 0),    gsSP2Triangles(0, 5, 6, 0, 0, 6, 7, 0),
    gsSP2Triangles(0, 7, 8, 0, 0, 8, 9, 0),    gsSP2Triangles(0, 9, 10, 0, 0, 10, 11, 0),
    gsSP2Triangles(0, 11, 12, 0, 0, 12, 1, 0), gsSPEndDisplayList(),
};

static void SpringPetals_Draw(PlayState* play, unsigned index, u8 alpha) {
    static const u8 colors[][3] = { { 255, 184, 205 }, { 255, 224, 232 }, { 255, 244, 220 }, { 255, 202, 219 } };
    const u8* color = colors[index & 3];
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    gDPPipeSync(POLY_XLU_DISP++);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING | G_CULL_BACK);
    gSPTexture(POLY_XLU_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetCombineLERP(POLY_XLU_DISP++, SHADE, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, 0, 0, 0, COMBINED, 0, 0, 0,
                      COMBINED);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, color[0], color[1], color[2], alpha);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, sSpringPetalGeometry);
    CLOSE_DISPS(gfxCtx);
}
#endif
