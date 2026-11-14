// Included once per host; the adapter supplies native setup/allocation macros.
static bool ItemGetLightShaft_ArenaHasRoom(GraphicsContext* gfx) {
    if (!gfx)
        return false;
    const auto remaining = [](const void* head, const void* tail, bool align) {
        const uintptr_t begin = reinterpret_cast<uintptr_t>(head);
        uintptr_t end = reinterpret_cast<uintptr_t>(tail);
        if (align)
            end &= ~uintptr_t(15);
        return begin && end >= begin ? size_t(end - begin) : size_t(0);
    };
    // Graph_Alloc and matrix allocation both consume the OPA tail. Reserve
    // native OPEN/CLOSE debug nodes as well as the explicit XLU commands.
    constexpr size_t vertexBytes =
        (sizeof(Vtx) * ItemGetLightShaft::kRows * ItemGetLightShaft::kColumns + 15) & ~size_t(15);
    constexpr size_t matrixBytes = 2 * ((sizeof(Mtx) + 15) & ~size_t(15));
    constexpr size_t scrollBytes = (7 * sizeof(Gfx) + 15) & ~size_t(15);
    return remaining(gfx->polyOpa.p, gfx->polyOpa.d, true) >=
               vertexBytes + matrixBytes + scrollBytes + 24 * sizeof(Gfx) &&
           remaining(gfx->polyXlu.p, gfx->polyXlu.d, false) >= 112 * sizeof(Gfx) &&
           remaining(gfx->overlay.p, gfx->overlay.d, false) >= 8 * sizeof(Gfx);
}

static void ItemGetLightShaft_Draw(PlayState* play, const ItemGetLightShaft::Frame& frame) {
    if (!play || !frame.visible || frame.opacity <= 0 || !ItemGetLightShaft_ArenaHasRoom(play->state.gfxCtx))
        return;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    const auto mesh = ItemGetLightShaft::BuildMesh(frame, { play->view.eye.x, play->view.eye.y, play->view.eye.z });
    auto* vertices = static_cast<Vtx*>(GI_SHAFT_ALLOC(gfxCtx, sizeof(Vtx) * mesh.vertices.size()));
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        const auto& v = mesh.vertices[i];
        vertices[i] = {};
        vertices[i].v.ob[0] = static_cast<s16>(std::lround(v.position.x));
        vertices[i].v.ob[1] = static_cast<s16>(std::lround(v.position.y));
        vertices[i].v.ob[2] = static_cast<s16>(std::lround(v.position.z));
        vertices[i].v.tc[0] = v.s;
        vertices[i].v.tc[1] = v.t;
        vertices[i].v.cn[0] = 255;
        vertices[i].v.cn[1] = 244;
        vertices[i].v.cn[2] = 214;
        vertices[i].v.cn[3] = static_cast<u8>(std::lround(v.alpha * 255.f));
    }
    // No dynamic segment ownership: execute the native tile list directly.
    Gfx* scroll = Gfx_TexScrollEx(gfxCtx, 0, frame.scroll, ItemGetLightShaft::kTextureWidth,
                                  ItemGetLightShaft::kTextureHeight, 0, frame.scrollStep);
    static const int interpolationIdentity = 0;
    FrameInterpolation_RecordOpenChild(&interpolationIdentity, static_cast<int>(frame.generation));
    Matrix_Push();
    Matrix_Translate(frame.origin.x, frame.origin.y, frame.origin.z, MTXMODE_NEW);
    OPEN_DISPS(gfxCtx);
    GI_SHAFT_SETUP_XLU(gfxCtx);
    gDPPipeSync(POLY_XLU_DISP++);
    gSPGrayscale(POLY_XLU_DISP++, false);
    gSPClearGeometryMode(POLY_XLU_DISP++,
                         G_LIGHTING | G_FOG | G_CULL_BACK | G_CULL_FRONT | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gSPSetGeometryMode(POLY_XLU_DISP++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
    gDPSetCycleType(POLY_XLU_DISP++, G_CYC_1CYCLE);
    gDPSetRenderMode(POLY_XLU_DISP++, G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2);
    gDPSetAlphaCompare(POLY_XLU_DISP++, G_AC_NONE);
    gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
    gDPSetTextureFilter(POLY_XLU_DISP++, G_TF_BILERP);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPLoadTextureBlock(POLY_XLU_DISP++, ItemGetLightShaft::Texture().data(), G_IM_FMT_I, G_IM_SIZ_8b,
                        ItemGetLightShaft::kTextureWidth, ItemGetLightShaft::kTextureHeight, 0, G_TX_CLAMP, G_TX_WRAP,
                        5, 6, G_TX_NOLOD, G_TX_NOLOD);
    gSPDisplayList(POLY_XLU_DISP++, scroll);
    // Texture density controls transparency only. Multiplying the RGB as well
    // turns low-density light into gray haze that darkens brighter scenery.
    gDPSetCombineLERP(POLY_XLU_DISP++, 0, 0, 0, SHADE, TEXEL0, 0, SHADE, 0, 0, 0, 0, SHADE, TEXEL0, 0, SHADE, 0);
    GI_SHAFT_LOAD_MATRIX(gfxCtx);
    gSPVertex(POLY_XLU_DISP++, reinterpret_cast<uintptr_t>(vertices), mesh.vertices.size(), 0);
    for (size_t row = 0; row + 1 < ItemGetLightShaft::kRows; ++row) {
        for (size_t column = 0; column + 1 < ItemGetLightShaft::kColumns; ++column) {
            const int index = static_cast<int>(row * ItemGetLightShaft::kColumns + column);
            gSP2Triangles(POLY_XLU_DISP++, index, index + 5, index + 6, 0, index, index + 6, index + 1, 0);
        }
    }
    Matrix_Pop();
    GI_SHAFT_LOAD_MATRIX(gfxCtx);
    gDPPipeSync(POLY_XLU_DISP++);
    GI_SHAFT_SETUP_XLU(gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    CLOSE_DISPS(gfxCtx);
    FrameInterpolation_RecordCloseChild();
}

#undef GI_SHAFT_ALLOC
#undef GI_SHAFT_SETUP_XLU
#undef GI_SHAFT_LOAD_MATRIX
