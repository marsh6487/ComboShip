// Included after the native traversal fixture and its resource boundary types.
#include "catalog_production.inc"

struct SoulCatalogRawResource : Ship::IResource {
    Gfx commands[2]{};
    SoulCatalogRawResource() { metadata->Type = 0x4F444C54; }
    void* GetRawPointer() override { return commands; }
    size_t GetPointerSize() override { return sizeof(commands); }
};

struct SoulCatalogSkeletonResource : SOH::Skeleton {
    CfaLoadedFlexSkeletonHeader header{};
    std::vector<StandardLimb> limbs;
    std::vector<std::string> names;
    explicit SoulCatalogSkeletonResource(const CwItemAnimDrawInfo& info, bool replacement)
        : limbs(info.limbCount - 1), names(limbs.size()) {
        metadata->Type = 0x4F534B4C;
        skeletonHeaderSegments.resize(limbs.size());
        for (size_t i = 0; i < limbs.size(); ++i) {
            names[i] = "__OTR__fixture/soul_limb_" + std::to_string(i);
            limbs[i].dList = (Gfx*)names[i].c_str();
            limbs[i].jointPos = {(s16)(i + 1), 2, 3};
            // A valid bounded binary hierarchy exercises all limbs without
            // inventing a 30+ deep chain that exceeds the native 20-slot stack.
            // Real owner hierarchies are checked separately against archives.
            if (2 * i + 1 < limbs.size()) limbs[i].child = (u8)(2 * i + 1);
            if ((i & 1) && i + 1 < limbs.size()) limbs[i].sibling = (u8)(i + 1);
            skeletonHeaderSegments[i] = &limbs[i];
        }
        const bool flex = replacement ? info.nonFlexSkeleton != 0 : info.nonFlexSkeleton == 0;
        header.sh = {skeletonHeaderSegments.data(), (u8)limbs.size(), (u8)flex};
        header.dListCount = flex ? (u8)limbs.size() : 0;
        // Exercise the shipped Fsn header correction through the same production
        // validation that handles the 17-entry native resource table.
        if (!replacement && std::strstr(info.skelPath, "objects/object_fsn/gFsnSkel")) header.sh.limbCount = 18;
    }
    void* GetRawPointer() override { return &header; }
    size_t GetPointerSize() override { return sizeof(header); }
};

struct SoulCatalogPoseResource : Ship::IResource {
    std::vector<Vec3s> pose;
    explicit SoulCatalogPoseResource(int count) : pose(count) {
        metadata->Type = 0x4F50414D;
        for (int i = 0; i < count; ++i) pose[i] = {(s16)i, (s16)(i * 2), (s16)-i};
    }
    void* GetRawPointer() override { return pose.data(); }
    size_t GetPointerSize() override { return pose.size() * sizeof(Vec3s); }
};

struct SoulCatalogMaterialResource : Ship::IResource {
    CfaTexScrollEntry params[2]{{1, 2, 16, 32}, {0, -1, 16, 32}};
    CfaPrimColor prim[4]{{120, 240, 30, 255, 0}, {20, 40, 210, 180, 100},
                        {240, 30, 40, 255, 200}, {240, 30, 40, 255, 200}};
    CfaEnvColor env[4]{{160, 40, 100, 255}, {30, 210, 20, 180},
                      {240, 10, 180, 255}, {240, 10, 180, 255}};
    uint16_t keys[4]{0, 12, 23, 24};
    CfaColorParams color{24, 3, prim, env, keys};
    std::vector<CfaMatEntry> entries;
    explicit SoulCatalogMaterialResource(const char* path) {
        metadata->Type = 0x4F54414E;
        // Preserve every actual catalog material's types, segment list, period
        // and declared table counts. Colors/key positions are invented fixture
        // values; the fourth LERP entry models the factory's owned terminal data.
        if (std::strstr(path, "gGomessCoreMatAnim")) {
            entries = {{2, 3, &color}, {3, 1, params}, {-4, 1, params}};
        } else if (std::strstr(path, "gGomessBodyMatAnim")) {
            color.keyFrameCount = 2; color.envColors = nullptr; keys[1] = 24;
            entries = {{-1, 4, &color}};
        } else if (std::strstr(path, "gFamosNormalGlowingEmblemTexAnim")) {
            color.keyFrameLength = 12; keys[1] = 6; keys[2] = 12;
            entries = {{-1, 4, &color}};
        } else if (std::strstr(path, "gEyegoreEyeLaserTexAnim")) {
            color.keyFrameLength = 2; color.keyFrameCount = 0; color.keyFrames = nullptr;
            entries = {{1, 2, &color}, {-2, 1, params}};
        } else if (std::strstr(path, "gChuchuSlimeFlowTexAnim")) {
            entries = {{-3, 1, params}};
        } else {
            std::cerr << "Unmodeled catalog material " << path << '\n';
            assert(false);
        }
    }
    void* GetRawPointer() override { return entries.data(); }
    size_t GetPointerSize() override { return entries.size() * sizeof(CfaMatEntry); }
};

static void RegisterSoulCatalogResources(const CwItemAnimDrawInfo& info,
                                         std::shared_ptr<Ship::ResourceManager> owner) {
    auto add = [&](const char* path) {
        if (!path) return;
        owner->normal[path] = std::make_shared<SoulCatalogRawResource>();
        owner->replacement[path] = std::make_shared<SoulCatalogRawResource>();
    };
    for (int i = 0; i < info.segCount; ++i) add(info.segs[i].path);
    for (int i = 0; i < info.limbDLCount; ++i) {
        add(info.limbDLs[i].dlPath); add(info.limbDLs[i].postDlPath);
    }
    for (int i = 0; i < info.partCount; ++i) {
        add(info.parts[i].dlPath); add(info.parts[i].alternateDlPath);
        for (int j = 0; j < info.parts[i].frameCount; ++j) add(info.parts[i].framePaths[j]);
    }
    add(info.postModelDlPath); add(info.flameDlPath); add(info.fallbackTokenPath);
    for (const char* path : {info.texAnimPath, info.texAnimPath2}) {
        if (!path) continue;
        owner->normal[path] = std::make_shared<SoulCatalogMaterialResource>(path);
        owner->replacement[path] = std::make_shared<SoulCatalogMaterialResource>(path);
    }
    if (info.skelPath) {
        auto native = std::make_shared<SoulCatalogSkeletonResource>(info, false);
        auto replacement = std::make_shared<SoulCatalogSkeletonResource>(info, true);
        for (const auto& path : native->names) add(path.c_str());
        owner->normal[info.skelPath] = native;
        owner->replacement[info.skelPath] = replacement;
    }
    if (info.animPath) {
        owner->normal[info.animPath] = std::make_shared<AnimationResource>();
        owner->replacement[info.animPath] = std::make_shared<AnimationResource>();
    }
    if (info.posePath) {
        owner->normal[info.posePath] = std::make_shared<SoulCatalogPoseResource>(info.limbCount);
        owner->replacement[info.posePath] = std::make_shared<SoulCatalogPoseResource>(info.limbCount);
    }
}

static void CheckSoulCatalogDraw(const CwItemAnimDrawInfo& info, const char* ownerName, const char* label,
                                 int& draws) {
    auto owner = std::make_shared<Ship::ResourceManager>();
    auto host = std::make_shared<Ship::ResourceManager>();
    Ship::CrossRMRegistry::owners[ownerName] = owner;
    Ship::Context::GetRawInstance()->rm = host;
    RegisterSoulCatalogResources(info, owner);
    CfaClearCaches();
    for (bool alt : {false, true, false}) {
        owner->alt = alt;
        for (uint32_t frame : {0u, 1u, 8u, 23u}) {
            GraphicsContext c; PlayState p{{&c, frame}, frame, {}};
            current = {}; current.x = 19; current.y = 23; current.sx = 2;
            if (!ComboForeignAnim_Draw(&info, ownerName, &p)) {
                std::cerr << "Soul catalog draw rejected " << ownerName << ":" << label
                          << " alt=" << alt << " frame=" << frame << '\n';
                assert(false);
            }
            int body = 0;
            for (auto range : {std::pair{c.opa, c.op}, std::pair{c.xlu, c.xp}}) {
                std::vector<std::string> scopes;
                for (Gfx* cmd = range.first; cmd < range.second; ++cmd) {
                    if (cmd->kind == 8) {
                        scopes.emplace_back((const char*)cmd->target);
                    }
                    if (cmd->kind == 9) { assert(!scopes.empty()); scopes.pop_back(); }
                    if (cmd->kind == 3) {
                        const char* path = (const char*)cmd->target;
                        if (!std::strncmp(path, "__OTR__", 7)) {
                            // MM deliberately draws both owners' soul flames with
                            // its native flame pass; model resources retain owner routing.
                            if (std::strstr(path, "gBossSoulFlameDL") ||
                                std::strstr(path, "gameplay_keep_DL_01ACF0")) continue;
                            const std::string expected = std::string("__OTR__@") + ownerName + ":";
                            assert(!std::strncmp(path, expected.c_str(), expected.size()));
                            assert(!scopes.empty() && scopes.back() == ownerName);
                            ++body;
                        }
                    }
                }
                assert(scopes.empty());
            }
            assert(body > 0); // A flame-only success is a regression.
            assert(current.x == 19 && current.y == 23 && current.sx == 2 && matrices.empty());
            assert(Ship::Context::GetRawInstance()->rm == host);
            ++draws;
        }
    }
    CfaClearCaches();
}

static void RunSoulCatalogChecks() {
    int records = 0, draws = 0;
    for (const auto& model : ComboSoulMiniOOT::Catalog) {
        CwItemAnimDrawInfo info{};
        assert(ComboSoulMiniOOT::Describe(model.id, &info));
        const std::string label = std::to_string(model.id);
        CheckSoulCatalogDraw(info, "oot", label.c_str(), draws);
        ++records;
    }
    for (size_t i = 0; i < std::size(SoulCatalogMm::EnemyIds); ++i) {
        CwItemAnimDrawInfo info{};
        assert(SoulCatalogMm::MM_FillEnemySoulAnim(SoulCatalogMm::EnemyIds[i], &info));
        CheckSoulCatalogDraw(info, "mm", SoulCatalogMm::EnemyNames[i], draws);
        ++records;
    }
    for (size_t i = 0; i < std::size(SoulCatalogMm::NpcIds); ++i) {
        CwItemAnimDrawInfo info{};
        assert(SoulCatalogMm::MM_FillNpcSoulAnim(SoulCatalogMm::NpcIds[i], &info));
        CheckSoulCatalogDraw(info, "mm", SoulCatalogMm::NpcNames[i], draws);
        ++records;
    }
    assert(records == 189 && draws == 2268);
    std::cout << "Soul catalog: " << records << " production recipes, " << draws
              << " host draws, native/replacement/native assets, four frames, body + owner + state checks passed\n";
}
