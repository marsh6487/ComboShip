struct RendererPoseResource : Ship::IResource {
    std::vector<Vec3s> joints;
    explicit RendererPoseResource(int count) : joints(count) { metadata->Type = 0x4F50414D; }
    void* GetRawPointer() override { return joints.data(); }
    size_t GetPointerSize() override { return joints.size() * sizeof(Vec3s); }
};
struct RendererMaterialResource : Ship::IResource {
    CfaTexScrollEntry scroll[2]{{1, 2, 32, 32}, {3, 4, 16, 16}};
    CfaMatEntry mat;
    explicit RendererMaterialResource(int segment) : mat{(int8_t)-segment, 1, scroll} { metadata->Type = 0x4F54414E; }
    void* GetRawPointer() override { return &mat; }
    size_t GetPointerSize() override { return sizeof(mat); }
};
static void RunRendererFailureChecks() {
    for (const char* game : {"oot", "mm"}) {
        auto owner = std::make_shared<Ship::ResourceManager>();
        auto host = std::make_shared<Ship::ResourceManager>();
        Ship::CrossRMRegistry::owners[game] = owner;
        Ship::Context::GetRawInstance()->rm = host;
        CfaClearCaches();
        auto rig = std::make_shared<SkeletonResource>(0);
        auto anim = std::make_shared<AnimationResource>();
        owner->normal["__OTR__rig"] = rig;
        owner->normal["__OTR__anim"] = anim;
        CwItemAnimDrawInfo info{};
        info.opa = 1; info.limbCount = 3; info.skelPath = "__OTR__rig"; info.animPath = "__OTR__anim";
        auto reject = [&](const CwItemAnimDrawInfo& recipe) {
            GraphicsContext gfx; PlayState play{{&gfx, 41}, 41, {}};
            current = {}; current.x = 123; current.sy = 3;
            assert(!ComboForeignAnim_Draw(&recipe, game, &play));
            assert(gfx.op == gfx.opa && gfx.xp == gfx.xlu && matrices.empty());
            assert(current.x == 123 && current.sy == 3 && Ship::Context::GetRawInstance()->rm == host);
            assert(!sCfaInfo);
        };
        auto draw = [&](const CwItemAnimDrawInfo& recipe) {
            GraphicsContext gfx; PlayState play{{&gfx, 41}, 41, {}};
            current = {}; current.x = 123; current.sy = 3;
            assert(ComboForeignAnim_Draw(&recipe, game, &play));
            assert(current.x == 123 && current.sy == 3 && matrices.empty());
            assert(Ship::Context::GetRawInstance()->rm == host && !sCfaInfo);
            assert(gfx.op != gfx.opa);
        };
        // Rejected resource is retried in this same frame after it becomes resident.
        owner->normal.erase("__OTR__anim"); reject(info);
        owner->normal["__OTR__anim"] = anim; draw(info);
        CfaClearCaches(); rig->limbs[1].child = 0; reject(info); rig->limbs[1].child = 255;
        CfaClearCaches(); rig->limbs[1].child = 2; reject(info); rig->limbs[1].child = 255;
        CfaClearCaches(); rig->pointers[1] = nullptr; reject(info); rig->pointers[1] = &rig->limbs[1];
        CfaClearCaches(); anim->rotationIndices.resize(2); reject(info); anim->rotationIndices.resize(256);
        CfaClearCaches(); anim->rotationIndices[2].z = 256; reject(info); anim->rotationIndices[2].z = 0;
        CfaClearCaches(); anim->type = SOH::AnimationType::Link; reject(info); anim->type = SOH::AnimationType::Normal;
        CfaClearCaches(); anim->metadata->Type = 0x4F50414D; reject(info); anim->metadata->Type = 0x4F414E4D;
        anim->animationData.animationHeader.jointIndices = anim->rotationIndices.data();
        CfaClearCaches(); draw(info);
        // Native callback exceptions after the first mesh must roll back both
        // command heads, nested native matrix pushes and interpolation nesting.
        for (int point : {0, 1}) {
            throwAtDisplayList = point;
            fixtureRecordingDepth = 2;
            reject(info);
            assert(fixtureRecordingDepth == 2);
            fixtureRecordingDepth = 0;
            draw(info);
        }
        CfaClearCaches(); owner->throwPath = "__OTR__rig"; reject(info); owner->throwPath.clear();
        // Interrupted skeleton load retries on the very next draw in this frame.
        draw(info);
        // An empty flex limb cannot grow a palette slot through a replacement DL.
        CfaClearCaches(); rig->h.sh.skeletonType = 1;
        rig->limbs[1].dList = nullptr; rig->h.dListCount = 1;
        info.limbDLCount = 1; info.limbDLs[0].limbIndex = 2; info.limbDLs[0].dlPath = "__OTR__replacement";
        reject(info);
        info.limbDLCount = 0; rig->limbs[1].dList = (Gfx*)rig->jaw.c_str(); rig->h.dListCount = 2;
        rig->h.sh.skeletonType = 0; CfaClearCaches(); draw(info);
        // A cached model must also respect the caller's current stack headroom.
        for (int i = 0; i < 16; ++i) Matrix_Push();
        { GraphicsContext gfx; PlayState p{{&gfx,41},41,{}};
          const size_t depth = matrices.size();
          assert(!ComboForeignAnim_Draw(&info, game, &p));
          assert(matrices.size() == depth && gfx.op == gfx.opa && gfx.xp == gfx.xlu); }
        for (int i = 0; i < 16; ++i) Matrix_Pop();
        // A valid acyclic table can still exceed the engine's 20-slot stack.
        struct DeepRig : Ship::IResource {
            CfaLoadedSkeletonHeader h{}; StandardLimb limbs[20]{}; void* ptrs[20]{};
            DeepRig() {
                h = {ptrs,20,0};
                for (int i = 0; i < 20; ++i) { ptrs[i] = &limbs[i]; if (i < 19) limbs[i].child = i + 1; }
            }
            void* GetRawPointer() override { return &h; }
        };
        owner->normal["__OTR__deep"] = std::make_shared<DeepRig>();
        auto deep = info; deep.skelPath = "__OTR__deep"; deep.limbCount = 21; reject(deep);
        rig->limbs[0].sibling = 1; CfaClearCaches(); reject(info); rig->limbs[0].sibling = 255;

        // Iron Knuckle-style post-Xlu geometry needs its own stream setup even
        // when the recipe has no static parts and no flame.
        info.limbDLCount = 1; info.limbDLs[0] = {};
        info.limbDLs[0].limbIndex = 2; info.limbDLs[0].postDlPath = "__OTR__post"; info.limbDLs[0].postXlu = 1;
        { GraphicsContext gfx; PlayState p{{&gfx,41},41,{}};
          assert(ComboForeignAnim_Draw(&info, game, &p));
          bool setup = false, mesh = false;
          for (Gfx* cmd = gfx.xlu; cmd < gfx.xp; ++cmd) {
            if (cmd->kind == 10) setup = true;
            if (cmd->kind == 3) { assert(setup); mesh = true; }
          }
          assert(mesh); }
        info.limbDLCount = 0;
        // The selected type and owner Alt state are sampled each draw, including
        // two draws in one host frame. Bad selected replacement safely retries.
        owner->alt = true; reject(info);
        owner->replacement["__OTR__rig"] = std::make_shared<SkeletonResource>(1);
        owner->replacement["__OTR__anim"] = anim; draw(info);
        owner->alt = false; draw(info);
        // OPAM frame is loaded from its owner, copied with exact joint bound and
        // held still; no player animation task or gameplay actor is involved.
        info.bindPose = 1; info.animPath = nullptr; info.posePath = "__OTR__pose";
        reject(info);
        auto pose = std::make_shared<RendererPoseResource>(2);
        owner->normal[info.posePath] = pose; reject(info);
        pose->joints.resize(3); pose->joints[0] = {57, 3377, 9};
        pose->metadata->Type = 0x4F414E4D; reject(info);
        pose->metadata->Type = 0x4F50414D;
        int steps = updates; draw(info); assert(updates == steps);
        bool foundPose = false;
        for (const auto& pair : sCfaSkelCache) if (pair.second.poseRes) {
            const auto& j = pair.second.jointTable[0];
            assert(j.x == 57 && j.y == 3377 && j.z == 9); foundPose = true;
        }
        assert(foundPose);
        // Full composite material/mesh preflight precedes all graphics. Failure
        // of the second material must not leave the first material's segment.
        info = {}; info.opa = 1; info.partCount = 1; info.parts[0].dlPath = "__OTR__body";
        reject(info);
        owner->normal["__OTR__body"] = std::make_shared<AnimationResource>(); reject(info);
        owner->normal["__OTR__body"] = std::make_shared<RendererDisplayListResource>();
        owner->throwPath = "__OTR__body"; reject(info); owner->throwPath.clear();
        auto wrongMesh = info; wrongMesh.postModelDlPath = "__OTR__anim"; reject(wrongMesh);
        wrongMesh = info; wrongMesh.limbDLCount = 1; wrongMesh.limbDLs[0].postDlPath = "__OTR__anim"; reject(wrongMesh);
        info.texAnimPath = "__OTR__material"; info.texAnimPath2 = "__OTR__material2";
        auto first = std::make_shared<RendererMaterialResource>(1);
        auto second = std::make_shared<RendererMaterialResource>(2);
        owner->normal[info.texAnimPath] = first; reject(info);
        owner->normal[info.texAnimPath2] = second;
        owner->throwPath = info.texAnimPath2; reject(info); owner->throwPath.clear();
        info.texAnimStep = 24;
        scrollRequests.clear(); draw(info);
        assert(scrollRequests.size() == 2 && scrollRequests[0][1] == 23 && scrollRequests[1][1] == 41);
        owner->replacement = owner->normal;
        auto altMaterial = std::make_shared<RendererMaterialResource>(1); altMaterial->scroll[0].xStep = 5;
        owner->replacement[info.texAnimPath] = altMaterial;
        owner->alt = true; scrollRequests.clear(); draw(info); assert(scrollRequests[0][1] == 115);
        owner->alt = false;
        // Unterminated animation resource stops at its actual loaded byte size.
        CfaClearCaches(); first->mat.segment = 1; reject(info); first->mat.segment = -1;
        CfaClearCaches(); second->mat.segment = -7; reject(info); second->mat.segment = -2;
        CfaClearCaches(); second->metadata->Type = 0x4F414E4D; reject(info); second->metadata->Type = 0x4F54414E;
        info.texAnimPath = info.texAnimPath2 = nullptr;
        auto bad = info; bad.partCount = CW_ANIM_MAX_PARTS + 1; reject(bad);
        bad = info; bad.matrixCount = CW_ANIM_MAX_MATRICES + 1; reject(bad);
        bad = info; bad.matrixCount = 1; bad.matrixSegment = 7; reject(bad);
        bad = info; bad.parts[0].transform.scale[0] = NAN; reject(bad);
        bad = info; bad.parts[0].dlPath = "unrouted"; reject(bad);
        bad = info; bad.parts[0].frameCount = 10; reject(bad);
        bad = info; bad.parts[0].frameCount = 1; reject(bad);
        bad = info; bad.colorSegmentCount = 7; reject(bad);
        bad = info; bad.colorSegmentCount = 1; bad.colorSegments[0].segment = 8; reject(bad);
        info.parts[0].frameCount = 2;
        info.parts[0].framePaths[0] = "__OTR__body";
        info.parts[0].framePaths[1] = "__OTR__alternate";
        reject(info); owner->normal["__OTR__alternate"] = std::make_shared<RendererDisplayListResource>(); draw(info);
        CfaClearCaches();
    }
    std::cout << "Both owners: malformed rigs/animations/OPAM, bounded native stack, flex slot safety, typed meshes, injected factory/GBI rollback, XLU setup, exact-frame retries and dual material/Alt caches passed\n";
}
