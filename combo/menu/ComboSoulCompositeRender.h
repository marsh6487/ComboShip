/* Included by ComboForeignAnim.h after its shared engine/material helpers. */
#ifndef COMBO_SOUL_COMPOSITE_RENDER_H
#define COMBO_SOUL_COMPOSITE_RENDER_H

inline s32 CfaOverrideLimbDrawXlu(PlayState*, s32 limbIndex, Gfx** dl, Vec3f* pos, Vec3s*, CFA_LIMB_ARG, Gfx** gfx) {
    const auto* info = sCfaInfo;
    for (int i = 0; i < info->limbColorCount; ++i) {
        const auto& c = info->limbColors[i];
        if (limbIndex >= c.limbFrom && limbIndex <= c.limbTo)
            gDPSetEnvColor((*gfx)++, c.env[0], c.env[1], c.env[2], c.env[3]);
    }
    const auto* rule = CfaFindLimbDL(limbIndex);
    if (rule) {
        pos->x += rule->posDx;
        pos->y += rule->posDy;
        pos->z += rule->posDz;
        if (rule->hide) {
            *dl = nullptr;
            return false;
        }
        if (rule->dlPath) {
            *dl = CfaRouteLimbDList((Gfx*)rule->dlPath);
            return false;
        }
    }
    if (limbIndex == info->hiddenLimb ||
        (limbIndex >= 0 && limbIndex < 64 && (info->hiddenLimbMask & (UINT64_C(1) << limbIndex))))
        *dl = nullptr;
    else
        *dl = CfaRouteLimbDList(*dl);
    return false;
}
inline void CfaPostLimbDrawXlu(PlayState* play, s32 limbIndex, Gfx** original, Vec3s*, CFA_LIMB_ARG, Gfx** gfx) {
    for (int i = 0; i < sCfaInfo->limbDLCount; ++i) {
        const auto& rule = sCfaInfo->limbDLs[i];
        if (rule.limbIndex != limbIndex || (!rule.postDlPath && !rule.postSelf))
            continue;
        Gfx* dl = rule.postDlPath ? CfaRouteLimbDList((Gfx*)rule.postDlPath) : CfaRouteLimbDList(*original);
        if (!dl)
            continue;
        Matrix_Push();
        if (rule.postBillboard)
            Matrix_ReplaceRotation(&play->billboardMtxF);
        CFA_LOAD_MTX((*gfx)++, play->state.gfxCtx);
        gSPDisplayList((*gfx)++, dl);
        Matrix_Pop();
    }
}

inline bool CfaValidTransform(const CwAnimTransform& t) {
    for (int i = 0; i < 3; ++i)
        if (!std::isfinite(t.translate[i]) || !std::isfinite(t.scale[i]))
            return false;
    return true;
}
inline bool CfaValidateComposite(const CwItemAnimDrawInfo* info) {
    if (info->partCount < 0 || info->partCount > CW_ANIM_MAX_PARTS || info->matrixCount < 0 ||
        info->matrixCount > CW_ANIM_MAX_MATRICES || info->colorSegmentCount < 0 || info->colorSegmentCount > 6 ||
        info->texAnimStep < 0 || info->texAnimStep2 < 0 || (info->bindPose != 0 && info->bindPose != 1) ||
        (info->bindPose && (!info->skelPath || info->animPath)) ||
        (info->posePath && (!info->bindPose || !CfaIsOtrPath(info->posePath))) || info->fixedPoseCount < 0 ||
        info->fixedPoseCount > 128 ||
        (info->fixedPoseCount && (!info->bindPose || info->fixedPoseCount != info->limbCount)) ||
        (info->matrixCount && info->matrixSegment && (info->matrixSegment < 8 || info->matrixSegment > 13)) ||
        (info->texAnimPath && !CfaIsOtrPath(info->texAnimPath)) ||
        (info->texAnimPath2 && !CfaIsOtrPath(info->texAnimPath2)) ||
        (!info->opa && (info->partCount || info->matrixCount || info->colorSegmentCount || info->texAnimPath2)) ||
        !std::isfinite(info->scale))
        return false;
    for (int i = 0; i < 3; ++i)
        if (!std::isfinite(info->translatePre[i]) || !std::isfinite(info->translatePost[i]) ||
            !std::isfinite(info->skeletonTranslate[i]))
            return false;
    for (int i = 0; i < info->partCount; ++i) {
        const auto& p = info->parts[i];
        if ((!p.frameCount && !CfaIsOtrPath(p.dlPath)) || p.frameCount < 0 || p.frameCount > 9 ||
            (p.alternateDlPath && !CfaIsOtrPath(p.alternateDlPath)) || !CfaValidTransform(p.transform) ||
            p.materialProfile < CW_ANIM_MATERIAL_DEFAULT || p.materialProfile > CW_ANIM_MATERIAL_FREEZARD)
            return false;
        for (int j = 0; j < p.frameCount; ++j)
            if (!CfaIsOtrPath(p.framePaths[j]))
                return false;
    }
    for (int i = 0; i < info->matrixCount; ++i)
        if (!CfaValidTransform(info->matrices[i]))
            return false;
    for (int i = 0; i < info->colorSegmentCount; ++i) {
        const auto& c = info->colorSegments[i];
        if (c.segment < 8 || c.segment > 13 || (!c.onOpa && !c.onXlu))
            return false;
    }
    return true;
}

inline bool CfaResourceType(const std::shared_ptr<Ship::IResource>& resource, uint32_t type, size_t bytes) {
    return resource && resource->GetInitData() && resource->GetInitData()->Type == type &&
           resource->GetPointerSize() >= bytes && resource->GetRawPointer();
}
// Resolve only the owner-local validator. No concrete C++ resource class layout
// crosses this boundary; IResource's virtual interface and the returned POD do.
inline bool CfaValidateOwnedResource(const char* game, const char* path, int kind, int count,
                                     const std::shared_ptr<Ship::IResource>& resource,
                                     CwSoulResourceInfo* validated = nullptr) {
    if (!resource || !resource->GetRawPointer())
        return false;
    const bool mm = std::strcmp(game, "mm") == 0;
    auto query = (Fn_GetSoulResourceInfo)Combo_ResolveSym(mm ? "2ship" : "soh",
                                                          mm ? "MM_GetSoulResourceInfo" : "OOT_GetSoulResourceInfo");
    if (!query)
        return false;
    CwSoulResourceInfo info{};
    if (query(path, kind, count, &info) != 1 || info.rawPointer != resource->GetRawPointer())
        return false;
    if (validated)
        *validated = info;
    return true;
}
inline CfaLoadedSkeletonHeader* CfaSelectLoadedHeader(CfaSkelEntry& entry, const CwItemAnimDrawInfo* info,
                                                      CfaLoadedSkeletonHeader* source, const char* game) {
    if (!CfaResourceType(entry.skelRes, 0x4F534B4C, sizeof(CfaLoadedSkeletonHeader)))
        return nullptr;
    CwSoulResourceInfo validated{};
    if (!CfaValidateOwnedResource(game, info->skelPath, CW_SOUL_RESOURCE_SKELETON, info->limbCount, entry.skelRes,
                                  &validated) ||
        validated.elementCount < 1 || validated.elementCount > 255 || validated.elementCount + 1 != info->limbCount ||
        validated.skeletonType < 0 || validated.skeletonType > 1 || validated.dListCount < 0 ||
        validated.dListCount > validated.elementCount)
        return nullptr;
    entry.localHeader.sh = *source;
    entry.localHeader.sh.limbCount = (uint8_t)validated.elementCount;
    entry.localHeader.sh.skeletonType = (uint8_t)validated.skeletonType;
    entry.localHeader.dListCount = (uint8_t)validated.dListCount;
    return &entry.localHeader.sh;
}

// Validate every loaded limb link before native recursion. Reject cycles, repeated
// children and out-of-range references; a malformed replacement must never recurse.
inline bool CfaValidateLoadedRig(const CfaLoadedSkeletonHeader* skel, const CwItemAnimDrawInfo* info) {
    if (!skel || !skel->segment || skel->limbCount == 0 || info->limbCount > 256 ||
        info->limbCount != (int)skel->limbCount + 1 || skel->skeletonType > kCfaSkeletonFlex)
        return false;
    std::vector<uint8_t> visited(skel->limbCount, 0);
    std::vector<std::pair<int, int>> pending{ { 0, 1 } };
    int meshCount = 0;
    while (!pending.empty()) {
        const auto node = pending.back();
        pending.pop_back();
        int index = node.first;
        // One skeleton-local push and one possible billboard post-limb push
        // surround native recursion; caller/transaction depth is already live.
        if (node.second + 2 > Matrix_GetRemainingStack())
            return false;
        if (index < 0 || index >= skel->limbCount || visited[index] || !skel->segment[index])
            return false;
        visited[index] = 1;
        const auto* limb = (const StandardLimb*)skel->segment[index];
        if (index == 0 && limb->sibling != LIMB_DONE)
            return false;
        if (limb->dList)
            ++meshCount;
        if (skel->skeletonType == kCfaSkeletonFlex && !limb->dList) {
            // Native flex traversal allocates a slot for a replacement mesh even
            // when the original limb was empty, shifting all subsequent indices.
            for (int j = 0; j < info->limbDLCount; ++j)
                if (info->limbDLs[j].limbIndex == index + 1 && info->limbDLs[j].dlPath && !info->limbDLs[j].hide)
                    return false;
        }
        if (limb->child != LIMB_DONE)
            pending.push_back({ limb->child, node.second + 1 });
        if (limb->sibling != LIMB_DONE)
            pending.push_back({ limb->sibling, node.second });
    }
    if (skel->skeletonType == kCfaSkeletonFlex) {
        const auto* flex = (const CfaLoadedFlexSkeletonHeader*)skel;
        if (flex->dListCount < meshCount || flex->dListCount > skel->limbCount)
            return false;
    }
    return true;
}

inline bool CfaValidateRigMeshes(const CfaLoadedSkeletonHeader* skel,
                                 const std::shared_ptr<Ship::ResourceManager>& rm) {
    Ship::ResourceManagerScope scope(rm);
    for (int i = 0; i < skel->limbCount; ++i) {
        if (!skel->segment[i])
            return false;
        const auto* limb = (const StandardLimb*)skel->segment[i];
        const char* path = (const char*)limb->dList;
        if (!path || !CfaIsOtrPath(path))
            continue; // loaded raw-pointer display lists retain factory storage
        const auto resource = rm->LoadResource(path);
        if (!CfaResourceType(resource, 0x4F444C54, sizeof(Gfx)) || resource->GetPointerSize() % sizeof(Gfx))
            return false;
    }
    return true;
}

inline bool CfaPreflightComposite(const CwItemAnimDrawInfo* info, const std::shared_ptr<Ship::ResourceManager>& rm) {
    // Legacy recipes retain their historical resource policy. New detailed souls
    // validate their explicit materials and meshes before either graphics stream.
    if (!info->partCount && !info->fallbackTokenPath)
        return true;
    Ship::ResourceManagerScope scope(rm);
    const auto exists = [&](const char* path) {
        if (!path)
            return true;
        auto resource = rm->LoadResource(path);
        return CfaResourceType(resource, 0x4F444C54, sizeof(Gfx)) && resource->GetPointerSize() % sizeof(Gfx) == 0;
    };
    const auto segmentExists = [&](const char* path) {
        auto resource = path ? rm->LoadResource(path) : nullptr;
        return CfaResourceType(resource, 0x4F544558, 1) ||
               (CfaResourceType(resource, 0x4F444C54, sizeof(Gfx)) && resource->GetPointerSize() % sizeof(Gfx) == 0);
    };
    for (int i = 0; i < info->partCount; ++i) {
        const auto& part = info->parts[i];
        if (!exists(part.dlPath) || !exists(part.alternateDlPath))
            return false;
        for (int j = 0; j < part.frameCount; ++j)
            if (!exists(part.framePaths[j]))
                return false;
    }
    for (int i = 0; i < info->segCount; ++i)
        if (info->segs[i].kind == CW_ANIM_SEG_PATH && !segmentExists(info->segs[i].path))
            return false;
    for (int i = 0; i < info->limbDLCount; ++i)
        if (!exists(info->limbDLs[i].dlPath) || !exists(info->limbDLs[i].postDlPath))
            return false;
    return exists(info->postModelDlPath);
}
inline CfaTexAnimEntry* CfaGetTexAnim(const char* path, const char* game,
                                      const std::shared_ptr<Ship::ResourceManager>& rm) {
    if (!path)
        return nullptr;
    const std::string key = std::string(game) + (rm->IsAltAssetsEnabled() ? "|alt|" : "|vanilla|") + path;
    auto found = sCfaTexAnimCache.find(key);
    if (found == sCfaTexAnimCache.end()) {
        CfaTexAnimEntry entry;
        Ship::ResourceManagerScope scope(rm);
        entry.res = rm->LoadResource(path);
        entry.mat = CfaResourceType(entry.res, 0x4F54414E, sizeof(CfaMatEntry))
                        ? (const CfaMatEntry*)entry.res->GetRawPointer()
                        : nullptr;
        entry.ok = CfaValidateTexAnim(entry.mat, entry.res ? entry.res->GetPointerSize() / sizeof(CfaMatEntry) : 0);
        if (!entry.ok)
            return nullptr;
        found = sCfaTexAnimCache.emplace(key, std::move(entry)).first;
    }
    return &found->second;
}
inline void CfaApplyCompositeTransform(const CwAnimTransform& t) {
    Vec3f pos{ t.translate[0], t.translate[1], t.translate[2] };
    Vec3s rot{ t.rotate[0], t.rotate[1], t.rotate[2] };
    Matrix_TranslateRotateZYX(&pos, &rot);
    Matrix_Scale(t.scale[0] == 0 ? 1.f : t.scale[0], t.scale[1] == 0 ? 1.f : t.scale[1],
                 t.scale[2] == 0 ? 1.f : t.scale[2], MTXMODE_APPLY);
}
inline void CfaBuildCompositePalette(PlayState* play, const CwItemAnimDrawInfo* info) {
    if (!info->matrixCount)
        return;
    auto* palette = (Mtx*)CFA_ALLOC(play->state.gfxCtx, info->matrixCount * sizeof(Mtx));
    for (int i = 0; i < info->matrixCount; ++i) {
        Matrix_Push();
        CfaApplyCompositeTransform(info->matrices[i]);
        CFA_TO_MTX(&palette[i]);
        Matrix_Pop();
    }
    const int segment = info->matrixSegment ? info->matrixSegment : 13;
    OPEN_DISPS(play->state.gfxCtx);
    gSPSegment(POLY_OPA_DISP++, segment, (uintptr_t)palette);
    gSPSegment(POLY_XLU_DISP++, segment, (uintptr_t)palette);
    CLOSE_DISPS(play->state.gfxCtx);
}
inline void CfaBindColorSegments(PlayState* play, const CwItemAnimDrawInfo* info) {
    for (int i = 0; i < info->colorSegmentCount; ++i) {
        const auto& c = info->colorSegments[i];
        auto* dl = (Gfx*)CFA_ALLOC(play->state.gfxCtx, sizeof(Gfx) * 3);
        gDPSetPrimColor(dl, 0, 0, c.prim[0], c.prim[1], c.prim[2], c.prim[3]);
        gDPSetEnvColor(dl + 1, c.env[0], c.env[1], c.env[2], c.env[3]);
        gSPEndDisplayList(dl + 2);
        OPEN_DISPS(play->state.gfxCtx);
        if (c.onOpa)
            gSPSegment(POLY_OPA_DISP++, c.segment, (uintptr_t)dl);
        if (c.onXlu)
            gSPSegment(POLY_XLU_DISP++, c.segment, (uintptr_t)dl);
        CLOSE_DISPS(play->state.gfxCtx);
    }
}
inline void CfaDrawParts(PlayState* play, const CwItemAnimDrawInfo* info, bool before) {
    for (int i = 0; i < info->partCount; ++i) {
        const auto& part = info->parts[i];
        if ((part.beforeSkeleton != 0) != before)
            continue;
        Matrix_Push();
        CfaApplyCompositeTransform(part.transform);
        if (part.billboard)
            Matrix_ReplaceRotation(&play->billboardMtxF);
        const char* path =
            part.frameCount
                ? part.framePaths[play->state.frames % part.frameCount]
                : (part.alternateDlPath && (play->state.frames & part.frameMask) ? part.alternateDlPath : part.dlPath);
        OPEN_DISPS(play->state.gfxCtx);
        Gfx*& stream = part.xlu ? POLY_XLU_DISP : POLY_OPA_DISP;

        if (part.hasPrimColor)
            gDPSetPrimColor(stream++, 0, part.primLodFrac, part.primColor[0], part.primColor[1], part.primColor[2],
                            part.primColor[3]);
        if (part.hasEnvColor)
            gDPSetEnvColor(stream++, part.envColor[0], part.envColor[1], part.envColor[2], part.envColor[3]);
        if (part.materialProfile == CW_ANIM_MATERIAL_FREEZARD) {
            gDPSetCombineLERP(stream++, TEXEL1, PRIMITIVE, PRIM_LOD_FRAC, TEXEL0, TEXEL1, TEXEL0, PRIMITIVE, TEXEL0,
                              PRIMITIVE, ENVIRONMENT, COMBINED, ENVIRONMENT, COMBINED, 0, ENVIRONMENT, 0);
        }
        CFA_LOAD_MTX(stream++, play->state.gfxCtx);
        gSPDisplayList(stream++, CfaRouteLimbDList((Gfx*)path));
        CLOSE_DISPS(play->state.gfxCtx);
        Matrix_Pop();
    }
}
inline void CfaRestoreCompositeSegments(PlayState* play, const CwItemAnimDrawInfo* info) {
    int32_t segments[7];
    int count = 0;
    if (info->matrixCount)
        segments[count++] = info->matrixSegment ? info->matrixSegment : 13;
    for (int i = 0; i < info->colorSegmentCount; ++i)
        segments[count++] = info->colorSegments[i].segment;
    CfaRestoreSegs(play, segments, count);
}
#endif
