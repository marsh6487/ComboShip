/* Spell-owned ground electricity. No collision, gameplay RNG or shared effect textures are changed. */
#ifndef COMBO_DEMISE_SPELL_FX_H
#define COMBO_DEMISE_SPELL_FX_H

#define DEMISE_FX_QUADS 384
#define DEMISE_FX_LIFETIME 28

typedef struct {
    PlayState* play;
    Vec3f origin;
    s16 scene;
    u8 phase;
    u32 lastFrame;
    u32 impactFrame;
} DemiseFxState;

static DemiseFxState sDemiseFx;

/* Native I8 upload/UV domain; a soft strip, not a replacement for the shared bomb flipbook.
 * The slight longitudinal variation scrolls through each ribbon independently of its geometry. */
#define DEMISE_FX_ROW(a, b) 0, 12, a, b, b, a, 12, 0
static const u8 sDemiseFxStrip[8 * 16] = {
    DEMISE_FX_ROW(72, 224), DEMISE_FX_ROW(80, 240), DEMISE_FX_ROW(92, 255), DEMISE_FX_ROW(80, 240),
    DEMISE_FX_ROW(68, 216), DEMISE_FX_ROW(80, 240), DEMISE_FX_ROW(92, 255), DEMISE_FX_ROW(84, 248),
    DEMISE_FX_ROW(72, 224), DEMISE_FX_ROW(84, 248), DEMISE_FX_ROW(92, 255), DEMISE_FX_ROW(80, 240),
    DEMISE_FX_ROW(68, 216), DEMISE_FX_ROW(80, 240), DEMISE_FX_ROW(92, 255), DEMISE_FX_ROW(80, 240),
};
#undef DEMISE_FX_ROW

static void DemiseFx_Clear(void) {
    sDemiseFx.phase = 0;
}

static void DemiseFx_Begin(PlayState* play, s16 scene, Vec3f* origin) {
    sDemiseFx = (DemiseFxState){ play, *origin, scene, 1, play->gameplayFrames, 0 };
}

static void DemiseFx_Release(PlayState* play, s16 scene, Vec3f* origin) {
    sDemiseFx = (DemiseFxState){ play, *origin, scene, 2, play->gameplayFrames, play->gameplayFrames };
}

static void DemiseFx_Line(Vtx* vertices, s32* count, Vec3f a, Vec3f b, f32 width, Color_RGBA8 color, s16 scroll) {
    f32 dx = b.x - a.x;
    f32 dz = b.z - a.z;
    f32 length = sqrtf(dx * dx + dz * dz);
    if (*count >= DEMISE_FX_QUADS || length < 0.01f || color.a == 0) {
        return;
    }
    f32 nx = -dz * width / length;
    f32 nz = dx * width / length;
    Vec3f corners[4] = {
        { a.x - nx, a.y, a.z - nz },
        { a.x + nx, a.y, a.z + nz },
        { b.x + nx, b.y, b.z + nz },
        { b.x - nx, b.y, b.z - nz },
    };
    for (s32 i = 0; i < 4; ++i) {
        Vtx* v = &vertices[*count * 4 + i];
        v->v.ob[0] = (s16)corners[i].x;
        v->v.ob[1] = (s16)corners[i].y;
        v->v.ob[2] = (s16)corners[i].z;
        v->v.flag = 0;
        v->v.tc[0] = (i == 1 || i == 2) ? 7 * 32 : 0;
        v->v.tc[1] = scroll + (i >= 2 ? 16 * 32 : 0);
        v->v.cn[0] = color.r;
        v->v.cn[1] = color.g;
        v->v.cn[2] = color.b;
        v->v.cn[3] = color.a;
    }
    ++*count;
}

static void DemiseFx_Arc(Vtx* vertices, s32* count, Vec3f a, Vec3f b, u8 alpha, s16 scroll) {
    DemiseFx_Line(vertices, count, a, b, 10.0f, (Color_RGBA8){ 142, 40, 244, (u8)(alpha * 3 / 5) }, scroll);
    a.y += 0.5f;
    b.y += 0.5f;
    DemiseFx_Line(vertices, count, a, b, 3.6f, (Color_RGBA8){ 242, 220, 255, alpha }, scroll);
}

static void DemiseFx_Ring(Vtx* vertices, s32* count, f32 radius, f32 width, Color_RGBA8 color, s16 scroll) {
    Vec3f previous = { 0, 2.0f, radius };
    for (s32 i = 1; i <= 40; ++i) {
        s16 angle = (s16)(i * 0x10000 / 40);
        Vec3f next = { Math_SinS(angle) * radius, 2.0f, Math_CosS(angle) * radius };
        DemiseFx_Line(vertices, count, previous, next, width, color, scroll + i * 32);
        previous = next;
    }
}

/* Produces the same bounded packets for both hosts and for the offline geometry preview. */
static s32 DemiseFx_Build(Vtx* vertices, s16 timer, u32 frame, u8 phase, u32 age) {
    s32 count = 0;
    f32 charge = (timer - 20) / 50.0f;
    f32 fade = phase == 2 ? (DEMISE_FX_LIFETIME - age) / (f32)DEMISE_FX_LIFETIME : charge;
    if (phase == 0 || (phase == 1 && timer <= 20) || (phase == 2 && age >= DEMISE_FX_LIFETIME)) {
        return 0;
    }
    if (charge > 1.0f) {
        charge = 1.0f;
    }
    u8 sealAlpha = (u8)(210.0f * fade);
    s16 scroll = (s16)((frame & 0xFF) * 64);
    Color_RGBA8 seal = { 185, 65, 255, sealAlpha };
    DemiseFx_Ring(vertices, &count, 68.0f, 2.4f, seal, 0);
    DemiseFx_Ring(vertices, &count, 87.0f, 2.0f, seal, 0);
    for (s32 i = 0; i < 12; ++i) {
        s16 angle = (s16)(i * 0x10000 / 12);
        Vec3f a = { Math_SinS(angle) * 72.0f, 2.0f, Math_CosS(angle) * 72.0f };
        Vec3f b = { Math_SinS(angle) * 83.0f, 2.0f, Math_CosS(angle) * 83.0f };
        DemiseFx_Line(vertices, &count, a, b, 2.0f, seal, 0);
    }

    f32 radius = phase == 2 ? (age < 5 ? 170.0f + age * 42.0f : 380.0f) : 110.0f + charge * 70.0f;
    u8 alpha = (u8)(phase == 2 ? 255.0f * fade : 220.0f * charge);
    for (s32 root = 0; root < 6; ++root) {
        s16 angle = (s16)(root * 0x10000 / 6);
        f32 sx = Math_SinS(angle);
        f32 sz = Math_CosS(angle);
        Vec3f previous = { sx * 20.0f, 3.0f, sz * 20.0f };
        for (s32 joint = 1; joint <= 7; ++joint) {
            f32 distance = 20.0f + (radius - 20.0f) * joint / 7.0f;
            f32 kink = Math_SinS((s16)(root * 0x2500 + joint * 0x6700 + (frame / 3) * 0x1900)) * 14.0f;
            Vec3f next = { sx * distance + sz * kink, 3.0f, sz * distance - sx * kink };
            u8 pulse = (u8)(alpha * (0.78f + 0.22f * Math_SinS((s16)(joint * 0x1800 + frame * 0x2000))));
            DemiseFx_Arc(vertices, &count, previous, next, pulse, scroll + root * 70);
            if (joint == 3 || joint == 5) {
                f32 side = joint == 3 ? 1.0f : -1.0f;
                Vec3f fork = { next.x + sx * 22.0f + sz * 27.0f * side, 3.0f, next.z + sz * 22.0f - sx * 27.0f * side };
                Vec3f tip = { fork.x + sx * 19.0f - sz * 10.0f * side, 3.0f, fork.z + sz * 19.0f + sx * 10.0f * side };
                DemiseFx_Arc(vertices, &count, next, fork, pulse / 2, -scroll);
                DemiseFx_Arc(vertices, &count, fork, tip, pulse / 3, -scroll);
            }
            previous = next;
        }
    }
    if (phase == 2 && age < 11) {
        f32 waveRadius = 48.0f + age * 34.0f;
        u8 waveAlpha = (u8)(240 * (11 - age) / 11);
        DemiseFx_Ring(vertices, &count, waveRadius, 12.0f, (Color_RGBA8){ 142, 40, 244, waveAlpha / 2 }, scroll);
        DemiseFx_Ring(vertices, &count, waveRadius, 3.6f, (Color_RGBA8){ 242, 220, 255, waveAlpha }, scroll);
    }
    if (phase == 2 && age < 3) {
        DemiseFx_Ring(vertices, &count, 12.0f + age * 5.0f, 3.5f,
                      (Color_RGBA8){ 244, 224, 255, (u8)(230 * (3 - age) / 3) }, scroll);
    }
    /* Thirteen dissipating motes replace the old center-plus-twelve visual bursts.
     * They're draw-only and continue after the unchanged collider has been turned off. */
    for (s32 i = 0; i < 13; ++i) {
        s16 angle = (s16)(i * 0x10000 / 13 + 0x900);
        f32 distance = phase == 2 ? 40.0f + (i % 4) * 64.0f + age * 3.0f : 160.0f - ((frame + i * 3) % 24) * 5.0f;
        f32 height = phase == 2 ? 6.0f + age * 1.7f : 6.0f + ((frame + i * 3) % 17);
        Vec3f a = { Math_SinS(angle) * distance - 3.0f, height, Math_CosS(angle) * distance - 3.0f };
        Vec3f b = { a.x + 6.0f, height, a.z + 6.0f };
        DemiseFx_Line(vertices, &count, a, b, 2.0f, (Color_RGBA8){ 230, 190, 255, alpha }, scroll);
    }
    return count;
}

static void DemiseFx_Draw(PlayState* play, s16 scene, s16 timer, u8 active, u8 equipped) {
    if (!sDemiseFx.phase) {
        return;
    }
    if (sDemiseFx.play != play || sDemiseFx.scene != scene || play->gameplayFrames < sDemiseFx.lastFrame || !equipped ||
        (sDemiseFx.phase == 1 && !active)) {
        DemiseFx_Clear();
        return;
    }
    sDemiseFx.lastFrame = play->gameplayFrames;
    u32 age = sDemiseFx.phase == 2 ? play->gameplayFrames - sDemiseFx.impactFrame : 0;
    if ((sDemiseFx.phase == 1 && timer <= 20) || (sDemiseFx.phase == 2 && age >= DEMISE_FX_LIFETIME)) {
        if (sDemiseFx.phase == 2) {
            DemiseFx_Clear();
        }
        return;
    }
    GraphicsContext* gfx = play->state.gfxCtx;
    Vtx* vertices = DEMISE_FX_ALLOC(gfx, DEMISE_FX_QUADS * 4 * sizeof(Vtx));
    if (vertices == NULL) {
        return;
    }
    s32 count = DemiseFx_Build(vertices, timer, play->gameplayFrames, sDemiseFx.phase, age);
    size_t commandBytes = (count + (count + 7) / 8 + 40) * sizeof(Gfx);
    uintptr_t commandHead = (uintptr_t)gfx->polyXlu.p;
    uintptr_t commandEnd = (uintptr_t)gfx->polyXlu.d;
    if (commandEnd < commandHead || commandEnd - commandHead < commandBytes) {
        return;
    }
    MtxF floor;
    /* Raise the ray's origin as the native shockwave does, then retain its floor normal.
     * All geometry stays slightly above that plane, without changing scene collision. */
    Vec3f probe = sDemiseFx.origin;
    probe.y += 20.0f;
    DEMISE_FX_FLOOR(play, &floor, &probe);
    FrameInterpolation_RecordOpenChild(&sDemiseFx, sDemiseFx.phase);
    Mtx* matrix = SkinMatrix_MtxFToNewMtx(gfx, &floor);
    if (matrix == NULL || count == 0) {
        FrameInterpolation_RecordCloseChild();
        return;
    }
    OPEN_DISPS(gfx);
    DEMISE_FX_SETUP(gfx);
    // SetupDL25's "Xlu" suffix selects a buffer, but its material is opaque fog.
    // Select the ribbon material explicitly so texture edges and lifetime alpha blend.
    gDPSetCycleType(POLY_XLU_DISP++, G_CYC_1CYCLE);
    gDPSetRenderMode(POLY_XLU_DISP++, G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2);
    gDPSetAlphaCompare(POLY_XLU_DISP++, G_AC_NONE);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING | G_CULL_BACK | G_CULL_FRONT | G_FOG);
    gDPSetCombineMode(POLY_XLU_DISP++, G_CC_MODULATEIA, G_CC_MODULATEIA);
    gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPLoadTextureBlock(POLY_XLU_DISP++, sDemiseFxStrip, G_IM_FMT_I, G_IM_SIZ_8b, 8, 16, 0, G_TX_CLAMP, G_TX_WRAP, 3, 4,
                        G_TX_NOLOD, G_TX_NOLOD);
    gSPMatrix(POLY_XLU_DISP++, matrix, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    for (s32 first = 0; first < count; first += 8) {
        s32 batch = count - first;
        if (batch > 8) {
            batch = 8;
        }
        gSPVertex(POLY_XLU_DISP++, vertices + first * 4, batch * 4, 0);
        for (s32 q = 0; q < batch; ++q) {
            s32 v = q * 4;
            gSP2Triangles(POLY_XLU_DISP++, v, v + 1, v + 2, 0, v, v + 2, v + 3, 0);
        }
    }
    /* Restore the host's normal textured XLU setup for the next item/effect. */
    DEMISE_FX_SETUP(gfx);
    CLOSE_DISPS(gfx);
    FrameInterpolation_RecordCloseChild();
}

#undef DEMISE_FX_ALLOC
#undef DEMISE_FX_FLOOR
#undef DEMISE_FX_SETUP
#endif
