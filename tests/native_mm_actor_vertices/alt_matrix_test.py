"""Exercise real skeleton swaps through Adult Zelda's real flex draw path.

The archive lookup and graphics API boundaries are finite test doubles. The
production swap, draw, allocation request and recursive limb traversal execute
unchanged. Both rigs retain 14 limbs and 15 animation entries while their
drawable counts differ (13/9).
This does not render a model or permit changing animation/joint-buffer capacity.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
CASES = ("regular_on", "regular_off", "custom_on", "custom_off",
         "regular_missing", "custom_missing", "custom_fallback")


def function(source, name):
    match = re.search(r"^[^\n;{}]*\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    if match is None:
        raise RuntimeError(f"Missing production function: {name}")
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


FIXTURE = r'''
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>
using u8 = uint8_t;
using s32 = int32_t;
#define REQUIRE(condition) do { if (!(condition)) { \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); exit(1); } } while (0)
struct Gfx { uintptr_t a, b; };
struct Mtx { float values[16]; };
struct Vec3f { float x, y, z; };
struct Vec3s { int16_t x, y, z; };
struct StandardLimb { Vec3s jointPos; uint8_t child, sibling; Gfx* dList; };
struct GraphicsContext { Gfx* opa; };
struct PlayState { struct { GraphicsContext* gfxCtx; } state; };
struct SkeletonHeader { void** segment; uint8_t limbCount, skeletonType; };
struct FlexSkeletonHeader { SkeletonHeader sh; uint8_t dListCount; };
struct SkelAnime {
    void** skeleton;
    SkeletonHeader* skeletonHeader;
    Vec3s* jointTable;
    Vec3s* morphTable;
    int limbCount, dListCount;
};
using OverrideLimbDraw = s32 (*)(PlayState*, s32, Gfx**, Vec3f*, Vec3s*, void*, Gfx**);
using PostLimbDraw = void (*)(PlayState*, s32, Gfx**, Vec3s*, void*, Gfx**);
struct EnViewer {
    struct { SkelAnime skelAnime; } skin;
    struct { unsigned eyeIndex; } staticState;
};
#define SEGMENTED_TO_VIRTUAL(value) (value)
#define MATRIX_TOMTX(value) storeMatrix(value)
#define LIMB_DONE 255
#define G_MTX_LOAD 1
#define MTXMODE_APPLY 1
#define OPEN_DISPS(value)
#define CLOSE_DISPS(value)
#define POLY_OPA_DISP play->state.gfxCtx->opa
#define VT_FGCOL(value) ""
#define VT_RST ""
#define RED 1
static unsigned allocatedMatrices, writtenMatrices;
static Mtx* frameMatrices;
static Mtx* Graph_Alloc(GraphicsContext*, size_t size) {
    REQUIRE(frameMatrices == nullptr && size % sizeof(Mtx) == 0);
    allocatedMatrices = size / sizeof(Mtx);
    return frameMatrices = static_cast<Mtx*>(malloc(size));
}
static void storeMatrix(Mtx* target) {
    ++writtenMatrices;
    // Preserve the graphics boundary's actual 64-byte matrix write. ASan
    // catches a stale, undersized allocation in the production traversal.
    memset(target, 0, sizeof(Mtx));
}
static void osSyncPrintf(const char*) {}
static void Matrix_Push() {}
static void Matrix_Pop() {}
static void Matrix_Scale(float, float, float, int) {}
static void Matrix_TranslateRotateZYX(Vec3f*, Vec3s*) {}
static const void* NativeNpcPose_GetSkinPlan(void**, s32) { return nullptr; }
static int NativeNpcPose_SkinMatrixCount(const void*) { return 0; }
static float NativeNpcPose_GetDrawScale(void**) { return 1; }
static void SkelAnime_ApplyNpcSkin(const void*, Mtx*, s32) {}
template <typename T> static void gSPSegment(Gfx*, int, T) {}
static void gSPMatrix(Gfx*, Mtx*, int) {}
static void gSPDisplayList(Gfx*, Gfx*) {}
static void Gfx_SetupDL_25Opa(GraphicsContext*) {}
static void gDPSetEnvColor(Gfx*, int, int, int, int) {}
static char gZelda2EyeOpenTex[8], gZelda2EyeHalfTex[8], gZelda2EyeShutTex[8], gZelda2MouthSeriousTex[8];
static Gfx D_80116280[3];
static s32 EnViewer_StaticAdultZeldaOverrideLimbDraw(PlayState*, s32, Gfx**, Vec3f*, Vec3s*, void*, Gfx**) {
    return false; // Face deformation is outside the shared matrix-count contract.
}
namespace SOH {
struct Skeleton {
    union { SkeletonHeader skeletonHeader; FlexSkeletonHeader flexSkeletonHeader; } skeletonData;
    int dListCount;
    void* GetPointer() { return &skeletonData; }
};
struct SkeletonPatchInfo { SkelAnime* skelAnime; std::string vanillaSkeletonPath; };
class SkeletonPatcher {
public:
    static std::vector<SkeletonPatchInfo> skeletons;
    static void UpdateSkeletons();
    static void UpdateCustomSkeletonFromPath(const std::string&, SkeletonPatchInfo&);
};
std::vector<SkeletonPatchInfo> SkeletonPatcher::skeletons;
}
struct ResourceManager {
    bool altEnabled;
    std::map<std::string, std::shared_ptr<SOH::Skeleton>> files;
    bool IsAltAssetsEnabled() { return altEnabled; }
    std::shared_ptr<SOH::Skeleton> LoadResource(const std::string& path, bool exact) {
        REQUIRE(exact); // Both production swap routes request exact namespaces.
        auto found = files.find(path);
        return found == files.end() ? nullptr : found->second;
    }
};
namespace Ship {
struct IResource { static std::string gAltAssetPrefix; };
std::string IResource::gAltAssetPrefix = "alt/";
struct Context {
    ResourceManager manager;
    static Context* GetRawInstance() { static Context context; return &context; }
    ResourceManager* GetResourceManager() { return &manager; }
};
}
/* PRODUCTION_FUNCTIONS */
struct Rig {
    StandardLimb limbs[14]{};
    void* pointers[14];
    Gfx geometry[1]{};
    std::shared_ptr<SOH::Skeleton> resource = std::make_shared<SOH::Skeleton>();
    explicit Rig(unsigned drawable) {
        for (unsigned i = 0; i < 14; ++i) {
            limbs[i].child = i == 13 ? 255 : i + 1;
            limbs[i].sibling = 255;
            limbs[i].dList = i > 0 && i <= drawable ? geometry : nullptr;
            pointers[i] = &limbs[i];
        }
        resource->dListCount = drawable;
        resource->skeletonData.flexSkeletonHeader = { { pointers, 14, 1 }, uint8_t(drawable) };
    }
};
static void draw(EnViewer& actor, unsigned expected, Vec3s* joints, Vec3s* morphs) {
    Gfx commands[128]{};
    GraphicsContext graphics{ commands };
    PlayState play{ { &graphics } };
    allocatedMatrices = writtenMatrices = 0;
    fprintf(stderr, "draw: cached=%d expected=%u\n", actor.skin.skelAnime.dListCount, expected);
    EnViewer_DrawStaticAdultZelda(&actor, &play);
    REQUIRE(allocatedMatrices == expected && writtenMatrices == expected);
    REQUIRE(actor.skin.skelAnime.dListCount == int(expected));
    REQUIRE(actor.skin.skelAnime.limbCount == 15);
    REQUIRE(actor.skin.skelAnime.jointTable == joints && actor.skin.skelAnime.morphTable == morphs);
    REQUIRE(joints[0].x == 123 && morphs[14].z == 456);
    free(frameMatrices);
    frameMatrices = nullptr;
}
int main(int argc, char** argv) {
    REQUIRE(argc == 2 && sizeof(Mtx) == 64);
    const std::string test = argv[1];
    const bool custom = test.starts_with("custom");
    const bool missing = test.ends_with("missing");
    const bool fallback = test == "custom_fallback";
    const bool targetAlt = test.ends_with("on") || missing;
    const std::string path = "objects/object_zl2/gZelda2Skel";
    Rig native(13), authored(9);
    auto manager = Ship::Context::GetRawInstance()->GetResourceManager();
    manager->files[path] = native.resource;
    if (!missing)
        manager->files["alt/" + path] = authored.resource;
    manager->altEnabled = targetAlt;
    const bool initialAlt = fallback || !targetAlt;
    auto initial = initialAlt ? authored.resource : native.resource;
    Vec3s joints[15]{}, morphs[15]{};
    joints[0].x = 123;
    morphs[14].z = 456;
    EnViewer actor{};
    actor.skin.skelAnime = { initial->skeletonData.skeletonHeader.segment,
        &initial->skeletonData.skeletonHeader, joints, morphs, 15, initial->dListCount };
    SOH::SkeletonPatcher::skeletons.push_back({ &actor.skin.skelAnime, path });
    auto& registered = SOH::SkeletonPatcher::skeletons.front();
    if (custom)
        SOH::SkeletonPatcher::UpdateCustomSkeletonFromPath(fallback ? "objects/missing/Skel" : path, registered);
    else
        SOH::SkeletonPatcher::UpdateSkeletons();
    const bool expectedAlt = targetAlt && !missing;
    auto expected = expectedAlt ? authored.resource : native.resource;
    REQUIRE(actor.skin.skelAnime.skeleton == expected->skeletonData.skeletonHeader.segment);
    REQUIRE(actor.skin.skelAnime.skeletonHeader == &expected->skeletonData.skeletonHeader);
    draw(actor, expectedAlt ? 9 : 13, joints, morphs);
    printf("PASS %s: matrix allocation matches traversal; joint and morph buffers preserved\n", argv[1]);
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--case", choices=CASES)
    parser.add_argument("--skeleton-source", type=Path,
                        default=ROOT / "soh/soh/resource/type/Skeleton.cpp")
    parser.add_argument("--viewer-source", type=Path,
                        default=ROOT / "soh/src/overlays/actors/ovl_En_Viewer/z_en_viewer.c")
    parser.add_argument("--skelanime-source", type=Path, default=ROOT / "soh/src/code/z_skelanime.c")
    args = parser.parse_args()
    skeleton, viewer, animation = (path.read_text() for path in
                                  (args.skeleton_source, args.viewer_source, args.skelanime_source))
    production = "\n".join(function(animation, name) for name in
                           ("SkelAnime_DrawFlexLimb", "SkelAnime_DrawFlex"))
    production += "\nnamespace SOH {\n" + "\n".join(function(skeleton, name) for name in
        ("SkeletonPatcher::UpdateSkeletons", "SkeletonPatcher::UpdateCustomSkeletonFromPath")) + "\n}\n"
    # C's local identifier `this` is a C++ keyword; only rename that identifier.
    production += re.sub(r"\bthis\b", "actor", function(viewer, "EnViewer_DrawStaticAdultZelda"))
    with tempfile.TemporaryDirectory(prefix="alt-flex-matrices-") as directory:
        source, binary = Path(directory) / "test.cpp", Path(directory) / "test"
        source.write_text(FIXTURE.replace("/* PRODUCTION_FUNCTIONS */", production))
        flags = ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-Wno-unused-variable"]
        if args.sanitize:
            flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                      "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
        subprocess.run([os.environ.get("CXX", "c++"), *flags, str(source), "-o", str(binary)], check=True)
        environment = {**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}
        failed = []
        for case in (args.case,) if args.case else CASES:
            result = subprocess.run([str(binary), case], env=environment, capture_output=True, text=True)
            if result.returncode:
                failed.append(case)
                print(result.stderr, end="")
            else:
                print(result.stdout, end="")
        if failed:
            raise SystemExit("FAILED skeleton Alt matrix cases: " + ", ".join(failed))
    print("PASS shared skeleton swaps: both directions and both paths, missing resources, same-joint storage")


if __name__ == "__main__":
    main()
