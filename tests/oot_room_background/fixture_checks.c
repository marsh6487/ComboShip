/* Contract assertions for the actual extracted production functions. */
static void reset_fixture(void) {
    memset(&fixture, 0, sizeof(fixture));
    memset(gSegments, 0, sizeof(gSegments));
    gGameInfo = &fixture.gameInfo;
    allowImageLookup = allowBackgroundDraw = 1;
    hookCameraSetting = -1;
    imageHookCalls = opaqueSetupCalls = translucentSetupCalls = backgroundCalls = 0;
    backgroundPolicyCalls = 0;
    drawnBackground = NULL;
    drawnWidth = drawnHeight = 0;
    drawnOffsetX = drawnOffsetY = 0;
    fixture.play.cameraPtrs[0] = &fixture.camera;
    fixture.play.activeCamera = 0;
    fixture.play.state.gfxCtx = &fixture.graphics;
    fixture.play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &fixture.player.actor;
    fixture.play.colCtx.colHeader = &fixture.collision;
    fixture.player.actor.params = (s16)0xab5a;
    fixture.camera.setting = CAM_SET_PREREND_FIXED;
    fixture.camera.camDataIdx = 3;
    fixture.collision.cameraDataList = fixture.cameraData;
    fixture.collision.cameraDataListLen = 4;
    fixture.cameraData[3].cameraSType = CAM_SET_PREREND_FIXED;
    fixture.cameraData[3].numCameras = 3;
    fixture.cameraData[3].camPosData = fixture.cameraVectors;
    fixture.cameraVectors[2].y = 7;
    fixture.backgrounds[0].id = 3;
    fixture.backgrounds[0].source = &fixture.backgroundPixels[0];
    fixture.backgrounds[0].width = 256;
    fixture.backgrounds[0].height = 128;
    fixture.backgrounds[1].id = 7;
    fixture.backgrounds[1].source = &fixture.backgroundPixels[1];
    fixture.backgrounds[1].width = 320;
    fixture.backgrounds[1].height = 240;
    fixture.backgrounds[1].fmt = G_IM_FMT_RGBA;
    fixture.backgrounds[1].siz = G_IM_SIZ_16b;
    fixture.polygons.opa = fixture.opaqueList;
    fixture.polygons.xlu = fixture.translucentList;
    fixture.mesh.polygon1.base.type = 1;
    fixture.mesh.polygon1.format = 2;
    fixture.mesh.polygon1.dlist = (Gfx*)&fixture.polygons;
    fixture.mesh.polygon1.multi.count = 2;
    fixture.mesh.polygon1.multi.list = fixture.backgrounds;
    fixture.room.meshHeader = &fixture.mesh;
    fixture.graphics.polyOpa.p = fixture.opaqueBuffer;
    fixture.graphics.polyXlu.p = fixture.translucentBuffer;
}

static int emitted_list_count(Gfx* start, Gfx* end, Gfx* expected) {
    int count = 0;
    assert(end >= start && end - start < 64);
    for (Gfx* packet = start; packet < end; ++packet) {
        if (((packet->words.w0 >> 24) & 0xff) == G_DL) {
            assert(packet->words.w1 == (uintptr_t)expected);
            ++count;
        }
    }
    return count;
}

static void check_geometry(u32 flags) {
    int opa = (flags & 1) != 0;
    int xlu = (flags & 2) != 0;
    assert(opaqueSetupCalls == opa);
    assert(translucentSetupCalls == xlu);
    assert(emitted_list_count(fixture.opaqueBuffer, fixture.graphics.polyOpa.p,
                              fixture.opaqueList) == opa);
    assert(emitted_list_count(fixture.translucentBuffer, fixture.graphics.polyXlu.p,
                              fixture.translucentList) == xlu);
}

int main(int argc, char** argv) {
    assert(argc == 2);
    const char* mode = argv[1];
    if (!strcmp(mode, "layout")) {
        printf("{\"PlayState.cameraPtrs\":%zu,\"PlayState.colCtx\":%zu,"
               "\"PlayState.actorCtx\":%zu,\"CollisionContext.colHeader\":%zu,"
               "\"Camera.setting\":%zu,\"Camera.camDataIdx\":%zu,"
               "\"CollisionHeader.cameraDataList\":%zu,\"CamData.size\":%zu,"
               "\"CamData.camPosData\":%zu,\"Vec3s.size\":%zu,"
               "\"BgCamData.remapYOffset\":%zu,\"VB_SHOULD_LOAD_BG_IMAGE\":%d}\n",
               offsetof(PlayState, cameraPtrs), offsetof(PlayState, colCtx),
               offsetof(PlayState, actorCtx), offsetof(CollisionContext, colHeader),
               offsetof(Camera, setting), offsetof(Camera, camDataIdx),
               offsetof(CollisionHeader, cameraDataList), sizeof(CamData),
               offsetof(CamData, camPosData), sizeof(Vec3s),
               2 * sizeof(Vec3s) + offsetof(Vec3s, y), VB_SHOULD_LOAD_BG_IMAGE);
        return 0;
    }
    long pageSize = sysconf(_SC_PAGESIZE);
    assert(pageSize > 0);
    void* inaccessible = mmap(NULL, (size_t)pageSize, PROT_NONE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(inaccessible != MAP_FAILED);
    const u32 drawFlags[] = { 3, 1, 2, 0 };
    for (unsigned int i = 0; i < sizeof(drawFlags) / sizeof(drawFlags[0]); ++i) {
        u32 flags = drawFlags[i];
        reset_fixture();
        int fixed = 1, veto = 0, sourceNull = 0, remap = 1;
        int backgroundCameraFixed = 1;
        if (!strcmp(mode, "fixed-remap")) {
            /* Valid native camera data remaps ID 3 to background ID 7. */
        } else if (!strcmp(mode, "fixed-negative-remap")) {
            fixture.cameraVectors[2].y = -1;
            remap = 0;
        } else if (!strcmp(mode, "fixed-null-source")) {
            fixture.backgrounds[1].source = NULL;
            sourceNull = 1;
        } else if (!strcmp(mode, "fixed-hook-veto")) {
            allowImageLookup = 0;
            veto = 1;
        } else if (!strcmp(mode, "door-hook-restores-fixed")) {
            fixture.camera.setting = CAM_SET_DOOR0;
            hookCameraSetting = CAM_SET_PREREND_FIXED;
            /* Preserve caller's existing pre-hook draw eligibility timing. */
            backgroundCameraFixed = 0;
        } else if (!strcmp(mode, "fixed-hook-switches-door")) {
            hookCameraSetting = CAM_SET_DOOR0;
            fixture.cameraData[3].camPosData = (Vec3s*)inaccessible;
            fixed = 0;
        } else if (!strcmp(mode, "door-protected-camera-data") ||
                   !strcmp(mode, "free-protected-camera-data") ||
                   !strcmp(mode, "door-hook-veto-control")) {
            fixed = 0;
            backgroundCameraFixed = 0;
            fixture.camera.setting = !strcmp(mode, "free-protected-camera-data")
                                         ? CAM_SET_FREE0 : CAM_SET_DOOR0;
            fixture.cameraData[3].camPosData = (Vec3s*)inaccessible;
            /* This simulates the existing background draw veto being enabled.
             * An eager camera metadata read happens before this boundary. */
            allowBackgroundDraw = 0;
            if (!strcmp(mode, "door-hook-veto-control")) {
                allowImageLookup = 0;
                veto = 1;
            }
        } else if (!strcmp(mode, "door-protected-image-list")) {
            fixed = 0;
            backgroundCameraFixed = 0;
            fixture.camera.setting = CAM_SET_DOOR0;
            fixture.mesh.polygon1.multi.list = (BgImage*)inaccessible;
            allowBackgroundDraw = 0;
        } else {
            fprintf(stderr, "Unknown case: %s\n", mode);
            return 2;
        }

        fprintf(stderr, "CASE %s flags=%u cameraSetting=0x%x camId=%d protected=%p\n",
                mode, flags, fixture.camera.setting, fixture.camera.camDataIdx, inaccessible);
        fflush(stderr);
        func_80096B6C(&fixture.play, &fixture.room, flags);
        check_geometry(flags);
        assert(fixture.camera.camDataIdx == 3);
        if (hookCameraSetting >= 0) {
            assert(fixture.camera.setting == hookCameraSetting);
        }
        if (fixed && !veto) {
            int index = remap ? 1 : 0;
            assert(imageHookCalls == 1);
            assert((u16)fixture.player.actor.params == (u16)(remap ? 0xab07 : 0xab03));
            int draws = (flags & 1) && backgroundCameraFixed && !sourceNull;
            assert(backgroundCalls == draws);
            if (draws) {
                assert(drawnBackground == fixture.backgrounds[index].source);
                assert(drawnWidth == fixture.backgrounds[index].width);
                assert(drawnHeight == fixture.backgrounds[index].height);
                assert(drawnOffsetX > 59.999f && drawnOffsetX < 60.001f);
                assert(drawnOffsetY > 59.999f && drawnOffsetY < 60.001f);
            }
        } else {
            assert(backgroundCalls == 0 && backgroundPolicyCalls == 0);
            assert((u16)fixture.player.actor.params == 0xab5a);
            assert(imageHookCalls == 1 && "Existing image hooks must retain their side effects");
        }
    }
    assert(munmap(inaccessible, (size_t)pageSize) == 0);
    printf("PASS %s: actual native lookup/draw path, opaque/xlu flags, identity and image arguments\n", mode);
    return 0;
}
