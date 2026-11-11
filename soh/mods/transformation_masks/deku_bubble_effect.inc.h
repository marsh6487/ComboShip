// OoT host adaptation of MM Effect_Ss_Sbn (Popped Deku Bubble). The native
// static-world impact owns this decal; actors/dynapoly and expiry do not.
// Snapshot the surface frame at impact so no collision-poly pointer survives
// room/scene teardown. Assets remain owned by the strict MM resource graph.
struct MmDekuBubbleSplat {
    MtxF surface;
    s16 life, scale, rotation, scroll, scrollStep, texture;
    u8 alpha, sliding;
};
static MmDekuBubbleSplat sDekuBubbleSplats[8];

// MM SoundSource_PlaySfxAtFixedWorldPos uses a 20-frame, projected emitter.
// Keep each impact position stable if another bubble is fired immediately.
struct MmDekuBubblePopSound {
    Vec3f world, projected;
    s16 life;
};
static MmDekuBubblePopSound sDekuBubblePopSounds[8];

static void MmForm_ProjectBubblePosition(PlayState* play, Vec3f* world, Vec3f* projected) {
    f32 w;
    SkinMatrix_Vec3fMtxFMultXYZW(&play->viewProjectionMtxF, world, projected, &w);
}

static void MmForm_PlayBubblePop(PlayState* play, Vec3f* position) {
    if (!MmSfx_IsAvailable())
        return;
    auto* emitter = &sDekuBubblePopSounds[0];
    for (auto& candidate : sDekuBubblePopSounds) {
        if (candidate.life < emitter->life)
            emitter = &candidate;
    }
    if (emitter->life != 0)
        MmSfx_StopAtPos(MM_NA_SE_IT_DEKUNUTS_BUBLE_VANISH, &emitter->projected);
    emitter->world = *position;
    emitter->life = 20;
    MmForm_ProjectBubblePosition(play, &emitter->world, &emitter->projected);
    MmSfx_PlayAtPos(MM_NA_SE_IT_DEKUNUTS_BUBLE_VANISH, &emitter->projected);
}

// MM func_800C0094. OoT's similarly named helper has a different fallback
// at +/-X walls, with non-unit tangents that stretch this native effect.
static void MmForm_BubbleSurfaceFrame(CollisionPoly* poly, f32 tx, f32 ty, f32 tz, MtxF* dest) {
    f32 nx;
    f32 ny;
    f32 nz;
    f32 z_f14;
    f32 phi_f14;
    f32 phi_f12;
    f32 inv_z_f14;

    if (poly == NULL) {
        return;
    }
    nx = poly->normal.x * (1.0f / 32767.0f);
    ny = poly->normal.y * (1.0f / 32767.0f);
    nz = poly->normal.z * (1.0f / 32767.0f);

    z_f14 = sqrtf(SQ(ny) + SQ(nz));
    if (!IS_ZERO(z_f14)) {
        inv_z_f14 = 1.0f / z_f14;
        phi_f14 = ny * inv_z_f14;
        phi_f12 = nz * inv_z_f14;
    } else {
        phi_f14 = 1.0f;
        phi_f12 = 0.0f;
    }
    dest->xx = z_f14;
    dest->yx = (-nx) * phi_f14;
    dest->zx = (-nx) * phi_f12;
    dest->xy = nx;
    dest->yy = ny;
    dest->zy = nz;
    dest->xz = 0.0f;
    dest->yz = -phi_f12;
    dest->zz = phi_f14;
    dest->xw = tx;
    dest->yw = ty;
    dest->zw = tz;
    dest->wx = 0.0f;
    dest->wy = 0.0f;
    dest->wz = 0.0f;
    dest->ww = 1.0f;
}

static void MmForm_AddDekuBubbleSplat(Vec3f* position, CollisionPoly* poly, f32 scale) {
    if (poly == NULL)
        return;
    auto* splat = &sDekuBubbleSplats[0];
    for (auto& candidate : sDekuBubbleSplats) {
        if (candidate.life < splat->life)
            splat = &candidate;
    }
    *splat = {};
    MmForm_BubbleSurfaceFrame(poly, position->x, position->y, position->z, &splat->surface);
    splat->scale = (s16)CLAMP(Rand_CenteredFloat(100.0f) + scale * 120.0f, 600.0f, 1500.0f);
    splat->life = 93;
    splat->alpha = 200;
    splat->scrollStep = 7;
    splat->scroll = 7;
    const f32 nx = poly->normal.x * (1.0f / 32767.0f);
    const f32 ny = poly->normal.y * (1.0f / 32767.0f);
    const f32 nz = poly->normal.z * (1.0f / 32767.0f);
    splat->sliding = !(ny > 0.95f || ny < -0.95f);
    if (!splat->sliding)
        return;
    // Native SBN rotates the surface texture downhill along the polygon.
    Vec3f downhill;
    if (ny > 0.05f) {
        downhill = { nx * 10.0f, -(SQ(nx) + SQ(nz)) * 10.0f / ny, nz * 10.0f };
    } else if (ny < -0.05f) {
        downhill = { -nx * 10.0f, -(SQ(nx) + SQ(nz)) * 10.0f / -ny, -nz * 10.0f };
    } else {
        downhill = { 0.0f, -10.0f, 0.0f };
    }
    Vec3f bubble = { -splat->surface.mf[2][0] * 10.0f, -splat->surface.mf[2][1] * 10.0f,
                     -splat->surface.mf[2][2] * 10.0f };
    f32 cosine;
    Math3D_CosOut(&downhill, &bubble, &cosine);
    const f32 sine = SQ(cosine) >= 1.0f ? 0.0f : sqrtf(1.0f - SQ(cosine));
    // MM Math_Atan2S_XY(cosine, sine) is OoT Math_Atan2S(cosine, sine).
    splat->rotation = Math_Atan2S(cosine, sine);
    if ((splat->surface.mf[0][0] * downhill.x + splat->surface.mf[0][1] * downhill.y +
         splat->surface.mf[0][2] * downhill.z) >= 0.0f)
        splat->rotation = -splat->rotation;
}

static void MmForm_UpdateDekuBubbleEffects(PlayState* play) {
    for (auto& splat : sDekuBubbleSplats) {
        if (splat.life == 0)
            continue;
        --splat.life;
        if (splat.life < 10)
            splat.alpha = (u8)CLAMP_MIN((s32)splat.alpha - 20, 0);
        if (splat.texture < (splat.sliding ? 11 : 4))
            ++splat.texture;
        else if (splat.sliding && splat.scrollStep >= 2 && (play->state.frames % 2) == 0)
            --splat.scrollStep;
        splat.scroll += splat.scrollStep;
    }
    for (auto& emitter : sDekuBubblePopSounds) {
        if (emitter.life == 0)
            continue;
        if (--emitter.life == 0)
            MmSfx_StopAtPos(MM_NA_SE_IT_DEKUNUTS_BUBLE_VANISH, &emitter.projected);
        else
            MmForm_ProjectBubblePosition(play, &emitter.world, &emitter.projected);
    }
}

static void MmForm_ResetDekuBubble(PlayState* play) {
    gFormState.bubble.active = 0;
    if (play != NULL && gFormState.bubbleColliderInit)
        Collider_ResetCylinderAT(play, &gFormState.bubbleCollider.base);
    if (play == NULL)
        gFormState.bubbleColliderInit = 0;
    gFormState.bubbleCharging = 0;
    gFormState.bubbleCharge = 0.0f;
    gFormState.bubbleChargeTimer = 0;
    gFormState.dekuCheekScale = 1.0f;
    // Also clear legacy flagged entries if a live form is being reset/reloaded.
    MmSfx_Stop(MM_NA_SE_PL_DEKUNUTS_BUBLE_BREATH);
    MmSfx_Stop(MM_NA_SE_PL_DEKUNUTS_BUBLE_BREATH - SFX_FLAG);
    MmSfx_Stop(MM_NA_SE_PL_DEKUNUTS_FIRE);
    MmSfx_StopAtPos(MM_NA_SE_IT_DEKUNUTS_BUBLE_SHOT_LEVEL, &gFormState.bubble.projectedPos);
    MmSfx_StopAtPos(MM_NA_SE_IT_DEKUNUTS_BUBLE_SHOT_LEVEL - SFX_FLAG, &gFormState.bubble.projectedPos);
    for (auto& emitter : sDekuBubblePopSounds) {
        MmSfx_StopAtPos(MM_NA_SE_IT_DEKUNUTS_BUBLE_VANISH, &emitter.projected);
        emitter = {};
    }
    for (auto& splat : sDekuBubbleSplats)
        splat = {};
}

static void MmForm_DrawDekuBubbleEffects(PlayState* play) {
    for (const auto& splat : sDekuBubbleSplats) {
        if (splat.life == 0)
            continue;
        // These two native graphs resolve vertices, child DLs and mask textures
        // in mm.o2r. Global/Alt gameplay_keep resources cannot shadow them.
        Gfx* dl =
            MmAssets_LoadDisplayListGraphStrict(splat.sliding ? "objects/gameplay_keep/gEffPoppedDekuBubbleSlidingDL"
                                                              : "objects/gameplay_keep/gEffPoppedDekuBubbleDL");
        char path[128];
        snprintf(path, sizeof(path), "objects/gameplay_keep/gEffPoppedDekuBubble%s%dTex",
                 splat.sliding ? "Sliding" : "", splat.texture + 1);
        void* texture = MmAssets_LoadResourceStrict(path);
        if (dl == NULL || texture == NULL) {
            static u8 reportedMissing = 0;
            if (!reportedMissing) {
                reportedMissing = 1;
                MMFORM_LOG("[MmForm] Native popped Deku bubble resources are missing from mm.o2r");
            }
            continue;
        }
        OPEN_DISPS(play->state.gfxCtx);
        Matrix_Push();
        MtxF surface = splat.surface;
        Matrix_Mult(&surface, MTXMODE_NEW);
        Matrix_RotateY(BINANG_TO_RAD(splat.rotation), MTXMODE_APPLY);
        const f32 scale = splat.scale * 0.001f * 0.05f;
        Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        gDPSetRenderMode(POLY_XLU_DISP++, G_RM_FOG_SHADE_A, G_RM_ZB_XLU_DECAL2);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 255, 255, 255, splat.alpha);
        gSPSegment(POLY_XLU_DISP++, 0x08, (uintptr_t)texture);
        if (splat.sliding) {
            gSPSegment(POLY_XLU_DISP++, 0x09,
                       (uintptr_t)Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, 0, 0x20, 0x40, 1, 0, splat.scroll, 0x20,
                                                     0x20, 0, 0, 0, splat.scrollStep));
        }
        gSPDisplayList(POLY_XLU_DISP++, dl);
        // Restore dynamic segments for actors drawn after the player.
        gSPSegment(POLY_XLU_DISP++, 0x08, (uintptr_t)gEmptyDL);
        gSPSegment(POLY_XLU_DISP++, 0x09, (uintptr_t)gEmptyDL);
        Matrix_Pop();
        CLOSE_DISPS(play->state.gfxCtx);
    }
}
