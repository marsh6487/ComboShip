// Shared by the OoT and MM host renderers. The enclosing TU supplies native GBI/matrix APIs.
NeiGi::Basis NeiGi_CameraBasis(PlayState* play) {
    MtxF m;
    Matrix_Get(&m);
    auto local = [&](NeiGi::Point p) {
        return NeiGi::Point{ p.x * m.xx + p.y * m.yx + p.z * m.zx, p.x * m.xy + p.y * m.yy + p.z * m.zy,
                             p.x * m.xz + p.y * m.yz + p.z * m.zz };
    };
    const auto& b = play->billboardMtxF;
    const auto right = NeiGi::Unit(local({ b.xx, b.yx, b.zx }));
    const auto up = NeiGi::Unit(local({ b.xy, b.yy, b.zy }));
    return { right, up, NeiGi::Unit(NeiGi::Cross(right, up)) };
}

static void NeiGi_DrawMeshMaterial(PlayState* play, const NeiGi::Mesh& mesh, Kind orb,
                                   const NeiGi::TextureMaterial* material) {
    if (mesh.count == 0)
        return;
    // Reuse shared vertices within each 32-entry RSP cache load. A full potion
    // shop must leave room in both the OPA arena and XLU command buffer.
    struct Batch {
        size_t vertexStart, vertexCount, indexStart, indexCount;
    };
    std::array<Vtx, 1536> packed{};
    std::array<uint8_t, 1536> indices{};
    std::array<Batch, 64> batches{};
    size_t vertexCount = 0, indexCount = 0, batchCount = 0;
    Batch batch{};
    auto equal = [](const Vtx& a, const Vtx& b) {
        return std::memcmp(a.v.ob, b.v.ob, sizeof(a.v.ob)) == 0 && std::memcmp(a.v.tc, b.v.tc, sizeof(a.v.tc)) == 0 &&
               std::memcmp(a.v.cn, b.v.cn, 4) == 0;
    };
    auto finish = [&]() {
        if (batch.indexCount)
            batches[batchCount++] = batch;
        batch = { vertexCount, 0, indexCount, 0 };
    };
    for (size_t i = 0; i < mesh.count; i += 3) {
        Vtx triangle[3]{};
        for (size_t j = 0; j < 3; ++j) {
            const auto& v = mesh.vertices[i + j];
            triangle[j] = { { { static_cast<int16_t>(std::lround(v.p.x * 16)),
                                static_cast<int16_t>(std::lround(v.p.y * 16)),
                                static_cast<int16_t>(std::lround(v.p.z * 16)) },
                              0,
                              { static_cast<int16_t>(std::lround(v.u * (material ? 32 : 63) * 32)),
                                static_cast<int16_t>(std::lround(v.v * (material ? 32 : 63) * 32)) },
                              { static_cast<uint8_t>(v.rgb >> 16), static_cast<uint8_t>(v.rgb >> 8),
                                static_cast<uint8_t>(v.rgb), v.alpha } } };
        }
        // Conservative reservation of three vertices keeps a complete triangle
        // in one load. Unused cache slots do not consume arena storage.
        if (batch.vertexCount > 29)
            finish();
        for (const auto& vertex : triangle) {
            size_t found = 0;
            while (found < batch.vertexCount && !equal(vertex, packed[batch.vertexStart + found]))
                ++found;
            if (found == batch.vertexCount) {
                packed[vertexCount++] = vertex;
                ++batch.vertexCount;
            }
            indices[indexCount++] = static_cast<uint8_t>(found);
            ++batch.indexCount;
        }
    }
    finish();
    auto* vertices = static_cast<Vtx*>(Graph_Alloc(play->state.gfxCtx, vertexCount * sizeof(Vtx)));
    if (vertices == nullptr)
        return;
    std::memcpy(vertices, packed.data(), vertexCount * sizeof(Vtx));
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gDPPipeSync(POLY_XLU_DISP++);
    gSPClearGeometryMode(POLY_XLU_DISP++,
                         G_LIGHTING | G_CULL_BACK | G_CULL_FRONT | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_FOG);
    gSPSetGeometryMode(POLY_XLU_DISP++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
    gDPSetCycleType(POLY_XLU_DISP++, G_CYC_2CYCLE);
    gDPSetAlphaCompare(POLY_XLU_DISP++, G_AC_NONE);
    gDPSetRenderMode(POLY_XLU_DISP++, G_RM_PASS, G_RM_AA_ZB_XLU_SURF2);
    if (material) {
        gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
        gDPSetTextureFilter(POLY_XLU_DISP++, G_TF_BILERP);
        gDPLoadTextureBlock(POLY_XLU_DISP++, material->path, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0,
                            material->repeatS ? G_TX_WRAP : G_TX_CLAMP, material->repeatT ? G_TX_WRAP : G_TX_CLAMP, 5,
                            5, G_TX_NOLOD, G_TX_NOLOD);
        // Resource RGBA multiplied by each sampled vertex's color AND alpha.
        gDPSetCombineMode(POLY_XLU_DISP++, G_CC_MODULATEIA, G_CC_PASS2);
    } else if (orb != Kind::Neutral) {
        const auto color = NeiGi::OrbPalette(orb);
        gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
        gDPSetTextureFilter(POLY_XLU_DISP++, G_TF_BILERP);
        gDPLoadTextureBlock(POLY_XLU_DISP++, NeiGi::OrbTexture(orb, play->gameplayFrames).data(), G_IM_FMT_I,
                            G_IM_SIZ_8b, 64, 64, 0, G_TX_CLAMP, G_TX_CLAMP, 6, 6, G_TX_NOLOD, G_TX_NOLOD);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, color.hot >> 16, (color.hot >> 8) & 255, color.hot & 255, 255);
        gDPSetEnvColor(POLY_XLU_DISP++, color.edge >> 16, (color.edge >> 8) & 255, color.edge & 255, 255);
        // I8 controls both the hot/edge gradient and alpha, exactly as previewed.
        gDPSetCombineMode(POLY_XLU_DISP++, G_CC_BLENDPE, G_CC_PASS2);
    } else {
        gSPTexture(POLY_XLU_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
        gDPSetCombineMode(POLY_XLU_DISP++, G_CC_SHADE, G_CC_SHADE);
    }
    Matrix_Push();
    Matrix_Scale(1.f / 16, 1.f / 16, 1.f / 16, MTXMODE_APPLY);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    for (size_t b = 0; b < batchCount; ++b) {
        const auto& part = batches[b];
        gSPVertex(POLY_XLU_DISP++, reinterpret_cast<uintptr_t>(vertices + part.vertexStart), part.vertexCount, 0);
        const auto* index = indices.data() + part.indexStart;
        for (size_t j = 0; j < part.indexCount; j += 6) {
            if (j + 3 < part.indexCount) {
                gSP2Triangles(POLY_XLU_DISP++, index[j], index[j + 1], index[j + 2], 0, index[j + 3], index[j + 4],
                              index[j + 5], 0);
            } else {
                gSP1Triangle(POLY_XLU_DISP++, index[j], index[j + 1], index[j + 2], 0);
            }
        }
    }
    Matrix_Pop();
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}

void NeiGi_DrawMesh(PlayState* play, const NeiGi::Mesh& mesh, Kind orb) {
    NeiGi_DrawMeshMaterial(play, mesh, orb, nullptr);
}

bool NeiGi_DrawTexturedMesh(PlayState* play, const NeiGi::Mesh& mesh, const NeiGi::TextureMaterial& material) {
    if (!play || !mesh.count || !HasResource(material.path))
        return false;
    NeiGi_DrawMeshMaterial(play, mesh, Kind::Neutral, &material);
    return true;
}
