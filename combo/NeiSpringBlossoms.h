#ifndef NEI_SPRING_BLOSSOMS_H
#define NEI_SPRING_BLOSSOMS_H

// Shared native geometry and material for both games. Call after the host's
// Vtx/GBI definitions. Unique native resources resolve without scene textures.
// Texture art is RGBA32, physically 256 square / logically 32 square (4096 TMEM
// bytes). Texture, shade and particle fade alpha multiply in two cycles.
static const ALIGN_ASSET(2) char sSpringBlossomTexture[] = "__OTR__objects/object_spring_blossoms/blossom_tex";
static const ALIGN_ASSET(2) char sSpringPetalTexture[] = "__OTR__objects/object_spring_blossoms/petal_tex";

static inline Gfx* NeiSpringBlossoms_SetMaterial(Gfx* cursor, unsigned index) {
    const char* texture = (index & 3) == 0 ? sSpringBlossomTexture : sSpringPetalTexture;
    gDPPipeSync(cursor++);
    gDPSetCycleType(cursor++, G_CYC_2CYCLE);
    gSPTexture(cursor++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetTextureLUT(cursor++, G_TT_NONE);
    gDPSetTextureFilter(cursor++, G_TF_BILERP);
    gDPSetCombineLERP(cursor++, TEXEL0, 0, SHADE, 0, TEXEL0, 0, SHADE, 0, COMBINED, 0, PRIMITIVE, 0, COMBINED, 0,
                      PRIMITIVE, 0);
    gDPLoadTextureBlock(cursor++, texture, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0, G_TX_CLAMP, G_TX_CLAMP, 5, 5,
                        G_TX_NOLOD, G_TX_NOLOD);
    return cursor;
}
// Five rounded blush petals follow cor's reference. The authored alpha art
// supplies the localized crimson core and fine gold anthers, avoiding raised
// polygon overlays. The single petal has no heart cleft.
static Vtx sSpringBlossomVertices[] = {
    VTX(0, 0, 28, 512, 512, 255, 255, 255, 255),      VTX(-56, 78, 18, 333, 262, 255, 255, 255, 255),
    VTX(-57, 129, 14, 330, 99, 255, 255, 255, 255),   VTX(-33, 154, 4, 406, 19, 255, 250, 252, 255),
    VTX(0, 160, -4, 512, 0, 255, 250, 252, 255),      VTX(33, 154, 4, 618, 19, 255, 250, 252, 255),
    VTX(57, 129, 14, 694, 99, 255, 255, 255, 255),    VTX(56, 78, 18, 691, 262, 255, 255, 255, 255),
    VTX(105, 94, 14, 848, 211, 255, 255, 255, 255),   VTX(136, 79, 4, 947, 259, 255, 250, 252, 255),
    VTX(152, 49, -4, 998, 355, 255, 250, 252, 255),   VTX(156, 16, 4, 1011, 461, 255, 250, 252, 255),
    VTX(140, -15, 14, 960, 560, 255, 255, 255, 255),  VTX(91, -30, 18, 803, 608, 255, 255, 255, 255),
    VTX(122, -70, 14, 902, 736, 255, 255, 255, 255),  VTX(117, -105, 4, 886, 848, 255, 250, 252, 255),
    VTX(94, -129, -4, 813, 925, 255, 250, 252, 255),  VTX(64, -143, 4, 717, 970, 255, 250, 252, 255),
    VTX(29, -138, 14, 605, 954, 255, 255, 255, 255),  VTX(0, -96, 18, 512, 819, 255, 255, 255, 255),
    VTX(-29, -138, 14, 419, 954, 255, 255, 255, 255), VTX(-64, -143, 4, 307, 970, 255, 250, 252, 255),
    VTX(-94, -129, -4, 211, 925, 255, 250, 252, 255), VTX(-117, -105, 4, 138, 848, 255, 250, 252, 255),
    VTX(-122, -71, 14, 122, 739, 255, 255, 255, 255), VTX(-91, -30, 18, 221, 608, 255, 255, 255, 255),
    VTX(-140, -15, 14, 64, 560, 255, 255, 255, 255),  VTX(-156, 16, 4, 13, 461, 255, 250, 252, 255),
    VTX(-152, 49, -4, 26, 355, 255, 250, 252, 255),   VTX(-136, 79, 4, 77, 259, 255, 250, 252, 255),
    VTX(-105, 94, 14, 176, 211, 255, 255, 255, 255),
};
static Gfx sSpringBlossomGeometry[] = {
    gsSPVertex(sSpringBlossomVertices, 31, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(0, 3, 4, 0, 0, 4, 5, 0),
    gsSP2Triangles(0, 5, 6, 0, 0, 6, 7, 0),
    gsSP2Triangles(0, 7, 8, 0, 0, 8, 9, 0),
    gsSP2Triangles(0, 9, 10, 0, 0, 10, 11, 0),
    gsSP2Triangles(0, 11, 12, 0, 0, 12, 13, 0),
    gsSP2Triangles(0, 13, 14, 0, 0, 14, 15, 0),
    gsSP2Triangles(0, 15, 16, 0, 0, 16, 17, 0),
    gsSP2Triangles(0, 17, 18, 0, 0, 18, 19, 0),
    gsSP2Triangles(0, 19, 20, 0, 0, 20, 21, 0),
    gsSP2Triangles(0, 21, 22, 0, 0, 22, 23, 0),
    gsSP2Triangles(0, 23, 24, 0, 0, 24, 25, 0),
    gsSP2Triangles(0, 25, 26, 0, 0, 26, 27, 0),
    gsSP2Triangles(0, 27, 28, 0, 0, 28, 29, 0),
    gsSP2Triangles(0, 29, 30, 0, 0, 30, 1, 0),
    gsSPEndDisplayList(),
};

static Vtx sSpringPetalVertices[] = {
    VTX(0, 10, 34, 512, 480, 255, 255, 255, 255),    VTX(0, -150, 3, 512, 992, 255, 250, 252, 255),
    VTX(46, -130, 3, 659, 928, 255, 250, 252, 255),  VTX(80, -90, 18, 768, 800, 255, 255, 255, 255),
    VTX(94, -15, 24, 813, 560, 255, 255, 255, 255),  VTX(86, 73, 12, 787, 278, 255, 255, 255, 255),
    VTX(53, 130, -6, 682, 96, 255, 250, 252, 255),   VTX(0, 147, -15, 512, 42, 255, 250, 252, 255),
    VTX(-44, 131, -12, 371, 93, 255, 250, 252, 255), VTX(-74, 85, 0, 275, 240, 255, 250, 252, 255),
    VTX(-91, 0, 15, 221, 512, 255, 255, 255, 255),   VTX(-78, -78, 18, 262, 762, 255, 255, 255, 255),
    VTX(-40, -125, 6, 384, 912, 255, 255, 255, 255),
};
static Gfx sSpringPetalGeometry[] = {
    gsSPVertex(sSpringPetalVertices, 13, 0),   gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(0, 3, 4, 0, 0, 4, 5, 0),    gsSP2Triangles(0, 5, 6, 0, 0, 6, 7, 0),
    gsSP2Triangles(0, 7, 8, 0, 0, 8, 9, 0),    gsSP2Triangles(0, 9, 10, 0, 0, 10, 11, 0),
    gsSP2Triangles(0, 11, 12, 0, 0, 12, 1, 0), gsSPEndDisplayList(),
};

static inline Gfx* NeiSpringBlossoms_Geometry(unsigned index) {
    // 24 intact blossoms and 72 smaller loose petals within the existing
    // 96-slot layer. Selection is stable across pause, redraw and recycling.
    return (index & 3) == 0 ? sSpringBlossomGeometry : sSpringPetalGeometry;
}

static inline const u8* NeiSpringBlossoms_Color(unsigned index) {
    static const u8 colors[][3] = {
        { 255, 229, 239 },
        { 255, 214, 230 },
        { 255, 238, 245 },
        { 255, 204, 225 },
    };
    // Intact flowers span every blush variant, rather than sharing one tint.
    return colors[(index + index / 4) & 3];
}
#endif
