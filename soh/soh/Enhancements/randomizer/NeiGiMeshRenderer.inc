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

// Bitwise finite checks remain effective under the engines' fast-math flags.
// Reject a whole malformed mesh before any arena allocation or GPU submission.
static bool NeiGi_Finite(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return (bits & 0x7F800000u) != 0x7F800000u;
}

static bool NeiGi_ValidScale(float scale) {
    return NeiGi_Finite(scale) && scale > 0.f && scale <= 2048.f;
}

// Both native allocators subtract the OPA tail without detecting exhaustion.
// Leave space for queued heads, aligned vertices/matrices and the debug nodes
// emitted by OPEN/CLOSE_DISPS (including each setup function's nested pair).
struct NeiGi_ArenaBudget {
    GraphicsContext* gfx = nullptr;
    size_t matrixBytes = 0, opaCommands = 0, xluCommands = 0;
};
static thread_local NeiGi_ArenaBudget NeiGi_ReservedArena;

static bool NeiGi_ArenaHasRoom(PlayState* play, size_t vertexBytes, size_t matrices, size_t opaCommands,
                               size_t xluCommands) {
    if (!play || !play->state.gfxCtx)
        return false;
    const auto* gfx = play->state.gfxCtx;
    const auto remaining = [](const void* head, const void* tail, bool alignTail) {
        uintptr_t begin = reinterpret_cast<uintptr_t>(head), end = reinterpret_cast<uintptr_t>(tail);
        if (alignTail)
            end &= ~uintptr_t(15); // OoT also discards a misaligned current tail.
        return begin && end >= begin ? size_t(end - begin) : size_t(0);
    };
    const size_t opaAvailable = remaining(gfx->polyOpa.p, gfx->polyOpa.d, true);
    const size_t xluAvailable = remaining(gfx->polyXlu.p, gfx->polyXlu.d, false);
    const size_t overlayAvailable = remaining(gfx->overlay.p, gfx->overlay.d, false);
    const auto reserved = NeiGi_ReservedArena.gfx == gfx ? NeiGi_ReservedArena : NeiGi_ArenaBudget{};
    const size_t tailBytes =
        ((vertexBytes + 15) & ~size_t(15)) + matrices * ((sizeof(Mtx) + 15) & ~size_t(15)) + reserved.matrixBytes;
    return tailBytes <= opaAvailable &&
           opaCommands + reserved.opaCommands <= (opaAvailable - tailBytes) / sizeof(Gfx) &&
           xluCommands + reserved.xluCommands <= xluAvailable / sizeof(Gfx) && overlayAvailable / sizeof(Gfx) >= 2;
}

// Child meshes must leave their caller's owner-pop, shell and restore commands
// plus matrices available. The reservation changes no native arena cursors.
class NeiGi_ArenaScope {
    NeiGi_ArenaBudget previous;
    bool active;

  public:
    NeiGi_ArenaScope(PlayState* play, size_t matrices, size_t opaCommands, size_t xluCommands, size_t tailBytes = 0)
        : previous(NeiGi_ReservedArena),
          active(NeiGi_ArenaHasRoom(play, tailBytes, matrices, opaCommands, xluCommands)) {
        if (active) {
            if (NeiGi_ReservedArena.gfx != play->state.gfxCtx)
                NeiGi_ReservedArena = { play->state.gfxCtx };
            NeiGi_ReservedArena.matrixBytes +=
                matrices * ((sizeof(Mtx) + 15) & ~size_t(15)) + ((tailBytes + 15) & ~size_t(15));
            NeiGi_ReservedArena.opaCommands += opaCommands;
            NeiGi_ReservedArena.xluCommands += xluCommands;
        }
    }
    ~NeiGi_ArenaScope() {
        if (active)
            NeiGi_ReservedArena = previous;
    }
    explicit operator bool() const {
        return active;
    }
};

bool NeiGi_CanDrawLayers(PlayState* play, size_t matrices, size_t opa, size_t xlu) {
    return NeiGi_ArenaHasRoom(play, 0, matrices, opa, xlu);
}

static bool NeiGi_ValidMesh(const NeiGi::Mesh& mesh, const NeiGi::TextureMaterial* material = nullptr,
                            bool seasonSunRays = false) {
    if (!mesh.count || mesh.count > mesh.vertices.size() || mesh.count % 3)
        return false;
    const auto fits = [](float value, float scale) {
        if (!NeiGi_Finite(value))
            return false;
        const float fixed = value * scale;
        return NeiGi_Finite(fixed) && fixed >= -32768.f && fixed <= 32767.f;
    };
    for (size_t i = 0; i < mesh.count; ++i) {
        const auto& v = mesh.vertices[i];
        if (!fits(v.p.x, 16) || !fits(v.p.y, 16) || !fits(v.p.z, 16) ||
            !fits(v.u, (material || seasonSunRays ? 32 : 63) * 32) ||
            !fits(v.v, (seasonSunRays ? 31
                        : material    ? 32
                                      : 63) *
                           32))
            return false;
    }
    return true;
}

static bool NeiGi_DrawMeshMaterial(PlayState* play, const NeiGi::Mesh& mesh, Kind orb,
                                   const NeiGi::TextureMaterial* material, bool seasonSunRays = false,
                                   const char* owner = nullptr) {
    if (!play || !play->state.gfxCtx || !NeiGi_ValidMesh(mesh, material, seasonSunRays) || int(orb) < 0 ||
        int(orb) > int(Kind::MarioMask))
        return false;
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
                              { static_cast<int16_t>(std::lround(v.u * (material || seasonSunRays ? 32 : 63) * 32)),
                                static_cast<int16_t>(std::lround(v.v *
                                                                 (seasonSunRays ? 31
                                                                  : material    ? 32
                                                                                : 63) *
                                                                 32)) },
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
    size_t triangleCommands = 0;
    for (size_t b = 0; b < batchCount; ++b)
        triangleCommands += (batches[b].indexCount + 5) / 6;
    // At most 30 fixed commands: debug pairs, two setup calls, texture-load
    // expansion (seven commands), material state, one matrix and owner pair.
    if (!NeiGi_ArenaHasRoom(play, vertexCount * sizeof(Vtx), 1, 2, 32 + batchCount + triangleCommands))
        return false;
    auto* vertices = static_cast<Vtx*>(Graph_Alloc(play->state.gfxCtx, vertexCount * sizeof(Vtx)));
    if (vertices == nullptr)
        return false;
    std::memcpy(vertices, packed.data(), vertexCount * sizeof(Vtx));
    OPEN_DISPS(play->state.gfxCtx);
    if (owner)
        gSPComboRMPush(POLY_XLU_DISP++, owner);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gDPPipeSync(POLY_XLU_DISP++);
    gSPClearGeometryMode(POLY_XLU_DISP++,
                         G_LIGHTING | G_CULL_BACK | G_CULL_FRONT | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_FOG);
    gSPSetGeometryMode(POLY_XLU_DISP++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
    gDPSetCycleType(POLY_XLU_DISP++, G_CYC_2CYCLE);
    gDPSetAlphaCompare(POLY_XLU_DISP++, G_AC_NONE);
    gDPSetRenderMode(POLY_XLU_DISP++, G_RM_PASS, G_RM_AA_ZB_XLU_SURF2);
    if (seasonSunRays) {
        gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
        gDPSetTextureFilter(POLY_XLU_DISP++, G_TF_BILERP);
        gDPLoadTextureBlock(POLY_XLU_DISP++, NeiGi::SeasonSunRayTexture().data(), G_IM_FMT_I, G_IM_SIZ_8b, 32, 32, 0,
                            G_TX_WRAP, G_TX_CLAMP, 5, 5, G_TX_NOLOD, G_TX_NOLOD);
        // Tile coordinates are quarter texels. Keep the same average speed
        // without jumping a whole texel every four game ticks.
        const uint32_t scroll = play->gameplayFrames % 128u;
        gDPSetTileSize(POLY_XLU_DISP++, G_TX_RENDERTILE, scroll, 0, scroll + 31 * 4, 31 * 4);
        gDPSetCombineMode(POLY_XLU_DISP++, G_CC_MODULATEIA, G_CC_PASS2);
    } else if (material) {
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
    if (owner)
        gSPComboRMPop(POLY_XLU_DISP++);
    CLOSE_DISPS(play->state.gfxCtx);
    return true;
}

void NeiGi_DrawMesh(PlayState* play, const NeiGi::Mesh& mesh, Kind orb) {
    NeiGi_DrawMeshMaterial(play, mesh, orb, nullptr);
}

bool NeiGi_DrawTexturedMesh(PlayState* play, const NeiGi::Mesh& mesh, const NeiGi::TextureMaterial& material) {
    if (!play || !play->state.gfxCtx || !NeiGi_ValidMesh(mesh, &material) || !HasResource(material.path))
        return false;
    return NeiGi_DrawMeshMaterial(play, mesh, Kind::Neutral, &material);
}

void NeiGi_DrawPresentation(PlayState* play, const char* opa, const char* xlu, float scale, int effect,
                            const float center[3], bool shimmer, const char* owner, bool shop, int mmPickup) {
    if (!play || !play->state.gfxCtx || !opa || !center || !NeiGi_ValidScale(scale) || !NeiGi_Finite(center[0]) ||
        !NeiGi_Finite(center[1]) || !NeiGi_Finite(center[2]) || effect < 0 || effect > int(Kind::MarioMask))
        return;
    const Kind kind = static_cast<Kind>(effect);
    const uint32_t bits = (uint32_t(play->gameplayFrames) * 2u) & 0xFFFFu;
    const float spin = (bits >= 0x8000u ? int32_t(bits) - 0x10000 : int32_t(bits)) * .01f;
    const auto* bounds = NeiGi::FindFrameBounds(opa);
    if (!bounds || bounds->effect != kind)
        return;
    NeiGi_ArenaScope arena(play, 3, 16, 24);
    if (!arena)
        return;
    Matrix_Push();
    const auto fit = NeiGi::FrameFit(*bounds, scale, shop, mmPickup);
    Matrix_Translate(0.f, fit.lift, 0.f, MTXMODE_APPLY);
    Matrix_Scale(fit.scale, fit.scale, fit.scale, MTXMODE_APPLY);
    OPEN_DISPS(play->state.gfxCtx);
    if (owner) {
        gSPComboRMPush(POLY_OPA_DISP++, owner);
        gSPComboRMPush(POLY_XLU_DISP++, owner);
    }
    gSPGrayscale(POLY_OPA_DISP++, false);
    gSPGrayscale(POLY_XLU_DISP++, false);
    Matrix_Push();
    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
#ifdef NEI_GI_ROTATE_Y
    NEI_GI_ROTATE_Y(spin, MTXMODE_APPLY);
#else
    Matrix_RotateY(spin, MTXMODE_APPLY);
#endif
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD);
    gDma1p(POLY_OPA_DISP++, G_DL_OTR_FILEPATH, opa, 0, G_DL_PUSH);
    // Child mesh renderers advance the same streams. Publish these cursors
    // before invoking them, then reopen after all procedural submissions.
    CLOSE_DISPS(play->state.gfxCtx);
    Matrix_Pop();
    Matrix_Push();
#ifdef NEI_GI_ROTATE_Y
    NEI_GI_ROTATE_Y(spin, MTXMODE_APPLY);
#else
    Matrix_RotateY(spin, MTXMODE_APPLY);
#endif
    const auto camera = NeiGi_CameraBasis(play);
    Matrix_Push();
    Matrix_Translate(center[0], center[1], center[2], MTXMODE_APPLY);
    if (NeiGi::IsRod(kind) || NeiGi::IsSpell(kind)) {
        NeiGi_DrawMesh(play, NeiGi::SampleOrb(kind, camera), kind);
        NeiGi_DrawMesh(play, NeiGi::SampleEnergy(kind, play->gameplayFrames, camera));
    }
    if (NeiGi::IsSpecial(kind))
        NeiGi_DrawMesh(play, NeiGi::SampleSpecial(kind, play->gameplayFrames, camera));
    Matrix_Pop();
    if (shimmer)
        NeiGi_DrawMesh(play, NeiGi::SampleShimmer(play->gameplayFrames, true, camera, kind));
    Matrix_Pop();
    if (kind == Kind::SeasonCycle)
        NeiGi_DrawSeasonOverlay(play, 5, nullptr);
    OPEN_DISPS(play->state.gfxCtx);
    if (xlu) {
        Matrix_Push();
        Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
#ifdef NEI_GI_ROTATE_Y
        NEI_GI_ROTATE_Y(spin, MTXMODE_APPLY);
#else
        Matrix_RotateY(spin, MTXMODE_APPLY);
#endif
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
        gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_MODELVIEW | G_MTX_LOAD);
        gDma1p(POLY_XLU_DISP++, G_DL_OTR_FILEPATH, xlu, 0, G_DL_PUSH);
        Matrix_Pop();
    }
    if (owner) {
        gSPComboRMPop(POLY_OPA_DISP++);
        gSPComboRMPop(POLY_XLU_DISP++);
    }
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD);
    CLOSE_DISPS(play->state.gfxCtx);
    Matrix_Pop();
}

void NeiGi_DrawExternalPresentation(PlayState* play, const char* opa, const char* xlu, float scale, int shimmerKind,
                                    bool shimmer, const char* owner, bool shop, float tilt, bool fit, int mmPickup) {
    if (!play || !play->state.gfxCtx || !opa || !NeiGi_ValidScale(scale) || shimmerKind < 0 ||
        shimmerKind > int(Kind::MarioMask))
        return;
    NeiGi_ArenaScope arena(play, 2, 16, 16);
    if (!arena)
        return;
    const uint32_t bits = (uint32_t(play->gameplayFrames) * 2u) & 0xFFFFu;
    const float spin = (bits >= 0x8000u ? int32_t(bits) - 0x10000 : int32_t(bits)) * .01f;
    const bool sword = NeiGi::IsSword(static_cast<Kind>(shimmerKind));
    Matrix_Push();
    if (fit && mmPickup && !sword) {
        const char* paths[] = { opa, xlu };
        ComboSwordGi_ApplyModelsFit(owner ? owner : "oot", paths, xlu ? 2 : 1, scale, tilt, shop, mmPickup);
    }
    Matrix_Push();
    if (fit && sword) {
        const char* paths[] = { opa, xlu };
        ComboSwordGi_ApplyModelsFit(owner ? owner : "oot", paths, xlu ? 2 : 1, scale, tilt, shop, mmPickup);
    }
    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
#ifdef NEI_GI_ROTATE_Y
    NEI_GI_ROTATE_Y(spin, MTXMODE_APPLY);
#else
    Matrix_RotateY(spin, MTXMODE_APPLY);
#endif
    if (tilt != 0.f) {
#ifdef NEI_GI_ROTATE_Z
        NEI_GI_ROTATE_Z(tilt, MTXMODE_APPLY);
#else
        Matrix_RotateZ(tilt, MTXMODE_APPLY);
#endif
    }
    OPEN_DISPS(play->state.gfxCtx);
    if (owner) {
        gSPComboRMPush(POLY_OPA_DISP++, owner);
        gSPComboRMPush(POLY_XLU_DISP++, owner);
    }
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gSPGrayscale(POLY_OPA_DISP++, false);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gDma1p(POLY_OPA_DISP++, G_DL_OTR_FILEPATH, opa, 0, G_DL_PUSH);
    if (xlu) {
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        gSPGrayscale(POLY_XLU_DISP++, false);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
        gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gDma1p(POLY_XLU_DISP++, G_DL_OTR_FILEPATH, xlu, 0, G_DL_PUSH);
    }
    if (owner) {
        gSPComboRMPop(POLY_OPA_DISP++);
        gSPComboRMPop(POLY_XLU_DISP++);
    }
    CLOSE_DISPS(play->state.gfxCtx);
    if (tilt != 0.f && NeiGi::IsSword(static_cast<Kind>(shimmerKind)))
        ComboDinSwordGi_DrawLayers(play, owner ? owner : "oot", opa);
    Matrix_Pop();
    // Sword particles belong to the awarded sword, including mod geometry.
    if (sword) {
        ComboSwordGi_ApplyEffectFit(static_cast<Kind>(shimmerKind), shop, mmPickup);
        NeiGi_DrawMesh(play, NeiGi::SampleSpecial(static_cast<Kind>(shimmerKind), play->gameplayFrames,
                                                 NeiGi_CameraBasis(play)));
    }
    if (shimmer)
        NeiGi_DrawMesh(play, NeiGi::SampleShimmer(play->gameplayFrames, true, NeiGi_CameraBasis(play),
                                                  static_cast<Kind>(shimmerKind)));
    Matrix_Pop();
}

void NeiGi_DrawSeasonOverlay(PlayState* play, int profile, const char* owner) {
    if (!play || !play->state.gfxCtx || profile < 1 || profile > 6)
        return;
    NeiGi_ArenaScope arena(play, 1, 2, 16);
    if (!arena)
        return;
    const bool stormSong = profile == 6;
    const bool sun = NeiGi::SeasonProfile(play->gameplayFrames, profile) == 2;
    if (sun) {
        Matrix_Push();
        // Record the billboard through the native matrix interpolator. Baking
        // its inverse parent rotation into vertices snaps at game ticks while
        // the held GI's parent spin continues between those ticks.
        Matrix_ReplaceRotation(&play->billboardMtxF);
    }
    const auto camera = sun ? NeiGi::Basis{} : NeiGi_CameraBasis(play);
    auto mesh = NeiGi::SampleSeason(play->gameplayFrames, stormSong ? 1 : profile, camera);
    OPEN_DISPS(play->state.gfxCtx);
    gSPGrayscale(POLY_XLU_DISP++, false);
    if (owner)
        gSPComboRMPush(POLY_XLU_DISP++, owner);
    CLOSE_DISPS(play->state.gfxCtx);
    NeiGi_DrawMesh(play, mesh, Kind::Neutral);
    if (sun) {
        const auto rays = NeiGi::SampleSeasonSunRays(play->gameplayFrames, profile, camera);
        NeiGi_DrawMeshMaterial(play, rays, Kind::Neutral, nullptr, true);
        Matrix_Pop();
    }
    OPEN_DISPS(play->state.gfxCtx);
    if (owner)
        gSPComboRMPop(POLY_XLU_DISP++);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    CLOSE_DISPS(play->state.gfxCtx);
}

// Shared by native and foreign mask/remains and progressive sword overlays.
// Reuse the exact optional NEI shimmer sampler and renderer; only the requested
// transformation/remains palette changes. Ordinary items retain NEI's palette.
void NeiGi_DrawShimmerOverlay(PlayState* play, const uint8_t color[4], const char* owner) {
    if (!play || !play->state.gfxCtx || !color)
        return;
    NeiGi_ArenaScope arena(play, 1, 2, 16);
    if (!arena)
        return;
    const bool ordinary = color[0] == 220 && color[1] == 225 && color[2] == 240;
    const bool dark = color[0] == 0 && color[1] == 0 && color[2] == 0;
    auto mesh = NeiGi::SampleShimmer(play->gameplayFrames, true, NeiGi_CameraBasis(play));
    if (!ordinary) {
        const uint32_t rgb = uint32_t(color[0]) << 16 | uint32_t(color[1]) << 8 | color[2];
        for (size_t i = 0; i < mesh.count; ++i) {
            auto& vertex = mesh.vertices[i];
            vertex.rgb = vertex.rgb == 0xFFFFFF ? (dark ? 0x505060 : 0xFFFFFF) : rgb;
        }
    }
    OPEN_DISPS(play->state.gfxCtx);
    gSPGrayscale(POLY_XLU_DISP++, false);
    if (owner)
        gSPComboRMPush(POLY_XLU_DISP++, owner);
    CLOSE_DISPS(play->state.gfxCtx);
    NeiGi_DrawMesh(play, mesh, Kind::Neutral);
    OPEN_DISPS(play->state.gfxCtx);
    if (owner)
        gSPComboRMPop(POLY_XLU_DISP++);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    CLOSE_DISPS(play->state.gfxCtx);
}

void NeiGi_DrawSongOverlay(PlayState* play, int song, const char* owner) {
    if (!play || !play->state.gfxCtx || song < 0 || song >= CW_SONG_COUNT)
        return;
    if (song == CW_SONG_STORMS) {
        NeiGi_DrawSeasonOverlay(play, 6, owner);
        return;
    }
    if (ComboSongAlwaysShimmers(song)) {
        uint8_t color[4];
        ComboSongShimmerColor(song, color);
        NeiGi_DrawShimmerOverlay(play, color, owner);
    }
    const auto mesh = NeiGi::SampleSong(song, play->gameplayFrames, NeiGi_CameraBasis(play));
    if (!mesh.count)
        return;
    NeiGi_ArenaScope arena(play, 1, 2, 16);
    if (!arena)
        return;
    OPEN_DISPS(play->state.gfxCtx);
    gSPGrayscale(POLY_XLU_DISP++, false);
    if (owner)
        gSPComboRMPush(POLY_XLU_DISP++, owner);
    CLOSE_DISPS(play->state.gfxCtx);
    NeiGi_DrawMesh(play, mesh);
    OPEN_DISPS(play->state.gfxCtx);
    if (owner)
        gSPComboRMPop(POLY_XLU_DISP++);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    CLOSE_DISPS(play->state.gfxCtx);
}
